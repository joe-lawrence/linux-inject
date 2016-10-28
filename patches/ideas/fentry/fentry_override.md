# Patching functions via __fentry__ override

## Compiler options

Consider a simple C function:

	int my_function(int a)
	{
		printf("my function: %d!\n", a);
	}

Normal compilation looks like this:

	0000000000400591 <my_function>:
	  400591:	55                   	push   %rbp
	  400592:	48 89 e5             	mov    %rsp,%rbp
	  400595:	48 83 ec 10          	sub    $0x10,%rsp
	  400599:	89 7d fc             	mov    %edi,-0x4(%rbp)
	  40059c:	8b 45 fc             	mov    -0x4(%rbp),%eax
	  40059f:	89 c6                	mov    %eax,%esi
	  4005a1:	bf 7e 06 40 00       	mov    $0x40067e,%edi
	  4005a6:	b8 00 00 00 00       	mov    $0x0,%eax
	  4005ab:	e8 a0 fe ff ff       	callq  400450 <printf@plt>
	  4005b0:	c9                   	leaveq 
	  4005b1:	c3                   	retq   

With the ```-pg``` option to add extra code for writing profiling
information (additional instructions prefixed with '>').  Adds 5 bytes
to the function size:

	00000000004005e6 <my_function>:
	  4005e6:	55                   	push   %rbp
	  4005e7:	48 89 e5             	mov    %rsp,%rbp
	  4005ea:	48 83 ec 10          	sub    $0x10,%rsp
	> 4005ee:	e8 cd fe ff ff       	callq  4004c0 <mcount@plt>
	  4005f3:	89 7d fc             	mov    %edi,-0x4(%rbp)
	  4005f6:	8b 45 fc             	mov    -0x4(%rbp),%eax
	  4005f9:	89 c6                	mov    %eax,%esi
	  4005fb:	bf de 06 40 00       	mov    $0x4006de,%edi
	  400600:	b8 00 00 00 00       	mov    $0x0,%eax
	  400605:	e8 86 fe ff ff       	callq  400490 <printf@plt>
	  40060a:	c9                   	leaveq 
	  40060b:	c3                   	retq 

With ```-pg -mfentry``` options to put the profiling counter call
*before* the prologue.  Adds 5 bytes to function size:

	00000000004005c6 <my_function>:
	> 4005c6:       e8 d5 fe ff ff          callq  4004a0 <__fentry__@plt>
	  4005cb:       55                      push   %rbp
	  4005cc:       48 89 e5                mov    %rsp,%rbp
	  4005cf:       48 83 ec 10             sub    $0x10,%rsp
	  4005d3:       89 7d fc                mov    %edi,-0x4(%rbp)
	  4005d6:       8b 45 fc                mov    -0x4(%rbp),%eax
	  4005d9:       89 c6                   mov    %eax,%esi
	  4005db:       bf ae 06 40 00          mov    $0x4006ae,%edi
	  4005e0:       b8 00 00 00 00          mov    $0x0,%eax
	  4005e5:       e8 86 fe ff ff          callq  400470 <printf@plt>
	  4005ea:       c9                      leaveq
	  4005eb:       c3                      retq

With ```-pg -finstrument-functions``` options, which will generate
instrumentation calls for entry and exit to functions.  Adds 34 bytes to
the function size:

	0000000000400613 <my_function>:
	  400613:	55                   	push   %rbp
	  400614:	48 89 e5             	mov    %rsp,%rbp
	  400617:	48 83 ec 10          	sub    $0x10,%rsp
	  40061b:	89 7d fc             	mov    %edi,-0x4(%rbp)
	> 40061e:	48 8b 45 08          	mov    0x8(%rbp),%rax
	> 400622:	48 89 c6             	mov    %rax,%rsi
	> 400625:	bf 13 06 40 00       	mov    $0x400613,%edi
	> 40062a:	e8 27 00 00 00       	callq  400656 <__cyg_profile_func_enter>
	  40062f:	8b 45 fc             	mov    -0x4(%rbp),%eax
	  400632:	89 c6                	mov    %eax,%esi
	  400634:	bf 4e 07 40 00       	mov    $0x40074e,%edi
	  400639:	b8 00 00 00 00       	mov    $0x0,%eax
	  40063e:	e8 5d fe ff ff       	callq  4004a0 <printf@plt>
	> 400643:	48 8b 45 08          	mov    0x8(%rbp),%rax
	> 400647:	48 89 c6             	mov    %rax,%rsi
	> 40064a:	bf 13 06 40 00       	mov    $0x400613,%edi
	> 40064f:	e8 7c fe ff ff       	callq  4004d0 <__cyg_profile_func_exit@plt>
	  400654:	c9                   	leaveq 
	  400655:	c3                   	retq   

## The experiment

For purposes of this experiment, we'll be using the  ```-pg --mfentry```
options.  It minimally affects the function code and stack.  This also
what the kernel's ftrace facility currently uses.

Compile the .c file with ```-pg --mfentry``` so the compiler inserts
calls to ```__fentry___``` at the beginning of every function.  Omit
these flags when linking the final executable to avoid generating the
```gmon.out``` profiling data file.

	% gcc fentry_override.c -pg -mfentry -c -o fentry_override.o
	% gcc -o fentry_override fentry_override.o

        % objdump -DS /tmp/fentry_override
        ...

	00000000004005c6 <my_function>:

	int my_function(int a)
	{
	4005c6:	e8 d5 fe ff ff       	callq  4004a0 <__fentry__@plt>
        ...

Now that every function call is routed through ```__fentry__```, we can
provide our own implementation to patches the return address ... sending
the caller off to a patched function and not the original function:

	/* fentry_override.c */
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

	% ./fentry_override
	my_patch: 123

## Additional thoughts

```__fentry__``` could be provided in a shared library:

1. Target build is modified: ```-l```
2. ```LD_PRELOAD``` when starting target
3. Injected via ```ptrace``` + call to ```dlopen```

The next hurdle becomes patch management, i.e. how would our
```__fentry__``` implementation know which functions to patch and where
to go?  Maybe something like the following:

1. ```__fentry__``` will look at some global variable, if it is set,
then it will ```dlopen``` a shared library patch.
2. The shared library patch will write to a global ```patches``` array,
providing some reference to the "old" function and the "new" function.
3. ```__fentry__``` continues execution, running through the global
```patch``` array testing to see if the current caller should be
patched.

## Links

[Ftrace Kernel Hooks:More than just
tracing](https://linuxplumbersconf.org/2014/ocw/system/presentations/1773/original/ftrace-kernel-hooks-2014.pdf) - slides on kernel ftrace implementation

[lkml: Re: RFC PATCH Make ftrace able to trace function return](https://lkml.org/lkml/2008/10/30/372) - mailing list conversation about compiler options

