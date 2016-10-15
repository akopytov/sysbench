local ffi = require("ffi")

for i=1, 100 do
    str = ffi.string("test")
end

assert("test" == str, "Got "..str..", expect test")
