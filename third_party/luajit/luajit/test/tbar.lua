local mytable = {}

for i = 1, 100 do
    mytable["test"] = i
end

res = mytable["test"]
expect = 100
assert(res == expect, "Expect "..expect..", get "..res)
