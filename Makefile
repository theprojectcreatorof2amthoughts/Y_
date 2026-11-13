CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic

.PHONY: all clean libraries FORCE install

LIBRARY_FILES := $(shell find . -type f -name '*.hly' -not -path './.git/*' -not -path './build/*' | LC_ALL=C sort)
EMBEDDER := build/embed_hly
EMBEDDED_SOURCE := build/embedded_libraries.c

all: ylang gyy
libraries: $(EMBEDDED_SOURCE)
FORCE:

$(EMBEDDER): tools/embed_hly.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $<

$(EMBEDDED_SOURCE): FORCE $(EMBEDDER) src/embedded_libraries.h
	$(EMBEDDER) $@.tmp $(LIBRARY_FILES)
	@if cmp -s $@.tmp $@; then rm -f $@.tmp; else mv $@.tmp $@; fi

gyy: compiler/gyy.c src/y_.c src/y_.h src/memory.c src/memory.h src/file_io.c src/file_io.h $(EMBEDDED_SOURCE)
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc -DGY_Y_PROJECT_ROOT='"$(CURDIR)"' -DGY_Y_EMBEDDED_SOURCE='"$(CURDIR)/$(EMBEDDED_SOURCE)"' -o $@ compiler/gyy.c src/y_.c src/memory.c src/file_io.c $(EMBEDDED_SOURCE) -lm

ylang: src/main.c src/y_.c src/memory.c src/file_io.c src/y_.h src/memory.h src/file_io.h src/embedded_libraries.h $(EMBEDDED_SOURCE)
	$(CC) $(CFLAGS) -Isrc -o $@ src/main.c src/y_.c src/memory.c src/file_io.c $(EMBEDDED_SOURCE) -lm

clean:
	rm -f ylang y_ gyy $(EMBEDDER) $(EMBEDDED_SOURCE)
