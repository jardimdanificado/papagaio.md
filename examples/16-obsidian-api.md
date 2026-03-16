# Example 16 — Obsidian Bridge API

When running inside the Obsidian plugin, a global `obsidian` module is available.
This allows your Lua scripts to interact directly with your vault.

Note: This API is ONLY available in the Obsidian plugin version.

```lua
-- 1. Show a native Obsidian notification
obsidian.notice("Starting vault scan... 🦜")

-- 2. Read current note information
local active_path = obsidian.active_path
local active_content = obsidian.active_content
print("Current File: " .. active_path)

-- 3. List all files in the vault
local files = obsidian.list_files()
print("Vault contains " .. #files .. " files.")

-- 4. Read a specific file (Asyncify handles the promise behind the scenes)
local readme_content, err = obsidian.read("README.md")
if readme_content then
    print("README length: " .. #readme_content)
else
    print("Error: " .. err)
end

-- 5. Create a folder and write a new file
obsidian.mkdir("Generated")
local success = obsidian.write("Generated/Summary.md", "Scan finished at " .. os.date())
if success then
    obsidian.notice("Summary generated in 'Generated/Summary.md' 🦜")
end

-- 6. Get frontmatter metadata as a JSON string
local meta_json = obsidian.get_metadata_json(active_path)
print("Frontmatter metadata: " .. meta_json)
```
