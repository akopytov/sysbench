########################################################################
Make sure all built-in tests are covered
########################################################################

  $ tests=$(sysbench help | sed -n '/Compiled-in tests:/,/^$/p' | tail -n +2 | cut -d ' ' -f 3)
  $ for t in $tests
  > do
  >   if [ ! -r ${SBTEST_SUITEDIR}/test_${t}.t ]
  >   then
  >     echo "Cannot find test(s) for 'sysbench --test=$t'!"
  >     exit 1
  >   fi
  > done
