-- test IR_GT for num type.
-- a = 2^31 is out of signed 4 byte integer range, which is promoted to num
-- type.
a = 2^31

for i = 1, 100 do
  if a > 2 then
    x = 1
  else
    x = 0
  end
end

y = 1

assert(x == y, "Got " .. x .. ", expect " .. y)
