########################################################################
--warmup-time tests
########################################################################

  $ cat >$CRAMTMP/warmup_time.lua <<EOF
  > local ffi = require("ffi")
  > ffi.cdef[[
  >   int usleep(unsigned int);
  > ]]
  > function event()
  >   if (sysbench.tid == 0) then
  >     ffi.C.usleep(2000000)
  >   else
  >     ffi.C.usleep(3000000)
  >   end
  > end
  > EOF

  $ sysbench --warmup-time=-1 $CRAMTMP/warmup_time.lua --threads=2 run
  FATAL: Invalid value for --warmup-time: -1.
  
  [1]

  $ sysbench --warmup-time=1 $CRAMTMP/warmup_time.lua --threads=2 --time=1 run
  sysbench * (glob)
  
  Running the test with following options:
  Number of threads: 2
  Warmup time: 1s
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  Threads started!
  
  Warming up for 1 seconds...
  
  
  Throughput:
      events/s (eps): *.* (glob)
      time elapsed:                        2.*s (glob)
      total number of events:              2
  
  Latency (ms):
           min: * (glob)
           avg: * (glob)
           max: * (glob)
           95th percentile: * (glob)
           sum: *.* (glob)
  
  Threads fairness:
      events (avg/stddev):           */* (glob)
      execution time (avg/stddev):   */* (glob)
  
  $ sysbench --warmup-time=1 $CRAMTMP/warmup_time.lua --threads=2 --time=0 --events=3 run
  sysbench * (glob)
  
  Running the test with following options:
  Number of threads: 2
  Warmup time: 1s
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  Threads started!
  
  Warming up for 1 seconds...
  
  
  Throughput:
      events/s (eps): *.* (glob)
      time elapsed:                        5.*s (glob)
      total number of events:              5
  
  Latency (ms):
           min: * (glob)
           avg: * (glob)
           max: * (glob)
           95th percentile: * (glob)
           sum: *.* (glob)
  
  Threads fairness:
      events (avg/stddev):           */* (glob)
      execution time (avg/stddev):   */* (glob)
  
