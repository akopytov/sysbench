local x = 10

for i = 1, 100 do
    res = math.log(x)
end

expect = "2.302585092994"

assert(tostring(res) == expect, "Expect "..expect..", get "..res)
