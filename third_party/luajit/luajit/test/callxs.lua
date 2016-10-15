local ffi = require("ffi")
ffi.cdef[[
int printf(const char *fmt, ...);
]]

for i = 1, 100 do
  ffi.C.printf(".")
end

io.write("Need to exam the result manually.")
