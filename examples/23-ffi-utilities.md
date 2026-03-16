# Example 23 — FFI: Core Utility Primitives

This final example covers the remaining low-level utility functions in `papagaio_ffi`. These primitives are the building blocks for creating safe and robust native integrations.

```lua
local ffi = require "papagaio_ffi"

-- 1. Architecture Detection
-- Returns 8 on 64-bit systems and 4 on 32-bit systems.
-- Useful for manual offset calculations.
print("System pointer size (bytes):", ffi.sizeof_ptr())

-- 2. Null Pointer Handling
-- ffi.nullptr() returns a consistent representation of a NULL pointer (0).
local my_ptr = ffi.nullptr()
if my_ptr == 0 then
    print("Pointer is NULL.")
end

-- 3. Fast Block Operations (Native memset and memcpy)
local size = 1024
local b1 = ffi.alloc(size)
local b2 = ffi.alloc(size)

-- ffi.zero(ptr, length): Fast zero-fill (memset to 0)
ffi.zero(b1, size)

-- ffi.set(ptr, byte_value, length): Fill with specific byte (memset)
-- Fills the buffer with 0xAA (170 in decimal)
ffi.set(b2, 170, size)

-- ffi.compare(ptr1, ptr2, length): Compare memory blocks (memcmp)
-- Returns 0 if identical, non-zero otherwise.
if ffi.compare(b1, b2, size) ~= 0 then
    print("Buffers are different (as expected).")
end

-- ffi.copy(dest, src, length): Fast memory copy (memcpy)
ffi.copy(b1, b2, size)

if ffi.compare(b1, b2, size) == 0 then
    print("Buffers are now identical after ffi.copy.")
end

-- 4. Automatic Resource Management
-- Objects returned by ffi.dl_open and ffi.ffi_callback have __gc metamethods.
-- They will be closed/freed automatically when the Lua variable goes out of scope,
-- but you can also close them manually using their respective close functions.

ffi.free(b1)
ffi.free(b2)
```
