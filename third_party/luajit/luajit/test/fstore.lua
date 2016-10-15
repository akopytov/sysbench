do
  local mt = {}
  local t = {}
  for i=1,100 do
    setmetatable(t, mt)
    assert(getmetatable(t) == mt)
  end
  assert(getmetatable(t) == mt)
end
