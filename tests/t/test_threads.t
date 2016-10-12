########################################################################
threads benchmark tests
########################################################################

  $ args="--test=threads --max-requests=100 --num-threads=2"
  $ sysbench $args help
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  threads options:
    --thread-yields=N      number of yields to do per request [1000]
    --thread-locks=N       number of locks per thread [8]
  
  $ sysbench $args prepare
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  'threads' test does not have the 'prepare' command.
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
      total number of events:              100
      total time taken by event execution: *s (glob)
      response time:
           min:                               *ms (glob)
           avg:                               *ms (glob)
           max:                               *ms (glob)
           approx.  95 percentile:            *ms (glob)
  
  Threads fairness:
      events (avg/stddev):           */* (glob)
      execution time (avg/stddev):   */* (glob)
  
  $ sysbench $args cleanup
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  'threads' test does not have the 'cleanup' command.
  [1]
