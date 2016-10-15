x = -1

for i = 1, 100 do
  x = bit.bxor(i, x)
end

y = -101

assert(x == y, "XOR: Got " .. x .. ", expect " .. y)

for i = 1, 100 do
  x = bit.band(i, 7)
end

y = 4

assert(x == y, "AND: Got " .. x .. ", expect " .. y)

x = -1

for i = 1, 100 do
  x = bit.bor(i, x)
end

y = -1

assert(x == y, "OR: Got " .. x .. ", expect " .. y)

x = -1

for i = 1, 100 do
  x = bit.lshift(i, x)
end

y = 6553600

assert(x == y, "lshift: Got " .. x .. ", expect " .. y)

x = -1

for i = 1, 100 do
  x = bit.rshift(i, x)
end

y = 100

assert(x == y, "rshift: Got " .. x .. ", expect " .. y)

x = -1

for i = 1, 100 do
  x = bit.arshift(i, x)
end

y = 100

assert(x == y, "arshift: Got " .. x .. ", expect " .. y)

x = -1

for i = 1, 100 do
  x = bit.ror(i, 1)
end

y = 50

assert(x == y, "ror: Got " .. x .. ", expect " .. y)

