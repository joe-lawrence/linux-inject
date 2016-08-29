#include <stdio.h>

extern int lib_function (void);

void pgm_function(void)
{
	printf("\e[91m%s\e[0m\n", __func__);
}


int main()
{
	while(1) {
		pgm_function();
		lib_function();
	}
	
        return 0;
}
