# Building Y_ and YXX projects with Make

This guide shows how to use the native compiler commands from an application
Makefile. It assumes the Y_ repository has already been built and that
`gyy` and/or `gy++` can be found through your shell's `PATH`.

## Build the compiler tools

From the root of the Y_ repository:

```sh
make
```

This builds `ylang`, `gyy`, and `gy++`. On MSYS2, install a Clang toolchain
that provides both `clang` and `clang++`, and make sure their MSYS2
`/clang64/bin` or `/mingw64/bin` directory is on `PATH`, matching the
environment in which you installed Clang.

## Important: two kinds of flags

- `CC` and `CFLAGS` are used by Make to build the compiler programs
  themselves (and the interpreter).
- `GYFLAGS` are passed to `gyy` when it compiles your `.yc` application.
- `GYXXFLAGS` are passed to `gy++` when it compiles your `.yxx` application.

The Y_ compiler flags and YXX compiler flags are independent. They are not
aliases for `CFLAGS` or `CXXFLAGS`.

## Y_ application Makefile

Create a Makefile beside your `.yc` files:

```make
GY ?= gyy
GYFLAGS ?= --warn-all --warn-extra --optimize=2

TARGET := myapp
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

Run it with:

```sh
make
make run
make clean
```

The first `.yc` source passed to `gyy` is the entry program. Additional
`.yc`, `.hly`, and `.yaq` files are bundled as modules. Put the entry
file first in `SOURCES`.

Override flags for one build:

```sh
make GYFLAGS="--warn-all --warnings-as-errors --optimize=3"
```

## YXX application Makefile

Create a Makefile beside your `.yxx` files:

```make
GYXX ?= gy++
GYXXFLAGS ?= --xx-warn-all --xx-warn-extra --xx-optimize=2

TARGET := myapp
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

Run it with:

```sh
make
make run
make clean
```

The first `.yxx` file is the YXX entry program. Put it first in
`SOURCES`. Supporting `.yc`, `.hly`, and `.yaq` modules can follow.

Override YXX flags independently:

```sh
make GYXXFLAGS="--xx-warn-all --xx-warnings-as-errors --xx-optimize=3"
```

## Native compiler flag reference

| Purpose | Y_ / `gyy` | YXX / `gy++` |
|---|---|---|
| No optimization | `--optimize=0` | `--xx-optimize=0` |
| Optimize | `--optimize=2` | `--xx-optimize=2` |
| Maximum optimization | `--optimize=3` | `--xx-optimize=3` |
| Debug information | `--debug-info` | `--xx-debug-info` |
| Common warnings | `--warn-all` | `--xx-warn-all` |
| Extra warnings | `--warn-extra` | `--xx-warn-extra` |
| Pedantic warnings | `--warn-pedantic` | `--xx-warn-pedantic` |
| Warnings are errors | `--warnings-as-errors` | `--xx-warnings-as-errors` |
| Clear warning choices | `--warn-none` | `--xx-warn-none` |
| Disable one warning | `--warn-no=NAME` | `--xx-warn-no=NAME` |
| Promote one warning | `--warn-error=NAME` | `--xx-warn-error=NAME` |
| Choose Clang driver | `--clang PATH` | `--clang++ PATH` |

Both commands also accept common GCC-style aliases such as `-O2`, `-Wall`,
`-Wextra`, `-Wpedantic`, `-Werror`, and `-g` for compatibility. Prefer
the language-specific long options in project Makefiles so the intent is
obvious.

## Selecting Clang on MSYS2

If Clang is not on `PATH`, pass the executable explicitly:

```make
GYFLAGS += --clang /clang64/bin/clang.exe
GYXXFLAGS += --clang++ /clang64/bin/clang++.exe
```

Or set the environment variables before running Make:

```sh
export LLVM_CLANG=/clang64/bin/clang.exe
export LLVM_CLANGXX=/clang64/bin/clang++.exe
```

Use the paths for the MSYS2 environment where you installed the packages.
For example, the CLANG64 package prefix is commonly `/clang64/bin`.

## Current backend note

The native executables bundle the Y_ source modules with the existing Y_
interpreter/runtime. The current YXX command uses the `clang++` driver to
build and link that runtime-backed executable; it is not yet a direct YXX to
LLVM IR compiler or a C++ language frontend.
