local x = 99

for i = 1, 100 do
    res = math.log10(x)
end

expect = "1.9956351945975"

assert(tostring(res) == expect, "Expect "..expect..", get "..res)
