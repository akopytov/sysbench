function foo1()
    local x = 0
    function foo2()
        for i = 1, 100 do
            x = x + 1
        end
    end

    foo2()

    expect = 100
    assert(x == expect, "Expect "..expect..", get "..x)
end

foo1()
