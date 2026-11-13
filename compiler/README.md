# gyy: Y_ native compiler

`gyy` accepts one Y_ entry file plus any number of Y_ modules. Source files may use
`.yc`, `.hly`, or `.yaq`. It compiles the whole set into one native executable.

```sh
make
gyy --check main.yc math.yc strings.hly tools.yaq
gyy main.yc math.yc strings.hly tools.yaq --optimize=2 --warn-all -o app
./app

make test-diagnostics
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

`gyy --check file.yc` parses source without executing it or invoking Clang. When it encounters independent syntax errors, it attempts to recover at the next Y_ statement and reports multiple GCC-style diagnostics, such as `source.yc:12:1: error: ...`. The check mode accepts linked `.yc`, `.hly`, and `.yaq` files and requires at least one `.yc` entry file.

Clang's native warnings are shown with source locations and warning option names. Supported Clang options include `-Wall`, `-Wextra`, `-Wpedantic`, `-Werror`, `-Wno-WARNING`, and `-Werror=WARNING`, plus `-g` and `-O0` through `-O3`. `--clang PATH` or the `LLVM_CLANG` environment variable selects Clang. Y_ also provides compiler-native aliases: `--warn-all` enables `-Wall`, `--warn-extra` enables `-Wextra`, `--optimize=0` through `--optimize=3` select optimization levels, and `--debug-info` includes debug symbols. These are `gyy` options, separate from `gy++`'s `--xx-*` options.

The current backend packages Y_ source and the existing interpreter/runtime into
a native executable. It is a native executable build with module bundling, not
yet a direct Y_-to-LLVM-IR frontend.
