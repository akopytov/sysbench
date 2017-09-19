########################################################################
cpu benchmark tests
########################################################################
  $ args="cpu --cpu-max-prime=1000 --events=100 --threads=2"
  $ sysbench $args help
  sysbench *.* * (glob)
  
  cpu options:
    --cpu-max-prime=N upper limit for primes generator [10000]
  
  $ sysbench $args prepare
  sysbench *.* * (glob)
  
  'cpu' test does not implement the 'prepare' command.
  [1]
  $ sysbench $args run
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 2
  Initializing random number generator from current time
  
  
  Prime numbers limit: 1000
  
  Initializing worker threads...
  
  Threads started!
  
  CPU speed:
      events per second: *.* (glob)
  
  Throughput:
      events/s (eps): *.* (glob)
      time elapsed:                        *s (glob)
      total number of events:              100
  
  Latency (ms):
           min:                              *.* (glob)
           avg:                              *.* (glob)
           max:                              *.* (glob)
           95th percentile:         *.* (glob)
           sum: *.* (glob)
  
  Threads fairness:
      events (avg/stddev):           50.0000/* (glob)
      execution time (avg/stddev):   */* (glob)
  
  $ sysbench $args cleanup
  sysbench *.* * (glob)
  
  'cpu' test does not implement the 'cleanup' command.
  [1]
