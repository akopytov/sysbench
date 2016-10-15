local mytable1 = {20, 1, key1 = "hello", key2 = "world", lang = "lua"}
local mytable2 = {key1 = "hello new", key2 = "hello new"}

for i = 1, 100 do
    setmetatable(mytable2, {__index = mytable1})
end

res = mytable1.lang
expect = mytable2.lang

assert(res == expect, "Expect "..expect..", get "..res)
