# Papagaio Ecosystem 🦜

The **Papagaio** project is a complete suite for text and data processing using Lua, designed for direct interaction with Markdown files. It is composed of three integrated tools:

1. **`papagaio`**: A standalone C library built natively for modern Lua pattern processing.
2. **`papagaio-ffi`**: A native C implementation for memory calls (FFI) optimized for the Lua stack.
3. **`papagaio-md` (CLI)**: A command-line interface that runs `.md` scripts natively with Lua.
4. **`obsidian-plugin`**: An Obsidian plugin that enables this entire engine on both Android and Desktop, *offline*, directly within an Obsidian view.

---

## 💻 1. The Obsidian Plugin (`obsidian-plugin`)

This magical plugin finds all ` ```lua ``` ` code blocks inside a note, **reads the outer Markdown structure** (converting headings like `# Title` and tables like `| Table |` into a `global.md` tree), and then executes the Lua code from your Markdown within its own virtual machine!

> [!TIP]
> **Total Compatibility**: Yes! It works flawlessly in the Obsidian app on **Android** as well as all **Desktop** systems. Because the entire Lua 5.4 engine (along with _papagaio_ and _papagaio-ffi_ core) is compiled into WebAssembly and injected as a single JavaScript bundle, no native Node.js functionality (`fs` or external binaries) gets in the way. It runs smoothly via the internal V8 / Chrome engine.

### ⬇️ How to build/install into Obsidian

The entire Papagaio suite can be recompiled with a single command, automatically generating the WASM distribution and injecting it into your Vault:

```bash
cd ~/repos/papagaio

# Compiles the whole ecosystem, builds the WASM Lua API and installs it to the target folder
make install-obsidian VAULT=~/Documents/my_vault
```

> **Attention**: Replace `~/Documents/my_vault` with the *actual* path to your Obsidian Vault on your development machine.
After running `make`, open Obsidian ➜ Settings ➜ Community Plugins and enable the `obsidian-plugin`.

### ⚡ How to use the Plugin Tools

In Obsidian, you can execute your codes using:
- **Command Palette** (Ctrl+P / Cmd+P):
  - `Run Lua blocks in current note`
  - `Run selected Lua code`
  - `Show output panel`
- **Sidebar Icons (Ribbon)**:
  - ▶️ "Run Lua file"
  - ⌨️ "Run selected Lua code"
- **View Panel Footer 🦜**:
  - In the "Papagaio Output" panel, there is a "Clear Output" button to clean the panel's standard output log.

Every `print()` standard output will render in this panel, formatted as copyable and selectable text.

### 🧩 How Markdown is translated (`global.md`)

All outer Markdown text is mapped into a global Lua table (`global.md`) so your Lua scripts can query it dynamically.

If your Markdown looks like this:
```markdown
# person
## name
john
## age 
45

| a | b |
|---|---|
| 1 | 2 |
| hi | bye |
```

The data structure available to your ` ```lua ` blocks will be generated exactly like this:
```lua
{
  ["person"] = {
    ["age"] = { 
      [1] = 45,                  -- Automatically converted to a number
      [2] = {                    -- The table generated continuous arrays
        [1] = { [1] = "a", [2] = "b" },
        [2] = { [1] = 1,   [2] = 2 },
        [3] = { [1] = "hi", [2] = "bye" },
      }
    },
    ["name"] = {
      [1] = "john",
    },
  },
}
```
From here, inside a Lua block within the same note, you just interact with it directly via `global.md.person.age[1]`.

---

## 📟 2. The CLI Tool (`papagaio-md`)

For real local/Linux environments, beyond the virtual notes, you have access to the **native CLI runner**, which comes with unrestricted Operating System features (pure Libffi with Dynamic Linking Callbacks via libdl).

### ⬇️ Compiling native modules

```bash
cd ~/repos/papagaio

# Downloads Lua and compiles the modules (papagaio) natively for the OS
make all
```
*This will generate the dynamic libraries `papagaio.so` and `papagaio_ffi.so` in their respective folders and will build the executable CLI tool `papagaio-md/papagaio-md`.*

### ⚡ How to use the CLI
Using a text file `notes.md` containing `#` headings or tables, exactly following the same structure used in the Obsidian plugin shown above:
```bash
./papagaio-md/papagaio-md ./any_folder/notes.md
```

Everything will be transformed into the `global.md` AST via C natively, and your embedded scripts will be executed in the generated Lua state perfectly.
