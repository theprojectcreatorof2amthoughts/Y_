# Architecture

Y_ is currently a compact tree-walk interpreter organized around explicit subsystem boundaries.

## C modules

- `src/main.c` is the operating-system entry point and forwards command-line arguments.
- `src/y_.c` owns source tokenization, recursive-descent parsing, AST execution, function environments, built-in dispatch, and the REPL.
- `src/memory.c` owns zero-initialized buffers and checked byte reads/writes. The interpreter enforces the language-level allocation limit before calling it.
- `src/file_io.c` owns whole-file loading shared by script execution and `!#libinc`.
- Public declarations are in `src/y_.h`, `src/memory.h`, and `src/file_io.h`.

## Execution path

1. The CLI loads a script or reads a REPL line.
2. The lexer converts source characters into tokens with line metadata.
3. The parser builds a linked statement tree and expression tree.
4. The evaluator resolves values in chained environments and executes statements directly.
5. Imports reuse the caller's environment; memory built-ins delegate to the checked memory module.

The interpreter currently favors a small dependency-free implementation over bytecode compilation. Its next natural boundaries are separate lexer/parser/runtime translation units and a public embedding API.

## Build and checks

`make` builds the executable with a C11 compiler. `make test` executes language-level smoke tests, including a rejected out-of-range memory access. No external runtime libraries are required beyond the C math library.