########################################################################
UUID Lua API tests
########################################################################

  $ SB_ARGS="--verbosity=0 --events=1"

  $ cat >$CRAMTMP/api_uuid.lua <<EOF
  > function init()
  >   -- Ensure consistent sort order
  >   mod_funcs = {}
  >   for f in pairs(sysbench.uuid) do
  >     table.insert(mod_funcs, f)
  >   end
  >   table.sort(mod_funcs)
  >   for i, f in ipairs(mod_funcs) do
  >     print(string.format("sysbench.uuid.%s", f))
  >   end
  > end
  > function event()
  >   print("sysbench.uuid.new() = " .. sysbench.uuid.new())
  > end
  > EOF

  $ sysbench $SB_ARGS $CRAMTMP/api_uuid.lua run
  sysbench.uuid.new
  sysbench.uuid.new\(\) = [0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12} (re)

########################################################################
Make sure UUID generation is thread-safe and unique per thread
########################################################################
  $ cat >$CRAMTMP/api_uuid_uniq.lua <<EOF
  > function event()
  >   print(sysbench.uuid.new())
  > end
  > EOF

  $ sysbench $SB_ARGS --events=2 --threads=2 $CRAMTMP/api_uuid_uniq.lua run |
  >   sort -n | uniq | wc -l | sed -e 's/ //g'
  2
