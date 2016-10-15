-- test IR_HREFK when key is primitive type.
local a = true
local b = false
local mytable = {[true] = 5, [false] = 4}

for i = 1, 100 do
    res = mytable[true] + mytable[false]
end

expect = 9

assert(res == expect, "Expect "..expect..", get "..res)
