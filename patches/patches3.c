#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "trampoline.h"


void patch_constructor(void)
{
	pid_t x = syscall(__NR_gettid);
	printf("\e[1m\e[33m%s(%d)\e[0m\n", __func__, x);
}

void patch_destructor(void)
{
	pid_t x = syscall(__NR_gettid);
	printf("\e[1m\e[33m%s(%d)\e[0m\n", __func__, x);
}


/* test3 patches */
void patched_my_print(void)
{
	pid_t x = syscall(__NR_gettid);
	printf("\e[1m\e[33m%s(%d)\e[0m\n", __func__, x);
	sleep(1);
}


struct trampolines tramps[] = {

	{ .map_name = "/tmp/linux-inject/patches/test/test3",
	  .build_id = "ce6abfcc4da99179f9f22219b1fcb4057e6033b3",	/* readelf -n test/test | grep Build */
	  .oldname  = "my_print",
	  .new_addr = patched_my_print,
	  .offset   = 0x000000000040076d,				/* nm test/test | grep my_print */
	  .old_size = 0x0000000000000040,				/* nm -S test/libtest.so | grep my_print */
	  .type     = T_OFFSET_ABS, },

	{ 0 } };
