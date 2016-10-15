local ffi = require("ffi")

do
  -- IR_CONV int.num (positive)
  for i=200,399 do
    assert(math.ldexp(2,i/200) == 4)
  end
end

do
  -- IR_CONV int.num (negative)
  for i=-399,-200 do
    assert(math.ldexp(2,-(i/200)) == 4)
  end
end

do
  -- IR_CONV num.u64
  local x = ffi.new("uint64_t", 0xfffffffe)
  local y
  for i=1,100 do
    y = ffi.cast("double", x) + 1
  end
  assert(y == 0xffffffff)
end

do
  -- IR_CONV num.i64
  local x = ffi.new("int64_t", 0xfffffffe)
  local y
  for i=1,100 do
    y = ffi.cast("double",x) + 1
  end
  assert(y == 0xffffffff)
end

do
  -- IR_CONV flt.u64
  local x = ffi.new("uint64_t", 500)
  local y
  for i=1,100 do
    y = ffi.cast("float",x) + 1
  end
  assert(y == 501)
end

do
  -- IR_CONV flt.i64
  local x = ffi.new("int64_t", 500)
  local y
  for i=1,100 do
    y = ffi.cast("float",x) + 1
  end
  assert(y == 501)
end

do
  -- IR_CONV u64.num
  local x = ffi.new("double", 500)
  local y
  for i=1,100 do
    y = ffi.cast("uint64_t",x) + 1
  end
  assert(y == 501)
end

do
  -- IR_CONV i64.num
  local x = ffi.new("double", 500)
  local y
  for i=1,100 do
    y = ffi.cast("int64_t",x) + 1
  end
  assert(y == 501)
end

do
  -- IR_CONV u64.flt
  local x = ffi.new("float", 500)
  local y
  for i=1,100 do
    y = ffi.cast("uint64_t",x) + 1
  end
  assert(y == 501)
end

do
  -- IR_CONV i64.flt
  local x = ffi.new("float", 500)
  local y
  for i=1,100 do
    y = ffi.cast("int64_t",x) + 1
  end
  assert(y == 501)
end

do
  -- IR_CONV i64.int
  local x = ffi.new("uint8_t", 20)
  local y
  for i=1,100 do
    y = ffi.cast("int32_t",x) + 1
  end
  assert(y == 21)
end