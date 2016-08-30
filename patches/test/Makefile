# CFLAGS="-O -fPIE -fstack-protector-all -D_FORTIFY_SOURCE=2" 
# LDFLAGS="-Wl,-z,now -Wl,-z,relro"

CFLAGS=-Wall -ggdb -MP -MD

-include $(SRC:.cpp=.d)


all: test

testlib.o: testlib.c
	gcc $(CFLAGS) -c -fPIC -o testlib.o testlib.c

libtest.so: testlib.o
	gcc $(CFLAGS) -shared -Wl,-soname,libtest.so -o libtest.so testlib.o

test: test.c libtest.so
	gcc $(CFLAGS) -o test test.c -L. -ltest

clean:
	$(RM) -f test *.d *.o *.so
