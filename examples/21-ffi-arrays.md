# Example 21 — FFI: Bulk Array Operations

When dealing with large sets of numeric data, individual calls to `readi32` or `writei32` can be slow. `papagaio_ffi` provides bulk operations to move data between Lua tables and raw memory buffers in a single step.

```lua
local ffi = require "papagaio_ffi"

-- 1. Prepare raw memory for 10 integers (40 bytes)
local count = 10
local buf = ffi.alloc(count * 4)

-- 2. write_array: Copy an entire Lua table to memory
-- Supports: i8, u8, i16, u16, i32, u32, i64, u64, f32, f64, pointer
local my_data = { 100, -200, 300, -400, 500 }
ffi.write_array(buf, "i32", my_data)

-- 3. read_array: Extract a range of memory back into a Lua table
local result = ffi.read_array(buf, "i32", 5)
for i, val in ipairs(result) do
    print(string.format("Index %d: %d", i, val))
end

-- 4. Using floats
local float_buf = ffi.alloc(3 * 4)
ffi.write_array(float_buf, "f32", { 1.1, 2.2, 3.3 })
local floats = ffi.read_array(float_buf, "f32", 3)
print("First float:", string.format("%.1f", floats[1]))

ffi.free(buf)
ffi.free(float_buf)
```
