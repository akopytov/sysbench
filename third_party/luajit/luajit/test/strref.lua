local mystring = "test"
local res = nil

for i = 1, 100 do
    res = string.sub(mystring, 1, 3)
end

expect = "tes"

assert(res == expect, "Expect "..expect..", get "..res)
