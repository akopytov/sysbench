########################################################################
PRNG Lua API tests
########################################################################

  $ SB_ARGS="--verbosity=0 --events=1"

  $ cat >$CRAMTMP/api_rand.lua <<EOF
  > ffi.cdef[[int printf(const char *fmt, ...);]]
  > function init()
  >   for k, v in pairs(sysbench.rand) do
  >     print(string.format("sysbench.rand.%s", k))
  >   end
  > end
  > function event()
  >   print("sysbench.rand.default(0, 99) = " .. sysbench.rand.default(0, 99))
  >   print("sysbench.rand.unique(0, 4294967295) = " .. sysbench.rand.unique(0, 4294967295))
  >   ffi.C.printf("sysbench.rand.uniform_uint64() = %llu\n", sysbench.rand.uniform_uint64())
  >   print([[sysbench.rand.string("abc-###-@@@-xyz") = ]] .. sysbench.rand.string("abc-###-@@@-xyz"))
  >   print("sysbench.rand.uniform(0, 99) = " .. sysbench.rand.uniform(0, 99))
  >   print("sysbench.rand.gaussian(0, 99) = " .. sysbench.rand.gaussian(0, 99))
  >   print("sysbench.rand.special(0, 99) = " .. sysbench.rand.special(0, 99))
  >   print("sysbench.rand.pareto(0, 99) = " .. sysbench.rand.pareto(0, 99))
  >   print("sysbench.rand.zipfian(0, 99) = " .. sysbench.rand.zipfian(0, 99))
  > end
  > EOF

  $ sysbench $SB_ARGS $CRAMTMP/api_rand.lua run
  sysbench.rand.uniform
  sysbench.rand.special
  sysbench.rand.zipfian
  sysbench.rand.unique
  sysbench.rand.gaussian
  sysbench.rand.uniform_uint64
  sysbench.rand.uniform_double
  sysbench.rand.pareto
  sysbench.rand.string
  sysbench.rand.default
  sysbench.rand.default\(0, 99\) = [0-9]{1,2} (re)
  sysbench.rand.unique\(0, 4294967295\) = [0-9]{1,10} (re)
  sysbench.rand.uniform_uint64\(\) = [0-9]+ (re)
  sysbench.rand.string\(".*"\) = abc-[0-9]{3}-[a-z]{3}-xyz (re)
  sysbench.rand.uniform\(0, 99\) = [0-9]{1,2} (re)
  sysbench.rand.gaussian\(0, 99\) = [0-9]{1,2} (re)
  sysbench.rand.special\(0, 99\) = [0-9]{1,2} (re)
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
