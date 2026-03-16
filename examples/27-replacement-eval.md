# Example 27 — Replacement $eval

The `$eval` directive can also be used inside the **replacement string** of a `pap.process` or `pap.process_ex` call. This allows you to perform calculations or logic directly during the substitution phase.

```lua
local pap = require "papagaio"

-- 1. Calculations in replacement
-- The variable 'n' is automatically available in the $eval block 
-- if it was captured as $n in the pattern.
local result = pap.process("item cost: 50", 
    "item cost: $n", "final price: $eval{ return tonumber(n) * 1.2 }")

print(result) 
-- final price: 60.0

-- 2. Formatting in replacement
result = pap.process("user: JARDEL",
    "user: $name", "Welcome, $eval{ return name:lower():gsub('^%l', string.upper) }!")

print(result)
-- Welcome, Jardel!

-- 3. Complex logic
result = pap.process("score: 85",
    "score: $s", "Grade: $eval{
        local s = tonumber(s)
        if s > 90 then return 'A'
        elseif s > 80 then return 'B'
        else return 'C'
        end
    }")

print(result)
-- Grade: B
```
