-- test IR_GCSTEP.
local x = 0ll
for i = 1, 200 do
  local y = x + i
  if i > 100 then
    x = y
  end
end

res = 15050ll
assert(x == res, "Got " .. tostring(x) .. ", expect " .. tostring(res))
