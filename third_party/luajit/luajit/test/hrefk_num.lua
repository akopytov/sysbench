-- test IR_HREFK when key type is number.
local mytable = {[1.3] = 5, [0.0] = 10000}

for i = 1, 100 do
    res = mytable[0.0] + mytable[1.3]
end

expect = 10005

assert(res == expect, "Expect "..expect..", get "..res)
