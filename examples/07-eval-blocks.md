# Example 07 — Eval Blocks (Inline Lua Computation)

`$eval{ ... }` inside a replacement string runs a Lua chunk in the
caller's state and splices its return value into the output.
The global variable `match` holds the full matched substring so you can
extract captures from it with standard `string.match`.

```lua
local pap = require "papagaio"

-- Multiply two captured integers at pattern-match time.
io.write(pap.process_text([[
$pattern {$a$int * $b$int} {$eval{
    local a, b = match:match("(%d+) %* (%d+)")
    return tostring(tonumber(a) * tonumber(b))
}}
3 * 4
10 * 7
2 * 2
]]))
-- 12
-- 70
-- 4

-- Convert a decimal integer to its hexadecimal form.
io.write(pap.process_text([[
$pattern {dec: $v$int} {$eval{
    return string.format("0x%x", tonumber(match:match("%d+")))
}}
dec: 42
dec: 255
dec: 16
]]))
-- 0x2a
-- 0xff
-- 0x10

-- Compute string length inline.
io.write(pap.process_text([[
$pattern {word: $w$word} {$eval{
    local w = match:match("%a+")
    return string.format('"%s" (%d chars)', w, #w)
}}
word: hello
word: papagaio
]]))
-- "hello" (5 chars)
-- "papagaio" (8 chars)
```
