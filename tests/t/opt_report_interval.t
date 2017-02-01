########################################################################
# --report-interval tests
########################################################################

  $ if [ -z "$SBTEST_HAS_MYSQL" ]
  > then
  >   exit 80
  > fi

  $ sysbench ${SBTEST_SCRIPTDIR}/oltp_read_write.lua --db-driver=mysql --mysql-dry-run --time=3 --events=0 --report-interval=1 run | grep '\[ 2s \]'
  [ 2s ] thds: 1 tps: * qps: * (r/w/o: */*/*) lat (ms,95%): *.* err/s: 0.00 reconn/s: 0.00 (glob)

# Run a test that does not support intermediate reports

  $ sysbench cpu --report-interval=1 --time=2 run | grep '\[ 1s \]'
  [ 1s ] thds: 1 eps: * lat (ms,95%): * (glob)
