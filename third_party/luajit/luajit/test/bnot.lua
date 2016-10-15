local bit = require("bit")
local res = 0

function numtox(x)
    return ("0x"..bit.tohex(x))
end

function bnotloop(x)
    for i = 1, 100 do
        res = bit.bnot(x)
    end
    return res
end

res = bnotloop(0)
expect = -1
assert(res == expect, "Test1: expect "..expect..", get "..res)

res = bnotloop(-1)
expect = 0
assert(res == expect, "Test2: expect "..expect..", get "..res)

res = numtox(bnotloop(0x12345678))
expect = "0xedcba987"
assert(res == expect, "Test3: expect "..expect..", get "..res)
