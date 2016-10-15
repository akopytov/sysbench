function sinloop(x)
    for i = 1, 100 do
        res = math.sin(x)
    end
    return res
end

result = sinloop(0)
expect = 0
assert(result == expect, "test1: Expect "..expect..", get "..result)

result = sinloop(math.rad(30))
expect = "0.5"
assert(tostring(result) == expect, "test2: Expect "..expect..", get "..result)

result = sinloop(math.rad(90))
expect = 1
assert(result == expect, "test3: Expect "..expect..", get "..result)

result = sinloop(math.rad(210))
expect = "-0.5"
assert(tostring(result) == expect, "test4: Expect "..expect..", get "..result)
