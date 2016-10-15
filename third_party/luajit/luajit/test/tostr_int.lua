-- test IR_TOSTR for int type.
x = 0

for i = 1, 100 do
  x = tostring(i)
end

y = "100"

assert(x == y, "Got " .. x .. ", expect " .. y)
