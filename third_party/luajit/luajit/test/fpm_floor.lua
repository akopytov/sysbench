function floorloop(x)
    for i = 1, 100 do
        res = math.floor(i + x)
    end
    return res
end

result = floorloop(0.8)
expect = 100
assert(result == expect, "test1: Expect "..expect..", get "..res)

result = floorloop(0)
expect = 100
assert(result == expect, "test2: Expect "..expect..", get "..res)

result = floorloop(-101.8)
expect = -2
assert(result == expect, "test3: Expect "..expect..", get "..res)

