function foo1(x)
    function loop()
        for i = 1, 100 do
           x = "test1"
        end
        return x
    end
    return loop
end

myfunc = foo1("test")

res = myfunc()
expect = "test1"
assert(res == expect, "Expect "..expect..", get "..res)
