# Y_ Language Guide

Y_ is an interpreted language with its own vocabulary. Source files use `.yc`; statements can be separated by newlines or semicolons. Comments begin with `##` and continue to the end of the line.

## Values and variables

The runtime has numbers, strings, booleans (`yes` and `no`), `void`, procedures, and allocated memory buffers.

```y_
stash name = "Ada"
stash score = 96
!$session_count = 1
stash !${score-level@v2} = 96
```

`stash` creates a function-local variable initialized to `void`; `stash name = value` provides an initial value. Ordinary assignment updates an existing variable or creates one in the current environment when no variable exists. `!$name` is the explicit custom-variable form and uses the same storage rules. Wrap a custom name in `${...}` after `!$` to include punctuation, spaces, or operator characters; escape `}` and `\` with a backslash.

## Functions and control flow

```y_
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

Original operators remain unchanged: `+`, `-`, `*`, `/`, `%`, `==`, `~=`, `<`, `<=`, `>`, `>=`, `both`, `either`, and `invert`.

Additional operators:
- `**` performs right-associative exponentiation.
- `//` performs floor division.
- `..` concatenates two strings.

`+` continues to concatenate two strings as well. Only `void` and `no` are falsey. String positions and lengths are byte-based and zero-indexed; `substring` and `charat` accept negative positions counted from the end. `replace` replaces every non-overlapping match, and `repeat` is limited to a 16 MiB result.

## Built-in functions

- `type(value)` returns `"void"`, `"number"`, `"string"`, `"boolean"`, `"function"`, or `"memory"`.
- `length(text)` returns the number of bytes in a string.
- `tonumber(value)` converts a numeric string to a number and reports invalid input.
- Math: `abs`, `sqrt`, `floor`, `ceil`, `round`, `pow`, `min`, `max`, `sin`, `cos`, `tan`, `log`, and `exp`. Trigonometric functions use radians.
- Predicates: `isnumber`, `isstring`, `isboolean`, and `isvoid` return booleans.
- Strings: `upper`, `lower`, `contains`, `startswith`, `endswith`, `substring`, `charat`, `replace`, `repeat`, and `reverse`.
- Numeric aggregation: `sum(value, ...)` and `average(value, ...)` accept one or more numbers.
- Numeric checks: `isfinite(value)` and `isnan(value)` inspect numeric values.
- Geometry and angles: `hypot(x, y)`, `atan2(y, x)` (radians), `radians(degrees)`, and `degrees(radians)`.
- Numeric helpers: `sign`, `clamp(value, low, high)`, and `lerp(from, to, amount)` (unclamped interpolation).

`name define = yes` checks whether a variable has been defined, even if its value is `void`; use `no` to check that it has not been defined. This operator does not evaluate the variable's value.

## Custom statements and libraries

```y_
writescrn >^ "hello"
errstate <^ "a diagnostic message"
memdesig !$bytes, 32
memwrite(!$bytes, 0, 72)
stash first_byte = memread(!$bytes, 0)
!#inclib %math.yl%
```

`writescrn >^` prints one value and a newline to standard output. `errstate <^` prints one value and a newline to standard error without marking the program as failed. `memdesig` creates a zero-filled byte buffer up to 16 MiB; reads and writes use zero-based integer indexes. `!#inclib %path.yl%` imports a `.yl` library embedded in the executable at build time; its path can include folders. `!#libinc "path.yc"` remains available for external `.yc` source files.

## Reading input

Use `!#inptype("prompt")` as an expression to display a prompt, read one line from standard input, and return the entered text as a string. An empty prompt reads input without displaying anything:

```y_
stash username = !#inptype("")
writescrn >^ "Hello, " + username
```

The prompt must be a quoted string. End-of-file before any characters are read returns `void`.

## Interactive mode

Run `./y_` with no arguments to evaluate one-line snippets in a persistent environment. Run `./y_ path/to/script.yc` to execute a source file.

Y_ keeps its own vocabulary. It does not use conventional `print`, `let`, `if`, `while`, or `return` keywords. Tables, coroutines, and bytecode compilation are not currently supported.
