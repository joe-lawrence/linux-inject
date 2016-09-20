CFLAGS=-fPIC -Wall -ggdb -MP -MD
SRC=constructor.c utils.c

-include $(SRC:.c=.d)

all: libupatch.so

libupatch.so: $(SRC:.c=.o)
	$(CC) -shared -lsystemd -lunwind -Wl,-soname,$@ -o $@ $^

install:
	install -D libupatch.so /usr/lib64
	install -D upatch.h /usr/include/upatch.h
	ldconfig -n /usr/lib64

uninstall:
	$(RM) /usr/lib64/libupatch.so
	ldconfig -n /usr/lib64

clean:
	$(RM) -f *.d *.o *.so

.PHONY: install uninstall clean
