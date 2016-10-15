-- test IR_UGT for number type.
a = 2.3

for i = 1, 100 do
  if a <= 2 then
    x = 0
  else
    x = 1
  end
end

y = 1

assert(x == y, "Got " .. x .. ", expect " .. y)
