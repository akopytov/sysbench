ffi = require("ffi")

u = ffi.new([[
union {
  int8_t i8;
  uint8_t u8;
}
]])

x = 0
for i=-100,100 do
  u.i8 = i
  x = x + u.u8
end

y = 25600
assert(x == y, "Expect "..y..", get "..x)
