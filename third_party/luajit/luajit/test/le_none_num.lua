-- test for IR_LE for none-num type, which could be int, tab etc. Here int is
-- tested.
a = 1

for i = 1, 100 do
  if a <= 88 then
    x = 1
  else
    x = 0
  end
end

y = 1

assert(x == y, "Got " .. x .. ", expect " .. y)
