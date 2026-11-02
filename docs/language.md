# Y_ Language Guide

Y_ is a small interpreted language. Source files use `.y`; statements can be separated by newlines or semicolons. Comments begin with `--` and continue to the end of the line.

## Values and variables

The runtime has numbers, strings, booleans, `nil`, functions, and allocated memory buffers.

```lua
local name = "Ada"
local score = 96
!$session_count = 1
```

`local` creates a function-local variable. Ordinary assignment updates an existing variable or creates a global when no variable exists. `!$name` is the explicit custom-variable form and uses the same storage rules.

## Functions and control flow

```lua
function classify(score)
    if score >= 90 then
        return "excellent"
    else
        return "keep going"
    end
end

local attempts = 3
while attempts > 0 do
    writescrn >^ classify(95)
    attempts = attempts - 1
end
```

Functions support positional parameters, closures over their defining environment, and `return`. `if` supports `then`, optional `else`, and `end`. `while` supports `do`, `break`, and `end`. Each loop has a safety limit of one million iterations.

## Operators

Arithmetic: `+`, `-`, `*`, `/`, `%`. Comparisons: `==`, `~=`, `<`, `<=`, `>`, `>=`. Boolean operators: `and`, `or`, `not`. `+` also concatenates two strings. Only `nil` and `false` are falsey.

## Custom statements and libraries

```lua
writescrn >^ "hello"
memdesig !$bytes, 32
memwrite(!$bytes, 0, 72)
local first_byte = memread(!$bytes, 0)
!#libinc "stdlib/math.y"
```

`writescrn >^` prints one value and a newline. `memdesig` creates a zero-filled byte buffer up to 16 MiB; reads and writes use zero-based integer indexes. `!#libinc` executes a source file in the current environment; paths are relative to the process working directory.

## Interactive mode

Run `./y_` with no arguments to evaluate one-line snippets in a persistent environment. Run `./y_ path/to/script.y` to execute a source file.

This syntax is Lua-inspired, not a Lua implementation. Tables, coroutines, modules, and bytecode compilation are not currently supported.