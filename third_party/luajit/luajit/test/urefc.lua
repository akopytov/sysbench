function foo1()
    local x = 0

    function foo2()
        for i = 1, 100 do
            x = x + 1
        end
        return x
    end

    return foo2
end

func = foo1()
res = func()
expect = 100

assert(res == expect, "Expect "..expect..", get "..res)
