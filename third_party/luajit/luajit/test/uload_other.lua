-- test IR_ULOAD for primitive type. This case tests "false".
local eq, x
local t, u = {}, {}
local mt = {
  __eq = function() return eq end,
}
t = setmetatable(t, mt)
u = setmetatable(u, mt)

eq = false
for i = 1, 100 do
  x = t == u
end
assert(x == false, "Got x = true expect: false")

for i = 1, 100 do
  x = t ~= u
end

assert(x == true, "Got x = false expect: true")
