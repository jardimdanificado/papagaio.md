/*
 * papagaio_ffi.c — standalone Lua FFI module
 * Build:
 *   cc -shared -fPIC -o papagaio_ffi.so papagaio_ffi.c $(pkg-config --cflags --libs lua5.4)
 *   cc -shared -fPIC -DPAPAGAIO_FFI_HAS_DL -o papagaio_ffi.so papagaio_ffi.c $(pkg-config --cflags --libs lua5.4) -ldl
 *   cc -shared -fPIC -DPAPAGAIO_FFI_HAS_ALL -o papagaio_ffi.so papagaio_ffi.c $(pkg-config --cflags --libs lua5.4) $(pkg-config --cflags --libs libffi) -ldl
 */

#include "papagaio_ffi.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef PAPAGAIO_FFI_HAS_DL
#  ifndef _WIN32
#    include <dlfcn.h>
#  else
#    include <windows.h>
#  endif
#endif

#ifdef PAPAGAIO_FFI_HAS_FFI
#  include <ffi.h>
#  ifndef _WIN32
#    include <pthread.h>
#  endif
#endif

/* =========================================================================
 * Lua compat shims
 * ====================================================================== */
#if LUA_VERSION_NUM < 502
#undef  luaL_newlib
#define luaL_newlib(L, funcs) \
    (lua_newtable(L), luaL_register(L, NULL, (funcs)))
#ifndef lua_rawlen
#define lua_rawlen lua_objlen
#endif
#define lua_isinteger(L,i) 0
#define lua_tointeger(L,i) ((lua_Integer)lua_tonumber(L,i))
#define LUA_OK 0
static void luaL_setmetatable(lua_State *L, const char *name) {
    luaL_getmetatable(L, name);
    lua_setmetatable(L, -2);
}
#define lua_newuserdatauv(L,s,n) lua_newuserdata(L,s)
#define luaL_checkversion(L) ((void)0)
#endif

/* =========================================================================
 * Helpers
 * ====================================================================== */
static int pf_error(lua_State *L, const char *msg) {
    return luaL_error(L, "%s", msg ? msg : "papagaio_ffi error");
}

static int pf_is_nullish(lua_State *L, int idx) {
    int t = lua_type(L, idx);
    return t == LUA_TNIL || t == LUA_TNONE;
}

static int pf_get_uint64(lua_State *L, int idx, uint64_t *out, char *err, size_t ecap) {
    if (lua_type(L, idx) == LUA_TNUMBER) {
#if LUA_VERSION_NUM >= 503
        if (lua_isinteger(L, idx)) {
            lua_Integer i = lua_tointeger(L, idx);
            if (i < 0) { snprintf(err, ecap, "expected non-negative integer"); return 0; }
            *out = (uint64_t)i; return 1;
        }
#endif
        lua_Number n = lua_tonumber(L, idx);
        if (n < 0.0) { snprintf(err, ecap, "expected non-negative number"); return 0; }
        *out = (uint64_t)n; return 1;
    }
    if (lua_type(L, idx) == LUA_TBOOLEAN) { *out = lua_toboolean(L, idx) ? 1u : 0u; return 1; }
    snprintf(err, ecap, "expected unsigned integer"); return 0;
}

static int pf_get_int64(lua_State *L, int idx, int64_t *out, char *err, size_t ecap) {
    if (lua_type(L, idx) == LUA_TNUMBER) {
#if LUA_VERSION_NUM >= 503
        if (lua_isinteger(L, idx)) { *out = (int64_t)lua_tointeger(L, idx); return 1; }
#endif
        *out = (int64_t)lua_tonumber(L, idx); return 1;
    }
    if (lua_type(L, idx) == LUA_TBOOLEAN) { *out = lua_toboolean(L, idx) ? 1 : 0; return 1; }
    snprintf(err, ecap, "expected integer"); return 0;
}

static int pf_get_double(lua_State *L, int idx, double *out, char *err, size_t ecap) {
    if (!lua_isnumber(L, idx)) { snprintf(err, ecap, "expected number"); return 0; }
    *out = (double)lua_tonumber(L, idx); return 1;
}

static int pf_get_pointer(lua_State *L, int idx, uintptr_t *out, char *err, size_t ecap) {
    if (pf_is_nullish(L, idx)) { *out = 0; return 1; }
    int t = lua_type(L, idx);
    if (t == LUA_TNUMBER) {
        uint64_t v; if (!pf_get_uint64(L, idx, &v, err, ecap)) return 0;
        *out = (uintptr_t)v; return 1;
    }
    if (t == LUA_TLIGHTUSERDATA) { *out = (uintptr_t)lua_touserdata(L, idx); return 1; }
    if (t == LUA_TTABLE || t == LUA_TUSERDATA) {
        int ai = lua_absindex(L, idx);
        lua_getfield(L, ai, "ptr");
        if (!lua_isnil(L, -1)) {
            int ok = pf_get_pointer(L, -1, out, err, ecap);
            lua_pop(L, 1); return ok;
        }
        lua_pop(L, 1);
    }
    snprintf(err, ecap, "expected pointer-like value"); return 0;
}

