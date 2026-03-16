# Example 17 — FFI: Dynamic Loading (dlopen)

`papagaio_ffi` can load external shared libraries (.so, .dll, .dylib) at runtime. This allows Lua to call any function from any library on your system (like libc, libm, or custom game engines).

Note: This is available in the CLI runner (`papagaio-md`).

```lua
local ffi = require "papagaio_ffi"

if not ffi.has_dl then
    print("Dynamic loading is not available on this platform.")
    return
end

-- 1. Open a shared library (libc is a safe bet on Linux)
local libc = ffi.dl_open("libc.so.6", ffi.dl_flags.NOW)
if not libc then
    print("Could not load libc: " .. (ffi.dl_error() or "unknown error"))
    return
end

-- 2. Find the address of a symbol
local puts_ptr = ffi.dl_sym(libc, "puts")
print("Address of 'puts':", puts_ptr)

-- 3. Close the library when done
ffi.dl_close(libc)

-- You can also search for symbols in the current executable
local self_puts = ffi.dl_sym_self("puts")
print("Address of 'puts' in self:", self_puts)
```
