CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic

.PHONY: all clean test

all: y_

test: y_
	bash tests/run.sh

y_: src/main.c src/y_.c src/memory.c src/file_io.c src/y_.h src/memory.h src/file_io.h
	$(CC) $(CFLAGS) -o $@ src/main.c src/y_.c src/memory.c src/file_io.c -lm

clean:
	rm -f y_