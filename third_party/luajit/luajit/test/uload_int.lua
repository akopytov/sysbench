-- test IR_ULOAD for IRT_INT type.
local x = 0
(function()
  local function foo()
    x = x + 1
  end

  for i = 1, 100 do
    foo();
  end
end)()

local y = 100
assert(x == y, "Got " .. x .. ", expect " .. y)
