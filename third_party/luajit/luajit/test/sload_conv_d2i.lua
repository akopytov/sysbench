-- test IR_SLOAD with number to integer conversion.
k = 1
a, b, c = 1/k, 100/k, 1/k
x = 0
for j = a, b, c do
   x = j
end

assert(x == b, "Got " .. x .. ", expect " .. b)