static void pf_push_uintptr(lua_State *L, uintptr_t v) {
    lua_pushinteger(L, (lua_Integer)(intptr_t)v);
}

static void pf_push_int64(lua_State *L, int64_t v) { lua_pushinteger(L, (lua_Integer)v); }
static void pf_push_uint64(lua_State *L, uint64_t v) {
#if LUA_VERSION_NUM >= 503
    if (v <= (uint64_t)LUA_MAXINTEGER) lua_pushinteger(L, (lua_Integer)v);
    else lua_pushnumber(L, (lua_Number)v);
#else
    lua_pushnumber(L, (lua_Number)v);
#endif
}

/* =========================================================================
 * Memory functions
 * ====================================================================== */
static int pf_alloc(lua_State *L) {
    uint64_t sz = 0; char e[128];
    if (!pf_get_uint64(L, 1, &sz, e, sizeof(e))) return pf_error(L, e);
    pf_push_uintptr(L, (uintptr_t)malloc((size_t)sz)); return 1;
}
static int pf_free(lua_State *L) {
    if (!pf_is_nullish(L, 1)) {
        uintptr_t p = 0; char e[128];
        if (!pf_get_pointer(L, 1, &p, e, sizeof(e))) return pf_error(L, e);
        free((void *)p);
    }
    return 0;
}
static int pf_realloc(lua_State *L) {
    uintptr_t p = 0; uint64_t sz = 0; char e[128];
    if (!pf_get_pointer(L, 1, &p, e, sizeof(e)) || !pf_get_uint64(L, 2, &sz, e, sizeof(e)))
        return pf_error(L, e);
    pf_push_uintptr(L, (uintptr_t)realloc((void *)p, (size_t)sz)); return 1;
}
static int pf_zero(lua_State *L) {
    uintptr_t p = 0; uint64_t sz = 0; char e[128];
    if (!pf_get_pointer(L, 1, &p, e, sizeof(e)) || !pf_get_uint64(L, 2, &sz, e, sizeof(e)))
        return pf_error(L, e);
    if (p && sz) memset((void *)p, 0, (size_t)sz);
    return 0;
}
static int pf_copy(lua_State *L) {
    uintptr_t d = 0, s = 0; uint64_t sz = 0; char e[128];
    if (!pf_get_pointer(L, 1, &d, e, sizeof(e)) || !pf_get_pointer(L, 2, &s, e, sizeof(e))
        || !pf_get_uint64(L, 3, &sz, e, sizeof(e))) return pf_error(L, e);
    if (d && s && sz) memcpy((void *)d, (const void *)s, (size_t)sz);
    return 0;
}
static int pf_set(lua_State *L) {
    uintptr_t p = 0; int64_t bv = 0; uint64_t sz = 0; char e[128];
    if (!pf_get_pointer(L, 1, &p, e, sizeof(e)) || !pf_get_int64(L, 2, &bv, e, sizeof(e))
        || !pf_get_uint64(L, 3, &sz, e, sizeof(e))) return pf_error(L, e);
    if (p && sz) memset((void *)p, (int)(bv & 0xFF), (size_t)sz);
    return 0;
}
static int pf_compare(lua_State *L) {
    uintptr_t a = 0, b = 0; uint64_t sz = 0; char e[128]; int cmp = 0;
    if (!pf_get_pointer(L, 1, &a, e, sizeof(e)) || !pf_get_pointer(L, 2, &b, e, sizeof(e))
        || !pf_get_uint64(L, 3, &sz, e, sizeof(e))) return pf_error(L, e);
    if (a && b && sz) cmp = memcmp((const void *)a, (const void *)b, (size_t)sz);
    lua_pushinteger(L, cmp); return 1;
}
static int pf_nullptr(lua_State *L) { pf_push_uintptr(L, 0); return 1; }
static int pf_sizeof_ptr(lua_State *L) { lua_pushinteger(L, (lua_Integer)sizeof(void*)); return 1; }

/* ptr read/write */
static int pf_readptr(lua_State *L) {
    uintptr_t p = 0, v = 0; char e[128];
    if (!pf_get_pointer(L, 1, &p, e, sizeof(e))) return pf_error(L, e);
    if (p) memcpy(&v, (const void *)p, sizeof(v));
    pf_push_uintptr(L, v); return 1;
}
static int pf_writeptr(lua_State *L) {
    uintptr_t p = 0, v = 0; char e[128];
    if (!pf_get_pointer(L, 1, &p, e, sizeof(e)) || !pf_get_pointer(L, 2, &v, e, sizeof(e)))
        return pf_error(L, e);
    if (p) memcpy((void *)p, &v, sizeof(v));
    return 0;
}
static int pf_readcstring(lua_State *L) {
    uintptr_t p = 0; char e[128];
    if (!pf_get_pointer(L, 1, &p, e, sizeof(e))) return pf_error(L, e);
    if (!p) lua_pushnil(L); else lua_pushstring(L, (const char *)p);
    return 1;
}
static int pf_writecstring(lua_State *L) {
    uintptr_t p = 0; char e[128];
    if (!pf_get_pointer(L, 1, &p, e, sizeof(e))) return pf_error(L, e);
    if (pf_is_nullish(L, 2)) { if (p) ((char *)p)[0] = '\0'; return 0; }
    const char *s = luaL_checkstring(L, 2);
    if (p) memcpy((void *)p, s, strlen(s) + 1);
    return 0;
}
static int pf_alloc_str(lua_State *L) {
    size_t len; const char *s = luaL_checklstring(L, 1, &len);
    void *p = malloc(len + 1);
    if (p) { memcpy(p, s, len); ((char *)p)[len] = '\0'; }
    pf_push_uintptr(L, (uintptr_t)p); return 1;
}

