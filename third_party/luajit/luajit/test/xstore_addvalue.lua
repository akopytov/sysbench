local ffi = require("ffi")
ffi.cdef[[
  typedef struct { double v; } point_t;
]]

local point
local mt = {
  __add = function(a, b) return point(a.v + b.v) end,
  __index = {
    getv = function(a) return a.v end,
  },
}

point = ffi.metatype("point_t", mt)

for i = 1, 100 do
  pointc = point(3) + point(4)
end

x = pointc:getv()
y = 7

assert(x == y, "Got " .. x .. ", expect " .. y)
