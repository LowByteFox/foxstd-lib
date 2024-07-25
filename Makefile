AR = ar
CC = c99
CFLAGS = -g -Iinclude

OBJS = src/alloc.o src/vec.o src/utils.o

all: lib/libfoxstd.a

tests: all
	@mkdir -p bin
	$(MAKE) -C tests

lib/libfoxstd.a: $(OBJS)
	@mkdir -p lib
	$(AR) rcs $@ $(OBJS)

clean:
	rm -rf lib bin $(OBJS) *.foxstd

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

.PHONY: all clean tests
.SUFFIXES: .c
