-- Test IR_NE.
l = {"a", "b"}

for i = 1, 100 do
   x = #table.concat(l)
end

y = 2

assert(x == y, "Got " .. x .. ", expect " .. y)
