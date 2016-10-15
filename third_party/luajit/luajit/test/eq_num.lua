-- test IR_EQ for number type.
a = 1.3

for i = 1, 100 do
  if a == 1.3 then
    x = 1
  else
    x = 0
  end
end

y = 1

assert(x == y, "Got " .. x .. ", expect " .. y)
