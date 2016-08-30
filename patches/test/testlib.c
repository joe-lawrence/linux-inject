#include <stdio.h>
#include <unistd.h>

int interval = 1;

static int __attribute__ ((noinline)) lib_private(int a, int b, int c, int d, int e, int f, int g, int h)
{
	printf("\e[91m%s\e[0m\targs: [%d %d %d %d  %d %d %d %d]\n", __func__,
		a, b, c, d, e, f, g, h);

	return 10;
}

void lib_function()
{
	int ret;

	ret = lib_private(1, 2, 3, 4, 5, 6, 7, 8);
	printf("%s lib_private ret=%d, sleep(%d)\n", __func__, ret, interval);
	sleep(interval);
}
