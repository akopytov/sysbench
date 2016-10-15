-- test IR_RETF.
-- Refer more about "Ackermann Function" in wikipedia.
local function Ack(m, n)
  if m == 0 then
    return n + 1
  end

  if n == 0 then
    return Ack(m - 1, 1)
  end

  return Ack(m - 1, Ack(m, n - 1))
end

assert(Ack(3, 5) == 253)
