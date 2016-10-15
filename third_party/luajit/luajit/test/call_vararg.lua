local ffi = require("ffi")
ffi.cdef[[
int printf(const char *fmt, ...);
]]

for i = 1, 100 do
  ffi.C.printf("%s%s%s%s%s%s%s%s%s-%.0f%.0f%.0f%.0f%.0f%.0f%.0f%.0f%.0f\n", "0", "1", "2", "3", "4", "5", "6", "7", "8", 0, 1, 2, 3, 4, 5, 6, 7, 8)
end

io.write("Need to exam the result manually.")
