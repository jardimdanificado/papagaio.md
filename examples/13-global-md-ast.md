# Example 13 — Reading the Markdown AST (global.md)

When a `.md` file is executed by `papagaio-md` (or the Obsidian plugin),
its Markdown structure is parsed into a Lua table available as `global.md`.

Headings become nested table keys. Text paragraphs and tables become
sequential array entries inside their parent heading's table.

## person
### name
Alice
### age
30
### scores

| subject | score |
|---------|-------|
| math    | 92    |
| english | 88    |

```lua
-- Access structured data parsed from the Markdown above this code block.
local p = global.md.person

print("Name:", p.name[1])
print("Age:",  p.age[1])

-- The table under 'scores' is the first array element of that section.
local scores = p.scores[1]
-- scores[1] is the header row: {"subject", "score"}
-- scores[2], scores[3] are data rows.
for i = 2, #scores do
    local row = scores[i]
    print(row[1], "->", row[2])
end
```
