########################################################################
# Command line syntax tests
########################################################################

  $ sysbench foo bar
  sysbench * (glob)
  
  Cannot find script foo: No such file or directory
  [1]

  $ sysbench foo bar baz
  Unrecognized command line argument: baz
  [1]

  $ sysbench --unknown <<EOF
  > EOF
  sysbench * (glob)
  

  $ sysbench fileio
  sysbench * (glob)
  
  The 'fileio' test requires a command argument. See 'sysbench fileio help'
  [1]

  $ sysbench --help foo | grep Usage:
  Usage:

  $ sysbench <<EOF
  > print('test')
  > EOF
  sysbench * (glob)
  
  test

  $ sysbench run <<EOF
  > print('script body')
  > function event()
  >   print('event function')
  > end
  > EOF
  sysbench * (glob)
  
  Cannot find script run: No such file or directory
  [1]

  $ cat >$CRAMTMP/cmdline.lua <<EOF
  > #!/usr/bin/env sysbench
  > print('script body')
  > function event()
  >   print('event function')
  > end
  > EOF
  $ sysbench --max-requests=1 $CRAMTMP/cmdline.lua
  sysbench * (glob)
  
  script body

  $ sysbench --max-requests=1 $CRAMTMP/cmdline.lua run
  sysbench * (glob)
  
  script body
  Running the test with following options:
  Number of threads: 1
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  script body
  Threads started!
  
  event function
  
  General statistics:
      total time: *s (glob)
      total number of events:              1
      total time taken by event execution: *s (glob)
  
  Latency statistics:
           min: *ms (glob)
           avg: *ms (glob)
           max: *ms (glob)
           approx.  95th percentile: *ms (glob)
  
  Threads fairness:
      events (avg/stddev):           1.0000/0.00
      execution time (avg/stddev):   0.0000/0.00
  