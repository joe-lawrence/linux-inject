/*

gcc fentry_override.c -pg -mfentry -c -o fentry_override.o
gcc -o fentry_override fentry_override.o

 */

#include <stdio.h>
#include <stdint.h>

#define FENTRY_BYTES (5)

int my_patch(int a)
{
	printf("my_patch: %d\n", a);
}

int my_function(int a)
{
	printf("my function: %d!\n", a);
}

__attribute__((no_instrument_function))
void __fentry__(void)
{
 	uint64_t returnaddr;
 
	asm("mov 8(%%rbp),%0" : "=r"(returnaddr) : : );

	if (returnaddr == (uint64_t) my_function + FENTRY_BYTES) {
		returnaddr = (uint64_t) my_patch + FENTRY_BYTES;
 		asm("mov %0,8(%%rbp)" : "=r"(returnaddr) : : );
	}
}

int main()
{
	my_function(123);

	return 0;
}
