local bit = require("bit")
local res = 0

function numtox(x)
    return ("0x"..bit.tohex(x))
end

function bswaploop(x)
    for i = 1, 100 do
        res = bit.bswap(x)
    end
    return res
end

res = numtox(bswaploop(0x12345678))
expect = "0x78563412"
assert(res == expect, "Test1: expect "..expect..", get "..res)

res = numtox(bswaploop(0x78563412))
expect = "0x12345678"
assert(res == expect, "Test2: expect "..expect..", get "..res)
