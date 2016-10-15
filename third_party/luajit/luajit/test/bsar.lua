local bit = require("bit")
local res = 0
local x1, x2 = 256, -256

function bsarloop(x)
    for i = 1, 100 do
        res = bit.arshift(x, 8)
    end
    return res
end

res = bsarloop(x1)
expect = 1
assert(res == expect, "Test1: expect "..expect..", get "..res)

res = bsarloop(x2)
expect = -1
assert(res == expect, "Test2: expect "..expect..", get "..res)
