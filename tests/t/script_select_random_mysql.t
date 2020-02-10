########################################################################
select_random_*.lua + MySQL tests 
########################################################################

  $ . $SBTEST_INCDIR/mysql_common.sh
  $ . $SBTEST_INCDIR/script_select_random_common.sh
  sysbench * (glob)
  
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  Creating a secondary index on 'sbtest1'...
  *************************** 1. row ***************************
  sbtest1
  CREATE TABLE `sbtest1` (
    `id` int* NOT NULL AUTO_INCREMENT, (glob)
    `k` int* NOT NULL DEFAULT '0', (glob)
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_1` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest2' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest3' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest4' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest5' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest6' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest7' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest8' doesn't exist
  sysbench * (glob)
  
  Running the test with following options:
  Number of threads: 1
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  Threads started!
  
  SQL statistics:
      queries performed:
          read:                            100
          write:                           0
          other:                           0
          total:                           100
      transactions:                        100    (* per sec.) (glob)
      queries:                             100    (* per sec.) (glob)
      ignored errors:                      0      (0.00 per sec.)
      reconnects:                          0      (0.00 per sec.)
  
  General statistics:
      total time:                          *s (glob)
      total number of events:              100
  
  Latency (ms):
           min:                              *.* (glob)
           avg:                              *.* (glob)
           max:                              *.* (glob)
           95th percentile:         *.* (glob)
           sum: *.* (glob)
  
  Threads fairness:
      events (avg/stddev):* (glob)
      execution time (avg/stddev):* (glob)
  
  sysbench * (glob)
  
  Dropping table 'sbtest1'...
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest1' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest2' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest3' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest4' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest5' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest6' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest7' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest8' doesn't exist
  sysbench * (glob)
  
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  Creating a secondary index on 'sbtest1'...
  *************************** 1. row ***************************
  sbtest1
  CREATE TABLE `sbtest1` (
    `id` int* NOT NULL AUTO_INCREMENT, (glob)
    `k` int* NOT NULL DEFAULT '0', (glob)
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_1` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest2' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest3' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest4' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest5' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest6' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest7' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest8' doesn't exist
  sysbench * (glob)
  
  Running the test with following options:
  Number of threads: 1
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  Threads started!
  
  SQL statistics:
      queries performed:
          read:                            100
          write:                           0
          other:                           0
          total:                           100
      transactions:                        100    (* per sec.) (glob)
      queries:                             100    (* per sec.) (glob)
      ignored errors:                      0      (0.00 per sec.)
      reconnects:                          0      (0.00 per sec.)
  
  General statistics:
      total time:                          *s (glob)
      total number of events:              100
  
  Latency (ms):
           min:                              *.* (glob)
           avg:                              *.* (glob)
           max:                              *.* (glob)
           95th percentile:         *.* (glob)
           sum: *.* (glob)
  
  Threads fairness:
      events (avg/stddev):* (glob)
      execution time (avg/stddev):* (glob)
  
  sysbench * (glob)
  
  Dropping table 'sbtest1'...
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest1' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest2' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest3' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest4' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest5' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest6' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest7' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest8' doesn't exist
