x = 0
y = math.sinh(51)

for i = -50, 51 do
  x = x + math.sinh(i)
end

assert(x == y, "Got " .. x .. ", expect " .. y)
