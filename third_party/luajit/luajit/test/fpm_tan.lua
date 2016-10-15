function tanloop(x)
    for i = 1, 100 do
        res = math.tan(x)
    end
    return res
end

result = tanloop(0)
expect = 0
assert(result == expect, "test1: Expect "..expect..", get "..result)

result = tanloop(math.rad(45))
expect = "1"
assert(tostring(result) == expect, "test2: Expect "..expect..", get "..result)

result = tanloop(math.rad(135))
expect = "-1"
assert(tostring(result) == expect, "test3: Expect "..expect..", get "..result)

