-- test IR_HREFK when key type is string.
local mytable = {["data1"] = 5, ["data2"] = 10000, ["data3"] = -20}

for i = 1, 100 do
    res = mytable.data2
end

expect = 10000

assert(res == expect, "Expect "..expect..", get "..res)
