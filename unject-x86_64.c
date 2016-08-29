#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/user.h>
#include <wait.h>

#include "utils.h"
#include "ptrace.h"

/*
 * injectSharedLibrary()
 *
 * This is the code that will actually be injected into the target process.
 * This code is responsible for loading the shared library into the target
 * process' address space.  First, it calls malloc() to allocate a buffer to
 * hold the filename of the library to be loaded. Then, it calls
 * __libc_dlopen_mode(), libc's implementation of dlopen(), to load the desired
 * shared library. Finally, it calls free() to free the buffer containing the
 * library name. Each time it needs to give control back to the injector
 * process, it breaks back in by executing an "int $3" instruction. See the
 * comments below for more details on how this works.
 *
 */

void injectSharedLibrary(long handle, long dlcloseaddr)
{
	// here are the assumptions I'm making about what data will be located
	// where at the time the target executes this code:
	//
	//   rdi = (previously injected) dynamic library handle
	//   rsi = address of __libc_dlclose() in target process

	asm(
		"callq %rsi \n"
	);

	// we already overwrote the RET instruction at the end of this function
	// with an INT 3, so at this point the injector will regain control of
	// the target's execution.
}

/*
 * injectSharedLibrary_end()
 *
 * This function's only purpose is to be contiguous to injectSharedLibrary(),
 * so that we can use its address to more precisely figure out how long
 * injectSharedLibrary() is.
 *
 */

void injectSharedLibrary_end()
{
}

int main(int argc, char** argv)
{
	if(argc < 4)
	{
		usage(argv[0]);
		return 1;
	}

	char* command = argv[1];
	char* commandArg = argv[2];

	char* strhandle = argv[3];
	long handle = strtol(strhandle, NULL, 16);

	char* processName = NULL;
	pid_t target = 0;

	if(!handle)
	{
		fprintf(stderr, "\tno handle!\n");
		return 1;
	}

	if(!strcmp(command, "-n"))
	{
		processName = commandArg;
		target = findProcessByName(processName);
		if(target == -1)
		{
			fprintf(stderr, "\tdoesn't look like a process named \"%s\" is running right now\n", processName);
			return 1;
		}

		printf("\ttargeting process \"%s\" with pid %d\n", processName, target);
	}
	else if(!strcmp(command, "-p"))
	{
		target = atoi(commandArg);
		printf("\ttargeting process with pid %d\n", target);
	}
	else
	{
		usage(argv[0]);
		return 1;
	}

	int mypid = getpid();
	long mylibcaddr = getlibcaddr(mypid);

	// find the addresses of the syscalls that we'd like to use inside the
	// target, as loaded inside THIS process (i.e. NOT the target process)
	long dlcloseAddr = getFunctionAddress("__libc_dlclose");

	// use the base address of libc to calculate offsets for the syscalls
	// we want to use
	long dlcloseOffset = dlcloseAddr - mylibcaddr;


	// get the target process' libc address and use it to find the
	// addresses of the syscalls we want to use inside the target process
	long targetLibcAddr = getlibcaddr(target);
	long targetDlcloseAddr = targetLibcAddr + dlcloseOffset;

	struct user_regs_struct oldregs, regs;
	memset(&oldregs, 0, sizeof(struct user_regs_struct));
	memset(&regs, 0, sizeof(struct user_regs_struct));

	ptrace_attach(target);

	ptrace_getregs(target, &oldregs);
	memcpy(&regs, &oldregs, sizeof(struct user_regs_struct));

	// find a good address to copy code to
	long addr = freespaceaddr(target) + sizeof(long);

	// now that we have an address to copy code to, set the target's rip to
	// it. we have to advance by 2 bytes here because rip gets incremented
	// by the size of the current instruction, and the instruction at the
	// start of the function to inject always happens to be 2 bytes long.
	regs.rip = addr + 2;

	// pass arguments to my function injectSharedLibrary() by loading them
	// into the right registers. note that this will definitely only work
	// on x64, because it relies on the x64 calling convention, in which
	// arguments are passed via registers rdi, rsi, rdx, rcx, r8, and r9.
	// see comments in injectSharedLibrary() for more details.
	regs.rdi = handle;
	regs.rsi = targetDlcloseAddr;
	ptrace_setregs(target, &regs);

	// figure out the size of injectSharedLibrary() so we know how big of a buffer to allocate.
	size_t injectSharedLibrary_size = (intptr_t)injectSharedLibrary_end - (intptr_t)injectSharedLibrary;

	// also figure out where the RET instruction at the end of
	// injectSharedLibrary() lies so that we can overwrite it with an INT 3
	// in order to break back into the target process. note that on x64,
	// gcc and clang both force function addresses to be word-aligned,
	// which means that functions are padded with NOPs. as a result, even
	// though we've found the length of the function, it is very likely
	// padded with NOPs, so we need to actually search to find the RET.
	intptr_t injectSharedLibrary_ret = (intptr_t)findRet(injectSharedLibrary_end) - (intptr_t)injectSharedLibrary;

	// back up whatever data used to be at the address we want to modify.
	char* backup = malloc(injectSharedLibrary_size * sizeof(char));
	ptrace_read(target, addr, backup, injectSharedLibrary_size);

	// set up a buffer to hold the code we're going to inject into the
	// target process.
	char* newcode = malloc(injectSharedLibrary_size * sizeof(char));
	memset(newcode, 0, injectSharedLibrary_size * sizeof(char));

	// copy the code of injectSharedLibrary() to a buffer.
	memcpy(newcode, injectSharedLibrary, injectSharedLibrary_size - 1);
	// overwrite the RET instruction with an INT 3.
	newcode[injectSharedLibrary_ret] = INTEL_INT3_INSTRUCTION;

	// copy injectSharedLibrary()'s code to the target address inside the
	// target process' address space.
	ptrace_write(target, addr, newcode, injectSharedLibrary_size);

	// now that the new code is in place, let the target run our injected
	// code.
	ptrace_cont(target);

	// check out what the registers look like after calling dlopen.
	struct user_regs_struct dlopen_regs;
	memset(&dlopen_regs, 0, sizeof(struct user_regs_struct));
	ptrace_getregs(target, &dlopen_regs);
	unsigned long long ret = dlopen_regs.rax;

	// if rax is 0 here, then __libc_dlclose_mode failed, and we should bail
	// out cleanly.
	if(ret == 0)
	{
		printf("\t__libc_dlclose() unloaded handle 0x%x\n", handle);
		restoreStateAndDetach(target, addr, backup, injectSharedLibrary_size, oldregs);
		free(backup);
		free(newcode);
		return 1;
	}

	// at this point, if everything went according to plan, we've loaded
	// the shared library inside the target process, so we're done. restore
	// the old state and detach from the target.
	restoreStateAndDetach(target, addr, backup, injectSharedLibrary_size, oldregs);
	free(backup);
	free(newcode);

	return 0;
}
