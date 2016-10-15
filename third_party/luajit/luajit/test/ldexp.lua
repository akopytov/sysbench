local x = 10

for i = 1, 100 do
    res = math.ldexp(10, x)
end

expect = 10240

assert(res == expect, "Expect "..expect..", get "..res)
