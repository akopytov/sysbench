-- test IR_TOSTR for number type.
x = 0

for i = 1, 100 do
  x = tostring(i+0.2)
end

y = "100.2"

assert(x == y, "Got " .. x .. ", expect " .. y)
