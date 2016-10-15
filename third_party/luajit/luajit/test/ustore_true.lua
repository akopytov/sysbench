-- test upvalue store for "ture" value.
function setgen ()
  local x = 0.3
  return function (b)
           for i = 1, 100 do
             x = b
           end
         return x
         end
end

set = setgen()
x = set(true)

assert(x == true, "Got x = true, expect true")
