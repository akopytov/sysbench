a = {1, 2}

for i = 1, 100 do
  x = a[2]
end

y = 2

assert(x == y, "Got " .. x .. ", expect " .. y)