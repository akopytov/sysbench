local mytable = {["test"] = 100}
local res = 0
local x = "test"

for i = 1, 100 do
    res = mytable[x]
end

expect = 100

assert(res == expect, "Expect "..expect..", get "..expect)
