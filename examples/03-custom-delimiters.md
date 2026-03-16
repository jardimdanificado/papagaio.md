# Example 03 — Custom Sigil and Delimiters (process_ex)

`pap.process_ex` lets you choose a different marker character and
block delimiters instead of the default `$` sigil and `{}` blocks.
This is useful when the input already contains dollar signs or curly
braces and you want to avoid escaping conflicts.

```lua
local pap = require "papagaio"

-- Use "@" as sigil and "<>" as block delimiters.
-- This rewrites a SQL template without touching any literal dollar signs.
local result = pap.process_ex(
    "INSERT INTO <table> VALUES (<val>)",
    "@", "<", ">",
    "INSERT INTO <table>", "INSERT INTO users",
    "(<val>)",             "('alice@example.com')"
)
print(result)
-- INSERT INTO users VALUES ('alice@example.com')

-- Use "#" as sigil and "[]" as delimiters for Markdown-like syntax.
result = pap.process_ex(
    "See [ref:intro] and [ref:faq] for details.",
    "#", "[", "]",
    "[ref:intro]", "[Introduction](#introduction)",
    "[ref:faq]",   "[FAQ](#faq)"
)
print(result)
-- See [Introduction](#introduction) and [FAQ](#faq) for details.
```
