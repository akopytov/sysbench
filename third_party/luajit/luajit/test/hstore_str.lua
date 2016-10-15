-- test string type
x = "0"

for i = 1, 100 do
  x = "hello_world"
end

y = "hello_world"

assert(x == y, "Got " .. x .. ", expect " .. y)
