/*
 * papagaio-md CLI
 * Executes Markdown files with Lua blocks natively.
 * Embeds lua, papagaio, papagaio_ffi (with libffi + dl).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

extern int luaopen_papagaio(lua_State *L);
extern int luaopen_papagaio_ffi(lua_State *L);

/* Auto-generated table of the papagaio_md script */
extern const char papagaio_md_script[];

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t rb = fread(buf, 1, sz, f);
    buf[rb] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: papagaio-md <file.md>\n");
        return 1;
    }

    const char *filepath = argv[1];
    char *md_content = read_file(filepath);
    if (!md_content) {
        fprintf(stderr, "Error: cannot read file '%s'\n", filepath);
        return 1;
    }

    lua_State *L = luaL_newstate();
    if (!L) {
        fprintf(stderr, "Error: cannot create Lua state\n");
        free(md_content);
        return 1;
    }

    luaL_openlibs(L);

    luaL_requiref(L, "papagaio", luaopen_papagaio, 1);
    lua_pop(L, 1);

    luaL_requiref(L, "papagaio_ffi", luaopen_papagaio_ffi, 1);
    lua_pop(L, 1);

    /* Load papagaio_md module */
    if (luaL_dostring(L, papagaio_md_script) != LUA_OK) {
        fprintf(stderr, "Fatal: failed to load papagaio_md script: %s\n", lua_tostring(L, -1));
        lua_close(L);
        free(md_content);
        return 1;
    }
    
    /* Call papagaio_md.run(md_content) */
    lua_getfield(L, -1, "run");
    lua_pushstring(L, md_content);
    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        fprintf(stderr, "Error executing '%s':\n%s\n", filepath, lua_tostring(L, -1));
        lua_close(L);
        free(md_content);
        return 1;
    }

    lua_close(L);
    free(md_content);
    return 0;
}
