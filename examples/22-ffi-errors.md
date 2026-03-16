# Example 22 — FFI: Error Handling and errno

When interacting with native code, failures often set a global error code called `errno`. `papagaio_ffi` provides tools to retrieve both the numeric code and the human-readable description of why a native operation failed.

Note: Available in the CLI runner.

```lua
local ffi = require "papagaio_ffi"

if not ffi.has_dl then return end

-- 1. Try to open a non-existent library
local handle = ffi.dl_open("/path/to/nothing.so")

if not handle then
    -- Get the last error message from the dynamic linker
    local linker_error = ffi.dl_error()
    print("Linker failed:", linker_error)

    -- Get the system errno
    local code = ffi.errno()
    print("System errno:", code)
    
    -- On Linux, error 2 is usually ENOENT (No such file or directory)
    if code == 2 then
        print("Diagnosis: The file truly does not exist on disk.")
    end
end

-- 2. Checking errno after a custom FFI call
-- If you bind a function like 'open' or 'mkdir' from libc,
-- you can check ffi.errno() immediately after the call returns an error value.
local libc = ffi.dl_open("libc.so.6")
if libc then
    local mkdir = ffi.ffi_bind(ffi.dl_sym(libc, "mkdir"), ffi.ffi_describe("i32 mkdir(cstring, u32)"))
    
    -- Attempting to create a folder in a restricted location
    local res = ffi.ffi_call(mkdir, {"/root/secret_folder", 511})
    if res == -1 then
        print("mkdir failed with errno:", ffi.errno())
    end
    
    ffi.dl_close(libc)
end
```
