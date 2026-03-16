# Example 01 — Basic Token Substitution

This example demonstrates the simplest form of pattern matching:
capturing a token from a context string and placing it in a new position.

The `$name` sigil in a pattern captures any single whitespace-delimited
token found after the literal anchor text that precedes it.

```lua
local pap = require "papagaio"

-- Capture the value after the literal "name: " and reuse it in the output.
local result = pap.process("name: Alice, age: 30",
    "name: $n,", "person=$n,")
print(result)
-- person=Alice, age: 30

-- Capture the value assigned to x= and rename its key.
result = pap.process("x=10 y=20 z=30", "x=$v", "X=$v")
print(result)
-- X=10 y=20 z=30
```
