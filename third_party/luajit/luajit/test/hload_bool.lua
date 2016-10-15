-- test boolean type
a = false

for i = 1, 100 do
  if a == false then
    x = 2
  else
    x = 1
  end
end

y = 2

assert(x == y, "Got " .. x .. ", expect " .. y)
