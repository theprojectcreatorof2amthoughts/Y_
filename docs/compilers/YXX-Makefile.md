# Makefile guide: YXX

Use `gy++` to compile a `.yxx` entry file into a native executable.

```make
GYXX ?= gy++
GYXXFLAGS ?= --xx-warn-all --xx-warn-extra --xx-optimize=2

TARGET := program
SOURCES := main.yxx math.yc helpers.yaq
LIBRARIES := strings.hly

.PHONY: all run clean
all: $(TARGET)

$(TARGET): $(SOURCES) $(LIBRARIES)
	$(GYXX) $(SOURCES) $(LIBRARIES) $(GYXXFLAGS) -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
```

The first `.yxx` file must be the entry source. Supporting `.yc`, `.hly`,
and `.yaq` files can follow it.

## YXX native flags

- `--xx-optimize=0..3`: optimization level.
- `--xx-debug-info`: include debug information.
- `--xx-warn-all`: enable common warnings.
- `--xx-warn-extra`: enable additional warnings.
- `--xx-warn-pedantic`: enable pedantic warnings.
- `--xx-warnings-as-errors`: treat warnings as errors.
- `--xx-warn-none`: clear selected warnings.
- `--xx-warn-no=NAME`: disable a warning.
- `--xx-warn-error=NAME`: promote a warning to an error.
- `--clang++ PATH`: choose the native Clang++ driver.

Override flags without editing the Makefile:

```sh
make GYXXFLAGS="--xx-warn-all --xx-warnings-as-errors --xx-optimize=3"
```

The C++ driver is selected with `LLVM_CLANGXX` or `--clang++ PATH`.
The current backend uses the Clang++ driver to link the Y_ interpreter/runtime;
it does not yet compile YXX directly to LLVM IR.
