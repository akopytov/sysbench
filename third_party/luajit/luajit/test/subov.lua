local res = 0

for i = 1, 100 do
    res = -i
end

assert(res == -100, "Expect -100, get "..res)
