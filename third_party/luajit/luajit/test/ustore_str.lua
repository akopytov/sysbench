-- test upvalue store for string type.
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
x = set("2.3")
y = "2.3"

assert(x == y, "Got " .. x .. ", expect " .. y)
