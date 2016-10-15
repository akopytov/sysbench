-- test vararg slot load
function add (...)
   local s = 0
   for i = 1, 100 do
     s = s + ...
   end
   return s
end

x = add(3)
y = 300

assert(x == y, "Got " .. x .. ", expect " .. y)
