-- test IR_GT for none-num type, which could be int, tab etc.
-- IR_GT is only emitted in string.rep().

for i = 1, 100 do
  x = string.rep("Lua", i / 50, " ")
end

y = "Lua Lua"

assert(x == y, "Got " .. x .. ", expect " .. y)
