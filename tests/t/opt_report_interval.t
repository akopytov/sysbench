########################################################################
# --report-interval tests
########################################################################

  $ sysbench --test=${SBTEST_SCRIPTDIR}/oltp.lua --db-driver=mysql --mysql-dry-run --max-time=3 --max-requests=0 --report-interval=2 run | grep '\[   2s\]'
  [   2s] threads: 1, tps: *, reads: *, writes: *, response time: *ms (95%), errors: 0.00, reconnects:  0.00 (glob)

# Run a test that does not support intermediate reports

  $ sysbench --test=cpu --report-interval=1 --max-time=1 --debug run | grep 'not supported'
  DEBUG: Reporting is not supported by the current test, terminating the reporting thread
