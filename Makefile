# Root Makefile — builds papagaio, papagaio-ffi, papagaio-md CLI, and obsidian-plugin
# ======================================================================================
#
# Targets:
#   make            build native modules (papagaio + papagaio-ffi) + CLI
#   make papagaio   build papagaio standalone module
#   make ffi        build papagaio-ffi (full: DL + FFI)
#   make cli        build papagaio-md CLI
#   make obsidian   build obsidian-plugin (WASM)
#   make install-obsidian  build everything and install plugin to Obsidian
#   make test       build + test native modules
#   make clean      clean all (including CLI and obsidian plugin)
#
# Variables:
#   LUA            Lua flavour (default: lua5.4)
#   FEATURES       papagaio-ffi feature tier: core, dl, full (default: full)
#   VAULT          path to your Obsidian vault (for install-obsidian)

LUA      ?= lua5.4
FEATURES ?= full

PKG_CONFIG ?= pkg-config
HAS_LUA_PC := $(shell $(PKG_CONFIG) --exists $(LUA) 2>/dev/null && echo "yes" \
               || $(PKG_CONFIG) --exists lua-5.4 2>/dev/null && echo "yes" \
               || $(PKG_CONFIG) --exists lua54 2>/dev/null && echo "yes" \
               || echo "no")

CC     ?= cc
AR     ?= ar
RANLIB ?= ranlib

ifeq ($(HAS_LUA_PC),yes)
  # System has Lua via pkg-config
  DEPS = fetch_lua_if_needed
  LUA_ARGS = LUA=$(LUA)
else
  # No system Lua found; use local fetch & build
  LUA_LOCAL_PATH = $(PWD)/lua
  LUA_ARGS = LUA=$(LUA) \
             LUA_CFLAGS="-I$(LUA_LOCAL_PATH)" \
             LUA_LIBS="-L$(LUA_LOCAL_PATH) -llua -lm" \
             LUA_BIN="$(LUA_LOCAL_PATH)/lua"
  DEPS = build_local_lua
endif

.PHONY: all papagaio ffi cli obsidian test clean fetch_lua_if_needed build_local_lua install-obsidian

all: $(DEPS) papagaio ffi cli

fetch_lua_if_needed:
	@if [ ! -d "lua" ]; then \
		echo "Fetching Lua source from GitHub..."; \
		git clone https://github.com/lua/lua.git lua; \
	fi

build_local_lua: fetch_lua_if_needed
	@if [ ! -f "lua/liblua.a" ]; then \
		echo "Building local Lua for target..."; \
		$(MAKE) -C lua CC="$(CC)" AR="$(AR) rc" RANLIB="$(RANLIB)" MYCFLAGS="-fPIC" MYLDFLAGS="-fPIC" a; \
	fi

papagaio: $(DEPS)
	$(MAKE) -C papagaio $(LUA_ARGS)

ffi: $(DEPS)
	$(MAKE) -C papagaio-ffi $(LUA_ARGS) FEATURES=$(FEATURES)

cli: $(DEPS)
	$(MAKE) -C papagaio-md $(LUA_ARGS)

obsidian: fetch_lua_if_needed
	$(MAKE) -C obsidian-plugin

wasm: fetch_lua_if_needed
	$(MAKE) -C obsidian-plugin wasm

install-obsidian:
	@if [ -z "$(VAULT)" ]; then echo "VAULT not set. Use: make install-obsidian VAULT=~/Documents/teste"; exit 1; fi
	$(MAKE) clean
	$(MAKE) all
	cd obsidian-plugin && npm install && $(MAKE) clean && $(MAKE) install VAULT="$(VAULT)"

test: $(DEPS) papagaio ffi
	$(MAKE) -C papagaio $(LUA_ARGS) test
	$(MAKE) -C papagaio-ffi $(LUA_ARGS) FEATURES=$(FEATURES) test

clean:
	$(MAKE) -C papagaio clean
	$(MAKE) -C papagaio-ffi clean
	$(MAKE) -C papagaio-md clean
	$(MAKE) -C obsidian-plugin clean
	@if [ -d "lua" ]; then $(MAKE) -C lua clean 2>/dev/null || true; fi