/* typed read/write macros */
#define PF_READ(name, ctype, push) \
static int pf_read_##name(lua_State *L) { \
    uintptr_t p = 0; char e[128]; ctype v = 0; \
    if (!pf_get_pointer(L, 1, &p, e, sizeof(e))) return pf_error(L, e); \
    if (p) memcpy(&v, (const void *)p, sizeof(v)); \
    push; return 1; }

#define PF_WRITE(name, ctype, get) \
static int pf_write_##name(lua_State *L) { \
    uintptr_t p = 0; char e[128]; ctype v = 0; \
    if (!pf_get_pointer(L, 1, &p, e, sizeof(e))) return pf_error(L, e); \
    get; if (p) memcpy((void *)p, &v, sizeof(v)); return 0; }

PF_READ(i8, int8_t, lua_pushinteger(L, v))
PF_READ(u8, uint8_t, lua_pushinteger(L, v))
PF_READ(i16, int16_t, lua_pushinteger(L, v))
PF_READ(u16, uint16_t, lua_pushinteger(L, v))
PF_READ(i32, int32_t, lua_pushinteger(L, v))
PF_READ(u32, uint32_t, lua_pushinteger(L, v))
PF_READ(i64, int64_t, pf_push_int64(L, v))
PF_READ(u64, uint64_t, pf_push_uint64(L, v))
PF_READ(f32, float, lua_pushnumber(L, (lua_Number)v))
PF_READ(f64, double, lua_pushnumber(L, (lua_Number)v))

#define PF_WRITE_I(name, ctype) PF_WRITE(name, ctype, do { int64_t _v; if (!pf_get_int64(L, 2, &_v, e, sizeof(e))) return pf_error(L, e); v = (ctype)_v; } while(0))
#define PF_WRITE_U(name, ctype) PF_WRITE(name, ctype, do { uint64_t _v; if (!pf_get_uint64(L, 2, &_v, e, sizeof(e))) return pf_error(L, e); v = (ctype)_v; } while(0))
#define PF_WRITE_F(name, ctype) PF_WRITE(name, ctype, do { double _v; if (!pf_get_double(L, 2, &_v, e, sizeof(e))) return pf_error(L, e); v = (ctype)_v; } while(0))

PF_WRITE_I(i8, int8_t)   PF_WRITE_U(u8, uint8_t)
PF_WRITE_I(i16, int16_t) PF_WRITE_U(u16, uint16_t)
PF_WRITE_I(i32, int32_t) PF_WRITE_U(u32, uint32_t)
PF_WRITE_I(i64, int64_t) PF_WRITE_U(u64, uint64_t)
PF_WRITE_F(f32, float)   PF_WRITE_F(f64, double)

/* =========================================================================
 * Schema system — struct_sizeof, struct_offsetof, view, view_array
 * These are implemented in pure Lua via the wrapper (see papagaio_ffi_wrapper.lua)
 * The C module exports the low-level primitives above.
 * ====================================================================== */

/* =========================================================================
 * Dynamic Loading (Tier 1)
 * ====================================================================== */
#ifdef PAPAGAIO_FFI_HAS_DL

#ifndef _WIN32
enum { PF_DL_LAZY=1, PF_DL_NOW=2, PF_DL_LOCAL=4, PF_DL_GLOBAL=8, PF_DL_NODELETE=16, PF_DL_NOLOAD=32 };

static int pf_dl_to_native(int flags) {
    int f = 0;
    if (flags & PF_DL_LAZY)     f |= RTLD_LAZY;
    if (flags & PF_DL_NOW)      f |= RTLD_NOW;
    if (flags & PF_DL_LOCAL)    f |= RTLD_LOCAL;
    if (flags & PF_DL_GLOBAL)   f |= RTLD_GLOBAL;
#ifdef RTLD_NODELETE
    if (flags & PF_DL_NODELETE) f |= RTLD_NODELETE;
#endif
#ifdef RTLD_NOLOAD
    if (flags & PF_DL_NOLOAD)   f |= RTLD_NOLOAD;
#endif
    if (!f) f = RTLD_NOW | RTLD_LOCAL;
    return f;
}
#endif /* !_WIN32 */

#define PF_LIB_MT "papagaio_ffi.lib"

typedef struct { void *handle; int closed; } UfLib;

