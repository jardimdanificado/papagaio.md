# Example 19 — FFI: Callbacks (Lua functions to C)

You can create a C-compatible function pointer from a Lua function. This is essential for C functions that take callbacks, like `qsort` or event handlers.

Note: Available in the CLI runner.

```lua
local ffi = require "papagaio_ffi"

if not ffi.has_ffi then return end

local libc = ffi.dl_open("libc.so.6")
if not libc then return end

-- Signature for qsort: void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *))
local qsort_desc = ffi.ffi_describe("void qsort(pointer, u64, u64, pointer)")
local qsort_ptr = ffi.dl_sym(libc, "qsort")
local qsort = ffi.ffi_bind(qsort_ptr, qsort_desc)

-- Define an array in raw memory
local data = { 50, 10, 40, 20, 30 }
local buf = ffi.alloc(#data * 4) -- 4 bytes per i32
for i, v in ipairs(data) do ffi.writei32(buf + (i-1)*4, v) end

-- Define the callback signature for the comparator
local cmp_desc = ffi.ffi_describe("i32 compare(pointer, pointer)")

-- Create the callback from a Lua function
local callback = ffi.ffi_callback(cmp_desc, function(pa, pb)
    local a = ffi.readi32(pa)
    local b = ffi.readi32(pb)
    if a < b then return -1 end
    if a > b then return 1 end
    return 0
end)

-- Call qsort passing our Lua callback as the 4th argument (callback.ptr)
ffi.ffi_call(qsort, {buf, #data, 4, callback.ptr})

print("Sorted via C qsort + Lua callback:")
for i = 0, #data-1 do
    io.write(ffi.readi32(buf + i*4) .. " ")
end
print()

ffi.free(buf)
ffi.dl_close(libc)
```
