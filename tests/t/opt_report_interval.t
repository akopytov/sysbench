########################################################################
# --report-interval tests
########################################################################

  $ if [ -z "$SBTEST_HAS_MYSQL" ]
  > then
  >   exit 80
  > fi

  $ sysbench ${SBTEST_SCRIPTDIR}/oltp_read_write.lua --db-driver=mysql --mysql-dry-run --max-time=3 --max-requests=0 --report-interval=1 run | grep '\[   2s\]'
  [   2s] threads: 1 tps: * qps: * (r/w/o: */*/*) latency: *ms (95%) errors/s: 0.00 reconnects/s: 0.00 (glob)

# Run a test that does not support intermediate reports

  $ sysbench cpu --report-interval=1 --max-time=1 --debug run | grep 'not supported'
  DEBUG: Reporting is not supported by the current test, terminating the reporting thread
