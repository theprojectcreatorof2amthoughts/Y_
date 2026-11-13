# Makefile guide: Y_

Use `gyy` to compile a `.yc` entry file into a native executable.

```make
GY ?= gyy
GYFLAGS ?= --warn-all --warn-extra --optimize=2

TARGET := program
SOURCES := main.yc math.yc helpers.yaq
LIBRARIES := strings.hly

.PHONY: all run clean
all: $(TARGET)

$(TARGET): $(SOURCES) $(LIBRARIES)
	$(GY) $(SOURCES) $(LIBRARIES) $(GYFLAGS) -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
```

The first `.yc` file must be the entry source. Additional `.yc`, `.hly`,
and `.yaq` files are bundled as modules.

## Y_ native flags

- `--optimize=0..3`: optimization level.
- `--debug-info`: include debug information.
- `--warn-all`: enable common warnings.
- `--warn-extra`: enable additional warnings.
- `--warn-pedantic`: enable pedantic warnings.
- `--warnings-as-errors`: treat warnings as errors.
- `--warn-none`: clear selected warnings.
- `--warn-no=NAME`: disable a warning.
- `--warn-error=NAME`: promote a warning to an error.
- `--clang PATH`: choose the native Clang driver.

Override flags at the command line:

```sh
make GYFLAGS="--warn-all --warnings-as-errors --optimize=3"
```

The native driver is selected with `LLVM_CLANG` or `--clang PATH`.
