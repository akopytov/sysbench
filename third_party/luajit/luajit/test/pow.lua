x = 10

for i = 1, 60 do
    res = 5^x
end

y = 9765625

assert(res == y, "Expect "..y..", get "..res)

