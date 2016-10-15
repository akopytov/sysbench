-- test number type
a = 0.3

for i = 1, 100 do
  x = a
end

y = 0.3

assert(x == y, "Got " .. x .. ", expect " .. y)
