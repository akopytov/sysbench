local x = 0.5

for i = 1, 100 do
    res = math.pow(3, x)
end

expect = "1.7320508075689"

assert(tostring(res) == expect, "Expect "..expect..", get "..res)

