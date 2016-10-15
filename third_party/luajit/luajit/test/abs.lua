local x = -5

for i = 1, 100 do
    x = math.abs(x)
end

y = 5

assert(x == y, "Except "..y..", get "..x)
