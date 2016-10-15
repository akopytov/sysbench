s = "hello, world!"

for i = 0, 100 do
  x = string.upper(s)
end

y = string.upper(s)

assert(x == y, "Got " .. x .. ", expect " .. y)
