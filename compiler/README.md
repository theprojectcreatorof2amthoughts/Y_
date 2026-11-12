# gyy and the LLVM backend

## Bootstrap capability

The initial `gyy` driver accepts textual LLVM IR (`.ll`) and invokes Clang
to produce a native executable:

```sh
gyy program.ll -o program
gyy program.ll -O2 -o program
LLVM_CLANG=/path/to/clang gyy program.ll -o program
```

This is the backend/toolchain bootstrap only. The Y_ source frontend is not
implemented yet, and this version intentionally rejects `.yc` files rather
than pretending to compile them.

## Frontend requirements

The next stage will add a separate Y_ source frontend that lowers supported
language constructs to LLVM IR. The existing interpreter remains available.
The compiler must preserve the language vocabulary and constructs, including
`stash`, `define`, `give`, `when`, `doit`, `else`, `done`,
`repeatwhile`, `execute`, `escape`, `writescrn >^`, `errstate <^`,
`both`, `either`, `invert`, custom `!$name` variables, and `!#`
directives. Unsupported syntax must produce explicit diagnostics.
