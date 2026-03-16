# Example 06 — Optional Captures

Adding `?` after a capture variable makes it optional. If the token is
absent, the variable expands to an empty string instead of causing a
match failure. This lets a single pattern handle both forms of a line.

```lua
local pap = require "papagaio"

-- The comment field is optional.
io.write(pap.process_text([[
$pattern {order $id$int $note?} {id=$id note=[$note]}
order 42 urgent
order 99
order 7 low-priority
]]))
-- id=42 note=[urgent]
-- id=99 note=[]
-- id=7 note=[low-priority]

-- Optional unit suffix: works with and without it.
io.write(pap.process_text([[
$pattern {weight: $v$float $unit?} {$v $unit}
weight: 72.5 kg
weight: 80.0
]]))
-- 72.5 kg
-- 80.0 
```
