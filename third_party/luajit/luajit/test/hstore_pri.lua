-- test primitive type
x = 0

for i = 1, 100 do
  x = true
end

y = true

assert(x == y, "x is not true, failed.")
