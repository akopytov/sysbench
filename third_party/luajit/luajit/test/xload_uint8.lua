-- test IR_XLOAD for uint8_t type.
ffi = require("ffi")

a = ffi.new("uint8_t[?]", 1)
a[0] = 2

x = 0
for i = 0, 99 do
  x = x + a[0]
end

y = 200

assert(x == y, "Got " .. x .. ", expect " .. y)
