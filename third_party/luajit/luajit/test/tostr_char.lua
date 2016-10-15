-- test IR_TOSTR for char type.

for i = 1, 100 do
   x = tostring(string.char(i))
end

y = "d"
assert(x == y, "Got " .. x .. ", expect " .. y)