static int pf_dl_open(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    int flags = (int)luaL_optinteger(L, 2, 0);
#ifndef _WIN32
    void *h = dlopen(path, pf_dl_to_native(flags));
#else
    (void)flags;
    void *h = (void *)LoadLibraryA(path);
#endif
    if (!h) { lua_pushnil(L); return 1; }
    UfLib *w = (UfLib *)lua_newuserdatauv(L, sizeof(UfLib), 0);
    w->handle = h; w->closed = 0;
    luaL_setmetatable(L, PF_LIB_MT);
    return 1;
}

static UfLib *pf_check_lib(lua_State *L, int idx) {
    return (UfLib *)luaL_checkudata(L, idx, PF_LIB_MT);
}

static int pf_dl_close(lua_State *L) {
    UfLib *w = pf_check_lib(L, 1);
    if (!w->closed && w->handle) {
#ifndef _WIN32
        dlclose(w->handle);
#else
        FreeLibrary((HMODULE)w->handle);
#endif
        w->handle = NULL; w->closed = 1;
    }
    return 0;
}

static int pf_dl_gc(lua_State *L) { return pf_dl_close(L); }

static int pf_dl_sym(lua_State *L) {
    UfLib *w = pf_check_lib(L, 1);
    const char *name = luaL_checkstring(L, 2);
    if (w->closed || !w->handle) { pf_push_uintptr(L, 0); return 1; }
#ifndef _WIN32
    void *s = dlsym(w->handle, name);
#else
    void *s = (void *)GetProcAddress((HMODULE)w->handle, name);
#endif
    pf_push_uintptr(L, (uintptr_t)s); return 1;
}

static int pf_dl_sym_self(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
#ifndef _WIN32
    void *s = dlsym(RTLD_DEFAULT, name);
#else
    void *s = (void *)GetProcAddress(GetModuleHandle(NULL), name);
#endif
    pf_push_uintptr(L, (uintptr_t)s); return 1;
}

static int pf_dl_error(lua_State *L) {
#ifndef _WIN32
    const char *e = dlerror();
    if (e) lua_pushstring(L, e); else lua_pushnil(L);
#else
    lua_pushnil(L);
#endif
    return 1;
}

static int pf_errno_val(lua_State *L) {
    lua_pushinteger(L, (lua_Integer)errno); return 1;
}

#endif /* PAPAGAIO_FFI_HAS_DL */

/* =========================================================================
 * FFI Calls (Tier 2)
 * ====================================================================== */
#ifdef PAPAGAIO_FFI_HAS_FFI

#define PF_MAX_ARGS 32
#define PF_DESC_MT  "papagaio_ffi.desc"
#define PF_BOUND_MT "papagaio_ffi.bound"
#define PF_CB_MT    "papagaio_ffi.cb"

typedef enum {
    PF_VOID=0, PF_I8, PF_U8, PF_I16, PF_U16, PF_I32, PF_U32,
    PF_I64, PF_U64, PF_F32, PF_F64, PF_BOOL, PF_CSTRING, PF_POINTER
} UfBaseType;

typedef struct { UfBaseType base; } UfArgType;
typedef struct {
    char name[128];
    UfArgType ret;
    UfArgType args[PF_MAX_ARGS];
    int argc;
    int has_varargs;
} UfSigParsed;

typedef struct {
    UfSigParsed sig;
} UfDescriptor;

typedef struct {
    void *fn_ptr;
    ffi_cif cif;
    ffi_type *ret_type;
    ffi_type *arg_types[PF_MAX_ARGS];
    UfSigParsed sig;
} UfBoundFn;

typedef struct UfCallbackData {
    lua_State *L;
    int fn_ref;
    UfSigParsed sig;
    ffi_closure *closure;
    void *fn_ptr;
    ffi_cif cif;
    ffi_type *arg_types[PF_MAX_ARGS];
#ifndef _WIN32
    pthread_t creator;
#endif
} UfCallbackData;

/* type parsing */
static UfBaseType pf_parse_type_name(const char *s) {
    if (!s) return PF_VOID;
    if (strcmp(s,"void")==0) return PF_VOID;
    if (strcmp(s,"i8")==0||strcmp(s,"int8")==0) return PF_I8;
    if (strcmp(s,"u8")==0||strcmp(s,"uint8")==0||strcmp(s,"byte")==0) return PF_U8;
    if (strcmp(s,"i16")==0||strcmp(s,"int16")==0) return PF_I16;
    if (strcmp(s,"u16")==0||strcmp(s,"uint16")==0) return PF_U16;
    if (strcmp(s,"i32")==0||strcmp(s,"int32")==0||strcmp(s,"int")==0) return PF_I32;
    if (strcmp(s,"u32")==0||strcmp(s,"uint32")==0||strcmp(s,"uint")==0) return PF_U32;
    if (strcmp(s,"i64")==0||strcmp(s,"int64")==0||strcmp(s,"long")==0) return PF_I64;
    if (strcmp(s,"u64")==0||strcmp(s,"uint64")==0||strcmp(s,"ulong")==0) return PF_U64;
    if (strcmp(s,"f32")==0||strcmp(s,"float32")==0||strcmp(s,"float")==0) return PF_F32;
    if (strcmp(s,"f64")==0||strcmp(s,"float64")==0||strcmp(s,"double")==0) return PF_F64;
    if (strcmp(s,"bool")==0||strcmp(s,"boolean")==0) return PF_BOOL;
    if (strcmp(s,"cstring")==0||strcmp(s,"string")==0) return PF_CSTRING;
    if (strcmp(s,"pointer")==0||strcmp(s,"ptr")==0) return PF_POINTER;
    return PF_VOID; /* fallback */
}

