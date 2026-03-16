# Example 12 — Table Form with process_pairs

`pap.process_pairs(input, pairs_table)` accepts a Lua table of
`{pattern, replacement}` arrays. All pairs are applied to the input in
the order they appear in the table. Captures work identically to the
other functions.

```lua
local pap = require "papagaio"

-- Structured log level mapping.
io.write(pap.process_pairs(
    "level: debug\nlevel: error\nlevel: info\n",
    {
        { "level: debug", "[DBG]" },
        { "level: error", "[ERR]" },
        { "level: info",  "[INF]" },
    }
))
-- [DBG]
-- [ERR]
-- [INF]

-- Capture and reformat user/role pairs.
io.write(pap.process_pairs(
    "user=alice role=admin\nuser=bob role=viewer\n",
    {
        { "user=$u role=$r", "$u:$r" },
    }
))
-- alice:admin
-- bob:viewer

-- HTTP method translation, multiple patterns from a table.
local verb_map = {
    { "method: GET",    "read"   },
    { "method: POST",   "create" },
    { "method: PUT",    "update" },
    { "method: DELETE", "delete" },
}
io.write(pap.process_pairs(
    "method: GET\nmethod: POST\nmethod: DELETE\n",
    verb_map
))
-- read
-- create
-- delete
```
