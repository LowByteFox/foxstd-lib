AR = ar
CC = c99
CFLAGS = -g -Iinclude

OBJS = src/alloc.o

all: lib/libfoxstd.a test

test: lib/libfoxstd.a test.o
	$(CC) $(CFLAGS) -o $@ test.o -Llib -lfoxstd

lib/libfoxstd.a: $(OBJS)
	@mkdir -p lib
	$(AR) rcs $@ $(OBJS)

clean:
	rm -rf test lib $(OBJS) test.o *.foxstd

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

.SUFFIXES: .c
