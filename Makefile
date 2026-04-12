CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

all: test

ephemeral.o: ephemeral.c ephemeral.h
	$(CC) $(CFLAGS) -c ephemeral.c -o ephemeral.o

test_ephemeral: test_ephemeral.c ephemeral.o ephemeral.h
	$(CC) $(CFLAGS) test_ephemeral.c ephemeral.o -o test_ephemeral

test: test_ephemeral
	./test_ephemeral

clean:
	rm -f *.o test_ephemeral

.PHONY: all test clean
