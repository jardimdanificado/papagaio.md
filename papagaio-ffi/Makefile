# papagaio_ffi — Lua FFI module with optional libffi / dlopen
# ======================================================
#
# Targets:
#   make               build with ALL features (DL + FFI)
#   make core          build core-only (no external deps)
#   make dl            build with dynamic loading only
#   make full          build with ALL features (same as default)
#   make test          build full + run teste.lua
#   make install       copy .so to LUA_LIBDIR
#   make clean         remove build artifacts
#
# Variables:
#   LUA            Lua flavour for pkg-config (default: lua5.4)
#   LUA_CFLAGS     override Lua header flags
#   LUA_LIBS       override Lua linker flags
#   FFI_CFLAGS     override libffi header flags
#   FFI_LIBS       override libffi linker flags
#   FEATURES       set to: core, dl, or full (default: full)
#   PREFIX         install prefix (default: ~/.local)
#   LUA_LIBDIR     where to install the .so (default: PREFIX/lib/lua/5.4)

LUA        ?= lua5.4
CC         ?= cc
PKG_CONFIG ?= pkg-config
FEATURES   ?= full

# ── Lua flags (auto-detect) ─────────────────────────────────────────────
LUA_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags $(LUA) 2>/dev/null \
              || $(PKG_CONFIG) --cflags lua-5.4 2>/dev/null \
              || $(PKG_CONFIG) --cflags lua54 2>/dev/null \
              || echo "")
LUA_LIBS   ?= $(shell $(PKG_CONFIG) --libs $(LUA) 2>/dev/null \
              || $(PKG_CONFIG) --libs lua-5.4 2>/dev/null \
              || $(PKG_CONFIG) --libs lua54 2>/dev/null \
              || echo "-llua -lm")

# ── libffi flags (auto-detect) ──────────────────────────────────────────
FFI_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags libffi 2>/dev/null || echo "")
FFI_LIBS   ?= $(shell $(PKG_CONFIG) --libs libffi 2>/dev/null || echo "-lffi")

# ── Platform ────────────────────────────────────────────────────────────
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  SHARED_FLAGS = -shared -fPIC -undefined dynamic_lookup
  SO_EXT       = .so
else
  SHARED_FLAGS = -shared -fPIC
  SO_EXT       = .so
endif

CFLAGS  ?= -O2 -Wall -Wextra -std=c11
LDFLAGS ?=

TARGET  = papagaio_ffi$(SO_EXT)
SRC     = papagaio_ffi.c

LUA_BIN := $(shell command -v $(LUA) 2>/dev/null || command -v lua 2>/dev/null)
PREFIX     ?= $(HOME)/.local
LUA_LIBDIR ?= $(PREFIX)/lib/lua/5.4

# ── Feature-dependent flags ─────────────────────────────────────────────
ifeq ($(FEATURES),full)
  FEAT_CFLAGS  = -DPAPAGAIO_FFI_HAS_ALL $(FFI_CFLAGS)
  FEAT_LDFLAGS = $(FFI_LIBS) -ldl
else ifeq ($(FEATURES),dl)
  FEAT_CFLAGS  = -DPAPAGAIO_FFI_HAS_DL
  FEAT_LDFLAGS = -ldl
else
  FEAT_CFLAGS  =
  FEAT_LDFLAGS =
endif

# ── Targets ─────────────────────────────────────────────────────────────
.PHONY: all core dl full clean test install

all: $(TARGET)

$(TARGET): $(SRC) papagaio_ffi.h
	$(CC) $(SHARED_FLAGS) $(CFLAGS) $(FEAT_CFLAGS) $(LUA_CFLAGS) -o $@ $< $(LDFLAGS) $(LUA_LIBS) $(FEAT_LDFLAGS)

core:
	$(MAKE) FEATURES=core $(TARGET)

dl:
	$(MAKE) FEATURES=dl $(TARGET)

full:
	$(MAKE) FEATURES=full $(TARGET)

test: $(TARGET)
	@if [ -z "$(LUA_BIN)" ]; then echo "Lua not found in PATH"; exit 1; fi
	@echo "=== papagaio_ffi test ($(FEATURES)) ==="
	LUA_CPATH="./?.so;;" $(LUA_BIN) teste.lua

install: $(TARGET)
	install -d $(LUA_LIBDIR)
	install -m 755 $(TARGET) $(LUA_LIBDIR)/$(TARGET)
	@echo "Installed $(TARGET) to $(LUA_LIBDIR)/"

clean:
	rm -f $(TARGET)
