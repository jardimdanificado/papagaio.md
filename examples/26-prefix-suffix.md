# Example 26 — Prefix and Suffix Constraints

You can restrict matches based on how a token starts or ends using the `$starts{}` and `$ends{}` modifiers.

```lua
local pap = require "papagaio"

-- 1. $starts{prefix}: Matches tokens starting with the given string
io.write(pap.process_text([[
$pattern {action: $a$starts{GIT_}} {VCS_COMMAND($a)}
action: GIT_COMMIT
action: GIT_PUSH
action: SVN_UPDATE
]]))
-- VCS_COMMAND(GIT_COMMIT)
-- VCS_COMMAND(GIT_PUSH)
-- action: SVN_UPDATE (no match)

-- 2. $ends{suffix}: Matches tokens ending with the given string
io.write(pap.process_text([[
$pattern {load: $f$ends{.lua}} {REQUIRE($f)}
load: main.lua
load: helper.lua
load: styles.css
]]))
-- REQUIRE(main.lua)
-- REQUIRE(helper.lua)
-- load: styles.css (no match)

-- 3. Combining with $eval for complex logic
io.write(pap.process_text([[
$pattern {plugin: $p$starts{pa}} {$eval{
    local name = match:match("plugin: (%S+)")
    return "INTERNAL(" .. name:upper() .. ")"
}}
plugin: papagaio
plugin: pandas
]]))
-- INTERNAL(PAPAGAIO)
-- INTERNAL(PANDAS)
```
