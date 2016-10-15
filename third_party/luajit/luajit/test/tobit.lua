bit = require("bit")
m = 0xffffffffffffffff

for i = 1, 100 do
    x = bit.bxor(i, m)
end

y = 100

assert(x == y, "Expect "..y..", get "..x)