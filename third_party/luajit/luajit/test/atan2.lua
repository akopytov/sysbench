function atan2loop(x, y)
    for i = 1, 100 do
        z = math.atan2(x, y)
    end
    return z
end

res = atan2loop(1, 1)
expect = math.pi/4
assert(res == expect, "test1: Expect "..expect..", get "..res)

res = atan2loop(1, -1)
expect = 3 * math.pi/4
assert(res == expect, "test2: Expect "..expect..", get "..res)

res = atan2loop(-1, -1)
expect = -3 * math.pi/4
assert(res == expect, "test3: Expect "..expect..", get "..res)

res = atan2loop(-1, 1)
expect = -math.pi/4
assert(res == expect, "test4: Expect "..expect..", get "..res)
