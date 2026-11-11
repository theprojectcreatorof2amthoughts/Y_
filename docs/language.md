# Y_ Language Guide

Y_ is an interpreted language with its own keywords. Source files use `.yc`; statements can be separated by newlines or semicolons. Comments begin with `##` and continue to the end of the line.

## Values and variables

The runtime has numbers, strings, booleans (`yes` and `no`), `void`, procedures, and allocated memory buffers.

```lua
stash name = "Ada"
stash score = 96
!$session_count = 1
stash !${score-level@v2} = 96
```

`stash` creates a function-local variable initialized to `void`; `stash name = value` provides an initial value. Ordinary assignment updates an existing variable or creates one in the current environment when no variable exists. `!$name` is the explicit custom-variable form and uses the same storage rules. Wrap a custom name in `${...}` after `!$` to include punctuation, spaces, or operator characters; escape `}` and `\` with a backslash. For example, `stash !${score-level@v2} = 96` is valid.

## Functions and control flow

```lua
define classify(score)
    when score >= 90 doit
        give "excellent"
    else
        give "keep going"
    done
done

stash attempts = 3
repeatwhile attempts > 0 execute
    writescrn >^ classify(95)
    attempts = attempts - 1
done
```

`define procedure_name(...)` declares a procedure with positional parameters and closures over its defining environment. `give` returns a value. `when` starts a branch, `doit` starts its body, `else` is optional, and `done` closes it. `repeatwhile` starts a loop body with `execute`; `escape` exits a loop. Each loop has a safety limit of one million iterations.

## Operators

Arithmetic: `+`, `-`, `*`, `/`, `//` (floor division), `%`, and `**` (right-associative exponentiation). Comparisons: `==`, `~=`, `!=`, `<`, `<=`, `>`, `>=`. Boolean operators: `both`, `either`, `invert`, plus the aliases `and`, `or`, and `not`. String concatenation works with both `+` (two strings) and `..` (two strings). Only `void` and `no` are falsey.

## Additional vocabulary

The original vocabulary is preserved. These aliases are also accepted:

| Original | Additional spelling |
|---|---|
| `stash` | `let`, `var` |
| `define` | `fn`, `func` |
| `give` | `return` |
| `when` | `if` |
| `doit` | `then` |
| `else` | `otherwise` |
| `done` | `end` |
| `repeatwhile` | `while` |
| `execute` | `do` |
| `escape` | `break` |
| `writescrn >^` | `print`, `println`, `emit` |

`print` / `println` / `emit` each print one value followed by a newline. These aliases can be mixed with original vocabulary, but each opened block must be closed with one matching block-ending word (`done` or `end`).

## Built-in functions

- `type(value)` returns `"void"`, `"number"`, `"string"`, `"boolean"`, `"function"`, or `"memory"`.
- `length(text)` returns the number of bytes in a string.
- `tonumber(value)` converts a numeric string to a number and reports invalid input.
- Math: `abs`, `sqrt`, `floor`, `ceil`, `round`, `pow`, `min`, `max`, `sin`, `cos`, `tan`, `log`, and `exp`. Trigonometric functions use radians.

`name define = yes` checks whether a variable has been defined, even if its value is `void`; use `no` to check that it has not been defined. This operator does not evaluate the variable's value:

```y_
stash status
when status define = yes doit
    writescrn >^ "status exists"
done
```

## Custom statements and libraries

```lua
writescrn >^ "hello"
errstate <^ "a diagnostic message"
memdesig !$bytes, 32
memwrite(!$bytes, 0, 72)
stash first_byte = memread(!$bytes, 0)
!#inclib %math.yl%
```

`writescrn >^` prints one value and a newline to standard output. `errstate <^` prints one value and a newline to standard error without marking the program as failed. Use it for diagnostics; runtime errors still use the interpreter's error path. `memdesig` creates a zero-filled byte buffer up to 16 MiB; reads and writes use zero-based integer indexes. `!#inclib %path.yl%` imports a `.yl` library embedded in the executable at build time; its path can include folders. `!#libinc "path.yc"` remains available for external `.yc` source files.

## Reading input

Use `!#inptype("prompt")` as an expression to display a prompt, read one line from standard input, and return the entered text as a string. An empty prompt reads input without displaying anything:

```lua
stash username = !#inptype("Your name: ")
writescrn >^ "Hello, " + username
```

The prompt must be a quoted string. End-of-file before any characters are read returns `void`. This syntax uses the same input behavior as the `input("prompt")` builtin.

## Interactive mode

Run `./y_` with no arguments to evaluate one-line snippets in a persistent environment. Run `./y_ path/to/script.yc` to execute a source file.

Y_ does not accept Lua's `local`, `function`, `return`, `if`, `while`, or boolean keyword spellings. Tables, coroutines, modules, and bytecode compilation are not currently supported.