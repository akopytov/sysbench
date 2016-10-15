local x = 2016.236

for i = 1, 100 do
    res = math.modf(x)
end

expect = 2016

assert(res == expect, "Expect "..expect..", get "..res)
