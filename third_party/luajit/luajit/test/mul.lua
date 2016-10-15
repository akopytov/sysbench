x = -1

for i = 1, 100 do
  x = x * -1
end

y = -1

assert(x == y, "Got " .. x .. ", expect " .. y)
