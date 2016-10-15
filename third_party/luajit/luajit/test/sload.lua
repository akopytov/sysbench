-- Test several sload modes, include RI, TI, R.
-- SLOAD have 6 modes, they are:
-- IRSLOAD_PARENT (P)
-- IRSLOAD_FRAME (F)
-- IRSLOAD_TYPECHECK (T)
-- IRSLOAD_CONVERT (C)
-- IRSLOAD_READONLY (R)
-- IRSLOAD_INHERIT (I)
--
-- Following case tests RI, TI, R mode.

for i = 1, 200, "2" do
  x = 2
end

y = 2

assert(x == y, "Got " .. x .. ", expect " .. y)
