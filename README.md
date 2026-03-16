# Papagaio Ecosystem 🦜

The Papagaio ecosystem is a high-performance text processing and automation suite powered by Lua 5.4. It is designed to bridge the gap between structured Markdown documents and programmable logic, offering a unified engine that runs natively on Desktop (CLI) and seamlessly within Obsidian (Plugin/WASM).

## Components

1.  **papagaio**: A standalone C library implementing a non-regex pattern matching engine. It uses a custom grammar for token capture and transformation.
2.  **papagaio-ffi**: A memory-manipulation library that provides raw pointer access, typed memory reads/writes, and dynamic loading (DL) capabilities for Lua.
3.  **papagaio-md**: A CLI runner that parses Markdown files into a structured Abstract Syntax Tree (AST) and executes embedded Lua blocks.
4.  **obsidian-plugin**: A WebAssembly-powered bridge that brings the entire engine to Obsidian, supporting Desktop, Android, and iOS.

---

## The Pattern Matching Engine

Papagaio uses a unique pattern matching syntax that prioritizes context and readability over complex regular expressions.

### Basic Syntax
- `$name`: Captures a single whitespace-delimited token.
- `$name$type`: Captures a token only if it matches a specific type (e.g., `$v$int`, `$v$hex`).
- `$name?`: Makes the capture optional.
- `$block{OPEN}{CLOSE}name`: Captures content between delimiters (e.g., square brackets or parentheses).
- `$eval{ code }`: Executes inline Lua code during the transformation process.

---

## Obsidian Plugin

The Obsidian plugin enables "Programmable Notes". It parses the active note's headings, lists, and tables into a global Lua table and executes ` ```lua ` blocks.

### Global Data Structure (global.md)
Markdown headings are transformed into nested Lua tables. Sequential content (paragraphs, code blocks, tables) are stored as array elements within their respective heading.

**Markdown Input:**
```markdown
# project
## tasks
- fix bug
- add feature
```

**Lua Access:**
```lua
print(global.md.project.tasks[1]) -- Outputs the list content
```

### High-Performance Bridge
The plugin uses a single-file WebAssembly bundle containing a complete Lua 5.4 VM. On Desktop and Mobile, it runs completely offline with zero dependencies on Node.js.

---

## CLI Tool (papagaio-md)

The CLI tool allows you to treat Markdown files as executable scripts in a local environment.

### Compilation
```bash
make all
```
This builds the native `papagaio.so`, `papagaio_ffi.so`, and the `papagaio-md` executable.

### Execution
```bash
./papagaio-md/papagaio-md script.md
```

---

## 16 Detailed Examples

We provide 16 comprehensive examples covering every aspect of the engine.

1.  **[Basic Token Substitution](examples/01-basic-substitution.md)**: Introduction to $variable capture.
2.  **[Multiple Patterns](examples/02-multiple-patterns.md)**: Applying sequential rules in a single pass.
3.  **[Custom Delimiters](examples/03-custom-delimiters.md)**: Using process_ex for non-standard sigils.
4.  **[Inline Patterns](examples/04-process-text-inline.md)**: Embedding $pattern rules in documents.
5.  **[Type Modifiers](examples/05-type-modifiers.md)**: Restricting captures to int, hex, word, etc.
6.  **[Optional Captures](examples/06-optional-captures.md)**: Handling missing data with the ? suffix.
7.  **[Eval Blocks](examples/07-eval-blocks.md)**: Inline math and logic during transformation.
8.  **[Eval With Globals](examples/08-eval-globals.md)**: Accessing script variables inside patterns.
9.  **[Pure Eval](examples/09-pure-eval.md)**: Dynamic injections like timestamps and counters.
10. **[Block Captures](examples/10-block-captures.md)**: Handling multi-delimited content ($block).
11. **[Escaping](examples/11-escaping.md)**: How to use literal dollar signs in patterns.
12. **[Process Pairs](examples/12-process-pairs.md)**: Using tables to define transformation rules.
13. **[Markdown AST](examples/13-global-md-ast.md)**: Navigation through global.md structure.
14. **[Shared State](examples/14-shared-state.md)**: Persistence between different code blocks.
15. **[FFI and Memory](examples/15-ffi-memory.md)**: Using papagaio_ffi for raw memory management.
16. **[Obsidian Bridge API](examples/16-obsidian-api.md)**: Interacting with the vault (read/write/notice).
17. **[Dynamic Loading (DL)](examples/17-ffi-dlopen.md)**: Loading shared libraries (.so/.dll).
18. **[Native Calls](examples/18-ffi-calls.md)**: Binding and calling C functions from Lua.
19. **[Callbacks](examples/19-ffi-callbacks.md)**: Passing Lua functions as arguments to C.
20. **[Structs and Views](examples/20-ffi-structs.md)**: Managing C-style data structures in Lua.
21. **[Bulk Arrays](examples/21-ffi-arrays.md)**: Fast reading/writing of typed data arrays.
22. **[Error Handling](examples/22-ffi-errors.md)**: Using ffi.errno() and dl_error() for debugging.
23. **[Core Utilities](examples/23-ffi-utilities.md)**: Pointer sizing, nulls, and block memory ops.
24. **[Advanced Types](examples/24-advanced-types.md)**: Strict path, ID, and numeric validation.
25. **[Case and Aliases](examples/25-case-and-aliases.md)**: Case enforcement and allowed value lists.
26. **[Prefix and Suffix](examples/26-prefix-suffix.md)**: Matching tokens by partial content.
27. **[Replacement Eval](examples/27-replacement-eval.md)**: Using $eval inside replacement strings.
28. **[Block Variants](examples/28-block-variants.md)**: Single block vs. sequential ($block vs $blockseq).

---

## Obsidian Bridge API (Lua)

The `obsidian` module is available globally within the plugin environment:

- `obsidian.read(path)`: Reads vault file content.
- `obsidian.write(path, content)`: Writes or creates vault files.
- `obsidian.list_files()`: Returns an array of paths for all vault files.
- `obsidian.notice(message)`: Triggers a native Obsidian notification.
- `obsidian.mkdir(path)`: Creates a new directory in the vault.
- `obsidian.get_metadata_json(path)`: Retrieves note frontmatter as a JSON string.
- `obsidian.active_path`: Global variable containing the current note path.
- `obsidian.active_content`: Global variable containing current note content.

---

## FFI and Native Calls

The CLI version supports full FFI capabilities via `papagaio_ffi.so`, including dynamic loading of shared libraries and raw memory manipulation. The Obsidian plugin supports core memory primitives (alloc, read, write) but restricts platform-specific operations for security and compatibility.

---

## Cross-Platform Compatibility

Papagaio is built for portability:
- **Linux/macOS/Windows**: Native compilation supported (GCC, Clang, MSVC, MinGW).
- **Android/iOS**: Full support within Obsidian using the WASM-based plugin.
- **Architectures**: x86, x64, ARMv7, and ARM64.

---

## Technical Details

- **Lua Version**: 5.4.7
- **Binary Size**: ~400KB (WASM bundle)
- **Dependencies**: Standard C Library only.
- **License**: MIT
