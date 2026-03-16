# Example 24 — Advanced Type Modifiers

The Papagaio engine supports a wide range of specific type modifiers to ensure your patterns only match exactly what you expect.

```lua
local pap = require "papagaio"

-- 1. $path: Matches valid file system paths
-- 2. $identifier: Matches valid programming identifiers (start with letter/_, then alphanumeric)
-- 3. $number: Matches any numeric value (integer or float)
io.write(pap.process_text([[
$pattern {save to $p$path} {Destination: $p}
save to /home/jardel/notes.md
save to C:\Backup\config.json
save to "invalid path"

$pattern {var $id$identifier = $val$number} {assign($id, $val)}
var background_color = 255
var 123_invalid = 0.5
var threshold = 0.75
]]))
-- Destination: /home/jardel/notes.md
-- Destination: C:\Backup\config.json
-- save to "invalid path" (no match)
-- assign(background_color, 255)
-- var 123_invalid = 0.5 (no match, starts with digit)
-- assign(threshold, 0.75)

-- 4. $binary: Matches 0b or 0B followed by 0s and 1s
-- 5. $percent: Matches numbers followed by %
io.write(pap.process_text([[
$pattern {mask: $m$binary} {MASK_BIN($m)}
mask: 0b1010
mask: 0B11
mask: 255

$pattern {usage: $p$percent} {CPU: $p}
usage: 85.5%
usage: 100%
]]))
-- MASK_BIN(0b1010)
-- MASK_BIN(0B11)
-- CPU: 85.5%
-- CPU: 100%
```
