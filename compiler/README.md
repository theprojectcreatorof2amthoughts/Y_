# gyy: Y_ native compiler

`gyy` accepts Y_ source files (`.yc`) and textual LLVM IR (`.ll`). For Y_ source,
it creates a small C translation unit containing the source text and compiles that
translation unit together with the Y_ runtime using Clang. The resulting executable
contains the program and runtime; the original `.yc` file is not needed to start it.

```sh
make
gyy examples/hello.yc -o hello
./hello

gyy examples/modern.yc -O2 -o modern
./modern

gyy program.ll -o program
```

The native Y_ backend packages the existing lexer, parser, evaluator, memory
subsystem, file I/O, and build-embedded `.hly` libraries into each output. This
preserves the existing Y_ vocabulary and behavior. It is a native executable
compiler, but not yet a direct Y_-to-LLVM-IR frontend: the generated executable
embeds the Y_ source and runs it through the compiled runtime. A future LLVM
frontend can replace this backend without changing Y_ syntax.

`!#libinc` and `!#adeq` continue to load external files at runtime, so those
imported files must remain available. `!#inclib` libraries are embedded during
the Y_ project build. Set `LLVM_CLANG` or pass `--clang path` to select Clang.
The optimization flags `-O0`, `-O1`, `-O2`, and `-O3` are forwarded to Clang.
