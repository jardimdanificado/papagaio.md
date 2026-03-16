# Example 02 — Multiple Patterns in One Call

`pap.process` accepts an arbitrary number of pattern/replacement pairs.
Patterns are tried in order; the first one that matches is applied and
the engine continues scanning from where the match ended.

```lua
local pap = require "papagaio"

-- Two patterns, first match wins on each line segment.
local result = pap.process(
    "type: A value: 9\ntype: B value: 7\n",
    "type: A value: $v", "type_a($v)",
    "type: B value: $v", "type_b($v)"
)
print(result)
-- type_a(9)
-- type_b(7)

-- Log-level relabelling with three patterns in one pass.
result = pap.process(
    "level: debug\nlevel: warn\nlevel: error\n",
    "level: debug", "[DBG]",
    "level: warn",  "[WRN]",
    "level: error", "[ERR]"
)
print(result)
-- [DBG]
-- [WRN]
-- [ERR]
```
