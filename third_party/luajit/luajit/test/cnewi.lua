local ffi = require("ffi")

for i = 0, 100 do
    x = ffi.new("int")
end

assert(ffi.istype("int", x), "x is not int as expected!")


