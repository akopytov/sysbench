function cosloop(x)
    for i = 1, 100 do
        res = math.cos(x)
    end
    return res
end

result = cosloop(0)
expect = 1
assert(result == expect, "test1: Expect "..expect..", get "..result)

result = cosloop(math.rad(60))
expect = "0.5"
assert(tostring(result) == expect, "test2: Expect "..expect..", get "..result)

result = cosloop(math.rad(90))
expect = 0
assert(result == expect, "test3: Expect "..expect..", get "..result)

result = cosloop(math.rad(120))
expect = "-0.5"
assert(tostring(result) == expect, "test4: Expect "..expect..", get "..result)