static ffi_type *pf_ffi_type(UfBaseType b) {
    switch (b) {
    case PF_VOID: return &ffi_type_void;
    case PF_I8: return &ffi_type_sint8;  case PF_U8: case PF_BOOL: return &ffi_type_uint8;
    case PF_I16: return &ffi_type_sint16; case PF_U16: return &ffi_type_uint16;
    case PF_I32: return &ffi_type_sint32; case PF_U32: return &ffi_type_uint32;
    case PF_I64: return &ffi_type_sint64; case PF_U64: return &ffi_type_uint64;
    case PF_F32: return &ffi_type_float;  case PF_F64: return &ffi_type_double;
    case PF_CSTRING: case PF_POINTER: return &ffi_type_pointer;
    default: return &ffi_type_void;
    }
}

/* Parse "ret_type name(arg1, arg2, ...)" */
static int pf_parse_sig(const char *sig, UfSigParsed *out, char *err, size_t ecap) {
    memset(out, 0, sizeof(*out));
    const char *p = sig;
    while (*p && isspace((unsigned char)*p)) p++;
    /* return type */
    const char *ts = p;
    while (*p && !isspace((unsigned char)*p) && *p != '(') p++;
    char tbuf[64]; size_t tl = (size_t)(p - ts);
    if (tl >= sizeof(tbuf)) tl = sizeof(tbuf) - 1;
    memcpy(tbuf, ts, tl); tbuf[tl] = '\0';
    out->ret.base = pf_parse_type_name(tbuf);
    /* name */
    while (*p && isspace((unsigned char)*p)) p++;
    const char *ns = p;
    while (*p && *p != '(' && !isspace((unsigned char)*p)) p++;
    size_t nl = (size_t)(p - ns);
    if (nl >= sizeof(out->name)) nl = sizeof(out->name) - 1;
    memcpy(out->name, ns, nl); out->name[nl] = '\0';
    /* skip to ( */
    while (*p && *p != '(') p++;
    if (*p == '(') p++;
    /* args */
    out->argc = 0;
    while (*p && *p != ')') {
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == ')') break;
        if (*p == '.' && p[1] == '.' && p[2] == '.') {
            out->has_varargs = 1; p += 3; continue;
        }
        const char *as = p;
        while (*p && *p != ',' && *p != ')' && !isspace((unsigned char)*p)) p++;
        char abuf[64]; size_t al = (size_t)(p - as);
        if (al >= sizeof(abuf)) al = sizeof(abuf) - 1;
        memcpy(abuf, as, al); abuf[al] = '\0';
        if (out->argc >= PF_MAX_ARGS) { snprintf(err, ecap, "too many args"); return 0; }
        out->args[out->argc++].base = pf_parse_type_name(abuf);
        while (*p && (*p == ',' || isspace((unsigned char)*p))) p++;
    }
    return 1;
}

/* Lua value ↔ FFI value */
typedef union { int8_t i8; uint8_t u8; int16_t i16; uint16_t u16; int32_t i32; uint32_t u32;
    int64_t i64; uint64_t u64; float f32; double f64; uintptr_t ptr; } UfArgBuf;

static int pf_lua_to_value(lua_State *L, int idx, UfBaseType base, UfArgBuf *buf, char **tmp, char *err, size_t ecap) {
    int64_t iv; uint64_t uv; double dv; uintptr_t pv;
    if (tmp) *tmp = NULL;
    switch (base) {
    case PF_VOID: return 1;
    case PF_BOOL: buf->u8 = lua_toboolean(L, idx) ? 1 : 0; return 1;
    case PF_I8: case PF_I16: case PF_I32: case PF_I64:
        if (!pf_get_int64(L, idx, &iv, err, ecap)) return 0;
        if (base==PF_I8) buf->i8=(int8_t)iv; else if (base==PF_I16) buf->i16=(int16_t)iv;
        else if (base==PF_I32) buf->i32=(int32_t)iv; else buf->i64=iv;
        return 1;
    case PF_U8: case PF_U16: case PF_U32: case PF_U64:
        if (!pf_get_uint64(L, idx, &uv, err, ecap)) return 0;
        if (base==PF_U8) buf->u8=(uint8_t)uv; else if (base==PF_U16) buf->u16=(uint16_t)uv;
        else if (base==PF_U32) buf->u32=(uint32_t)uv; else buf->u64=uv;
        return 1;
    case PF_F32: if (!pf_get_double(L, idx, &dv, err, ecap)) return 0; buf->f32=(float)dv; return 1;
    case PF_F64: if (!pf_get_double(L, idx, &dv, err, ecap)) return 0; buf->f64=dv; return 1;
    case PF_POINTER:
        if (!pf_get_pointer(L, idx, &pv, err, ecap)) return 0;
        buf->ptr=pv; return 1;
    case PF_CSTRING:
        if (pf_is_nullish(L, idx)) { buf->ptr = 0; return 1; }
        if (lua_type(L, idx) == LUA_TSTRING) {
            const char *s = lua_tostring(L, idx);
            char *d = (char *)malloc(strlen(s)+1); strcpy(d, s);
            buf->ptr = (uintptr_t)d; if (tmp) *tmp = d; return 1;
        }
        if (!pf_get_pointer(L, idx, &pv, err, ecap)) return 0;
        buf->ptr=pv; return 1;
    default: snprintf(err, ecap, "unsupported type"); return 0;
    }
}

