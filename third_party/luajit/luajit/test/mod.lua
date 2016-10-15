local ffi = require("ffi")

dividend, divisor = ffi.cast("int", 99), ffi.cast("int", 2)

function mod(dividend, divisor)
    for i = 1, 100 do
        res = dividend % divisor
    end
    return res
end

x = mod(dividend, divisor)
y = mod(-dividend, divisor)
z = mod(dividend, -divisor)

assert(x == 1, "Expect 1 , get "..tonumber(x))
assert(y == -1, "Expect -1, get "..tonumber(y))
assert(z == 1, "Expect 1, get "..tonumber(z))
