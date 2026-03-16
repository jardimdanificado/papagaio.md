# obsidian-plugin — Obsidian plugin
# ========================================
#
# Targets:
#   make          build WASM + plugin
#   make wasm     build WASM only
#   make plugin   build plugin JS only
#   make install  copy to Obsidian vault plugins dir
#   make clean    clean all
#
# Variables:
#   VAULT   path to your Obsidian vault (for install)

VAULT ?= $(HOME)/obsidian-vault
NODE  ?= node

.PHONY: all wasm plugin install clean

all: wasm plugin

wasm:
	$(MAKE) -C wasm

plugin: node_modules dist/papagaio.js
	$(NODE) build.mjs

node_modules:
	npm install

dist/papagaio.js: wasm
	@true

install: all
	@DEST=$$(echo $(VAULT)/.obsidian/plugins/papagaio-md); \
	mkdir -p "$$DEST" && \
	cp dist/main.js "$$DEST/" && \
	cp manifest.json "$$DEST/" && \
	cp styles.css "$$DEST/" && \
	echo "Installed to $$DEST/" && \
	echo "Restart Obsidian and enable the plugin."

clean:
	$(MAKE) -C wasm clean
	rm -rf dist/main.js node_modules
