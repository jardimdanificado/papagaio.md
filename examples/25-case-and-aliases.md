# Example 25 — Case Sensitivity and Aliases

Papagaio can enforce specific cases for tokens and match against a list of pre-defined allowed values (aliases).

```lua
local pap = require "papagaio"

-- 1. Case modifiers: $upper, $lower, $capitalized
io.write(pap.process_text([[
$pattern {CMD: $c$upper} {EXEC_UPPER($c)}
$pattern {msg: $m$capitalized} {SENTENCE($m)}
CMD: RELOAD
CMD: reload
msg: Hello
msg: hello
]]))
-- EXEC_UPPER(RELOAD)
-- CMD: reload (no match)
-- SENTENCE(Hello)
-- msg: hello (no match)

-- 2. Aliases: $name$aliases{val1,val2,...}
-- Only matches if the token is exactly one of the values in the curly braces.
io.write(pap.process_text([[
$pattern {set mood $m$aliases{happy,sad,angry}} {current_mood = $m}
set mood happy
set mood excited
set mood sad
]]))
-- current_mood = happy
-- set mood excited (not in alias list)
-- current_mood = sad

-- 3. Optional content: $name$optional{fixed_text}
-- Matches a specific text block if present, or captures nothing.
io.write(pap.process_text([[
$pattern {user: $u $admin$optional{ (admin)}} {USER($u, is_admin=$admin)}
user: jardel (admin)
user: guest
]]))
-- USER(jardel, is_admin= (admin))
-- USER(guest, is_admin=)
```
