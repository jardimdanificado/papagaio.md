--[[
  exemplo.lua — papagaio
  Build: cc -shared -fPIC -o papagaio.so papagaio.c $(pkg-config --cflags --libs lua5.4)

  IMPORTANTE — como o motor de padrões funciona:

  O papagaio NÃO é um find-and-replace de string simples.
  "$nome" no padrão significa "capturar qualquer token nessa variável".
  Para casar texto específico, o padrão precisa de literais em volta:

    ERRADO:  padrão="$nome"           → casa qualquer token em qualquer posição
    CERTO:   padrão="nome: $nome"     → casa literalmente "nome: " e captura o que vier

  Para substituição simples de strings (find/replace), use process() diretamente
  com padrões que contenham os literais que você quer casar.
--]]

local pap = require "papagaio"

local function sec(s) print("\n--- " .. s .. " ---") end


-- ===========================================================================
-- 1. process — padrão com literais + captura
--    O padrão "nome: $n" casa o texto "nome: <qualquer coisa>"
--    e captura o valor em $n para usar na substituição.
-- ===========================================================================
sec("1. process — captura com contexto")

print(pap.process("nome: Alice, idade: 30",
    "nome: $n,", "pessoa=$n,"))
--> pessoa=Alice, idade: 30

print(pap.process("x=10 y=20 z=30",
    "x=$v", "X=$v"))
--> X=10 y=20 z=30


-- ===========================================================================
-- 2. process — múltiplos padrões aplicados em sequência
-- ===========================================================================
sec("2. process — múltiplos padrões")

print(pap.process("erro: arquivo não encontrado no caminho /tmp/foo",
    "erro: $msg", "ERRO[$msg]"))
--> ERRO[arquivo não encontrado no caminho /tmp/foo]

-- padrões são tentados em ordem; o primeiro que casar é usado
print(pap.process("tipo: A valor: 42",
    "tipo: A valor: $v", "tipo_a($v)",
    "tipo: B valor: $v", "tipo_b($v)"))
--> tipo_a(42)


-- ===========================================================================
-- 3. process_ex — delimitadores customizados
-- ===========================================================================
sec("3. process_ex — delimitadores customizados")

-- sigil="@" open="<" close=">"
print(pap.process_ex("INSERT INTO <tabela> VALUES (<val>)",
    "@", "<", ">",
    "INSERT INTO <tabela>", "INSERT INTO usuarios",
    "(<val>)",              "('pedro@email.com')"))
--> INSERT INTO usuarios VALUES ('pedro@email.com')


-- ===========================================================================
-- 4. process_text com $pattern — define regras inline no texto
--    Este é o modo mais poderoso: múltiplas regras aplicadas iterativamente.
-- ===========================================================================
sec("4. process_text com $pattern")

io.write(pap.process_text([[
$pattern {nome: $n, idade: $i} {$n tem $i anos}
nome: Alice, idade: 25
nome: Bob, idade: 30
]]))
--> Alice tem 25 anos
--> Bob tem 30 anos


-- ===========================================================================
-- 5. process_text — múltiplos $pattern
-- ===========================================================================
sec("5. múltiplos $pattern")

io.write(pap.process_text([[
$pattern {status: ok} {✓}
$pattern {status: erro} {✗}
$pattern {item $n: $v, $rest} {$n=$v | $rest}
item a: 1, status: ok
item b: 2, status: erro
]]))
--> a=1 | ✓
--> b=2 | ✗


-- ===========================================================================
-- 6. modificadores de tipo — $var$tipo
--    O modificador restringe o que o token casa.
-- ===========================================================================
sec("6. modificadores de tipo")

-- $v$int  → só inteiros
io.write(pap.process_text([[
$pattern {total: $v$int itens} {[int:$v]}
total: 42 itens
total: 3.5 itens
total: abc itens
]]))
--> [int:42]
--> total: 3.5 itens   (não casou)
--> total: abc itens   (não casou)

-- $v$word → só letras
io.write(pap.process_text([[
$pattern {tag: $v$word} {<$v>}
tag: importante
tag: 123
]]))
--> <importante>
--> tag: 123   (não casou)

