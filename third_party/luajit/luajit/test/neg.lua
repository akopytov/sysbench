local x = -5

for i = 1, 100 do
    x = -x
end

y = -5

assert(x == y, "Expect "..y..", get "..x)
