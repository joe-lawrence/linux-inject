#include <stdio.h>
#include "trampoline.h"


void patch_constructor(void)
{
}

void patch_destructor(void)
{
}


/* libtest.so patches */

int patched_lib_private2(int a, int b, int c, int d, int e, int f, int g, int h)
{
	printf("\e[32m%s\e[0m\targs: [%d %d %d %d  %d %d %d %d]\n", __func__,
		a, b, c, d, e, f, g, h);

	return 300;
}

/* test patches */
void patched_pgm_function2(int a, int b, int c, int d, int e, int f, int g, int h)
{
	printf("\e[32m%s\e[0m\targs: [%d %d %d %d  %d %d %d %d]\n", __func__,
		a, b, c, d, e, f, g, h);
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
