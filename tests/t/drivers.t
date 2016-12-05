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

# Try using a non-existing driver
  $ sysbench --db-driver=nonexisting --test=${SBTEST_SCRIPTDIR}/oltp.lua cleanup
  sysbench * (glob)
  
  Dropping table 'sbtest1'...
  (FATAL: invalid database driver name: 'nonexisting'|FATAL: No DB drivers available) (re)
  FATAL: failed to execute function `cleanup': */common.lua:122: DB initialization failed (glob)
  [1]
