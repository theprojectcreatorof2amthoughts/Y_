# gyy: Y_ native compiler

`gyy` accepts one Y_ entry file plus any number of Y_ modules. Source files may use
`.yc`, `.hly`, or `.yaq`. It compiles the whole set into one native executable.

```sh
make
gyy main.yc math.yc strings.hly tools.yaq -O2 -o app
./app
```

The first `.yc` file on the command line is the entry program. Any additional `.yc` files, along with `.hly` and `.yaq` files, are loaded as
modules into the same global environment **before** the entry program runs. This
allows the entry file to call procedures defined in any linked module. Module
files are embedded in the executable; they do not need to be present when the
executable starts. Existing `!#libinc` and `!#adeq` directives still load their
explicitly named external files at runtime. `!#inclib` continues to use the HLY
libraries embedded when the Y_ project is built.

`gyy` also accepts one textual LLVM IR `.ll` file by itself.

## Diagnostics and warning options

Diagnostics use GCC-style locations, for example
`source.yc:12:1: error: division by zero`. Clang's native warnings are shown
with source locations and warning option names. Supported options include
`-Wall`, `-Wextra`, `-Wpedantic`, `-Werror`, `-Wno-WARNING`, and
`-Werror=WARNING`, plus `-g` and `-O0` through `-O3`. `--clang PATH` or
the `LLVM_CLANG` environment variable selects Clang.

The current backend packages Y_ source and the existing interpreter/runtime into
a native executable. It is a native executable build with module bundling, not
yet a direct Y_-to-LLVM-IR frontend.


## gy++: C++-toolchain variant

`gy++` is built from the same driver implementation as `gyy` and supports the
same Y_ entry/module inputs, shared module environment, embedded source bundling,
standalone textual LLVM IR input, output naming, optimization levels, debug info,
and warning controls. It selects `clang++` by default, accepts
`--clang++ PATH`, and reads `LLVM_CLANGXX` when set.

```sh
make gy++
gy++ main.yc math.yc tools.yaq -Wall -Wextra -O2 -o app
./app
```

This makes gy++ the C++-toolchain counterpart for building Y_ programs; it does
not turn Y_ syntax into C++ or add a C++ foreign-function interface. The Y_
runtime and generated launcher remain C sources and are compiled by the selected
LLVM driver as C translation units before C++-toolchain linking.
