
x = 0
for i = 1,100 do
  print (x)
  x = x + i
end

t = 5050

assert(x == t, "Got " .. x .. ", expect " .. t)

