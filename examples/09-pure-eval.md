# Example 09 — Pure Eval (No Pattern, Dynamic Injection)

A bare `$eval{}` without a preceding `$pattern` fires once per
occurrence in the replacement string, with `match` set to an empty
string. This is how you inject dynamic values like timestamps, random
numbers, or environment data into any text at processing time.

```lua
local pap = require "papagaio"

-- Inject the current date and time into a document header.
print(pap.process_text(
    "Generated on $eval{ return os.date('%Y-%m-%d') } at $eval{ return os.date('%H:%M:%S') }"))
-- Generated on 2025-03-16 at 06:45:00

-- Inject a formatted constant.
print(pap.process_text(
    "pi = $eval{ return string.format('%.10f', math.pi) }"))
-- pi = 3.1415926536

-- Generate a simple sequential ID.
COUNTER = 0
local function next_id()
    COUNTER = COUNTER + 1
    return string.format("ID-%04d", COUNTER)
end

io.write(pap.process_text([[
$pattern {item: $name$word} {$eval{
    local name = match:match("%a+")
    return next_id() .. " " .. name
}}
item: apple
item: banana
item: cherry
]]))
-- ID-0001 apple
-- ID-0002 banana
-- ID-0003 cherry
```
