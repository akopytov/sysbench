function sqrtloop(x)
    for i = 1, 100 do
        res = math.sqrt(x)
    end
    return res
end

expect = 4
result = sqrtloop(16)
assert(result == expect, "test1: Expect "..expect..", get "..result)

-- NaN is the only value that doesn't equal itself
result = sqrtloop(-16)
assert(result ~= result, "test2: Expect NaN, get "..result)

expect = 0
result = sqrtloop(0)
assert(result == expect, "test1: Expect "..expect..", get "..result)
