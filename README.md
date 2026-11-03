# Y_

Y_ is a modular scripting language implemented in portable C11. It uses its own vocabulary for procedures, branching, loops, and values, alongside custom output, memory, import, and variable forms.

## Build and run

```sh
make
./y_ examples/hello.yc
./y_ examples/project.yc
make test
```

Run `./y_` without a file to start the interactive prompt. No third-party libraries are required.

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

`!#inclib %math.yl%`, `!#inclib %strings.yl%`, and `!#inclib %stdint/intstate.yl%` import libraries embedded in the executable from `.yl` files. `!#libinc "path.yc"` remains available for loading an external source file from the process working directory. Comments start with `##`. Y_ uses `stash` for variables, `define` for procedures, `give` for returned values, `when` / `doit` / `else` / `done` for branches, and `repeatwhile` / `execute` / `done` for loops. Boolean values are `yes` and `no`, the empty value is `void`, and boolean operators are `both`, `either`, and `invert`. `name define = yes` checks whether a variable name is bound, independently of its value.

## Project layout

- `src/main.c` provides the executable entry point.
- `src/y_.c` contains the lexer, parser, and interpreter; `src/y_.h` exposes the CLI entry point.
- `src/memory.c` / `src/memory.h` implement checked byte-buffer operations.
- `src/file_io.c` / `src/file_io.h` provide source-file loading for scripts and imports.
- `stdlib/` contains reusable Y_ source libraries.
- `stdint/` contains integer libraries that define easier expressions
- `examples/` contains complete scripts; `tests/` contains smoke and error-path tests.
- `docs/` describes the syntax and internal architecture.

Start with [the language guide](docs/language.md) and [the architecture notes](docs/architecture.md). The runtime is not fully Lua-compatible and does not yet include Lua's standard library or bytecode VM.
