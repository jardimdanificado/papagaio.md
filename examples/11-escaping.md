# Example 11 — Escaping the Sigil

To include a literal `$` in the output, prefix it with a backslash
(`\$`). The backslash is consumed and the dollar sign is emitted as a
plain character, not interpreted as the start of a variable or directive.

```lua
local pap = require "papagaio"

-- Output a dollar sign as part of a price label.
print(pap.process_text([[
$pattern {price: $v$float} {cost: \$$v}
price: 49.90
]]))
-- cost: $49.90

-- Mix of real captures and literal dollar signs.
print(pap.process_text([[
$pattern {rate: $v$float} {rate is $v% (\$100 -> \$$eval{
    local r = tonumber(match:match("[%d%.]+"))
    return string.format("%.2f", 100 * (1 + r / 100))
}}]
rate: 8.5
]]))
-- rate is 8.5% ($100 -> $108.50)

-- Literal dollar sign in a fixed replacement (no pattern needed).
print(pap.process("total 200", "total $v", "total: \$$v USD"))
-- total: $200 USD
```
