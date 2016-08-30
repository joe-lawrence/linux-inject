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

	{ .map_name = "/tmp/linux-inject/patches/test/libtest.so",
	  .build_id = "19549297ab65e6a2bba573f0ae070234b59593b1",	/* readelf -n test/libtest.so | grep Build */
	  .oldname  = "lib_private",
	  .new_addr = patched_lib_private2,
	  .offset   = 0x0000000000000758,				/* nm test/libtest.so | grep lib_private */
	  .old_size = 0x000000000000006c,				/* nm -S test/libtest.so | grep lib_private */
	  .type     = T_OFFSET_REL, },

	{ .map_name = "/tmp/linux-inject/patches/test/test",
	  .build_id = "8e3a59ed66f5ee6d04d0a90c0936b950997db4ab",	/* readelf -n test/test | grep Build */
	  .oldname  = "pgm_function",
	  .new_addr = patched_pgm_function2,
	  .offset   = 0x0000000000400730,				/* nm test/test | grep pgm_function */
	  .old_size = 0x0000000000000095,				/* nm -S test/libtest.so | grep pgm_function */
	  .type     = T_OFFSET_ABS, },

	{ 0 } };
