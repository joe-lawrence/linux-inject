#
# upatch specifics
#

UPATCH=cve_2015_1234
TRAMPOLINES := \
	/usr/lib64/libdns.so.100 dns_tkey_processquery
PRIV_FUNCS := \
	/usr/bin/postgres chr chrnamed \

PATCH_SRC=patches.c

patches.o: patches.c upatch_private.h
	# build line for patches.o

#
# upatch boilerplate
#

.DEFAULT_GOAL:=all
all: libupatch_$(UPATCH).so

CFLAGS=-fPIC -Wall -ggdb -MP -MD

libupatch_$(UPATCH).so: $(PATCH_SRC:.c=.o) trampolines.o
	$(CC) -shared -lupatch -lsystemd -Wl,-soname,$@ -o $@ $^

-include trampolines.d

trampolines.c:
	../gen_trampolines ${TRAMPOLINES} > $@

upatch_private.h:
	../gen_priv_funcs ${PRIV_FUNCS} > $@

install: all
	install -D libupatch_$(UPATCH).so /usr/lib64
	ldconfig -n /usr/lib64

uninstall:
	$(RM) /usr/lib64/libupatch_$(UPATCH).so
	ldconfig -n /usr/lib64

clean:
	$(RM) -f trampolines.c upatch_private.h *.d *.o *.so

.PHONY: install uninstall clean
