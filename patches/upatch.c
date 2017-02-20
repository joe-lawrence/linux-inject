#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <elf.h>
#include <systemd/sd-journal.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include "upatch.h"


/* TODO: replace with reading self? */
/* From linux kernel: tools/vm/page-types.c */
static unsigned long get_dso_base(const char *path, const char *sym)
{
	struct stat st;
	FILE *file;
	char buf[5000];
	unsigned long last_vm_start = -1;

	lstat(path, &st);
		
	sprintf(buf, "/proc/%d/maps", getpid());
	file = fopen(buf, "r");
	if (!file) {
		perror(buf);
		exit(EXIT_FAILURE);
	}

	while (fgets(buf, sizeof(buf), file) != NULL) {
		unsigned long vm_start;
		unsigned long vm_end;
		unsigned long long pgoff;
		int major, minor;
		char r, w, x, s;
		unsigned long ino;
		int n;

		n = sscanf(buf, "%lx-%lx %c%c%c%c %llx %x:%x %lu",
			   &vm_start,
			   &vm_end,
			   &r, &w, &x, &s,
			   &pgoff,
			   &major, &minor,
			   &ino);
		if (n < 10) {
			fprintf(stderr, "unexpected line: %s\n", buf);
			continue;
		}

		if ((st.st_ino == ino) &&
		    (r == 'r' && w == '-' && x == 'x' && s == 'p')) {
			last_vm_start = vm_start;
		}
	}
	fclose(file);

	return last_vm_start;
}


/* Read ELF note containing build id,
 * from: http://stackoverflow.com/questions/17637745/can-a-program-read-its-own-elf-section 
 * TODO: can libelf handle this? */

#if __x86_64__
#  define ElfW(type) Elf64_##type
#else
#  define ElfW(type) Elf32_##type
#endif

static int build_id_cmp(const char *thefilename, char *bid)
{
	FILE *thefile;
	struct stat statbuf;
	int i;

	unsigned char * build_id;
	char buildid[41];

	ElfW(Ehdr) *ehdr = 0;
	ElfW(Phdr) *phdr = 0;
	ElfW(Nhdr) *nhdr = 0;

	if (!(thefile = fopen(thefilename, "r"))) {
		perror(thefilename);
		exit(EXIT_FAILURE);
	}
	if (fstat(fileno(thefile), &statbuf) < 0) {
		perror(thefilename);
		exit(EXIT_FAILURE);
	}

	ehdr = (ElfW(Ehdr) *)mmap(0, statbuf.st_size,
			PROT_READ|PROT_WRITE, MAP_PRIVATE, fileno(thefile), 0);
	phdr = (ElfW(Phdr) *)(ehdr->e_phoff + (size_t)ehdr);

	while (phdr->p_type != PT_NOTE) {
		++phdr;
	}

	nhdr = (ElfW(Nhdr) *)(phdr->p_offset + (size_t)ehdr);

	while (nhdr->n_type != NT_GNU_BUILD_ID) {
		nhdr = (ElfW(Nhdr) *)((size_t)nhdr + sizeof(ElfW(Nhdr)) + nhdr->n_namesz + nhdr->n_descsz);
	}

	build_id = (unsigned char *)malloc(nhdr->n_descsz);
	memcpy(build_id, (void *)((size_t)nhdr + sizeof(ElfW(Nhdr)) + nhdr->n_namesz), nhdr->n_descsz);

	for (i = 0; i < nhdr->n_descsz; i++) {
		sprintf(buildid+i*2, "%02x",build_id[i]);
	}

	free(build_id);

	return strcmp(bid, buildid);
}


/* Call into libunwind to determine if function is currently 
 * on the call-stack. */

static int function_on_stack(void *func, size_t func_sz)
{
        unw_cursor_t cursor;
        unw_context_t uc;

        unw_getcontext(&uc);
        unw_init_local(&cursor, &uc);

        do {
		unw_word_t ip;

                unw_get_reg(&cursor, UNW_REG_IP, &ip);

		if ( ((void *) ip >= func) &&
		     ((void *) ip < (void *)((char *)func + func_sz)) )
			return -1;

        } while (unw_step (&cursor) > 0);

	return 0;
}


