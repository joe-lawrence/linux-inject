#include <stdio.h>
#include "trampoline.h"


void patch_constructor(void)
{
}

void patch_destructor(void)
{
}


/* libtest.so patches */

void patched_lib_private2()
{
	printf("\e[32m%s\e[0m\n", __func__);
}

/* test patches */
void patched_pgm_function2()
{
	printf("\e[32m%s\e[0m\n", __func__);
}


struct trampolines tramps[] = {

	{ .oldname  = "lib_private",
	  .new_addr = patched_lib_private2,
	  .map_name = "/tmp/linux-inject/patches/test/libtest.so",
	  .offset   = 0x0000000000000758,	/* nm libtest.so | grep lib_private */
	  .type     = T_OFFSET_REL, },

	{ .oldname  = "pgm_function",
	  .new_addr = patched_pgm_function2,
	  .map_name = "/tmp/linux-inject/patches/test/test",
	  .offset   = 0x00000000004006e0,	/* nm test | grep pgm_function */
	  .type     = T_OFFSET_ABS, },

	{ 0 } };
