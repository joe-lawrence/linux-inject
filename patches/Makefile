CFLAGS=-fPIC -Wall -ggdb -MP -MD

-include $(SRC:.cpp=.d)

all: libupatch.so test

libupatch.so: constructor.o patches.o utils.o trampolines.o
	$(CC) -shared -lunwind -Wl,-soname,$@ -o $@ $^

constructor.o: constructor.c
	$(CC) $(CFLAGS) -c -o $@ $<

utils.o: utils.c
	$(CC) $(CFLAGS) -c -o $@ $<

patches.o: patches.c trampolines.c
	$(CC) \
	$< \
	-fPIC \
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
	 \
	 \
	 \
	 \
	 \
	 \
	 \
	 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/include \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/build-x86_64-redhat-linux/nss \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/build-x86_64-redhat-linux \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/unix/sysv/linux/x86_64/64/nptl \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/unix/sysv/linux/x86_64/64 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/nptl/sysdeps/unix/sysv/linux/x86_64 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/nptl/sysdeps/unix/sysv/linux/x86 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/unix/sysv/linux/x86 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/rtkaio/sysdeps/unix/sysv/linux/x86_64 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/unix/sysv/linux/x86_64 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/unix/sysv/linux/wordsize-64 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/ports/sysdeps/unix/sysv/linux \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/nptl/sysdeps/unix/sysv/linux \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/nptl/sysdeps/pthread \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/rtkaio/sysdeps/pthread \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/pthread \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/rtkaio/sysdeps/unix/sysv/linux \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/unix/sysv/linux \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/gnu \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/unix/inet \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/ports/sysdeps/unix/sysv \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/nptl/sysdeps/unix/sysv \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/rtkaio/sysdeps/unix/sysv \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/unix/sysv \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/unix/x86_64 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/ports/sysdeps/unix \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/nptl/sysdeps/unix \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/rtkaio/sysdeps/unix \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/unix \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/posix \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/nptl/sysdeps/x86_64/64 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/x86_64/64 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/x86_64/fpu/multiarch \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/x86_64/fpu \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/x86/fpu \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/x86_64/multiarch \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/nptl/sysdeps/x86_64 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/x86_64 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/x86 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/ieee754/ldbl-96 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/ieee754/dbl-64/wordsize-64 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/ieee754/dbl-64 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/ieee754/flt-32 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/wordsize-64 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/ieee754 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/sysdeps/generic \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/ports \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/nptl \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/rtkaio \
	 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686 \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/libio \
	-I/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/nss \
	-nostdinc \
	-isystem \
	/usr/lib/gcc/x86_64-redhat-linux/4.8.5/include \
	-isystem \
	/usr/include \
	 \
	-D_LIBC_REENTRANT \
	-include \
	/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/build-x86_64-redhat-linux/libc-modules.h \
	-include \
	/tmp/rpmbuild/BUILD/glibc-2.17-c758a686/include/libc-symbols.h \
	 \
	 \
	 \
	 \
	 \
	 \
	-o \
	$@

TRAMPOLINES := \
	/usr/lib64/libc.so.6 __nss_hostname_digits_dots

trampolines.c:
	./gen_trampolines ${TRAMPOLINES} > $@

test:
	$(MAKE) -C $@

clean:
	$(RM) -f trampolines.c *.d *.o *.so
	$(MAKE) -C test/ clean

.PHONY: test
