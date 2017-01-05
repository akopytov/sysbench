########################################################################
PRNG Lua API tests
########################################################################

  $ SB_ARGS="--verbosity=0 --test=$CRAMTMP/api_rand.lua --max-requests=1 --num-threads=1"

  $ cat >$CRAMTMP/api_rand.lua <<EOF
  > function event()
  >   print("sb_rand(0, 9) = " .. sb_rand(0, 9))
  >   print("sb_rand_uniq(0, 9) = " .. sb_rand_uniq(0, 9))
  >   print("sb_rnd() = " .. sb_rnd())
  >   print([[sb_rand_str("abc-###-@@@-xyz") = ]] .. sb_rand_str("abc-###-@@@-xyz"))
  >   print("sb_rand_uniform(0, 9) = " .. sb_rand_uniform(0, 9))
  >   print("sb_rand_gaussian(0, 9) = " .. sb_rand_gaussian(0, 9))
  >   print("sb_rand_special(0, 9) = " .. sb_rand_special(0, 9))
  > end
  > EOF

  $ sysbench $SB_ARGS run
  sb_rand\(0, 9\) = [0-9]{1} (re)
  sb_rand_uniq\(0, 9\) = [0-9]{1} (re)
  sb_rnd\(\) = [0-9]+ (re)
  sb_rand_str\(".*"\) = abc-[0-9]{3}-[a-z]{3}-xyz (re)
  sb_rand_uniform\(0, 9\) = [0-9]{1} (re)
  sb_rand_gaussian\(0, 9\) = [0-9]{1} (re)
  sb_rand_special\(0, 9\) = [0-9]{1} (re)
