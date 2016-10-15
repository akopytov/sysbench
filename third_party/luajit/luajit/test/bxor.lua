local bit = require("bit")
local x1, x2 = 0xa5a5f0f0, 0xaa55ff00
local res = 0

function numtox(x)
    return ("0x"..bit.tohex(x))
end

for i = 1, 100 do
    res = bit.bxor(x1, x2)
end

res = numtox(res)
expect = "0x0ff00ff0"

assert(res == expect, "Expect "..expect..", get "..res)
