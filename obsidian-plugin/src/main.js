import {
  ItemView,
  MarkdownView,
  Notice,
  Plugin
} from "obsidian";

// @ts-ignore
import createPapagaioModule from "../dist/papagaio.js";

const VIEW_TYPE = "papagaio-output";

class LuaEngine {
  constructor() {
    this.mod = null;
    this.ready = false;
    this.initPromise = null;
  }

  async init() {
    if (this.ready) return;
    if (this.initPromise) return this.initPromise;

    this.initPromise = (async () => {
      try {
        this.mod = await createPapagaioModule();
        this.ready = true;
      } catch (e) {
        console.error("obsidian-plugin: failed to init WASM", e);
        throw e;
      }
    })();

    return this.initPromise;
  }

  async exec(code) {
    if (!this.mod || !this.ready) return "[ERROR] WASM not initialized";
    try {
      const result = await this.mod.ccall("papagaio_exec", "string", ["string"], [code]);
      return result || "";
    } catch (e) {
      return `[ERROR] ${e.message || e}`;
    }
  }

  async execMd(mdContent) {
    if (!this.mod || !this.ready) return "[ERROR] WASM not initialized";
    try {
      const result = await this.mod.ccall("papagaio_exec_md", "string", ["string"], [mdContent]);
      return result || "";
    } catch (e) {
      return `[ERROR] ${e.message || e}`;
    }
  }

  version() {
    if (!this.mod || !this.ready) return "not initialized";
    return this.mod.ccall("papagaio_version", "string", [], []);
  }
}

class PapagaioOutputView extends ItemView {
  constructor(leaf, plugin) {
    super(leaf);
    this.plugin = plugin;
    this.content = "";
    this.textarea = null;
  }

  getViewType() {
    return VIEW_TYPE;
  }

  getDisplayText() {
    return "🦜 Papagaio Output";
  }

  getIcon() {
    return "terminal";
  }

  async onOpen() {
    this.render();
  }

  setContent(text) {
    this.content = text;
    if (this.textarea) {
      this.textarea.value = text;
    } else {
      this.render();
    }
  }

  render() {
    const container = this.containerEl.children[1];
    container.empty();

    const wrapper = container.createDiv({ cls: "papagaio-output-wrapper" });

    // Header
    const header = wrapper.createDiv({ cls: "papagaio-output-header" });
    header.createSpan({ text: "🦜 Papagaio Output", cls: "papagaio-output-title" });

    // Editable Content
    this.textarea = wrapper.createEl("textarea", {
      cls: "papagaio-output-editor",
      placeholder: "(no output yet — run Lua blocks from a note)"
    });
    this.textarea.value = this.content;
    this.textarea.addEventListener("input", (e) => {
      this.content = e.target.value;
    });
  }
}

export default class PapagaioMdPlugin extends Plugin {
  constructor(app, manifest) {
    super(app, manifest);
    this.engine = new LuaEngine();
  }

  async onload() {
    console.log("obsidian-plugin: loading plugin");
    
    // Bridge for WASM
    window._papagaio_plugin = this;
    window.obsidian = require("obsidian");

    // Register view
    this.registerView(VIEW_TYPE, (leaf) => new PapagaioOutputView(leaf, this));

    // Init WASM
    try {
      await this.engine.init();
      console.log("obsidian-plugin:", this.engine.version());
    } catch (e) {
      new Notice("❌ Papagaio: failed to load Lua engine");
      console.error("obsidian-plugin: init error", e);
    }

    // Command: Run all Lua blocks
    this.addCommand({
      id: "run-lua-blocks",
      name: "Run Lua blocks in current note",
      callback: () => this.runCurrentNote(),
    });

    // Command: Run selected Lua code
    this.addCommand({
      id: "run-selected-lua",
      name: "Run selected Lua code",
      editorCallback: (editor) => {
        const selection = editor.getSelection();
        if (!selection || !selection.trim()) {
          new Notice("No text selected");
          return;
        }
        this.runCode(selection, "selection");
      },
    });

    // Ribbon Icons (Sidebar buttons)
    this.addRibbonIcon("play", "Run Lua file", () => {
      this.runCurrentNote();
    });

    this.addRibbonIcon("terminal", "Run selected Lua code", () => {
      this.runSelectedCode();
    });

    // Command: Show output panel
    this.addCommand({
      id: "show-output",
      name: "Show output panel",
      callback: () => this.activateView(),
    });

    // Inject styles
    this.injectStyles();
  }

  async onunload() {
    this.app.workspace.detachLeavesOfType(VIEW_TYPE);
  }

  async runCurrentNote() {
    const view = this.app.workspace.getActiveViewOfType(MarkdownView);
    if (!view) {
      new Notice("No active markdown note");
      return;
    }

    const content = view.editor.getValue();

    if (!content.includes("```lua")) {
      new Notice("No ```lua blocks found in this note");
      return;
    }

    new Notice(`🦜 Running markdown file...`);

    try {
      await this.engine.init();
    } catch {
      new Notice("❌ Lua engine not available");
      return;
    }

    const output = await this.engine.execMd(content);
    await this.showOutput(output);
    new Notice("🦜 Done!");
  }

  async runSelectedCode() {
    const view = this.app.workspace.getActiveViewOfType(MarkdownView);
    if (!view) {
      new Notice("No active markdown note");
      return;
    }
    const selection = view.editor.getSelection();
    if (!selection || !selection.trim()) {
      new Notice("No text selected");
      return;
    }
    this.runCode(selection, "selection");
  }

  async runCode(code, label) {
    try {
      await this.engine.init();
    } catch {
      new Notice("❌ Lua engine not available");
      return;
    }

    new Notice(`🦜 Running ${label}...`);
    const output = await this.engine.exec(code);
    await this.showOutput(output);
    new Notice("🦜 Done!");
  }

  async showOutput(output) {
    await this.activateView();

    const leaves = this.app.workspace.getLeavesOfType(VIEW_TYPE);
    if (leaves.length > 0) {
      const view = leaves[0].view;
      view.setContent(output);
    }
  }

  async activateView() {
    const existing = this.app.workspace.getLeavesOfType(VIEW_TYPE);
    if (existing.length > 0) {
      this.app.workspace.revealLeaf(existing[0]);
      return;
    }

    const leaf = this.app.workspace.getRightLeaf(false);
    if (leaf) {
      await leaf.setViewState({ type: VIEW_TYPE, active: true });
      this.app.workspace.revealLeaf(leaf);
    }
  }

  injectStyles() {
    const style = document.createElement("style");
    style.id = "obsidian-plugin-styles";
    style.textContent = `
      .papagaio-output-wrapper {
        height: 100%;
        display: flex;
        flex-direction: column;
        overflow: hidden;
      }
      .papagaio-output-header {
        padding: 12px 16px;
        border-bottom: 1px solid var(--background-modifier-border);
        background: var(--background-secondary);
        flex-shrink: 0;
      }
      .papagaio-output-title {
        font-weight: 600;
        font-size: 14px;
        color: var(--text-normal);
      }
      .papagaio-output-editor {
        flex: 1;
        margin: 0;
        padding: 16px;
        border: none;
        resize: none;
        background: var(--background-primary);
        font-family: var(--font-monospace);
        font-size: 13px;
        line-height: 1.5;
        color: var(--text-normal);
        outline: none;
      }
    `;
    document.head.appendChild(style);
    this.register(() => style.remove());
  }
}
