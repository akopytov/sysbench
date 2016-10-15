function ceilloop(x)
    for i = 1, 100 do
        res = math.ceil(i + x)
    end
    return res
end

result = ceilloop(0.8)
expect = 101
assert(res == expect, "test1: Expect "..expect..", get "..res)

result = ceilloop(0)
expect = 100
assert(res == expect, "test2: Expect "..expect..", get "..res)

result = ceilloop(-101.8)
expect = -1
assert(res == expect, "test3: Expect "..expect..", get "..res)

