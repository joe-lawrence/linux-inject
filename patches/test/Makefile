# CFLAGS="-O -fPIE -fstack-protector-all -D_FORTIFY_SOURCE=2"
# LDFLAGS="-Wl,-z,now -Wl,-z,relro"

CFLAGS=-Wall -ggdb -MP -MD

-include $(SRC:.cpp=.d)


all: test test3 GHOST

testlib.o: testlib.c
	gcc $(CFLAGS) -c -fPIC -o testlib.o testlib.c

libtest.so: testlib.o
	gcc $(CFLAGS) -shared -Wl,-soname,libtest.so -o libtest.so testlib.o

test: test.c libtest.so
	gcc $(CFLAGS) -o test test.c -L. -ltest

test3: test3.c libtest.so
	gcc $(CFLAGS) -o test3 test3.c -lpthread

GHOST: GHOST.c
	gcc GHOST.c -o GHOST

clean:
	$(RM) -f test test3 GHOST *.d *.o *.so
