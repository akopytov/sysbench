########################################################################
PRNG Lua API tests
########################################################################

  $ SB_ARGS="--verbosity=0 --events=1"

  $ cat >$CRAMTMP/api_rand.lua <<EOF
  > ffi.cdef[[int printf(const char *fmt, ...);]]
  > function init()
  >   -- Ensure a consistent sort order...
  >   mod_funcs = {}
  >   for f in pairs(sysbench.rand) do
  >     table.insert(mod_funcs, f)
  >   end
  >   table.sort(mod_funcs)
  >   for i, f in ipairs(mod_funcs) do
  >     print(string.format("sysbench.rand.%s", f))
  >   end
  > end
  > function event()
  >   print("sysbench.rand.default(0, 99) = " .. sysbench.rand.default(0, 99))
  >   print("sysbench.rand.unique(0, 4294967295) = " .. sysbench.rand.unique(0, 4294967295))
  >   ffi.C.printf("sysbench.rand.uniform_uint64() = %llu\n", sysbench.rand.uniform_uint64())
  >   print([[sysbench.rand.string("abc-###-@@@-xyz") = ]] .. sysbench.rand.string("abc-###-@@@-xyz"))
  >   print([[sysbench.rand.varstring(1, 23) = ]] .. sysbench.rand.varstring(1, 23))
  >   print("sysbench.rand.uniform(0, 99) = " .. sysbench.rand.uniform(0, 99))
  >   print("sysbench.rand.gaussian(0, 99) = " .. sysbench.rand.gaussian(0, 99))
  >   print("sysbench.rand.pareto(0, 99) = " .. sysbench.rand.pareto(0, 99))
  >   print("sysbench.rand.zipfian(0, 99) = " .. sysbench.rand.zipfian(0, 99))
  > end
  > EOF

  $ sysbench $SB_ARGS $CRAMTMP/api_rand.lua run
  sysbench.rand.default
  sysbench.rand.gaussian
  sysbench.rand.pareto
  sysbench.rand.string
  sysbench.rand.uniform
  sysbench.rand.uniform_double
  sysbench.rand.uniform_uint64
  sysbench.rand.unique
  sysbench.rand.varstring
  sysbench.rand.zipfian
  sysbench.rand.default\(0, 99\) = [0-9]{1,2} (re)
  sysbench.rand.unique\(0, 4294967295\) = [0-9]{1,10} (re)
  sysbench.rand.uniform_uint64\(\) = [0-9]+ (re)
  sysbench.rand.string\(".*"\) = abc-[0-9]{3}-[a-z]{3}-xyz (re)
  sysbench.rand.varstring\(1, 23\) = [0-z]{1,23} (re)
  sysbench.rand.uniform\(0, 99\) = [0-9]{1,2} (re)
  sysbench.rand.gaussian\(0, 99\) = [0-9]{1,2} (re)
  sysbench.rand.pareto\(0, 99\) = [0-9]{1,2} (re)
  sysbench.rand.zipfian\(0, 99\) = [0-9]{1,2} (re)

########################################################################
GH-96: sb_rand_uniq(1, oltp_table_size) generate duplicate value
########################################################################
  $ cat >$CRAMTMP/api_rand_uniq.lua <<EOF
  > function event()
  >   print(sysbench.rand.unique())
  > end
  > EOF

  $ sysbench $SB_ARGS --events=100000 $CRAMTMP/api_rand_uniq.lua run |
  >   sort -n | uniq | wc -l | sed -e 's/ //g'
  100000
