x = 0

for i = 1, 100 do
  x = x + i
end

y = 5050

assert(x == y, "Got " .. x .. ", expect " .. y)
