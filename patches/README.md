# patches
## Proof-of-concept tool for patching a running process
The patches/ subdirectory provides a shared library "patch" for use with [linux-inject](https://github.com/gaffe23/linux-inject) project.  Code inside the shared library patch is intended to replace broken or vulnerable code inside a running process with out restart.

## How it works

### patches.c

A patches.c file includes new code ```patched_lib_private``` and ```patched_pgm_function``` (not shown here) but referenced in a global ```trampolines``` structure.  Many of these fields (```.map_name```, ```.offset```, ```.type```) are only required as part of the proof-of-concept.  (This is understandably a bit clunky to hardcode into the shared library patch, but they simplify the POC by avoding ELF or DWARF header parsing.):

	struct trampolines tramps[] = {
	
		{ .oldname  = "lib_private",
		  .new_addr = patched_lib_private,
		  .map_name = "/tmp/libtest.so",
		  .offset   = 0x0000000000000758,	/* nm test/libtest.so | grep lib_private */
		  .type     = T_OFFSET_REL, },
	
		{ .oldname  = "pgm_function",
		  .new_addr = patched_pgm_function,
		  .map_name = "/tmp/test",
		  .offset   = 0x00000000004006e0,	/* nm test/test | grep pgm_function */
		  .type     = T_OFFSET_ABS, },
	
		{ 0 } };

### linux-inject

The shared library patch is injected into the test program using the [linux-inject](https://github.com/gaffe23/linux-inject).  When the shared library patch is loaded, its constructor uses ```trampoline[]``` data to save the first 16 bytes of a given entry point and replaces it with a code "trampoline" to the new one.

For example, before [linux-inject](https://github.com/gaffe23/linux-inject):

	(gdb) disassemble pgm_function
	Dump of assembler code for function pgm_function:
	   0x0000000000400780 <+0>:     sub    $0x18,%rsp
	   0x0000000000400784 <+4>:     mov    %fs:0x28,%rax
	   0x000000000040078d <+13>:    mov    %rax,0x8(%rsp)
	   0x0000000000400792 <+18>:    xor    %eax,%eax
	   0x0000000000400794 <+20>:    lea    0xf2(%rip),%rdx        # 0x40088d <__func__.2386>
	   0x000000000040079b <+27>:    lea    0xde(%rip),%rsi        # 0x400880
	   0x00000000004007a2 <+34>:    mov    $0x1,%edi
	   0x00000000004007a7 <+39>:    callq  0x400680 <__printf_chk@plt>
	   0x00000000004007ac <+44>:    mov    0x8(%rsp),%rax
	   0x00000000004007b1 <+49>:    xor    %fs:0x28,%rax
	   0x00000000004007ba <+58>:    je     0x4007c1 <pgm_function+65>
	   0x00000000004007bc <+60>:    callq  0x400640 <__stack_chk_fail@plt>
	   0x00000000004007c1 <+65>:    add    $0x18,%rsp
	   0x00000000004007c5 <+69>:    retq   
	End of assembler dump.

After [linux-inject](https://github.com/gaffe23/linux-inject) is executed, the process's ```/proc/PID/maps``` file is updated to reflect the new shared library mappings:

	/proc/<PID>/maps
	00400000-00401000 r-xp 00000000 fd:00 17609433                           /tmp/linux-inject/patches/test/test
	00600000-00601000 r--p 00000000 fd:00 17609433                           /tmp/linux-inject/patches/test/test
	00601000-00602000 rw-p 00001000 fd:00 17609433                           /tmp/linux-inject/patches/test/test
	...
	7f12bbbd5000-7f12bbbd7000 r-xp 00000000 fd:00 73423                      /tmp/linux-inject/patches/libupatch.so
	7f12bbbd7000-7f12bbdd6000 ---p 00002000 fd:00 73423                      /tmp/linux-inject/patches/libupatch.so
	7f12bbdd6000-7f12bbdd7000 r--p 00001000 fd:00 73423                      /tmp/linux-inject/patches/libupatch.so
	7f12bbdd7000-7f12bbdd8000 rw-p 00002000 fd:00 73423                      /tmp/linux-inject/patches/libupatch.so

### Code trampolines

After ```linux-inject``` is executed, code trampolines are set in place, patching old functions with the shared library patch:

	(gdb) disassemble pgm_function
	Dump of assembler code for function pgm_function:
	   0x0000000000400780 <+0>:     pushq  $0xffffffffbbbd6307             << trampoline
	   0x0000000000400785 <+5>:     movl   $0x7f12,0x4(%rsp)               << to new
	   0x000000000040078d <+13>:    retq                                   << code
	   0x000000000040078e <+14>:    add    %al,(%rax)
	   0x0000000000400790 <+16>:    and    $0x8,%al
	   0x0000000000400792 <+18>:    xor    %eax,%eax
	   0x0000000000400794 <+20>:    lea    0xf2(%rip),%rdx        # 0x40088d <__func__.2386>
	   0x000000000040079b <+27>:    lea    0xde(%rip),%rsi        # 0x400880
	   0x00000000004007a2 <+34>:    mov    $0x1,%edi
	   0x00000000004007a7 <+39>:    callq  0x400680 <__printf_chk@plt>
	   0x00000000004007ac <+44>:    mov    0x8(%rsp),%rax
	   0x00000000004007b1 <+49>:    xor    %fs:0x28,%rax
	   0x00000000004007ba <+58>:    je     0x4007c1 <pgm_function+65>
	   0x00000000004007bc <+60>:    callq  0x400640 <__stack_chk_fail@plt>
	   0x00000000004007c1 <+65>:    add    $0x18,%rsp
	   0x00000000004007c5 <+69>:    retq   
	End of assembler dump.

The trampoline code is fairly simple, but relies on a trick documented on Nikolay Igotti's blog in [Long absolute jumps on AMD64](https://blogs.oracle.com/nike/entry/long_absolute_jumps_on_amd64).  All told, the trampoline amounts to only 16 bytes that we need to manage.

## POC Shortcomings

This proof-of-concept inhereits any limitations of the [linux-inject](https://github.com/gaffe23/linux-inject) project as well as a few others:

1. The git repo should be cloned and run out of ```/tmp```
2. ```constructor.c``` has a bunch of magic number and other data that could be determined at runtime
3. Target functions (i.e., those to patch) must be at least 16 bytes (the size of the code trampoline) long
4. During patching operation, target process cannot be running as a gdb, strace or any other ptrace client
5. [linux-inject](https://github.com/gaffe23/linux-inject) supports x86, x86_64, and ARM, but patches only implements x86_64 code
6. No protection against patching functions currently executing
99. Many more, I'm sure

## Demo

*Bonus*: Because ```linux-inject``` utilizes the dynamic linker, gdb recognizes the shared library patch symbols:

	(gdb) info symbol 0x7f12bbbd6307
	patched_pgm_function in section .text of /tmp/linux-inject/patches/libupatch.so

![Demo](demo.gif)
