# Example 14 — Shared State Between Lua Blocks

All `lua` code blocks inside the same `.md` file share the same global
Lua state. Variables declared in one block are visible in all subsequent
blocks within the same run. This lets you split logic across multiple
annotated sections of the document.

## config

max_retries = 3
timeout_ms = 500
endpoint = https://api.example.com

```lua
-- Block 1: parse config values from the Markdown above.
local cfg = global.md.config

-- Paragraphs appear as indexed strings in the parent table.
local function parse_kv(s)
    local t = {}
    for line in s:gmatch("[^\n]+") do
        local k, v = line:match("^(%S+)%s*=%s*(.+)$")
        if k then t[k] = v end
    end
    return t
end

Config = parse_kv(cfg[1])
print("Endpoint:", Config.endpoint)
print("Timeout:", Config.timeout_ms, "ms")
```

## report

```lua
-- Block 2: uses Config defined in Block 1.
print("Max retries configured:", Config.max_retries)
print("Will retry at:", Config.endpoint, "up to", Config.max_retries, "times")
```
