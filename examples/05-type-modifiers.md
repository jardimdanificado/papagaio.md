# Example 05 — Type Modifiers

Appending `$int`, `$float`, `$word`, `$hex`, `$upper`, `$lower`, or
`$identifier` to a capture variable restricts what it will match.
Lines where the value does not satisfy the constraint are left unchanged.

```lua
local pap = require "papagaio"

-- $v$int only matches whole integers, not floats or words.
io.write(pap.process_text([[
$pattern {count: $v$int} {[integer: $v]}
count: 42
count: 3.5
count: abc
]]))
-- [integer: 42]
-- count: 3.5      (no match, left as-is)
-- count: abc      (no match, left as-is)

-- $v$word matches only alphabetic tokens.
io.write(pap.process_text([[
$pattern {tag: $v$word} {<$v>}
tag: important
tag: 123
tag: hello-world
]]))
-- <important>
-- tag: 123        (digits, no match)
-- tag: hello-world  (hyphen breaks word, no match)

-- $v$hex matches hexadecimal strings (with optional 0x prefix).
io.write(pap.process_text([[
$pattern {color: #$v$hex} {rgb(#$v)}
color: #ff3a00
color: #xyz
]]))
-- rgb(#ff3a00)
-- color: #xyz     (non-hex chars, no match)

-- $v$upper matches only uppercase sequences.
io.write(pap.process_text([[
$pattern {code: $v$upper} {CODE[$v]}
code: HTTP
code: http
code: Http
]]))
-- CODE[HTTP]
-- code: http      (no match)
-- code: Http      (no match)
```
