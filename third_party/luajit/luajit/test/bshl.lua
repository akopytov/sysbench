local bit = require("bit")
local res = 0
local x1, x2, x3 = 0, 8, 40

function bshlloop(x)
    for i = 1, 100 do
        res = bit.lshift(1, x)
    end
    return res
end

res = bshlloop(x1)
expect = 1
assert(res == expect, "Test1: expect "..expect..", get "..res)

res = bshlloop(x2)
expect = 256
assert(res == expect, "Test2: expect "..expect..", get "..res)

res = bshlloop(x3)
expect = 256
assert(res == expect, "Test2: expect "..expect..", get "..res)
