local ffi = require("ffi")

ffi.cdef[[
typedef struct {uint8_t red, green, blue, alpha;} rgba_pixel;
]]

data = ffi.new("rgba_pixel")
ms = ffi.new("rgba_pixel", 1, 2, 3, 4)
length = ffi.sizeof(ms)
s = ffi.string(ms, length)

for i = 0, 100 do
    ffi.copy(data, s, length)
end

assert(data.red == 1 and data.green == 2 and data.blue == 3 and data.alpha == 4, "fail to do ffi.copy")
