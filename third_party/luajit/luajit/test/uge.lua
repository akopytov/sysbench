-- test IR_UGE.
local t = setmetatable({}, {__index = function(t, k)
                                      return 100 - k
                                      end})

for i = 1, 100 do
  assert(t[i] == 100 - i)
end
