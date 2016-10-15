-- test IR_RENAME.
local x

for i = 1, 100 do
    x = -i
end

y = -100
assert(x == y, "Expect "..y.."get "..x)
