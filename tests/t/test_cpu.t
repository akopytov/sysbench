########################################################################
cpu benchmark tests
########################################################################
  $ args="--test=cpu --cpu-max-prime=1000 --max-requests=100 --num-threads=2"
  $ sysbench $args help
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  cpu options:
    --cpu-max-prime=N      upper limit for primes generator [10000]
  
  $ sysbench $args prepare
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  'cpu' test does not have the 'prepare' command.
  [1]
  $ sysbench $args run
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  Running the test with following options:
  Number of threads: 2
  Random number generator seed is 0 and will be ignored
  
  
  Prime numbers limit: 1000
  
  Initializing worker threads...
  
  Threads started!
  
  
  General statistics:
      total time:                          *s (glob)
      total number of events:              100
      total time taken by event execution: *s (glob)
      response time:
           min:                                  *ms (glob)
           avg:                                  *ms (glob)
           max:                                  *ms (glob)
           approx.  95 percentile:               *ms (glob)
  
  Threads fairness:
      events (avg/stddev):           50.0000/* (glob)
      execution time (avg/stddev):   */* (glob)
  
  $ sysbench $args cleanup
  sysbench 0.5:  multi-threaded system evaluation benchmark
  
  'cpu' test does not have the 'cleanup' command.
  [1]
