########################################################################
# --report-checkpoints tests
########################################################################

  $ sysbench --test=${SBTEST_SCRIPTDIR}/oltp.lua --db-driver=mysql --mysql-dry-run --max-time=3 --max-requests=0 --report-checkpoints=1,2 run | egrep '(Checkpoint report|OLTP test statistics)'
  [   0s] Checkpoint report:
  OLTP test statistics:
  [   1s] Checkpoint report:
  OLTP test statistics:
  OLTP test statistics:

# Run a test that does not support checkpoint reports

  $ sysbench --test=cpu --report-checkpoints=1 --max-time=1 --debug run | grep 'not supported'
  DEBUG: Reporting is not supported by the current test, terminating the checkpoints thread
