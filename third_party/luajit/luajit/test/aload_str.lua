-- test string type
a = {"hello"}

for i = 1, 100 do
  x = #a[1]
end

y = 5

assert(x == y, "Got " .. x .. ", expect " .. y)
