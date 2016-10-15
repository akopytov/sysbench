do
  -- IR_ASTORE: int
  local a = {}
  for i=1,100 do
    a[i] = i
  end
  assert(a[100]==100)
end

do
  -- IR_ASTORE: num
  local a = {}
  for i=1,100 do
    a[i] = 5.5
  end
  assert(a[100]==5.5)
end

do
  -- IR_ASTORE: false
  local a = {}
  for i=1,100 do
    a[i] = false
  end
  assert(not a[100])
end

do
  -- IR_ASTORE: true
  local a = {}
  for i=1,100 do
    a[i] = true
  end
  assert(a[100])
end

do
  -- IR_ASTORE: addr
  local a = {}
  for i=1,100 do
    a[i] = ""
  end
  assert(a[100] == "")
end
