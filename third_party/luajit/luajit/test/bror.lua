local bit = require("bit")
local x = 0x12345678
local res = 0

function numtox(x)
    return ("0x"..bit.tohex(x))
end

for i = 1, 100 do
    res = bit.ror(x, 12)
end

res = numtox(res)
expect = "0x67812345"
assert(res == expect, "Expect "..expect..", get "..res)
