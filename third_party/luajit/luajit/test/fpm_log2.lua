local x = 3
local y = 0.5

for i = 1, 100 do
    res = math.pow(x, y)
end

expect = "1.7320508075689"

assert(tostring(res) == expect, "Expect "..expect..", get "..res)
