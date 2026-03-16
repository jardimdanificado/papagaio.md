#ifndef PAPAGAIO_FFI_H
#define PAPAGAIO_FFI_H

/*
 * papagaio_ffi — standalone Lua FFI module
 *
 * Dependencies: libc + Lua 5.1 / 5.2 / 5.3 / 5.4 / LuaJIT 2.x
 * Optional:     libffi (for FFI calls/callbacks) + libdl (for dlopen)
 *
 * Compile-time feature flags:
 *   PAPAGAIO_FFI_HAS_DL   — enable dlopen / dlsym / dlclose
 *   PAPAGAIO_FFI_HAS_FFI  — enable libffi-based calls and callbacks (implies DL)
 *   PAPAGAIO_FFI_HAS_ALL  — shorthand: enable everything
 *
 * ---------------------------------------------------------------------
 * Build as shared library:
 *
 *   Core only (no external deps beyond Lua):
 *     cc -shared -fPIC -o papagaio_ffi.so papagaio_ffi.c \
 *        $(pkg-config --cflags --libs lua5.4)
 *
 *   + dynamic loading:
 *     cc -shared -fPIC -DPAPAGAIO_FFI_HAS_DL -o papagaio_ffi.so papagaio_ffi.c \
 *        $(pkg-config --cflags --libs lua5.4) -ldl
 *
 *   Full (DL + FFI):
 *     cc -shared -fPIC -DPAPAGAIO_FFI_HAS_ALL -o papagaio_ffi.so papagaio_ffi.c \
 *        $(pkg-config --cflags --libs lua5.4) \
 *        $(pkg-config --cflags --libs libffi) -ldl
 *
 * Then from Lua:
 *   local urb = require "papagaio_ffi"
 *
 * ---------------------------------------------------------------------
 * Lua API — Core (always available)
 *
 *   urb.has_ffi         → boolean  (true if FFI calls are available)
 *   urb.has_dl          → boolean  (true if dynamic loading is available)
 *
 * -- Memory --
 *   urb.alloc(size) → ptr
 *   urb.free(ptr)
 *   urb.realloc(ptr, size) → ptr
 *   urb.zero(ptr, size)
 *   urb.copy(dst, src, size)
 *   urb.set(ptr, byte, size)
 *   urb.compare(a, b, size) → int
 *   urb.nullptr() → 0
 *   urb.sizeof_ptr() → int
 *
 *   urb.readptr(ptr) → ptr
 *   urb.writeptr(ptr, value)
 *   urb.readcstring(ptr) → string|nil
 *   urb.writecstring(ptr, text)
 *   urb.alloc_str(text) → ptr
 *
 *   urb.readi8(ptr) .. urb.readf64(ptr)
 *   urb.writei8(ptr, v) .. urb.writef64(ptr, v)
 *
 *   urb.read_array(ptr, type, count) → table
 *   urb.write_array(ptr, type, values)
 *
 *   urb.struct_sizeof(schema) → int
 *   urb.struct_offsetof(schema, field) → int
 *   urb.view(ptr, schema [, totalSize]) → view
 *   urb.view_array(ptr, schema, count) → array
 *
 * -- Dynamic Loading (requires PAPAGAIO_FFI_HAS_DL) --
 *   urb.dl_flags          → table {LAZY, NOW, LOCAL, GLOBAL, ...}
 *   urb.dl_open(path [, flags]) → handle
 *   urb.dl_close(handle)
 *   urb.dl_sym(handle, name) → ptr
 *   urb.dl_sym_self(name) → ptr
 *   urb.dl_error() → string|nil
 *   urb.errno() → int
 *
 * -- FFI Calls (requires PAPAGAIO_FFI_HAS_FFI) --
 *   urb.ffi_describe(sig) → descriptor
 *   urb.ffi_bind(ptr, descriptor) → bound
 *   urb.ffi_call(bound, {args...}) → result
 *   urb.ffi_callback(descriptor, fn) → {ptr=..., handle=...}
 *
 * Signature format: "return_type name(arg1, arg2, ...)"
 * Supported types: void, bool, i8, u8, i16, u16, i32, u32, i64, u64,
 *                  f32, f64, pointer, cstring
 * Variadics: "i32 printf(cstring, ...)"
 */

#include <stddef.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Feature detection macros */
#ifdef PAPAGAIO_FFI_HAS_ALL
#  ifndef PAPAGAIO_FFI_HAS_DL
#    define PAPAGAIO_FFI_HAS_DL
#  endif
#  ifndef PAPAGAIO_FFI_HAS_FFI
#    define PAPAGAIO_FFI_HAS_FFI
#  endif
#endif

#ifdef PAPAGAIO_FFI_HAS_FFI
#  ifndef PAPAGAIO_FFI_HAS_DL
#    define PAPAGAIO_FFI_HAS_DL  /* FFI implies DL */
#  endif
#endif

/* Lua module entry point — called automatically by require "papagaio_ffi" */
LUALIB_API int luaopen_papagaio_ffi(lua_State *L);

#ifdef __cplusplus
}
#endif
#endif /* PAPAGAIO_FFI_H */
