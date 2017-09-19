########################################################################
--histogram tests
########################################################################

  $ cat >$CRAMTMP/histogram.lua <<EOF
  > local ffi = require("ffi")
  > ffi.cdef[[
  >   int usleep(unsigned int);
  > ]]
  > function event()
  >   if (sysbench.tid == 0) then
  >     ffi.C.usleep(1000000)
  >   else
  >     ffi.C.usleep(2000000)
  >   end
  > end
  > EOF
  $ sysbench --histogram $CRAMTMP/histogram.lua --events=2 --threads=2 run
  sysbench * (glob)
  
  Running the test with following options:
  Number of threads: 2
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  Threads started!
  
  Latency histogram (values are in milliseconds)
         value  ------------- distribution ------------- count
     * |\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\* 1 (glob)
     * |\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\* 1 (glob)
   
  
  Throughput:
      events/s (eps): *.* (glob)
      time elapsed:                     *s (glob)
      total number of events:              2
  
  Latency (ms):
           min: *.* (glob)
           avg: *.* (glob)
           max: *.* (glob)
           95th percentile: *.* (glob)
           sum: *.* (glob)
  
  Threads fairness:
      events (avg/stddev):           1.0000/0.00
      execution time (avg/stddev):   */* (glob)
  
