-- test number type
x = 0

for i = 1, 100 do
  x = 0.2
end

y = 0.2

assert(x == y, "Got " .. x .. ", expect " .. y)
