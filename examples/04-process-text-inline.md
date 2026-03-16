# Example 04 — Inline Patterns with process_text

`pap.process_text` reads `$pattern {match} {replacement}` directives
directly from the input string. This makes the pattern rules part of
the document itself, which is ideal for self-describing templates.

```lua
local pap = require "papagaio"

-- The $pattern directive lives inside the string.
-- Each subsequent line is processed against every defined pattern.
io.write(pap.process_text([[
$pattern {name: $n, age: $i} {$n is $i years old}
name: Alice, age: 25
name: Bob, age: 30
]]))
-- Alice is 25 years old
-- Bob is 30 years old

-- Multiple directives accumulate; all apply to every following line.
io.write(pap.process_text([[
$pattern {status: ok}    {[PASS]}
$pattern {status: error} {[FAIL]}
unit test A status: ok
unit test B status: error
unit test C status: ok
]]))
-- unit test A [PASS]
-- unit test B [FAIL]
-- unit test C [PASS]
```
