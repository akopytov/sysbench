local bit = require("bit")
local x1, x2, x3, x4 = 1, 2, 4, 8
local res = 0

for i = 1, 100 do
    res = bit.bor(x1, x2, x3, x4)
end

expect = 15
assert(res == expect, "Expect "..expect..", get "..res)
