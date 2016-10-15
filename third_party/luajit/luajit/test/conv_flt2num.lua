ffi = require("ffi")

-- IR_CONV num.flt
x = ffi.new("float", 20)
local y
for i=1,100 do
  y = ffi.cast("double",x) + 1
end
assert(y == 21)
