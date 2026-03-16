#ifndef PAPAGAIO_H
#define PAPAGAIO_H

/*
 * papagaio — standalone pattern-matching / text-processing engine
 *
 * Dependencies: libc + Lua 5.1 / 5.2 / 5.3 / 5.4 / LuaJIT 2.x
 * No external headers beyond lua.h / lauxlib.h / lualib.h.
 *
 * ---------------------------------------------------------------------
 * Build as shared library:
 *
 *   Linux:
 *     cc -shared -fPIC -o papagaio.so papagaio.c \
 *        $(pkg-config --cflags --libs lua5.4)   # or lua5.1 / luajit
 *
 *   macOS:
 *     cc -shared -fPIC -undefined dynamic_lookup \
 *        -o papagaio.so papagaio.c \
 *        $(pkg-config --cflags lua5.4)
 *
 *   LuaJIT:
 *     cc -shared -fPIC -o papagaio.so papagaio.c \
 *        $(pkg-config --cflags --libs luajit)
 *
 * Then from Lua:
 *   local pap = require "papagaio"
 *
 * ---------------------------------------------------------------------
 * Lua API
 *
 *   pap.process(input, pat, repl [, pat, repl ...]) -> string
 *
 *     Simple replacement, no eval.
 *
 *       pap.process("hi $name", "$name", "world")
 *       --> "hi world"
 *
 *   pap.process_ex(input, sigil, open, close, pat, repl [...]) -> string
 *
 *     Custom sigil and block delimiters, no eval.
 *
 *       pap.process_ex("hi %[name]", "%", "[", "]", "%[name]", "world")
 *       --> "hi world"
 *
 *   pap.process_pairs(input, table) -> string
 *
 *     Table form — key=value or array-of-pairs:
 *       { ["$pat"] = "repl", ... }
 *       { {"$pat", "repl"}, ... }
 *
 *     $eval{} blocks in replacements run in the caller's Lua state, so
 *     every global and loaded module is visible:
 *
 *       pap.process_pairs("double $x = $eval{ return tonumber(match)*2 }",
 *                         { ["$x"] = "3" })
 *       --> "double 3 = 6"
 *
 *   pap.process_text(input) -> string
 *
 *     Full mode: the input may contain inline $pattern and $eval directives.
 *     $eval{} runs in the caller's Lua state.
 *
 * Inside every $eval{} block the global variable `match` holds the matched
 * substring.  The chunk must return a value; it is coerced to string via
 * tostring() before being spliced in.
 *
 * ---------------------------------------------------------------------
 * C embedding API
 *
 *   Papagaio *papagaio_open()           -- create context (fresh Lua state)
 *   void      papagaio_close(ctx)       -- destroy context
 *   lua_State*papagaio_L(ctx)           -- borrow inner lua_State
 *
 *   char *papagaio_process(input, ...)              -- NULL-terminated pairs
 *   char *papagaio_process_ex(input, sig, o, c, ...)
 *   char *papagaio_process_pairs(ctx, input, pats, repls, n)
 *   char *papagaio_process_text(ctx, input, len)
 *
 *   All return malloc'd strings; caller must free().
 *   Pass NULL as ctx to disable $eval{} support.
 */

#include <stddef.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Papagaio Papagaio;

/* Lifecycle */
Papagaio  *papagaio_open(void);
void       papagaio_close(Papagaio *ctx);
lua_State *papagaio_L(Papagaio *ctx);

/* C API */
char *papagaio_process(const char *input, ...);
char *papagaio_process_ex(const char *input,
                          const char *sigil,
                          const char *open,
                          const char *close, ...);
char *papagaio_process_pairs(Papagaio   *ctx,
                             const char *input,
                             const char **patterns,
                             const char **repls,
                             int         pair_count);
char *papagaio_process_text(Papagaio   *ctx,
                            const char *input,
                            size_t      len);

/* Lua module entry point — called automatically by require "papagaio" */
LUALIB_API int luaopen_papagaio(lua_State *L);

#ifdef __cplusplus
}
#endif
#endif /* PAPAGAIO_H */
