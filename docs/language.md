# Y_ Language Guide

Y_ is an interpreted language with its own keywords. Source files use `.y`; statements can be separated by newlines or semicolons. Comments begin with `##` and continue to the end of the line.

## Values and variables

The runtime has numbers, strings, booleans (`yes` and `no`), `void`, procedures, and allocated memory buffers.

```lua
stash name = "Ada"
stash score = 96
!$session_count = 1
```

`stash` creates a function-local variable. Ordinary assignment updates an existing variable or creates a global when no variable exists. `!$name` is the explicit custom-variable form and uses the same storage rules.

## Functions and control flow

```lua
define classify(score)
    when score >= 90 doit
        give "excellent"
    otherwise
        give "keep going"
    done
done

stash attempts = 3
repeatwhile attempts > 0 execute
    writescrn >^ classify(95)
    attempts = attempts - 1
done
```

`define` declares a procedure with positional parameters and closures over its defining environment. `give` returns a value. `when` starts a branch, `doit` starts its body, `otherwise` is optional, and `done` closes it. `repeatwhile` starts a loop body with `execute`; `escape` exits a loop. Each loop has a safety limit of one million iterations.

## Operators

Arithmetic: `+`, `-`, `*`, `/`, `%`. Comparisons: `==`, `~=`, `<`, `<=`, `>`, `>=`. Boolean operators: `both`, `either`, `invert`. `+` also concatenates two strings. Only `void` and `no` are falsey.

## Custom statements and libraries

```lua
writescrn >^ "hello"
memdesig !$bytes, 32
memwrite(!$bytes, 0, 72)
stash first_byte = memread(!$bytes, 0)
!#libinc "stdlib/math.y"
```

`writescrn >^` prints one value and a newline. `memdesig` creates a zero-filled byte buffer up to 16 MiB; reads and writes use zero-based integer indexes. `!#libinc` executes a source file in the current environment; paths are relative to the process working directory.

## Interactive mode

Run `./y_` with no arguments to evaluate one-line snippets in a persistent environment. Run `./y_ path/to/script.y` to execute a source file.

Y_ does not accept Lua's `local`, `function`, `return`, `if`, `while`, or boolean keyword spellings. Tables, coroutines, modules, and bytecode compilation are not currently supported.