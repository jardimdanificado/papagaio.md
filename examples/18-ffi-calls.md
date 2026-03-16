# Example 18 — FFI: Calling C Functions (ffi_call)

Once you have a pointer to a C function (from `dl_sym`), you can bind it to a Lua function with a specific signature and call it.

Note: This requires `libffi` and is available in the CLI runner.

```lua
local ffi = require "papagaio_ffi"

if not ffi.has_ffi or not ffi.has_dl then
    print("FFI signatures/calls are not available.")
    return
end

local libc = ffi.dl_open("libc.so.6")
if not libc then return end

-- 1. Describe the signature: "return_type name(args...)"
-- Supported: i8..i64, u8..u64, f32, f64, bool, pointer, cstring
local strlen_desc = ffi.ffi_describe("u64 strlen(cstring)")

-- 2. Bind the raw pointer to the signature
local strlen_ptr = ffi.dl_sym(libc, "strlen")
local strlen = ffi.ffi_bind(strlen_ptr, strlen_desc)

-- 3. Call it like a normal Lua function
local len = ffi.ffi_call(strlen, {"Hello Papagaio!"})
print("Length via C strlen():", len)

-- Example with multiple arguments: printf-style
-- Special type '...' is supported for variadics
local snprintf_desc = ffi.ffi_describe("i32 snprintf(pointer, u64, cstring, ...)")
local snprintf_ptr = ffi.dl_sym(libc, "snprintf")
local snprintf = ffi.ffi_bind(snprintf_ptr, snprintf_desc)

local buffer = ffi.alloc(100)
ffi.ffi_call(snprintf, {buffer, 100, "Number: %d, Float: %.2f", 42, 3.14159})
print("Result from C snprintf():", ffi.readcstring(buffer))

ffi.free(buffer)
ffi.dl_close(libc)
```
