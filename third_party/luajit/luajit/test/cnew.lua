local ffi = require("ffi")

for i = 0, 100 do
    x = ffi.new("int[?]", 5)
end

assert(ffi.sizeof(x) == 20, "Expect the size of x is 20"..", get "..ffi.sizeof(x))
