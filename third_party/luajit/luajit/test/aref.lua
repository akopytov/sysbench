local mytable = {100}
local res  = 0

for i = 1, 100 do
    res =  mytable[1]
end

expect =  100

assert(res == expect, "Expect "..expect..", get "..res)
