-- segmentation fault occurs on CI machine(x86 and arm).
-- without the delcaration of 'local res = 0' the fault can be bypassed.

local res = 0
local step = 1
local limit = 100

for i = 1, limit, step do
    res = i
end

assert(res == limit, "Expect "..limit..", get "..res)