static int pf_value_to_lua(lua_State *L, UfBaseType base, void *ret) {
    switch (base) {
    case PF_VOID: return 0;
    case PF_BOOL: lua_pushboolean(L, *(uint8_t *)ret != 0); return 1;
    case PF_I8: lua_pushinteger(L, *(int8_t *)ret); return 1;
    case PF_U8: lua_pushinteger(L, *(uint8_t *)ret); return 1;
    case PF_I16: lua_pushinteger(L, *(int16_t *)ret); return 1;
    case PF_U16: lua_pushinteger(L, *(uint16_t *)ret); return 1;
    case PF_I32: lua_pushinteger(L, *(int32_t *)ret); return 1;
    case PF_U32: lua_pushinteger(L, *(uint32_t *)ret); return 1;
    case PF_I64: pf_push_int64(L, *(int64_t *)ret); return 1;
    case PF_U64: pf_push_uint64(L, *(uint64_t *)ret); return 1;
    case PF_F32: lua_pushnumber(L, *(float *)ret); return 1;
    case PF_F64: lua_pushnumber(L, *(double *)ret); return 1;
    case PF_CSTRING: { void *p = *(void **)ret; if(!p) lua_pushnil(L); else lua_pushstring(L,(const char*)p); return 1; }
    case PF_POINTER: pf_push_uintptr(L, *(uintptr_t *)ret); return 1;
    default: return 0;
    }
}

static UfBaseType pf_guess_vararg(lua_State *L, int idx) {
    switch (lua_type(L, idx)) {
    case LUA_TBOOLEAN: return PF_BOOL;
    case LUA_TSTRING: return PF_CSTRING;
    case LUA_TNUMBER:
#if LUA_VERSION_NUM >= 503
        if (lua_isinteger(L, idx)) return PF_I64;
#endif
        return PF_F64;
    default: return PF_POINTER;
    }
}

/* ffi.describe(sig) */
static int pf_ffi_describe(lua_State *L) {
    const char *sig = luaL_checkstring(L, 1);
    char err[256];
    UfDescriptor *d = (UfDescriptor *)lua_newuserdatauv(L, sizeof(UfDescriptor), 0);
    memset(d, 0, sizeof(*d));
    if (!pf_parse_sig(sig, &d->sig, err, sizeof(err))) {
        lua_pop(L, 1); return pf_error(L, err);
    }
    luaL_setmetatable(L, PF_DESC_MT);
    return 1;
}

/* ffi.bind(ptr, descriptor) */
static int pf_ffi_bind(lua_State *L) {
    uintptr_t ptr = 0; char err[256];
    if (!pf_get_pointer(L, 1, &ptr, err, sizeof(err))) return pf_error(L, err);
    if (!ptr) return pf_error(L, "ffi.bind: null pointer");
    UfDescriptor *desc = (UfDescriptor *)luaL_checkudata(L, 2, PF_DESC_MT);

    UfBoundFn *bf = (UfBoundFn *)lua_newuserdatauv(L, sizeof(UfBoundFn), 0);
    memset(bf, 0, sizeof(*bf));
    bf->fn_ptr = (void *)ptr;
    bf->sig = desc->sig;
    bf->ret_type = pf_ffi_type(bf->sig.ret.base);
    for (int i = 0; i < bf->sig.argc; i++)
        bf->arg_types[i] = pf_ffi_type(bf->sig.args[i].base);

    ffi_status st = bf->sig.has_varargs
        ? ffi_prep_cif_var(&bf->cif, FFI_DEFAULT_ABI, (unsigned)bf->sig.argc, (unsigned)bf->sig.argc, bf->ret_type, bf->arg_types)
        : ffi_prep_cif(&bf->cif, FFI_DEFAULT_ABI, (unsigned)bf->sig.argc, bf->ret_type, bf->arg_types);
    if (st != FFI_OK) { lua_pop(L, 1); return pf_error(L, "ffi.bind: ffi_prep_cif failed"); }
    luaL_setmetatable(L, PF_BOUND_MT);
    return 1;
}

