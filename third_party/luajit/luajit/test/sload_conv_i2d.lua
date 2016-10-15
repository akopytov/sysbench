-- test IR_SLOAD with integer to number conversion.
x = 0
for i = -2147483548, -2147483648, -1 do
  x = x + 1
end
y = 101

assert(x == y, "Got " .. x .. ", expect " .. y)
