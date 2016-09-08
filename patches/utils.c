#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "utils.h"

/* TODO: replace with reading self? */
/* From linux kernel: tools/vm/page-types.c */
unsigned long get_dso_base(const char *path, const char *sym)
{
	struct stat st;
	FILE *file;
	char buf[5000];

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

		if (st.st_ino == ino) {
			fclose(file);
			return vm_start;
		}
	}
	fclose(file);

	return -1;
}


/* Read ELF note containing build id,
 * from: http://stackoverflow.com/questions/17637745/can-a-program-read-its-own-elf-section 
 * TODO: can libelf handle this? */

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#if __x86_64__
#  define ElfW(type) Elf64_##type
#else
#  define ElfW(type) Elf32_##type
#endif

int build_id_cmp(const char *thefilename, char *bid)
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


#define UNW_LOCAL_ONLY
#include <libunwind.h>

/* Call into libunwind to determine if function is currently 
 * on the call-stack. */

int function_on_stack(void *func, size_t func_sz)
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