/* ffi.call(bound, {args}) */
static int pf_ffi_call(lua_State *L) {
    UfBoundFn *bf = (UfBoundFn *)luaL_checkudata(L, 1, PF_BOUND_MT);
    luaL_checktype(L, 2, LUA_TTABLE);

    int argc = (int)lua_rawlen(L, 2);
    if (argc > PF_MAX_ARGS) return pf_error(L, "too many arguments");

    UfArgBuf bufs[PF_MAX_ARGS]; void *ptrs[PF_MAX_ARGS];
    ffi_type *all_types[PF_MAX_ARGS];
    char *tmps[PF_MAX_ARGS]; char err[256];
    memset(tmps, 0, sizeof(tmps));

    for (int i = 0; i < argc; i++) {
        lua_rawgeti(L, 2, i + 1);
        UfBaseType base;
        if (i < bf->sig.argc) {
            base = bf->sig.args[i].base;
        } else {
            base = pf_guess_vararg(L, -1);
        }
        if (!pf_lua_to_value(L, -1, base, &bufs[i], &tmps[i], err, sizeof(err))) {
            lua_pop(L, 1);
            for (int j = 0; j < i; j++) free(tmps[j]);
            return pf_error(L, err);
        }
        ptrs[i] = &bufs[i];
        all_types[i] = pf_ffi_type(base);
        lua_pop(L, 1);
    }

    ffi_cif *cif = &bf->cif;
    ffi_cif var_cif;
    if (bf->sig.has_varargs && argc > bf->sig.argc) {
        for (int i = 0; i < bf->sig.argc; i++) all_types[i] = bf->arg_types[i];
        if (ffi_prep_cif_var(&var_cif, FFI_DEFAULT_ABI, (unsigned)bf->sig.argc, (unsigned)argc,
                             bf->ret_type, all_types) != FFI_OK) {
            for (int j = 0; j < argc; j++) free(tmps[j]);
            return pf_error(L, "ffi_prep_cif_var failed");
        }
        cif = &var_cif;
    }

    UfArgBuf retbuf; memset(&retbuf, 0, sizeof(retbuf));
    ffi_call(cif, FFI_FN(bf->fn_ptr), &retbuf, ptrs);
    for (int j = 0; j < argc; j++) free(tmps[j]);
    return pf_value_to_lua(L, bf->sig.ret.base, &retbuf);
}

/* callback trampoline */
static void pf_cb_trampoline(ffi_cif *cif, void *ret, void **args, void *user) {
    UfCallbackData *cb = (UfCallbackData *)user;
    if (!cb || !cb->L) return;
    lua_State *L = cb->L;
    int top = lua_gettop(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, cb->fn_ref);
    for (int i = 0; i < cb->sig.argc; i++)
        pf_value_to_lua(L, cb->sig.args[i].base, args[i]);

    if (lua_pcall(L, cb->sig.argc, 1, 0) != LUA_OK) {
        lua_settop(L, top);
        if (ret && cif->rtype && cif->rtype->size > 0) memset(ret, 0, cif->rtype->size);
        return;
    }
    if (cb->sig.ret.base != PF_VOID && ret) {
        UfArgBuf rbuf; memset(&rbuf, 0, sizeof(rbuf));
        char err[128];
        pf_lua_to_value(L, -1, cb->sig.ret.base, &rbuf, NULL, err, sizeof(err));
        memcpy(ret, &rbuf, cif->rtype->size > sizeof(rbuf) ? sizeof(rbuf) : cif->rtype->size);
    }
    lua_settop(L, top);
}

/* ffi.callback(descriptor, fn) */
static int pf_ffi_callback(lua_State *L) {
    UfDescriptor *desc = (UfDescriptor *)luaL_checkudata(L, 1, PF_DESC_MT);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    UfCallbackData *cb = (UfCallbackData *)lua_newuserdatauv(L, sizeof(UfCallbackData), 1);
    memset(cb, 0, sizeof(*cb));
    cb->L = L;
    cb->sig = desc->sig;
#ifndef _WIN32
    cb->creator = pthread_self();
#endif
    /* store fn ref */
    lua_pushvalue(L, 2);
    cb->fn_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    /* prepare cif */
    ffi_type *ret_type = pf_ffi_type(cb->sig.ret.base);
    for (int i = 0; i < cb->sig.argc; i++)
        cb->arg_types[i] = pf_ffi_type(cb->sig.args[i].base);

    if (ffi_prep_cif(&cb->cif, FFI_DEFAULT_ABI, (unsigned)cb->sig.argc, ret_type, cb->arg_types) != FFI_OK) {
        luaL_unref(L, LUA_REGISTRYINDEX, cb->fn_ref);
        return pf_error(L, "ffi.callback: ffi_prep_cif failed");
    }
    cb->closure = ffi_closure_alloc(sizeof(ffi_closure), &cb->fn_ptr);
    if (!cb->closure) {
        luaL_unref(L, LUA_REGISTRYINDEX, cb->fn_ref);
        return pf_error(L, "ffi.callback: ffi_closure_alloc failed");
    }
    if (ffi_prep_closure_loc(cb->closure, &cb->cif, pf_cb_trampoline, cb, cb->fn_ptr) != FFI_OK) {
        ffi_closure_free(cb->closure);
        luaL_unref(L, LUA_REGISTRYINDEX, cb->fn_ref);
        return pf_error(L, "ffi.callback: ffi_prep_closure_loc failed");
    }
    luaL_setmetatable(L, PF_CB_MT);

    /* return {ptr = ..., handle = userdata} */
    lua_newtable(L);
    pf_push_uintptr(L, (uintptr_t)cb->fn_ptr);
    lua_setfield(L, -2, "ptr");
    lua_pushvalue(L, -2); /* push the userdata */
    lua_setfield(L, -2, "handle");
    lua_remove(L, -2); /* remove the userdata, keep table */
    return 1;
}

