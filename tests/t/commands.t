  $ commands=$(sysbench --help | grep 'Commands' | cut -d ' ' -f 6-)
  $ for cmd in $commands; do
  > if [ ! -r ${SBTEST_SUITEDIR}/cmd_${cmd}.t ]
  > then
  >   echo "Cannot find test(s) for 'sysbench $cmd'!"
  >   exit 1
  > fi
  > done
