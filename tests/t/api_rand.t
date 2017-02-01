########################################################################
PRNG Lua API tests
########################################################################

  $ SB_ARGS="--verbosity=0 --events=1"

  $ cat >$CRAMTMP/api_rand.lua <<EOF
  > function event()
  >   print("sb_rand(0, 9) = " .. sb_rand(0, 9))
  >   print("sb_rand_uniq(0, 4294967295) = " .. sb_rand_uniq(0, 4294967295))
  >   print("sb_rnd() = " .. sb_rnd())
  >   print([[sb_rand_str("abc-###-@@@-xyz") = ]] .. sb_rand_str("abc-###-@@@-xyz"))
  >   print("sb_rand_uniform(0, 9) = " .. sb_rand_uniform(0, 9))
  >   print("sb_rand_gaussian(0, 9) = " .. sb_rand_gaussian(0, 9))
  >   print("sb_rand_special(0, 9) = " .. sb_rand_special(0, 9))
  > end
  > EOF

  $ sysbench $SB_ARGS $CRAMTMP/api_rand.lua run
  sb_rand\(0, 9\) = [0-9]{1} (re)
  sb_rand_uniq\(0, 4294967295\) = [0-9]{1,10} (re)
  sb_rnd\(\) = [0-9]+ (re)
  sb_rand_str\(".*"\) = abc-[0-9]{3}-[a-z]{3}-xyz (re)
  sb_rand_uniform\(0, 9\) = [0-9]{1} (re)
  sb_rand_gaussian\(0, 9\) = [0-9]{1} (re)
  sb_rand_special\(0, 9\) = [0-9]{1} (re)

########################################################################
issue #96: sb_rand_uniq(1, oltp_table_size) generate duplicate value
########################################################################
  $ cat >$CRAMTMP/api_rand_uniq.lua <<EOF
  > function event()
  >   local max = 1000000000
  >   local n = sb_rand_uniq(1, max)
  >   if n > max then
  >     error("n is out of range")
  >   end
  >   print(n)
  > end
  > EOF

  $ sysbench $SB_ARGS --events=100000 $CRAMTMP/api_rand_uniq.lua run |
  >   sort -n | uniq | wc -l | sed -e 's/ //g'
  100000