-- $v$hex → dígitos hex [0-9a-fA-F] ou prefixo 0x/0X
io.write(pap.process_text([[
$pattern {cor: #$v$hex} {rgb($v)}
cor: #ff3a00
cor: #xyz
]]))
--> rgb(ff3a00)
--> cor: #xyz   (não casou — x/y/z não são hex válidos sem prefixo 0)


-- ===========================================================================
-- 7. captura opcional — $var?
-- ===========================================================================
sec("7. opcional")

io.write(pap.process_text([[
$pattern {pedido $id$int $obs?} {id=$id obs=[$obs]}
pedido 42 urgente
pedido 99
]]))
--> id=42 obs=[urgente]
--> id=99 obs=[]


-- ===========================================================================
-- 8. $eval — Lua inline
--    `match` dentro do eval = o trecho completo que casou o padrão.
--    Use string.match para extrair as capturas individuais se precisar.
-- ===========================================================================
sec("8. $eval — match = trecho completo do padrão")

-- match aqui = "3 * 4", "10 * 7" etc.
io.write(pap.process_text([[
$pattern {$a$int * $b$int} {$eval{
    local a, b = match:match("(%d+) %* (%d+)")
    return tostring(tonumber(a) * tonumber(b))
}}
3 * 4
10 * 7
2 * 2
]]))
--> 12
--> 70
--> 4

-- match = "42", "100" etc.
io.write(pap.process_text([[
$pattern {hex: $v$int} {$eval{
    return string.format("0x%x", tonumber(match:match("%d+")))
}}
hex: 42
hex: 255
hex: 16
]]))
--> 0x2a
--> 0xff
--> 0x10


-- ===========================================================================
-- 9. $eval com globais do script chamador
--    process_text usa o lua_State do chamador — globais e módulos visíveis.
-- ===========================================================================
sec("9. $eval com globais")

TAXA = 0.1

io.write(pap.process_text([[
$pattern {preco: $v$float} {$eval{
    local v = tonumber(match:match("[%d%.]+"))
    return string.format("R$%.2f (+ %.0f%% = R$%.2f)", v, TAXA*100, v*(1+TAXA))
}}
preco: 99.90
preco: 200.00
]]))
--> R$99.90 (+ 10% = R$109.89)
--> R$200.00 (+ 10% = R$220.00)


-- ===========================================================================
-- 10. $eval sem captura — match = ""
--     Útil para injetar valores dinâmicos sem depender de um padrão.
-- ===========================================================================
sec("10. $eval puro (os.date, math etc.)")

print(pap.process_text(
    "gerado em $eval{ return os.date('%d/%m/%Y') } às $eval{ return os.date('%H:%M') }"))
--> gerado em 16/03/2026 às 01:02

print(pap.process_text(
    "pi = $eval{ return string.format('%.6f', math.pi) }"))
--> pi = 3.141593


-- ===========================================================================
-- 11. escaping — \$ não é tratado como sigil
-- ===========================================================================
sec("11. escaping")

-- O padrão "preco: $v$float" casa "preco: " + float, não "\\$"
-- Para ter $ literal no texto sem ser interpretado, use \$
print(pap.process_text([[
$pattern {preco: $v$float} {custo: \$$v}
preco: 49.90
]]))
--> custo: $49.90


-- ===========================================================================
-- 12. process_pairs — uso correto: padrões com literais
--     process_pairs aceita uma tabela de pares {padrão, substituição}.
--     Os padrões seguem a mesma gramática — precisam ter literais âncora.
-- ===========================================================================
sec("12. process_pairs correto")

io.write(pap.process_pairs(
    "nivel: debug\nnivel: erro\nnivel: info\n",
    {
        { "nivel: debug", "[D]" },
        { "nivel: erro",  "[E]" },
        { "nivel: info",  "[I]" },
    }
))
--> [D]
--> [E]
--> [I]

-- com captura
io.write(pap.process_pairs(
    "user=alice role=admin\nuser=bob role=viewer\n",
    {
        { "user=$u role=$r", "$u:$r" },
    }
))
--> alice:admin
--> bob:viewer


-- ===========================================================================
-- 13. $block{OPEN}{CLOSE}var — bloco com delimitadores customizados
--     Captura um ou mais blocos consecutivos OPEN...CLOSE e concatena
--     o conteúdo separado por espaço.
-- ===========================================================================
sec("13. $block{}{}")

-- colchetes: [a][b][c] → "a b c"
io.write(pap.process_text([[
$pattern {tags: $block{[}{]}items} {items=[$items]}
tags: [lua][c][papagaio]
tags: [single]
]]))
--> items=[lua c papagaio]
--> items=[single]

-- opcional: linha sem blocos não falha
io.write(pap.process_text([[
$pattern {lista $block{[}{]}items?} {$items}
lista [x][y][z]
lista
]]))
--> x y z
--> (linha vazia)

-- delimitadores customizados: <<...>>
io.write(pap.process_text([[
$pattern {$block{<<}{>>}body} {($body)}
<<hello>><<world>>
]]))
--> (hello world)
