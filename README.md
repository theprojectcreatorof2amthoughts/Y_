# Y_

Y_ is a modular scripting language implemented in portable C11. It uses its own vocabulary for procedures, branching, loops, and values, alongside custom output, memory, import, and variable forms.

## Build and run

```sh
make
./y_ examples/hello.y
./y_ examples/project.y
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

`writescrn >^ expression` prints a value. `!$name` reads a custom variable and `!$name = expression` assigns it. `memdesig !$name, size` allocates a zeroed byte buffer; `memread(buffer, index)` and `memwrite(buffer, index, byte)` access it. Indices are zero-based.

`!#libinc "path.y"` runs another source file in the current variable environment. Paths are relative to the process working directory. Comments start with `##`. Y_ uses `stash` for local variables, `define` for procedures, `give` for returned values, `when` / `doit` / `otherwise` / `done` for branches, and `repeatwhile` / `execute` / `done` for loops. Boolean values are `yes` and `no`, the empty value is `void`, and boolean operators are `both`, `either`, and `invert`.

## Project layout

- `src/main.c` provides the executable entry point.
- `src/y_.c` contains the lexer, parser, and interpreter; `src/y_.h` exposes the CLI entry point.
- `src/memory.c` / `src/memory.h` implement checked byte-buffer operations.
- `src/file_io.c` / `src/file_io.h` provide source-file loading for scripts and imports.
- `stdlib/` contains reusable Y_ source libraries.
- `examples/` contains complete scripts; `tests/` contains smoke and error-path tests.
- `docs/` describes the syntax and internal architecture.

Start with [the language guide](docs/language.md) and [the architecture notes](docs/architecture.md). The runtime is not fully Lua-compatible and does not yet include Lua's standard library or bytecode VM.