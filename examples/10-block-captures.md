# Example 10 — Block Captures ($block)

`$block{OPEN}{CLOSE}var` captures one or more consecutive
`OPEN...CLOSE` sections and concatenates their inner content separated
by a single space. Adding `?` after the variable name makes the entire
block optional.

```lua
local pap = require "papagaio"

-- Capture square-bracket tag lists: [lua][c][papagaio] -> "lua c papagaio"
io.write(pap.process_text([[
$pattern {tags: $block{[}{]}items} {items=[$items]}
tags: [lua][c][papagaio]
tags: [single]
]]))
-- items=[lua c papagaio]
-- items=[single]

-- Optional block: lines without brackets are not discarded.
io.write(pap.process_text([[
$pattern {list $block{[}{]}items?} {$items}
list [x][y][z]
list
]]))
-- x y z
-- (empty line)

-- Double-angle bracket delimiters.
io.write(pap.process_text([[
$pattern {$block{<<}{>>}body} {($body)}
<<hello>><<world>>
]]))
-- (hello world)

-- CSV-like blocks with parentheses.
io.write(pap.process_text([[
$pattern {args: $block{(}{)}vals} {call($vals)}
args: (alpha)(beta)(gamma)
]]))
-- call(alpha beta gamma)
```
