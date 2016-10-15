local x = 2

for i = 1, 100 do
    res = math.exp(x)
end

expect = "7.3890560989307"
assert(tostring(res) == expect, "Expect "..expect..", get "..res)
