# Example 20 — FFI: Structs and Views

`papagaio_ffi` allows you to define data schemas and map them to raw memory using "Views". This lets you access C-style structures using standard Lua table syntax.

```lua
local ffi = require "papagaio_ffi"

-- 1. Define a schema (struct)
local Player = {
    id    = "u32",
    score = "i32",
    x     = "f32",
    y     = "f32"
}

-- 2. Allocate memory based on the schema size
local size = ffi.struct_sizeof(Player)
local ptr = ffi.alloc(size)

-- 3. Create a view that "wraps" the raw memory
local p = ffi.view(ptr, Player)

-- 4. Set values using keys
p.id = 101
p.score = 5000
p.x = 125.5
p.y = 300.2

print("Player ID:", p.id)
print("Position:", p.x, p.y)

-- 5. Offset visualization: where is 'x' located?
print("Offset of 'x':", ffi.struct_offsetof(Player, "x"))

-- 6. View Array: multiple structs in one block
local group_ptr = ffi.alloc(size * 3)
local players = ffi.view_array(group_ptr, Player, 3)

players[1].id = 1
players[2].id = 2
players[3].id = 3

for i = 1, 3 do
    print("Slot " .. i .. " has content ID: " .. players[i].id)
end

ffi.free(ptr)
ffi.free(group_ptr)
```
