-- test IR_ABC: Array Bounds Check.
a = {1, 2, 3}

for i = 1, 100 do
  x = a[1]
end

y = 1

assert(x == y, "Got " .. x .. ", expect " .. y)
