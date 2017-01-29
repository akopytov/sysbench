########################################################################
MySQL driver tests
########################################################################

  $ . $SBTEST_INCDIR/mysql_common.sh
  $ . $SBTEST_INCDIR/drv_common.sh
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 2
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  Threads started!
  
  SQL statistics:
      queries performed:
          read:                            10
          write:                           0
          other:                           0
          total:                           10
      transactions:                        10     (*.* per sec.) (glob)
      queries:                             10     (*.* per sec.) (glob)
      ignored errors:                      0      (0.00 per sec.)
      reconnects:                          0      (0.00 per sec.)
  
  General statistics:
      total time:                          *.*s (glob)
      total number of events:              10
  
  Latency (ms):
           min:                              *.* (glob)
           avg:                              *.* (glob)
           max:                              *.* (glob)
           95th percentile:         *.* (glob)
           sum: *.* (glob)
  
  Threads fairness:
      events (avg/stddev):           *.*/*.* (glob)
      execution time (avg/stddev):   *.*/*.* (glob)
  
