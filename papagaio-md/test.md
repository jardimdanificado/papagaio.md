# pessoa
## nome
joao
## idade 
45

| a | b |
|---|---|
| 1 | 2 |
| oi | tchau |

```lua
local function dump(o, indent)
   if type(o) == 'table' then
      local s = '{\n'
      for k,v in pairs(o) do
         if type(k) ~= 'number' then k = '"'..k..'"' end
         s = s .. string.rep("  ", indent+1) .. '['..k..'] = ' .. dump(v, indent+1) .. ',\n'
      end
      return s .. string.rep("  ", indent) .. '}'
   else
      return tostring(o)
   end
end
print(dump(global.md, 0))
```
