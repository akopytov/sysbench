function maxloop(x, y)
    for i = 1, 100 do
        res = math.max(x, y)
    end
    return res
end

result = maxloop(1, 100.56)
expect = 100.56
assert(result == expect, "test1: Expect "..expect..", get "..result)

result = maxloop(-1, -100.56)
expect = -1
assert(result == expect, "test2: Expect "..expect..", get "..result)
