-- test primitive type
arr = {}

for i = 1, 100 do
  arr[0] = true
end

y = true

assert(arr[0] == y, "arr[0] is not true, failed.")
