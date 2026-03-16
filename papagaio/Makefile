# papagaio — Lua pattern-matching / text-processing module
# ========================================================
#
# Targets:
#   make            build papagaio.so (default Lua 5.4)
#   make LUA=luajit build for LuaJIT
#   make test       build + run teste.lua
#   make install    copy .so to LUA_LIBDIR (user-local by default)
#   make clean      remove build artifacts
#
# Variables:
#   LUA          Lua flavour for pkg-config (default: lua5.4)
#   LUA_CFLAGS   override compiler flags for Lua headers
#   LUA_LIBS     override linker flags for Lua library
#   PREFIX       install prefix (default: ~/.local)
#   LUA_LIBDIR   where to install the .so (default: PREFIX/lib/lua/5.4)

LUA        ?= lua5.4
CC         ?= cc
PKG_CONFIG ?= pkg-config

# ── Lua flags (auto-detect) ─────────────────────────────────────────────
LUA_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags $(LUA) 2>/dev/null \
              || $(PKG_CONFIG) --cflags lua-5.4 2>/dev/null \
              || $(PKG_CONFIG) --cflags lua54 2>/dev/null \
              || echo "")
LUA_LIBS   ?= $(shell $(PKG_CONFIG) --libs $(LUA) 2>/dev/null \
              || $(PKG_CONFIG) --libs lua-5.4 2>/dev/null \
              || $(PKG_CONFIG) --libs lua54 2>/dev/null \
              || echo "-llua -lm")

# ── Platform ────────────────────────────────────────────────────────────
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  SHARED_FLAGS = -shared -fPIC -undefined dynamic_lookup
  SO_EXT       = .so
else
  SHARED_FLAGS = -shared -fPIC
  SO_EXT       = .so
endif

CFLAGS  ?= -O2 -Wall -Wextra -std=c99
LDFLAGS ?=

TARGET  = papagaio$(SO_EXT)
SRC     = papagaio.c

LUA_BIN := $(shell command -v $(LUA) 2>/dev/null || command -v lua 2>/dev/null)
PREFIX     ?= $(HOME)/.local
LUA_LIBDIR ?= $(PREFIX)/lib/lua/5.4

# ── Targets ─────────────────────────────────────────────────────────────
.PHONY: all clean test install

all: $(TARGET)

$(TARGET): $(SRC) papagaio.h
	$(CC) $(SHARED_FLAGS) $(CFLAGS) $(LUA_CFLAGS) -o $@ $< $(LDFLAGS) $(LUA_LIBS)

test: $(TARGET)
	@if [ -z "$(LUA_BIN)" ]; then echo "Lua not found in PATH"; exit 1; fi
	@echo "=== papagaio test ==="
	LUA_CPATH="./?.so;;" $(LUA_BIN) teste.lua

install: $(TARGET)
	install -d $(LUA_LIBDIR)
	install -m 755 $(TARGET) $(LUA_LIBDIR)/$(TARGET)
	@echo "Installed $(TARGET) to $(LUA_LIBDIR)/"

clean:
	rm -f $(TARGET)
