#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#include "trampoline.h"
#include "utils.h"

/* defined by patches.c */
extern struct trampolines tramps[];
extern void patch_constructor(void);
extern void patch_destructor(void);

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

	mprotect((void *) pageStart, end - pageStart, PROT_READ | PROT_WRITE | PROT_EXEC);

	memcpy(dest, src, len);

	/* TODO: save and restore, or blindly reset? */
	mprotect((void *) pageStart, end - pageStart, PROT_READ | PROT_EXEC);

	return 0;
}


__attribute__((constructor))
static void constructor(void)
{
	int i;

	for (i=0; tramps[i].new_addr; i++) {

		if (!tramps[i].old_addr) {

			unsigned long addr;

			addr = tramps[i].offset;

			/* TODO: elf headers help us out here? */
			if (tramps[i].type == T_OFFSET_REL)
				addr += get_dso_base(tramps[i].map_name, 0);

			tramps[i].old_addr = (void *) addr;
		}
//		printf("tramps[%d].old_addr = %p, .new_addr = %p, .oldname = %s\n",
//			i, tramps[i].old_addr, tramps[i].new_addr, tramps[i].oldname);

		/* Stop now before we break anything */
		if (!tramps[i].new_addr || !tramps[i].old_addr)
			return;
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

		rewrite_code(tramps[i].old_addr, trampoline, tramps[i].old_code, TRAMPOLINE_BYTES);
	}

	patch_constructor();
}

__attribute__((destructor))
static void destructor(void)
{
	int i;

	patch_destructor();

	/* Restore the code that we trampled with our trampolines.
	 * This is probably unnecessary until we support un-patching. */

	for (i=0; tramps[i].new_addr; i++) {
		rewrite_code(tramps[i].old_addr, tramps[i].old_code, NULL, TRAMPOLINE_BYTES);
	}
}
