/*
 * papagaio_wasm.c — WASM glue for obsidian-plugin Obsidian plugin
 *
 * This file is compiled separately and linked with:
 *   - Lua 5.4 (onelua.c compiled as lib)
 *   - papagaio.c
 *   - papagaio_ffi.c (core only)
 */

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <emscripten.h>

/* External module openers */
extern int luaopen_papagaio(lua_State *L);
extern int luaopen_papagaio_ffi(lua_State *L);

/* Auto-generated table of the papagaio_md script */
extern const char papagaio_md_script[];

/* ========================================================================
 * Output capture
 * ===================================================================== */
static char *g_output_buf = NULL;
static size_t g_output_len = 0;
static size_t g_output_cap = 0;

static void output_reset(void) {
    if (g_output_buf) g_output_buf[0] = '\0';
    g_output_len = 0;
}

static void output_append(const char *s, size_t len) {
    if (!len) return;
    size_t need = g_output_len + len + 1;
    if (need > g_output_cap) {
        size_t newcap = g_output_cap ? g_output_cap * 2 : 4096;
        if (newcap < need) newcap = need;
        g_output_buf = (char *)realloc(g_output_buf, newcap);
        g_output_cap = newcap;
    }
    memcpy(g_output_buf + g_output_len, s, len);
    g_output_len += len;
    g_output_buf[g_output_len] = '\0';
}

static void output_append_str(const char *s) {
    output_append(s, strlen(s));
}

/* Custom print → buffer */
static int wasm_print(lua_State *L) {
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) {
        size_t len;
        const char *s = luaL_tolstring(L, i, &len);
        if (i > 1) output_append("\t", 1);
        output_append(s, len);
        lua_pop(L, 1);
    }
    output_append("\n", 1);
    return 0;
}

/* Custom io.write → buffer */
static int wasm_io_write(lua_State *L) {
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) {
        size_t len;
        const char *s = luaL_checklstring(L, i, &len);
        output_append(s, len);
    }
    return 0;
}

/* Setup sandbox */
static void setup_sandbox(lua_State *L) {
    lua_pushcfunction(L, wasm_print);
    lua_setglobal(L, "print");

    lua_getglobal(L, "io");
    if (lua_istable(L, -1)) {
        lua_pushcfunction(L, wasm_io_write);
        lua_setfield(L, -2, "write");
    }
    lua_pop(L, 1);

    lua_pushnil(L); lua_setglobal(L, "dofile");
    lua_pushnil(L); lua_setglobal(L, "loadfile");

    luaL_requiref(L, "papagaio", luaopen_papagaio, 1);
    lua_pop(L, 1);

    luaL_requiref(L, "papagaio_ffi", luaopen_papagaio_ffi, 1);
    lua_pop(L, 1);
}

/* ========================================================================
 * Exported WASM API
 * ===================================================================== */

EMSCRIPTEN_KEEPALIVE
const char *papagaio_exec(const char *code) {
    output_reset();

    lua_State *L = luaL_newstate();
    if (!L) {
        output_append_str("[ERROR] failed to create Lua state\n");
        return g_output_buf ? g_output_buf : "";
    }

    luaL_openlibs(L);
    setup_sandbox(L);

    if (luaL_dostring(L, code) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        output_append_str("[ERROR] ");
        output_append_str(err ? err : "unknown error");
        output_append_str("\n");
        lua_pop(L, 1);
    }

    lua_close(L);
    return g_output_buf ? g_output_buf : "";
}

EMSCRIPTEN_KEEPALIVE
const char *papagaio_exec_blocks(const char *blocks) {
    output_reset();

    lua_State *L = luaL_newstate();
    if (!L) {
        output_append_str("[ERROR] failed to create Lua state\n");
        return g_output_buf ? g_output_buf : "";
    }

    luaL_openlibs(L);
    setup_sandbox(L);

    const char *p = blocks;
    int block_num = 0;
    while (*p) {
        size_t len = strlen(p);
        block_num++;

        char header[64];
        snprintf(header, sizeof(header), "--- block %d ---\n", block_num);
        output_append_str(header);

        if (luaL_dostring(L, p) != LUA_OK) {
            const char *err = lua_tostring(L, -1);
            output_append_str("[ERROR] ");
            output_append_str(err ? err : "unknown error");
            output_append_str("\n");
            lua_pop(L, 1);
        }

        p += len + 1;
    }

    lua_close(L);
    return g_output_buf ? g_output_buf : "";
}

EMSCRIPTEN_KEEPALIVE
const char *papagaio_exec_md(const char *md_content) {
    output_reset();

    lua_State *L = luaL_newstate();
    if (!L) {
        output_append_str("[ERROR] failed to create Lua state\n");
        return g_output_buf ? g_output_buf : "";
    }

    luaL_openlibs(L);
    setup_sandbox(L);

    if (luaL_dostring(L, papagaio_md_script) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        output_append_str("Fatal: failed to load papagaio_md script: ");
        output_append_str(err ? err : "unknown");
        output_append_str("\n");
        lua_close(L);
        return g_output_buf ? g_output_buf : "";
    }

    lua_getfield(L, -1, "run");
    if (!lua_isfunction(L, -1)) {
        output_append_str("[ERROR] papagaio_md.run is not a function\n");
        lua_close(L);
        return g_output_buf ? g_output_buf : "";
    }
    lua_pushstring(L, md_content);
    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        output_append_str("[ERROR] ");
        output_append_str(err ? err : "unknown error");
        output_append_str("\n");
        lua_pop(L, 1);
    }

    lua_close(L);
    return g_output_buf ? g_output_buf : "";
}

EMSCRIPTEN_KEEPALIVE
void papagaio_cleanup(void) {
    free(g_output_buf);
    g_output_buf = NULL;
    g_output_len = 0;
    g_output_cap = 0;
}

EMSCRIPTEN_KEEPALIVE
const char *papagaio_version(void) {
    return "obsidian-plugin 1.0 (Lua " LUA_VERSION_MAJOR "." LUA_VERSION_MINOR ")";
}
