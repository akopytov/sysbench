########################################################################
threads benchmark tests
########################################################################

  $ args="threads --events=100 --threads=2"
  $ sysbench $args help
  sysbench *.* * (glob)
  
  threads options:
    --thread-yields=N number of yields to do per request [1000]
    --thread-locks=N  number of locks per thread [8]
  
  $ sysbench $args prepare
  sysbench *.* * (glob)
  
  'threads' test does not implement the 'prepare' command.
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
  
  Latency (ms):
           min:                              *.* (glob)
           avg:                              *.* (glob)
           max:                              *.* (glob)
           95th percentile:         *.* (glob)
           sum: *.* (glob)
  
  Threads fairness:
      events (avg/stddev):           */* (glob)
      execution time (avg/stddev):   */* (glob)
  
  $ sysbench $args cleanup
  sysbench *.* * (glob)
  
  'threads' test does not implement the 'cleanup' command.
  [1]
