local ffi = require("ffi")
ffi.cdef[[
  typedef struct { double v; } point_t;
]]

local point
local mt = {
  __index = {
    getv = function(a) return a.v end,
  },
}

point = ffi.metatype("point_t", mt)

for i = 1, 100 do
  x = point(3):getv()
end

y = 3

assert(x == y, "Got " .. x .. ", expect " .. y)