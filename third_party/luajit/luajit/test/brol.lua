-- memo: IR_BROL can't be emitted cause LJ_TARGET_UNIFYROT is defined as 2 in lj_arch.h
-- which means replace IR_BROL with IR_BROR notwithstanding bit.rol is called

local bit = require("bit")
local x = 0x12345678
local res = 0

function numtox(x)
    return ("0x"..bit.tohex(x))
end

for i = 1, 100 do
    res = bit.rol(x, 12)
end

res = numtox(res)
expect = "0x45678123"
assert(res == expect, "Expect "..expect..", get "..res)
