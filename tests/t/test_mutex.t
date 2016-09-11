########################################################################
mutex benchmark tests
########################################################################
  $ args="--test=mutex --max-requests=10 --num-threads=2"
  $ sysbench $args help
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  mutex options:
    --mutex-num=N        total size of mutex array [4096]
    --mutex-locks=N      number of mutex locks to do per thread [50000]
    --mutex-loops=N      number of empty loops to do inside mutex lock [10000]
  
  $ sysbench $args prepare
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  'mutex' test does not have the 'prepare' command.
  [1]
  $ sysbench $args run
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  Running the test with following options:
  Number of threads: 2
  Random number generator seed is 0 and will be ignored
  
  
  Initializing worker threads...
  
  Threads started!
  
  
  General statistics:
      total time:                          *s (glob)
      total number of events:              2
      total time taken by event execution: *s (glob)
      response time:
           min:                              *ms (glob)
           avg:                              *ms (glob)
           max:                              *ms (glob)
           approx.  95 percentile:           *ms (glob)
  
  Threads fairness:
      events (avg/stddev):           */* (glob)
      execution time (avg/stddev):   */* (glob)
  
  $ sysbench $args cleanup
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  'mutex' test does not have the 'cleanup' command.
  [1]
