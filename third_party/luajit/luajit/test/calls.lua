
math.randomseed(0)
for i = 1, 100 do
  x = math.random(9999)
end

math.randomseed(0)
for i = 1, 25 do
  y = math.random(9999)
end
for i = 1, 25 do
  y = math.random(9999)
end
for i = 1, 25 do
  y = math.random(9999)
end
for i = 1, 25 do
  y = math.random(9999)
end

assert(x == y, "Got " .. x .. ", expect " .. y)
