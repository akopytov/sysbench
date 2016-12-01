########################################################################
threads benchmark tests
########################################################################

  $ args="--test=threads --max-requests=100 --num-threads=2"
  $ sysbench $args help
  sysbench *.* * (glob)
  
  threads options:
    --thread-yields=N      number of yields to do per request [1000]
    --thread-locks=N       number of locks per thread [8]
  
  $ sysbench $args prepare
  sysbench *.* * (glob)
  
  'threads' test does not have the 'prepare' command.
  [1]
  $ sysbench $args run
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 2
  Initializing random number generator from current time
  
  
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
           approx.  95th percentile:          *ms (glob)
  
  Threads fairness:
      events (avg/stddev):           */* (glob)
      execution time (avg/stddev):   */* (glob)
  
  $ sysbench $args cleanup
  sysbench *.* * (glob)
  
  'threads' test does not have the 'cleanup' command.
  [1]
