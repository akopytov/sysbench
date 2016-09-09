########################################################################
Make sure all available DB drivers are covered
########################################################################

  $ drivers=$(sysbench help | sed -n '/Compiled-in database drivers:/,/^$/p' | tail -n +2 | cut -d ' ' -f 3)
  $ for drv in $drivers
  > do
  >   if [ ! -r ${SBTEST_SUITEDIR}/drv_${drv}.t ]
  >   then
  >     echo "Cannot find test(s) for the $drv driver!"
  >     exit 1
  >   fi
  > done
