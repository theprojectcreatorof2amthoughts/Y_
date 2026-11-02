CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic

.PHONY: all clean test libraries FORCE

LIBRARY_FILES := $(shell find . -type f -name '*.yl' -not -path './.git/*' -not -path './build/*' | LC_ALL=C sort)
EMBEDDER := build/embed_yl
EMBEDDED_SOURCE := build/embedded_libraries.c

all: y_

test: y_
	bash tests/run.sh

libraries: $(EMBEDDED_SOURCE)

FORCE:

$(EMBEDDER): tools/embed_yl.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $<

$(EMBEDDED_SOURCE): FORCE $(EMBEDDER) src/embedded_libraries.h
	$(EMBEDDER) $@.tmp $(LIBRARY_FILES)
	@if cmp -s $@.tmp $@; then rm -f $@.tmp; else mv $@.tmp $@; fi

y_: src/main.c src/y_.c src/memory.c src/file_io.c src/y_.h src/memory.h src/file_io.h src/embedded_libraries.h $(EMBEDDED_SOURCE)
	$(CC) $(CFLAGS) -Isrc -o $@ src/main.c src/y_.c src/memory.c src/file_io.c $(EMBEDDED_SOURCE) -lm

clean:
	rm -f y_ $(EMBEDDER) $(EMBEDDED_SOURCE)