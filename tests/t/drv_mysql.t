########################################################################
MySQL driver tests
########################################################################

  $ if [ -z "${SBTEST_MYSQL_ARGS:-}" ]
  > then
  >   exit 80
  > fi
  $ DB_DRIVER_ARGS="--db-driver=mysql $SBTEST_MYSQL_ARGS"
  $ . $SBTEST_INCDIR/drv_common.sh
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  Running the test with following options:
  Number of threads: 2
  Random number generator seed is 0 and will be ignored
  
  
  Initializing worker threads...
  
  Threads started!
  
  OLTP test statistics:
      queries performed:
          read:                            10
          write:                           0
          other:                           0
          total:                           10
      transactions:                        0      (0.00 per sec.)
      read/write requests:                 10     (*.* per sec.) (glob)
      other operations:                    0      (0.00 per sec.)
      ignored errors:                      0      (0.00 per sec.)
      reconnects:                          0      (0.00 per sec.)
  
  General statistics:
      total time:                          *.*s (glob)
      total number of events:              10
      total time taken by event execution: *.*s (glob)
      response time:
           min:                                 *.*ms (glob)
           avg:                                 *.*ms (glob)
           max:                                 *.*ms (glob)
           approx.  95 percentile:              *.*ms (glob)
  
  Threads fairness:
      events (avg/stddev):           *.*/*.* (glob)
      execution time (avg/stddev):   *.*/*.* (glob)
  
