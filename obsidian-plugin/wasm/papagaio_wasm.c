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

/* ========================================================================
 * Obsidian Bridge
 * ===================================================================== */

EM_ASYNC_JS(char*, js_obsidian_read, (const char* path), {
    const p = UTF8ToString(path);
    const plugin = window._papagaio_plugin;
    if (!plugin) return 0;
    try {
        const content = await plugin.app.vault.adapter.read(p);
        const lengthBytes = lengthBytesUTF8(content) + 1;
        const stringOnWasmHeap = _malloc(lengthBytes);
        stringToUTF8(content, stringOnWasmHeap, lengthBytes);
        return stringOnWasmHeap;
    } catch (e) {
        return 0;
    }
});

EM_ASYNC_JS(int, js_obsidian_write, (const char* path, const char* content), {
    const p = UTF8ToString(path);
    const c = UTF8ToString(content);
    const plugin = window._papagaio_plugin;
    if (!plugin) return 0;
    try {
        await plugin.app.vault.adapter.write(p, c);
        return 1;
    } catch (e) {
        console.error("obsidian-plugin: write error", e);
        return 0;
    }
});

EM_JS(char*, js_obsidian_get_active_path, (), {
    const plugin = window._papagaio_plugin;
    if (!plugin) return 0;
    const view = plugin.app.workspace.getActiveViewOfType(window.obsidian.MarkdownView);
    const path = view ? view.file.path : "";
    const lengthBytes = lengthBytesUTF8(path) + 1;
    const stringOnWasmHeap = _malloc(lengthBytes);
    stringToUTF8(path, stringOnWasmHeap, lengthBytes);
    return stringOnWasmHeap;
});

EM_JS(char*, js_obsidian_get_active_content, (), {
    const plugin = window._papagaio_plugin;
    if (!plugin) return 0;
    const view = plugin.app.workspace.getActiveViewOfType(window.obsidian.MarkdownView);
    const content = view ? view.editor.getValue() : "";
    const lengthBytes = lengthBytesUTF8(content) + 1;
    const stringOnWasmHeap = _malloc(lengthBytes);
    stringToUTF8(content, stringOnWasmHeap, lengthBytes);
    return stringOnWasmHeap;
});

EM_JS(void, js_obsidian_notice, (const char* text), {
    const t = UTF8ToString(text);
    new window.obsidian.Notice(t);
});

EM_ASYNC_JS(int, js_obsidian_mkdir, (const char* path), {
    const p = UTF8ToString(path);
    const plugin = window._papagaio_plugin;
    if (!plugin) return 0;
    try {
        await plugin.app.vault.adapter.mkdir(p);
        return 1;
    } catch (e) {
        return 0;
    }
});

EM_JS(char*, js_obsidian_get_metadata, (const char* path), {
    const p = UTF8ToString(path);
    const plugin = window._papagaio_plugin;
    if (!plugin) return 0;
    const file = plugin.app.vault.getAbstractFileByPath(p);
    if (!file) return 0;
    const cache = plugin.app.metadataCache.getFileCache(file);
    if (!cache || !cache.frontmatter) return 0;
    const fm = JSON.stringify(cache.frontmatter);
    const lengthBytes = lengthBytesUTF8(fm) + 1;
    const stringOnWasmHeap = _malloc(lengthBytes);
    stringToUTF8(fm, stringOnWasmHeap, lengthBytes);
    return stringOnWasmHeap;
});

EM_JS(char*, js_obsidian_list_files, (), {
// ... skipping unchanged lines until list_files ...
    return stringOnWasmHeap;
});

static int lua_obsidian_notice(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);
    js_obsidian_notice(text);
    return 0;
}

static int lua_obsidian_mkdir(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    int ok = js_obsidian_mkdir(path);
    lua_pushboolean(L, ok);
    return 1;
}

static int lua_obsidian_get_metadata(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    char *res = js_obsidian_get_metadata(path);
    if (!res) {
        lua_newtable(L);
        return 1;
    }
    // Simple JSON parsing hack for frontmatter (just keys and string/number values)
    // We could use a proper Lua JSON lib, but let's keep it simple for now
    // Actually, let's just push the raw JSON string and let the user handle it
    // or we can do a very basic parser here.
    lua_pushstring(L, res);
    free(res);
    return 1;
}

static int lua_obsidian_read(lua_State *L) {
// ... skipping unchanged lines until read ...
    return 1;
}

static int lua_obsidian_write(lua_State *L) {
// ... skipping unchanged lines until write ...
    return 1;
}

static int lua_obsidian_get_active_path(lua_State *L) {
// ... skipping unchanged lines until active_path ...
    return 1;
}

static int lua_obsidian_get_active_content(lua_State *L) {
// ... skipping unchanged lines until active_content ...
    return 1;
}

static int lua_obsidian_list_files(lua_State *L) {
// ... skipping unchanged lines until list_files ...
    return 1;
}

static const luaL_Reg obsidian_funcs[] = {
    {"read", lua_obsidian_read},
    {"write", lua_obsidian_write},
    {"get_active_path", lua_obsidian_get_active_path},
    {"get_active_content", lua_obsidian_get_active_content},
    {"list_files", lua_obsidian_list_files},
    {"notice", lua_obsidian_notice},
    {"mkdir", lua_obsidian_mkdir},
    {"get_metadata_json", lua_obsidian_get_metadata},
    {NULL, NULL}
};

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

    /* Register obsidian module */
    luaL_newlib(L, obsidian_funcs);
    
    char *ap = js_obsidian_get_active_path();
    if (ap) { lua_pushstring(L, ap); free(ap); } else { lua_pushstring(L, ""); }
    lua_setfield(L, -2, "active_path");

    char *ac = js_obsidian_get_active_content();
    if (ac) { lua_pushstring(L, ac); free(ac); } else { lua_pushstring(L, ""); }
    lua_setfield(L, -2, "active_content");

    lua_setglobal(L, "obsidian");

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
