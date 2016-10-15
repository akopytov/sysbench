-- test IR_ULOAD for addr type, which is IRT_LIGHTUD or IRT_UDATA.
function setgen ()
  local x = {1}
  return function ()
           for i = 1, 100 do
             b = x[1]
           end
           return b
         end
end

set = setgen()
x = set()
y = 1

assert(x == y, "Got " .. x .. ", expect " .. y)