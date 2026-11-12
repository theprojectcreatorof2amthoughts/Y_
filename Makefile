CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic

.PHONY: all clean libraries FORCE

LIBRARY_FILES := $(shell find . -type f -name '*.hly' -not -path './.git/*' -not -path './build/*' | LC_ALL=C sort)
EMBEDDER := build/embed_hly
EMBEDDED_SOURCE := build/embedded_libraries.c

all: y_

libraries: $(EMBEDDED_SOURCE)

FORCE:

$(EMBEDDER): tools/embed_hly.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $<

$(EMBEDDED_SOURCE): FORCE $(EMBEDDER) src/embedded_libraries.h
	$(EMBEDDER) $@.tmp $(LIBRARY_FILES)
	@if cmp -s $@.tmp $@; then rm -f $@.tmp; else mv $@.tmp $@; fi

y_: src/main.c src/y_.c src/memory.c src/file_io.c src/y_.h src/memory.h src/file_io.h src/embedded_libraries.h $(EMBEDDED_SOURCE)
	$(CC) $(CFLAGS) -Isrc -o $@ src/main.c src/y_.c src/memory.c src/file_io.c $(EMBEDDED_SOURCE) -lm

clean:
	rm -f y_ $(EMBEDDER) $(EMBEDDED_SOURCE)
