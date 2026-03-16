# Example 08 — Eval with Caller Globals

`$eval{}` runs inside the same Lua state that called `pap.process_text`.
This gives the eval block full access to every variable and module that
is loaded in the outer script, including user-defined globals.

```lua
local pap = require "papagaio"

-- Define a global tax rate in the outer script.
TAX_RATE = 0.1

io.write(pap.process_text([[
$pattern {price: $v$float} {$eval{
    local v = tonumber(match:match("[%d%.]+"))
    return string.format("$%.2f (+ %.0f%% = $%.2f)", v, TAX_RATE * 100, v * (1 + TAX_RATE))
}}
price: 99.90
price: 200.00
price: 14.99
]]))
-- $99.90 (+ 10% = $109.89)
-- $200.00 (+ 10% = $220.00)
-- $14.99 (+ 10% = $16.49)

-- A lookup table defined outside but used inside $eval.
COUNTRY_CODE = { US = "United States", BR = "Brazil", DE = "Germany" }

io.write(pap.process_text([[
$pattern {country: $c$upper} {$eval{
    local code = match:match("%u+")
    return COUNTRY_CODE[code] or ("unknown(" .. code .. ")")
}}
country: US
country: BR
country: JP
]]))
-- United States
-- Brazil
-- unknown(JP)
```
