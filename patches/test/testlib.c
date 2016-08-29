#include <stdio.h>
#include <unistd.h>

int interval = 1;

static void __attribute__ ((noinline)) lib_private(void)
{
	printf("\e[91m%s\e[0m\n", __func__);
}

void lib_function (void)
{
	lib_private();
	printf("%s sleep(%d)\n", __func__, interval);
	sleep(interval);
}
