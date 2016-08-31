CFLAGS=-fPIC -Wall -ggdb -MP -MD

-include $(SRC:.cpp=.d)

all: libupatch.so libupatch2.so libupatch3.so test

libupatch.so: constructor.o patches.o utils.o
	$(CC) -shared -lunwind -Wl,-soname,$@ -o $@ $^

libupatch2.so: constructor.o patches2.o utils.o
	$(CC) -shared -lunwind -Wl,-soname,$@ -o $@ $^

libupatch3.so: constructor.o patches3.o utils.o
	$(CC) -shared -lunwind -Wl,-soname,$@ -o $@ $^

constructor.o: constructor.c
	$(CC) $(CFLAGS) -c -o $@ $<

utils.o: utils.c
	$(CC) $(CFLAGS) -c -o $@ $<

patches2.o: patches2.c
	$(CC) $(CFLAGS) -c -o $@ $<

patches.o: patches.c
	$(CC) $(CFLAGS) -c -o $@ $<

test:
	$(MAKE) -C $@

clean:
	$(RM) -f *.d *.o *.so
	$(MAKE) -C test/ clean

.PHONY: test
