-- test IR_USE.
local step = 1
local stop = 100

for i = 1, stop, step do
    res = i
end

assert(res == stop, "Expect "..stop..", get "..res)