static int pf_cb_gc(lua_State *L) {
    UfCallbackData *cb = (UfCallbackData *)luaL_checkudata(L, 1, PF_CB_MT);
    if (cb->closure) { ffi_closure_free(cb->closure); cb->closure = NULL; }
    if (cb->fn_ref) { luaL_unref(L, LUA_REGISTRYINDEX, cb->fn_ref); cb->fn_ref = 0; }
    return 0;
}

#endif /* PAPAGAIO_FFI_HAS_FFI */

/* =========================================================================
 * Module registration
 * ====================================================================== */
static const luaL_Reg pf_funcs[] = {
    /* memory core */
    {"alloc", pf_alloc}, {"free", pf_free}, {"realloc", pf_realloc},
    {"zero", pf_zero}, {"copy", pf_copy}, {"set", pf_set}, {"compare", pf_compare},
    {"nullptr", pf_nullptr}, {"sizeof_ptr", pf_sizeof_ptr},
    {"readptr", pf_readptr}, {"writeptr", pf_writeptr},
    {"readcstring", pf_readcstring}, {"writecstring", pf_writecstring},
    {"alloc_str", pf_alloc_str},
    {"readi8", pf_read_i8}, {"readu8", pf_read_u8},
    {"readi16", pf_read_i16}, {"readu16", pf_read_u16},
    {"readi32", pf_read_i32}, {"readu32", pf_read_u32},
    {"readi64", pf_read_i64}, {"readu64", pf_read_u64},
    {"readf32", pf_read_f32}, {"readf64", pf_read_f64},
    {"writei8", pf_write_i8}, {"writeu8", pf_write_u8},
    {"writei16", pf_write_i16}, {"writeu16", pf_write_u16},
    {"writei32", pf_write_i32}, {"writeu32", pf_write_u32},
    {"writei64", pf_write_i64}, {"writeu64", pf_write_u64},
    {"writef32", pf_write_f32}, {"writef64", pf_write_f64},
#ifdef PAPAGAIO_FFI_HAS_DL
    {"dl_open", pf_dl_open}, {"dl_close", pf_dl_close},
    {"dl_sym", pf_dl_sym}, {"dl_sym_self", pf_dl_sym_self},
    {"dl_error", pf_dl_error}, {"errno", pf_errno_val},
#endif
#ifdef PAPAGAIO_FFI_HAS_FFI
    {"ffi_describe", pf_ffi_describe}, {"ffi_bind", pf_ffi_bind},
    {"ffi_call", pf_ffi_call}, {"ffi_callback", pf_ffi_callback},
#endif
    {NULL, NULL}
};

LUALIB_API int luaopen_papagaio_ffi(lua_State *L) {
#if LUA_VERSION_NUM >= 502
    luaL_checkversion(L);
#endif

#ifdef PAPAGAIO_FFI_HAS_DL
    /* lib metatable */
    if (luaL_newmetatable(L, PF_LIB_MT)) {
        lua_pushcfunction(L, pf_dl_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
#endif

#ifdef PAPAGAIO_FFI_HAS_FFI
    if (luaL_newmetatable(L, PF_DESC_MT)) { lua_pop(L, 1); } else lua_pop(L, 1);
    if (luaL_newmetatable(L, PF_BOUND_MT)) { lua_pop(L, 1); } else lua_pop(L, 1);
    if (luaL_newmetatable(L, PF_CB_MT)) {
        lua_pushcfunction(L, pf_cb_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
#endif

    luaL_newlib(L, pf_funcs);

    /* feature flags */
#ifdef PAPAGAIO_FFI_HAS_FFI
    lua_pushboolean(L, 1);
#else
    lua_pushboolean(L, 0);
#endif
    lua_setfield(L, -2, "has_ffi");

#ifdef PAPAGAIO_FFI_HAS_DL
    lua_pushboolean(L, 1);
#else
    lua_pushboolean(L, 0);
#endif
    lua_setfield(L, -2, "has_dl");

#ifdef PAPAGAIO_FFI_HAS_DL
    /* dl_flags table */
    lua_newtable(L);
    lua_pushinteger(L, PF_DL_LAZY); lua_setfield(L, -2, "LAZY");
    lua_pushinteger(L, PF_DL_NOW); lua_setfield(L, -2, "NOW");
    lua_pushinteger(L, PF_DL_LOCAL); lua_setfield(L, -2, "LOCAL");
    lua_pushinteger(L, PF_DL_GLOBAL); lua_setfield(L, -2, "GLOBAL");
    lua_pushinteger(L, PF_DL_NODELETE); lua_setfield(L, -2, "NODELETE");
    lua_pushinteger(L, PF_DL_NOLOAD); lua_setfield(L, -2, "NOLOAD");
    lua_setfield(L, -2, "dl_flags");
#endif

    return 1;
}
