# Y_ and YXX native compilers

## gyy — Y_ compiler

`gyy` accepts one `.yc` entry file plus any number of `.yc`, `.hly`, and
`.yaq` modules. It bundles the sources into one native executable using Clang
and the existing Y_ runtime.

```sh
make
gyy main.yc math.yc strings.hly tools.yaq --warn-all --warn-extra --optimize=2 -o app
./app
```

The first `.yc` file on the command line is the entry program. Additional
source modules are loaded into the same global environment before the entry
program runs. Module files are embedded in the executable. Existing
`!#libinc` and `!#adeq` directives still load their explicitly named external
files at runtime. `!#inclib` continues to use HLY libraries embedded when the
Y_ project is built.

### Native gyy flags

| Flag | Meaning |
|---|---|
| `--optimize=0..3` | Select optimization level |
| `--debug-info` | Include debug information |
| `--warn-all` | Enable common warnings |
| `--warn-extra` | Enable additional warnings |
| `--warn-pedantic` | Enable pedantic warnings |
| `--warnings-as-errors` | Treat warnings as errors |
| `--warn-none` | Clear warning selections |
| `--warn-no=NAME` | Disable a named warning |
| `--warn-error=NAME` | Treat a named warning as an error |
| `--clang PATH` | Select the Clang executable |

The familiar GCC-style aliases `-O0..-O3`, `-g`, `-Wall`, `-Wextra`,
`-Wpedantic`, `-Werror`, `-Wno-NAME`, and `-Werror=NAME` remain accepted.
Set `LLVM_CLANG` to choose the default Clang executable.

## gy++ — YXX compiler

`gy++` is the YXX compiler command. Its entry source extension is `.yxx`;
it accepts `.yc`, `.hly`, and `.yaq` files as supporting modules. The first
`.yxx` file on the command line is the entry program. It uses `clang++` as
the native driver, explicitly compiles the Y_ runtime and generated launcher
as C translation units, and links the result through the C++ toolchain.

```sh
make gy++
gy++ main.yxx helpers.yc tools.yaq --xx-warn-all --xx-warn-extra --xx-optimize=2 -o app
./app
```

### Native gy++ flags

These are deliberately separate from gyy's native flag names.

| Flag | Meaning |
|---|---|
| `--xx-optimize=0..3` | Select optimization level |
| `--xx-debug-info` | Include debug information |
| `--xx-warn-all` | Enable common warnings |
| `--xx-warn-extra` | Enable additional warnings |
| `--xx-warn-pedantic` | Enable pedantic warnings |
| `--xx-warnings-as-errors` | Treat warnings as errors |
| `--xx-warn-none` | Clear warning selections |
| `--xx-warn-no=NAME` | Disable a named warning |
| `--xx-warn-error=NAME` | Treat a named warning as an error |
| `--clang++ PATH` | Select the Clang++ executable |

The familiar GCC-style aliases are also accepted for compatibility. Set
`LLVM_CLANGXX` to choose the default Clang++ executable.

## LLVM IR input

Both drivers accept one textual LLVM IR `.ll` file by itself. Y_ source modules
cannot be mixed with a standalone IR input.

## Diagnostics and backend status

Diagnostics use GCC-style locations, for example
`source.yc:12:1: error: division by zero`. Native compiler warnings include
source locations and warning option names.

The current backend packages Y_ source and the existing interpreter/runtime
into a native executable with module bundling. It is not yet a direct Y_ or
YXX-to-LLVM-IR frontend, and YXX does not yet introduce C++ syntax or a
C++ foreign-function interface.
