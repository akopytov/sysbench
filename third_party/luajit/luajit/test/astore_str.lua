-- test string type
arr = {}

for i = 1, 100 do
  arr[0] = "hello_world"
end

y = "hello_world"

assert(arr[0] == y, "Got " .. arr[0] .. ", expect " .. y)
