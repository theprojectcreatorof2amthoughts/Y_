# Y_
## For Other Users
YXX can be mixed with Y_, if you are looking for YXX, go to [this link](https://github.com/theprojectcreatorof2amthoughts/YPP).
## Introduction
Y_ is a modular scripting language implemented in portable C11. It uses its original custom vocabulary and adds richer operators, extended string processing, numeric and geometry helpers, and expanded math built-ins.

## Build and run

```sh
make
./ylang examples/hello.yc
./ylang examples/project.yc

gyy examples/project.yc -o project
./project

gyy --check examples/modern.yc
gyy examples/modern.yc --optimize=2 --warn-all -o modern
./modern

make test-diagnostics
```

Run `./ylang` without a file to start the interactive prompt. No third-party libraries are required.

The interpreter executable is named `ylang`. `gyy` compiles Y_ source files (`.yc`) into native executables with Clang and the bundled Y_ runtime. It also accepts textual LLVM IR (`.ll`), because it uses LLVM in the first version. See [the readme for the compiler and guidelines](https://github.com/theprojectcreatorof2amthoughts/Y_#guides-and-notes).

## Language

```lua
stash greeting = "hello"

define announce(message)
	writescrn >^ message
done

announce(greeting)

!$count = 3
repeatwhile !$count > 0 execute
	writescrn >^ !$count
	!$count = !$count - 1
done

memdesig !$scratch, 16
memwrite(!$scratch, 0, 72)
writescrn >^ memread(!$scratch, 0)
```

`writescrn >^ expression` prints a value to standard output; `errstate <^ expression` prints a diagnostic to standard error. `!$name` reads a custom variable and `!$name = expression` assigns it. Use `!${name-with-symbols}` for custom variable names containing punctuation or spaces. `memdesig !$name, size` allocates a zeroed byte buffer; `memread(buffer, index)` and `memwrite(buffer, index, byte)` access it. Indices are zero-based.

`!#inclib %stdlib/math.hly%` and similar paths import `.hly` Header Library Y files embedded in the executable at build time. `!#libinc "path.hly"` loads and executes an external `.hly` Header Library Y file from disk. Both `!#inclib` and `!#libinc` accept `.hly` libraries: `!#inclib` selects a build-embedded module, while `!#libinc` reads a file at runtime. The old `.yl` extension is no longer supported. `!#adeq "path.yaq"` loads an external Adequit into the current program environment; an Adequit can import `.hly` libraries, calculate values, and define procedures for the `.yc` program to use. Adequits execute in the caller's environment, so their procedures and variables are available after the import. Example: `./ylang examples/adequit_demo.yc` runs a short program that imports `examples/quicktools.yaq`; that Adequit loads `stdlib/math.hly` and exposes reusable procedures. `.yaq` files are executable Adequits, while `.hly` means Header Library Y.

`!#inptype("prompt")` displays the prompt, reads one line from standard input, and returns the entered text as a string; use `!#inptype("")` for input without a prompt. Comments start with `##`. Y_ uses `stash` for variables, `define` for procedures, `give` for returned values, `when` / `doit` / `else` / `done` for branches, and `repeatwhile` / `execute` / `done` for loops. Boolean values are `yes` and `no`, the empty value is `void`, and boolean operators are `both`, `either`, and `invert`. `name define = yes` checks whether a variable name is bound, independently of its value.

## HTML with Y_

`stdhtml/` contains Y_ helpers for HTML elements, attributes, and complete document wrappers. Browse [the stdhtml catalog](stdhtml/index.html), then generate the demo page with:

```sh
make
./ylang stdhtml/demo.yc > page.html
```

The helpers insert supplied content as raw markup; escape untrusted text before using it in a page.

## Project layout

- `src/main.c` provides the executable entry point.
- `src/y_.c` contains the lexer, parser, and interpreter; `src/y_.h` exposes the CLI entry point.
- `src/memory.c` / `src/memory.h` implement checked byte-buffer operations.
- `src/file_io.c` / `src/file_io.h` provide source-file loading for scripts and imports.
- `stdlib/`, `stdint/`, `stdmodel/`, and `stdhtml/` contain reusable `.hly` Header Library Y modules.
- `stdint/` contains integer libraries that define easier expressions
- `stdmodel/` contains model libraries for software or code that focuses on models.
- `stdhtml/` contains HTML-generation libraries, a demo script, and their HTML reference page.
- `examples/` contains `.yc` programs and `.yaq` Adequits.
- `docs/` describes the syntax and internal architecture.

New capabilities include exponentiation, floor division, string concatenation, type inspection, numeric conversion, substring and replacement tools, string repetition and reversal, variadic aggregation, angle conversion, geometry math, interpolation, and expanded math functions. The original Y_ vocabulary remains the only keyword vocabulary.

## Guides and Notes

Start with [the language guide](docs/language.md) and [the architecture notes](docs/architecture.md), these help you learn how to code in Y_, for the compilation, use [the compiler notes](compiler/README.md).
