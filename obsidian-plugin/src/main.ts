import {
  App,
  ItemView,
  MarkdownView,
  Notice,
  Plugin,
  WorkspaceLeaf,
} from "obsidian";

// @ts-ignore — loaded at runtime
import createPapagaioModule from "../dist/papagaio.js";

const VIEW_TYPE = "papagaio-output";

/* ================================================================
 * Lua WASM Engine
 * ============================================================= */
interface PapagaioModule {
  ccall: (name: string, ret: string, args: string[], values: any[]) => any;
  cwrap: (name: string, ret: string, args: string[]) => Function;
  UTF8ToString: (ptr: number) => string;
  stringToUTF8: (str: string, ptr: number, max: number) => void;
  lengthBytesUTF8: (str: string) => number;
  _malloc: (size: number) => number;
  _free: (ptr: number) => void;
}

class LuaEngine {
  private mod: PapagaioModule | null = null;
  private ready = false;
  private initPromise: Promise<void> | null = null;

  async init(): Promise<void> {
    if (this.ready) return;
    if (this.initPromise) return this.initPromise;

    this.initPromise = (async () => {
      try {
        this.mod = await createPapagaioModule();
        this.ready = true;
      } catch (e) {
        console.error("papagaio-md: failed to init WASM", e);
        throw e;
      }
    })();

    return this.initPromise;
  }

  exec(code: string): string {
    if (!this.mod || !this.ready) return "[ERROR] WASM not initialized";
    try {
      const result = this.mod.ccall("papagaio_exec", "string", ["string"], [code]);
      return result || "";
    } catch (e: any) {
      return `[ERROR] ${e.message || e}`;
    }
  }

  execBlocks(codes: string[]): string {
    if (!this.mod || !this.ready) return "[ERROR] WASM not initialized";
    const parts: string[] = [];
    for (let i = 0; i < codes.length; i++) {
      parts.push(`--- block ${i + 1} ---`);
      try {
        const result = this.mod.ccall("papagaio_exec", "string", ["string"], [codes[i]]);
        if (result) parts.push(result);
      } catch (e: any) {
        parts.push(`[ERROR] ${e.message || e}`);
      }
    }
    return parts.join("\n");
  }

  execMd(mdContent: string): string {
    if (!this.mod || !this.ready) return "[ERROR] WASM not initialized";
    try {
      const result = this.mod.ccall("papagaio_exec_md", "string", ["string"], [mdContent]);
      return result || "";
    } catch (e: any) {
      return `[ERROR] ${e.message || e}`;
    }
  }

  version(): string {
    if (!this.mod || !this.ready) return "not initialized";
    return this.mod.ccall("papagaio_version", "string", [], []);
  }
}

/* ================================================================
 * Output View
 * ============================================================= */
class PapagaioOutputView extends ItemView {
  private content = "";

  constructor(leaf: WorkspaceLeaf) {
    super(leaf);
  }

  getViewType(): string {
    return VIEW_TYPE;
  }

  getDisplayText(): string {
    return "🦜 Papagaio Output";
  }

  getIcon(): string {
    return "terminal";
  }

  async onOpen(): Promise<void> {
    this.render();
  }

  setContent(text: string): void {
    this.content = text;
    this.render();
  }

  private render(): void {
    const container = this.containerEl.children[1] as HTMLElement;
    container.empty();

    const wrapper = container.createDiv({ cls: "papagaio-output-wrapper" });

    // Header
    const header = wrapper.createDiv({ cls: "papagaio-output-header" });
    header.createSpan({ text: "🦜 papagaio", cls: "papagaio-output-title" });

    // Content
    const pre = wrapper.createEl("pre", { cls: "papagaio-output-pre" });
    const code = pre.createEl("code", { cls: "papagaio-output-code" });
    code.textContent = this.content || "(no output yet — run Lua blocks from a note)";
  }
}

/* ================================================================
 * Plugin
 * ============================================================= */
export default class PapagaioMdPlugin extends Plugin {
  private engine = new LuaEngine();

  async onload(): Promise<void> {
    console.log("papagaio-md: loading plugin");

    // Register view
    this.registerView(VIEW_TYPE, (leaf) => new PapagaioOutputView(leaf));

    // Init WASM
    try {
      await this.engine.init();
      console.log("papagaio-md:", this.engine.version());
    } catch (e) {
      new Notice("❌ Papagaio: failed to load Lua engine");
      console.error("papagaio-md: init error", e);
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

  async onunload(): Promise<void> {
    this.app.workspace.detachLeavesOfType(VIEW_TYPE);
  }

  private async runCurrentNote(): Promise<void> {
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

    const output = this.engine.execMd(content);
    await this.showOutput(output);
    new Notice("🦜 Done!");
  }

  private async runSelectedCode(): Promise<void> {
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

  private async runCode(code: string, label: string): Promise<void> {
    try {
      await this.engine.init();
    } catch {
      new Notice("❌ Lua engine not available");
      return;
    }

    new Notice(`🦜 Running ${label}...`);
    const output = this.engine.exec(code);
    await this.showOutput(output);
    new Notice("🦜 Done!");
  }

  private async showOutput(output: string): Promise<void> {
    await this.activateView();

    const leaves = this.app.workspace.getLeavesOfType(VIEW_TYPE);
    if (leaves.length > 0) {
      const view = leaves[0].view as PapagaioOutputView;
      view.setContent(output);
    }
  }

  private async activateView(): Promise<void> {
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

  private injectStyles(): void {
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
      .papagaio-output-pre {
        flex: 1;
        margin: 0;
        padding: 16px;
        overflow: auto;
        background: var(--background-primary);
        font-family: var(--font-monospace);
        font-size: 13px;
        line-height: 1.5;
        white-space: pre-wrap;
        word-break: break-word;
        user-select: text;
        -webkit-user-select: text;
      }
      .papagaio-output-code {
        color: var(--text-normal);
      }
    `;
    document.head.appendChild(style);
    this.register(() => style.remove());
  }
}
