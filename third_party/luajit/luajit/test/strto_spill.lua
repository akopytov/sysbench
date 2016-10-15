-- test IR_STRTO implementation's spill branch.
x = 0
num = "10.0"

for i = 1, 100 do
  x = num + 1
end

y = 11

assert(x == y, "Got " .. x .. ", expect " .. y)