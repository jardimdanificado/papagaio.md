# Build Lua 5.4 + papagaio + papagaio_ffi → WebAssembly
# Requires: Emscripten (emcc)

EMCC     ?= emcc
OUT_DIR  ?= ../dist
OUT_JS   = $(OUT_DIR)/papagaio.js

LUA_DIR  = ../../lua
PAP_DIR  = ../../papagaio
URB_DIR  = ../../papagaio-ffi

# Source files — compiled separately to avoid symbol collisions
LUA_SRC   = $(LUA_DIR)/onelua.c
PAP_SRC   = $(PAP_DIR)/papagaio.c
FFI_SRC   = $(URB_DIR)/papagaio_ffi.c
GLUE_SRC  = papagaio_wasm.c
MD_LUA    = ../../papagaio-md/papagaio_md.lua

# Object files
OBJS = lua.o papagaio.o papagaio_ffi.o papagaio_md_script.o papagaio_wasm.o

COMMON_FLAGS = -O2 -I$(LUA_DIR) -I$(PAP_DIR) -I$(URB_DIR)

LINK_FLAGS = \
    -s WASM=1 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="createPapagaioModule" \
    -s EXPORTED_FUNCTIONS='["_papagaio_exec","_papagaio_exec_blocks","_papagaio_exec_md","_papagaio_cleanup","_papagaio_version","_malloc","_free"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","UTF8ToString","stringToUTF8","lengthBytesUTF8"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s INITIAL_MEMORY=4194304 \
    -s STACK_SIZE=1048576 \
    -s NO_EXIT_RUNTIME=1 \
    -s ASYNCIFY=1 \
    -s FILESYSTEM=0 \
    -s ENVIRONMENT='web,worker' \
    -s SINGLE_FILE=1

.PHONY: all clean

all: $(OUT_JS)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

papagaio_md_script.c: $(MD_LUA)
	@echo "/* Auto-generated from papagaio_md.lua */" > $@
	@echo "const char papagaio_md_script[] =" >> $@
	@sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$$/\\n"/' $< >> $@
	@echo ";" >> $@

lua.o: $(LUA_SRC)
	$(EMCC) $(COMMON_FLAGS) -DMAKE_LIB -c $< -o $@

papagaio.o: $(PAP_SRC)
	$(EMCC) $(COMMON_FLAGS) -c $< -o $@

papagaio_ffi.o: $(FFI_SRC)
	$(EMCC) $(COMMON_FLAGS) -c $< -o $@

papagaio_md_script.o: papagaio_md_script.c
	$(EMCC) $(COMMON_FLAGS) -c $< -o $@

papagaio_wasm.o: $(GLUE_SRC)
	$(EMCC) $(COMMON_FLAGS) -c $< -o $@

$(OUT_JS): $(OBJS) | $(OUT_DIR)
	$(EMCC) $(COMMON_FLAGS) $(LINK_FLAGS) -o $@ $(OBJS)

clean:
	rm -f $(OBJS) $(OUT_JS) papagaio_md_script.c
