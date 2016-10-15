-- test IR_STRTO implementation's no spill branch.
a = 12
n = "10"
for i = 1, 100 do
  x = string.rep(a, n)
end

y = "12121212121212121212"

assert(x == y, "Got " .. x .. ", expect " .. y)
