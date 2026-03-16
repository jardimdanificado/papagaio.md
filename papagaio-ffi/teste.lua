-- teste.lua — papagaio_ffi module test
local urb = require("papagaio_ffi")

print("=== papagaio_ffi module test ===")
print("has_ffi:", urb.has_ffi)
print("has_dl:", urb.has_dl)
print("sizeof_ptr:", urb.sizeof_ptr())

-- memory basics
print("\n--- memory ---")
local p = urb.alloc(64)
print("alloc(64) →", p)
assert(p ~= 0, "alloc failed")

urb.zero(p, 64)
urb.writei32(p, 42)
urb.writei32(p + 4, -7)
urb.writef64(p + 8, 3.14159)
print("readi32(p)     →", urb.readi32(p))
print("readi32(p+4)   →", urb.readi32(p + 4))
print("readf64(p+8)   →", urb.readf64(p + 8))

-- cstring
local s = urb.alloc_str("hello papagaio_ffi!")
print("readcstring    →", urb.readcstring(s))
urb.free(s)

-- ptr read/write
urb.writeptr(p + 16, 0xDEADBEEF)
print("readptr(p+16)  →", string.format("0x%X", urb.readptr(p + 16)))

-- compare
local p2 = urb.alloc(64)
urb.copy(p2, p, 64)
print("compare(eq)    →", urb.compare(p, p2, 64))
urb.writei8(p2, 99)
print("compare(ne)    →", urb.compare(p, p2, 64))
urb.free(p2)

-- realloc
p = urb.realloc(p, 128)
print("realloc(128)   →", p)
assert(urb.readi32(p) == 42, "realloc corrupted data")

urb.free(p)
print("free ok")

-- dynamic loading
if urb.has_dl then
    print("\n--- dl ---")
    print("dl_flags:", urb.dl_flags)
    for k, v in pairs(urb.dl_flags) do
        print("  " .. k .. " =", v)
    end

    local libc = urb.dl_open("libc.so.6", urb.dl_flags.NOW)
    if libc then
        print("dl_open(libc.so.6) → ok")
        local puts_ptr = urb.dl_sym(libc, "puts")
        print("dl_sym(puts) →", puts_ptr)

        local strlen_ptr = urb.dl_sym(libc, "strlen")
        print("dl_sym(strlen) →", strlen_ptr)

        -- sym_self
        local self_puts = urb.dl_sym_self("puts")
        print("dl_sym_self(puts) →", self_puts)

        urb.dl_close(libc)
        print("dl_close ok")
    else
        print("dl_open failed:", urb.dl_error())
    end

    print("errno:", urb.errno())
else
    print("\n(dl not available)")
end

-- FFI calls
if urb.has_ffi then
    print("\n--- ffi ---")
    -- describe
    local puts_desc = urb.ffi_describe("i32 puts(cstring)")
    print("ffi_describe(puts) → ok")

    local strlen_desc = urb.ffi_describe("u64 strlen(cstring)")
    print("ffi_describe(strlen) → ok")

    if urb.has_dl then
        local libc = urb.dl_open("libc.so.6")
        if libc then
            -- bind & call puts
            local puts_ptr = urb.dl_sym(libc, "puts")
            local puts = urb.ffi_bind(puts_ptr, puts_desc)
            print("ffi_bind(puts) → ok")
            local ret = urb.ffi_call(puts, {"hello from papagaio_ffi via ffi_call!"})
            print("puts returned:", ret)

            -- bind & call strlen
            local strlen_ptr = urb.dl_sym(libc, "strlen")
            local strlen_fn = urb.ffi_bind(strlen_ptr, strlen_desc)
            local len = urb.ffi_call(strlen_fn, {"hello world"})
            print("strlen('hello world') →", len)

            -- callback: qsort with Lua comparator
            print("\n--- callback (qsort) ---")
            local qsort_desc = urb.ffi_describe("void qsort(pointer, u64, u64, pointer)")
            local qsort_ptr = urb.dl_sym(libc, "qsort")
            local qsort = urb.ffi_bind(qsort_ptr, qsort_desc)

            local nums = {42, 7, 99, -3, 15}
            local buf = urb.alloc(#nums * 4)
            for i = 1, #nums do
                urb.writei32(buf + (i - 1) * 4, nums[i])
            end

            local cmp_desc = urb.ffi_describe("i32 cmp(pointer, pointer)")
            local cmp = urb.ffi_callback(cmp_desc, function(a, b)
                local va = urb.readi32(a)
                local vb = urb.readi32(b)
                if va < vb then return -1 end
                if va > vb then return 1 end
                return 0
            end)
            print("callback.ptr →", cmp.ptr)

            urb.ffi_call(qsort, {buf, #nums, 4, cmp.ptr})
            print("sorted:")
            for i = 1, #nums do
                io.write("  " .. urb.readi32(buf + (i - 1) * 4))
            end
            print()

            urb.free(buf)

            -- variadic: snprintf
            print("\n--- variadic ---")
            local snprintf_desc = urb.ffi_describe("i32 snprintf(pointer, u64, cstring, ...)")
            local snprintf_ptr = urb.dl_sym(libc, "snprintf")
            local snprintf_fn = urb.ffi_bind(snprintf_ptr, snprintf_desc)

            local out = urb.alloc(256)
            urb.zero(out, 256)
            urb.ffi_call(snprintf_fn, {out, 256, "answer=%d pi=%.2f", 42, 3.14159})
            print("snprintf →", urb.readcstring(out))
            urb.free(out)

            urb.dl_close(libc)
        end
    end
else
    print("\n(ffi not available)")
end

print("\n=== all tests passed ===")
