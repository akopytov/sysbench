-- test IR_PVAL.
local ffi = require("ffi")

local st = ffi.typeof("struct { int a; int64_t b; double c; }")

for i = 1, 200 do
  local x = st(i, i + 1, i + 2)
  if i > 100 then
    z = st(x.a, x.b, x.c)
  end
end

a = 200
b = 201
c = 202

assert(z.a == a, "Got " .. tostring(z.a) .. ", expect " .. a)
assert(z.b == b, "Got " .. tostring(z.b) .. ", expect " .. b)
assert(z.c == c, "Got " .. tostring(z.c) .. ", expect " .. c)