static int rewrite_code(void *dest, void *src, void *backup, size_t len)
{
	size_t pageSize;
	uintptr_t start, end, pageStart;

	/* TODO: worry about trampoline[] spilling into the next page? */
	pageSize = sysconf(_SC_PAGESIZE);
	start = (uintptr_t) dest;
	end = start + 1;
	pageStart = start & -pageSize;

	if (backup)
		memcpy(backup, dest, len);

	if (mprotect((void *) pageStart, end - pageStart, PROT_READ | PROT_WRITE | PROT_EXEC)) {
		sd_journal_print(LOG_ERR, "Not patching mprotect failed: %s\n", strerror(errno));
		return -1;
	}

	memcpy(dest, src, len);

	/* TODO: save and restore, or blindly reset? */
	mprotect((void *) pageStart, end - pageStart, PROT_READ | PROT_EXEC);

	return 0;
}


static int fully_loaded;

void libupatch_load(struct trampolines tramps[])
{
	int i;

	for (i=0; tramps[i].new_addr; i++) {

		if (tramps[i].build_id) {
			if (build_id_cmp(tramps[i].map_name, tramps[i].build_id)) {
				sd_journal_print(LOG_ERR, "Not patching, buildid mismatch: %s\n", tramps[i].map_name);
				return;
			}
		}

		if (!tramps[i].old_addr) {

			unsigned long addr;

			addr = tramps[i].offset;

			/* TODO: elf headers help us out here? */
			if (tramps[i].type == T_OFFSET_REL)
				addr += get_dso_base(tramps[i].map_name, 0);

			tramps[i].old_addr = (void *) addr;
		}
		sd_journal_print(LOG_INFO, "tramps[%d].old_addr = %p, .new_addr = %p, .oldname = %s\n",
			i, tramps[i].old_addr, tramps[i].new_addr, tramps[i].oldname);

		/* Stop now before we break anything */
		if (!tramps[i].new_addr || !tramps[i].old_addr) {
			sd_journal_print(LOG_ERR, "Not patching, %s .new_addr=%p, old_addr=%p!\n",
				tramps[i].oldname, tramps[i].new_addr, tramps[i].old_addr);
			return;
		}
		if (function_on_stack(tramps[i].old_addr, tramps[i].old_size)) {
			sd_journal_print(LOG_ERR, "Not patching, %s is on call stack!\n",
				tramps[i].oldname);
			return;
		}
	}

	for (i=0; tramps[i].new_addr; i++) {

		char trampoline[TRAMPOLINE_BYTES];

		/* trampoline code to our replacement function should look like,
		 * (ie, when destination virtual address is 0x7ffff79d78d5):
		 *
		 *  0:   68 35 78 9d f7          pushq  $0xf79d78d5
		 *  5:   c7 44 24 04 ff 7f 00    movl   $0x00007fff,0x4(%rsp)
		 *  c:   00 
		 *  d:   c3                      retq   
		 */

		trampoline[ 0] = 0x68;
		trampoline[ 1] = (unsigned long) tramps[i].new_addr & 0xFF;
		trampoline[ 2] = (unsigned long) tramps[i].new_addr >> 8 & 0xFF;
		trampoline[ 3] = (unsigned long) tramps[i].new_addr >> 16 & 0xFF;
		trampoline[ 4] = (unsigned long) tramps[i].new_addr >> 24 & 0xFF;

		trampoline[ 5] = 0xc7;
		trampoline[ 6] = 0x44;
		trampoline[ 7] = 0x24;
		trampoline[ 8] = 0x04;
		trampoline[ 9] = (unsigned long) tramps[i].new_addr >> 32 & 0xFF;
		trampoline[10] = (unsigned long) tramps[i].new_addr >> 40 & 0xFF;
		trampoline[11] = (unsigned long) tramps[i].new_addr >> 48 & 0xFF;
		trampoline[12] = (unsigned long) tramps[i].new_addr >> 56 & 0xFF;

		trampoline[13] = 0xc3;
		trampoline[14] = 0;
		trampoline[15] = 0;

		if (rewrite_code(tramps[i].old_addr, trampoline, tramps[i].old_code, TRAMPOLINE_BYTES))
			continue;

		tramps[i].patched = 1;
	}

	fully_loaded = 1;
}

void libupatch_unload(struct trampolines tramps[])
{
	int i;

	if (!fully_loaded)
		return;

	/* Restore the code that we trampled with our trampolines. */

	for (i=0; tramps[i].new_addr; i++) {
		if (tramps[i].patched)
			rewrite_code(tramps[i].old_addr, tramps[i].old_code, NULL, TRAMPOLINE_BYTES);
	}
}
