local mytable = {}

for i = 1, 100 do
    if i > 60
    then
        mytable["index"..i] =  5
    end
end

res = mytable["index100"]
expect = 5

assert(res == expect, "Expect "..expect..", get "..res)
