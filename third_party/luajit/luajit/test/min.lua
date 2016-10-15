function minloop(x, y)
    for i = 1, 100 do
        res = math.min(x, y)
    end
    return res
end

result = minloop(1, 100)
expect = 1
assert(result == expect, "test1: Expect "..expect..", get "..result)

result = minloop(-1, -100)
expect = -100
assert(result == expect, "test2: Expect "..expect..", get "..result)

