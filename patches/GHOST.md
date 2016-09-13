# upatch demo - CVE-2015-0235 aka GHOST
## Proof-of-concept for patching a running process

A heap-based buffer overflow was found in glibc's
```__nss_hostname_digits_dots()``` function, which is used by the
```mgethostbyname()``` and ```gethostbyname2()``` glibc function calls. A
remote attacker able to make an application call either of these functions
could use this flaw to execute arbitrary code with the permissions of the user
running the application.

In this demo, we will build a shared library patch that un-patches glibc.  We
do this backwards as a matter of convenience -- it's easier to patch the
currently installed glibc than to install or downgrade to a vulnerable
version.  The same mechanics would apply if we were actually patching a
vulnerability.


## Step 0 - Find a vulnerability detection program

Qualys, who reported the bug, introduced a small test program called GHOST.c
that checks whether a system is vulnerable or not.  See the source in section
4 - Case Studies from their [full
report](http://www.openwall.com/lists/oss-security/2015/01/27/9).

I modified GHOST.c to continuously loop, performing its check and then
sleeping for a few seconds.  This test program will be targeted to patch.  On
modern RHEL7, it should report "not vulnerable".  This demo will un-patch the
GHOST fix from this process, after which it should report "vulnerable".


## Step 1 - Build glibc

Download and build the glibc package so we can steal the compilation
arguments.  Here's the rpmbuild build line for ```digits_dots.c```:

	gcc \
		digits_dots.c \
		-c \
		-std=gnu99 \
		-fgnu89-inline \
		 \
		-DNDEBUG \
		-O3 \
		-Wall \
		-Winline \
		-Wwrite-strings \
		-fasynchronous-unwind-tables \
		-fmerge-all-constants \
		-frounding-math \
		-g \
		-mtune=generic \
		-Wstrict-prototypes \
		-I../include \
		-I/root/rpmbuild/BUILD/glibc-2.17-c758a686/build-x86_64-redhat-linux/nss \
		-I/root/rpmbuild/BUILD/glibc-2.17-c758a686/build-x86_64-redhat-linux \
		-I../sysdeps/unix/sysv/linux/x86_64/64/nptl \
		-I../sysdeps/unix/sysv/linux/x86_64/64 \
		-I../nptl/sysdeps/unix/sysv/linux/x86_64 \
		-I../nptl/sysdeps/unix/sysv/linux/x86 \
		-I../sysdeps/unix/sysv/linux/x86 \
		-I../rtkaio/sysdeps/unix/sysv/linux/x86_64 \
		-I../sysdeps/unix/sysv/linux/x86_64 \
		-I../sysdeps/unix/sysv/linux/wordsize-64 \
		-I../ports/sysdeps/unix/sysv/linux \
		-I../nptl/sysdeps/unix/sysv/linux \
		-I../nptl/sysdeps/pthread \
		-I../rtkaio/sysdeps/pthread \
		-I../sysdeps/pthread \
		-I../rtkaio/sysdeps/unix/sysv/linux \
		-I../sysdeps/unix/sysv/linux \
		-I../sysdeps/gnu \
		-I../sysdeps/unix/inet \
		-I../ports/sysdeps/unix/sysv \
		-I../nptl/sysdeps/unix/sysv \
		-I../rtkaio/sysdeps/unix/sysv \
		-I../sysdeps/unix/sysv \
		-I../sysdeps/unix/x86_64 \
		-I../ports/sysdeps/unix \
		-I../nptl/sysdeps/unix \
		-I../rtkaio/sysdeps/unix \
		-I../sysdeps/unix \
		-I../sysdeps/posix \
		-I../nptl/sysdeps/x86_64/64 \
		-I../sysdeps/x86_64/64 \
		-I../sysdeps/x86_64/fpu/multiarch \
		-I../sysdeps/x86_64/fpu \
		-I../sysdeps/x86/fpu \
		-I../sysdeps/x86_64/multiarch \
		-I../nptl/sysdeps/x86_64 \
		-I../sysdeps/x86_64 \
		-I../sysdeps/x86 \
		-I../sysdeps/ieee754/ldbl-96 \
		-I../sysdeps/ieee754/dbl-64/wordsize-64 \
		-I../sysdeps/ieee754/dbl-64 \
		-I../sysdeps/ieee754/flt-32 \
		-I../sysdeps/wordsize-64 \
		-I../sysdeps/ieee754 \
		-I../sysdeps/generic \
		-I../ports \
		-I../nptl \
		-I../rtkaio \
		-I.. \
		-I../libio \
		-I. \
		-nostdinc \
		-isystem \
		/usr/lib/gcc/x86_64-redhat-linux/4.8.5/include \
		-isystem \
		/usr/include \
		-D_LIBC_REENTRANT \
		-include \
		/root/rpmbuild/BUILD/glibc-2.17-c758a686/build-x86_64-redhat-linux/libc-modules.h \
		-DMODULE_NAME=libc \
		-include \
		../include/libc-symbols.h

In this demo, I adjusted all of the include paths accordingly.  A few other
build options I needed to change:

1. ```-fPIC``` - our changes will end up in a shared library, so we need to
generate position independent code.
2. Remove the ```-DMODULE_NAME=libc``` definition, else dlopen was throwing a
```undefined symbol: __libc_tsd_CTYPE_B``` error when attempting to load the
shared library.


## Step 2 - Download glibc git tree

Grab a copy of the upstream git tree so we build an unpatched version of
```digits_dots.c``` pre-GHOST fix.

The GHOST vulnerability was fixed by the following commit:

[d5dd6189d506 Fix parsing of numeric hosts in
gethostbyname_r](https://sourceware.org/git/?p=glibc.git;a=commit;h=d5dd6189d506068ed11c8bfa1e1e9bffde04decd)

So we'll be building the shared library with ```d5dd6189d506~1:nss/digits_dots.c```


## Step 3 - Adjust the patches Makefile

Add the giant gcc line from the rpmbuild to the Makefile with the necessary
updates (```-fPIC```, no ```-DMODULE_NAME=libc```, and updated include paths).

Also update the Makefile's ```TRAMPOLINES``` list to include:

	TRAMPOLINES := \
        	/usr/lib64/libc.so.6 __nss_hostname_digits_dots

The build will auto-generate ```trampolines.c``` with the glibc's build id and
```__nss_hostname_digits_dots``` offsets and size:

	#include <stdlib.h>
	#include "trampoline.h"

	extern void *patched___nss_hostname_digits_dots;

	struct trampolines tramps[] = {

	        { .map_name = "/usr/lib64/libc-2.17.so",
	          .build_id = "4ec510505f71aeacdf5a4035bec5b39afbb65538",
	          .oldname  = "__nss_hostname_digits_dots",
	          .offset   = 0x000000000010a4c0,
	          .old_size = 0x0000000000000522,
	          .new_addr = &patched___nss_hostname_digits_dots,
	          .type     = T_OFFSET_REL, },

		{ 0 } };


## Step 4 - Shared library dry-run

The [linux-inject](https://github.com/gaffe23/linux-inject) project was
supplemented with a feature that attempts to dlopen the shared library patch
with the ```RTLD_NOW``` flag set.  This instructs the dynamic linker to
resolve all undefined symbols at load time.

In this case, ```patched___nss_hostname_digits_dots``` calls
```__inet_aton```, which happens to be an internal glibc symbol.

There are a few ways to solve this issue:

1. Use public interfaces were available (```inet_aton``` for example)
2. Function pointers
3. Carry our own version

For demonstration purposes, the third solution was simple enough.  Luckily
```__inet_aton``` did not call any other glibc private routines, so a simple
copy-paste from ```resolv/inet_addr.c``` was sufficient.  All other functions
that ```__inet_aton``` called (```isdigit```, ```isspace```, etc) are exported
by glibc, so the dynamic linker happily resolved those.


## Step 5 - Success!

After injecting the shared library "un-patch" to the running GHOST process,
it's report changed from "not vulnerable" to "vulnerable".

Here's a .gif capture of the entire process, sans rpmbuild and git tree
cloning:

![ghost_demo](ghost_demo.gif)


## Step 6 - What's next

Although this demo was successful, it remains to be seen how useful the shared
library injection may be used against real-life vulnerabilities and running
processes.  On a RHEL7 system, I had no success injecting the GHOST shared
library un-patch into any binaries that call ```gethostbyname```:

	% for f in /sbin/*; do objdump $f -DS 2>/dev/null | grep -q gethostbyname && echo "$(basename $f) - $(pgrep $(basename $f))"; done

	arp
	auditd - 303 804
		ptrace(PTRACE_ATTACH) failed
	
		instead of expected SIGTRAP, target stopped with signal 11: Segmentation fault
		sending process 804 a SIGSTOP signal for debugging purposes
	
	aureport
	ausearch
	dhclient - 915
		__libc_dlopen_mode() failed
	
	ether-wake
	exportfs
	ifconfig
	mount.nfs
	mount.nfs4
	ntpd
	postalias
	postconf
	postmap
	postqueue
	pppd
	route
	rpc.mountd
	rsyslogd - 1194
		__libc_dlopen_mode() failed
	
	sendmail - 2358 2382
		__libc_dlopen_mode() failed
	
	sendmail.sendmail
	ss
	sshd - 1300 3452 7141 28805 28863 29367 29400 30434
		__libc_dlopen_mode() failed
	
	tcpd
	tcpdmatch
	tcsd
	try-from
	umount.nfs
	umount.nfs4

This highlights the number of potential points of failure in this technique:

1. Target process must be ptrace-able to change code, get signal info
2. Must be able to call dlopen from target
3. Shared library patch must be able to patch locations
4. ???

For now, the next step is to modify
[linux-inject](https://github.com/gaffe23/linux-inject) to also call
```dlerror``` so we can get some visibility into why ```dlopen``` consistently
fails against real world programs.


## Step 6 - Update

After a bit of debugging (I attached to a running dhclient, then from gdb
manually called dlopen and dlerror) I figured out that SELinux was preventing
me these processes from opening the shared library patch.  With apologies to
Dan Walsh, I set SELinux to permissive mode and viola -- I could inject the
GHOST vulnerability .so and verify it in each processes' /proc/PID/maps file.

(Note: auditd still fails the ptrace even with SELinux turned off).
