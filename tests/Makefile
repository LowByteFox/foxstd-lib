CC = c99
CFLAGS = -g -I../include -L../lib
LDFLAGS = -lfoxstd

TESTS = alloc vec

all: $(TESTS)

$(TESTS):
	$(CC) $(CFLAGS) $@.c -o ../bin/$@ $(LDFLAGS)

.PHONY: all
