# Example 28 — Single vs Sequential Blocks

Papagaio provides two ways to capture delimited content blocks. `$block` captures a single occurrence, while `$blockseq` captures multiple consecutive occurrences and joins them.

```lua
local pap = require "papagaio"

-- 1. $block{OPEN}{CLOSE}var
-- Only captures the content of the FIRST block matching the delimiters.
io.write(pap.process_text([[
$pattern {tags: $block{[}{]}item} {First tag: $item}
tags: [lua][c][papagaio]
]]))
-- Output:
-- First tag: lua (the [c] and [papagaio] parts are ignored as they are literal leftovers)

-- 2. $blockseq{OPEN}{CLOSE}var
-- Captures ALL consecutive blocks and joins their content with a single space.
io.write(pap.process_text([[
$pattern {tags: $blockseq{[}{]}items} {All tags: $items}
tags: [lua][c][papagaio]
]]))
-- Output:
-- All tags: lua c papagaio

-- 3. Optional Sequential Blocks
-- You can add '?' after the variable name.
io.write(pap.process_text([[
$pattern {list: $blockseq{(}{)}vals?} {values: ($vals)}
list: (a)(b)(c)
list: 
]]))
-- Output:
-- values: (a b c)
-- values: ()
```
