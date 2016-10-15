-- test number type
arr = {}

for i = 1, 100 do
  arr[0] = 0.2
end

y = 0.2

assert(arr[0] == y, "Got " .. arr[0] .. ", expect " .. y)
