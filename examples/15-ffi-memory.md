# Example 15 — FFI: Raw Memory and Typed Reads/Writes

`papagaio_ffi` exposes low-level memory operations directly to Lua.
This allows allocating raw buffers, writing typed values into them, and
reading them back — useful for interoperability with native libraries
or for compact binary data manipulation.

```lua
local ffi = require "papagaio_ffi"

-- Allocate a buffer, write typed integers, and read them back.
local buf = ffi.alloc(16)   -- 16 bytes
ffi.writei32(buf + 0, 1000)
ffi.writei32(buf + 4, 2000)
ffi.writef32(buf + 8, 3.14)

print("i32[0]:", ffi.readi32(buf + 0))   -- 1000
print("i32[4]:", ffi.readi32(buf + 4))   -- 2000
print("f32[8]:", string.format("%.2f", ffi.readf32(buf + 8)))  -- 3.14

ffi.free(buf)

-- Allocate a C-string, read it back, then free.
local ptr = ffi.alloc_str("hello from C memory")
print("cstring:", ffi.readcstring(ptr))   -- hello from C memory
ffi.free(ptr)

-- Memory comparison (like memcmp).
local a = ffi.alloc_str("abc")
local b = ffi.alloc_str("abc")
local c = ffi.alloc_str("xyz")
print("a==b:", ffi.compare(a, b, 3) == 0)  -- true
print("a==c:", ffi.compare(a, c, 3) == 0)  -- false
ffi.free(a); ffi.free(b); ffi.free(c)
```

> Note: `papagaio_ffi` runs with full capabilities in the CLI tool.
> In the Obsidian plugin (WASM), it is available in core mode
> (memory primitives only, no dlopen or libffi calls).
