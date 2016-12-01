########################################################################
cpu benchmark tests
########################################################################
  $ args="--test=cpu --cpu-max-prime=1000 --max-requests=100 --num-threads=2"
  $ sysbench $args help
  sysbench *.* * (glob)
  
  cpu options:
    --cpu-max-prime=N      upper limit for primes generator [10000]
  
  $ sysbench $args prepare
  sysbench *.* * (glob)
  
  'cpu' test does not have the 'prepare' command.
  [1]
  $ sysbench $args run
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 2
  Initializing random number generator from current time
  
  
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
           approx.  95th percentile:             *ms (glob)
  
  Threads fairness:
      events (avg/stddev):           50.0000/* (glob)
      execution time (avg/stddev):   */* (glob)
  
  $ sysbench $args cleanup
  sysbench *.* * (glob)
  
  'cpu' test does not have the 'cleanup' command.
  [1]
