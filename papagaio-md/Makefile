# papagaio-md CLI Makefile
# Compiles the standalone CLI tool which runs .md files using
# papagaio and papagaio_ffi (with libffi and dlopen).

TARGET = papagaio-md

CC         ?= cc
PKG_CONFIG ?= pkg-config
LUA        ?= lua5.4

PREFIX     ?= $(HOME)/.local
BINDIR     ?= $(PREFIX)/bin

LUA_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags $(LUA) 2>/dev/null \
              || $(PKG_CONFIG) --cflags lua-5.4 2>/dev/null \
              || $(PKG_CONFIG) --cflags lua54 2>/dev/null \
              || echo "")
LUA_LIBS   ?= $(shell $(PKG_CONFIG) --libs $(LUA) 2>/dev/null \
              || $(PKG_CONFIG) --libs lua-5.4 2>/dev/null \
              || $(PKG_CONFIG) --libs lua54 2>/dev/null \
              || echo "-llua -lm")

FFI_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags libffi 2>/dev/null || echo "")
FFI_LIBS   ?= $(shell $(PKG_CONFIG) --libs libffi 2>/dev/null || echo "-lffi")
DL_LIBS     = $(if $(findstring NT,$(OS)),,-ldl)

CFLAGS  += -O2 -Wall -Wextra -std=c11 $(LUA_CFLAGS) -DPAPAGAIO_FFI_HAS_ALL $(FFI_CFLAGS)
LDFLAGS += $(LUA_LIBS) $(FFI_LIBS) $(DL_LIBS)

PAP_DIR = ../papagaio
FFI_DIR = ../papagaio-ffi

# Sources
CLI_SRC = cli.c
PAP_SRC = $(PAP_DIR)/papagaio.c
FFI_SRC = $(FFI_DIR)/papagaio_ffi.c
GEN_SRC = papagaio_md_script.c

OBJS = cli.o papagaio.o papagaio_ffi.o papagaio_md_script.o

.PHONY: all clean install

all: $(TARGET)

papagaio_md_script.c: papagaio_md.lua
	@echo "/* Auto-generated from papagaio_md.lua */" > $@
	@echo "const char papagaio_md_script[] =" >> $@
	@sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$$/\\n"/' $< >> $@
	@echo ";" >> $@

cli.o: $(CLI_SRC) papagaio_md_script.c
	$(CC) $(CFLAGS) -c $< -o $@

papagaio.o: $(PAP_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

papagaio_ffi.o: $(FFI_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

papagaio_md_script.o: $(GEN_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/$(TARGET)
	@echo "Installed $(TARGET) to $(BINDIR)/"

clean:
	rm -f $(TARGET) $(OBJS) $(GEN_SRC)
