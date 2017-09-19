########################################################################
memory benchmark tests
########################################################################

  $ args="memory --memory-block-size=4K --memory-total-size=1G --events=1 --time=0 --threads=2"

The --memory-hugetlb option is supported and printed by 'sysbench
help' only on Linux.

  $ if [ "$(uname -s)" = "Linux" ]
  > then
  >   sysbench $args help | grep hugetlb
  > else
  >   echo "  --memory-hugetlb[=on|off]   allocate memory from HugeTLB pool [off]"
  > fi
    --memory-hugetlb[=on|off]   allocate memory from HugeTLB pool [off]

  $ sysbench $args help | grep -v hugetlb
  sysbench * (glob)
  
  memory options:
    --memory-block-size=SIZE    size of memory block for test [1K]
    --memory-total-size=SIZE    total size of data to transfer [100G]
    --memory-scope=STRING       memory access scope {global,local} [global]
    --memory-oper=STRING        type of memory operations {read, write, none} [write]
    --memory-access-mode=STRING memory access mode {seq,rnd} [seq]
  
  $ sysbench $args prepare
  sysbench *.* * (glob)
  
  'memory' test does not implement the 'prepare' command.
  [1]

  $ sysbench $args --memory-block-size=-1 run
  sysbench * (glob)
  
  FATAL: Invalid value for memory-block-size: -1
  [1]

  $ sysbench $args --memory-block-size=0 run
  sysbench * (glob)
  
  FATAL: Invalid value for memory-block-size: 0
  [1]

  $ sysbench $args --memory-block-size=3 run
  sysbench * (glob)
  
  FATAL: Invalid value for memory-block-size: 3
  [1]

  $ sysbench $args --memory-block-size=9 run
  sysbench * (glob)
  
  FATAL: Invalid value for memory-block-size: 9
  [1]

########################################################################
# Global reads
########################################################################

  $ sysbench $args --memory-oper=read run
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 2
  Initializing random number generator from current time
  
  
  Running memory speed test with the following options:
    block size: 4KiB
    total size: 1024MiB
    operation: read
    scope: global
  
  Initializing worker threads...
  
  Threads started!
  
  Total operations: 262144 (* per second) (glob)
  
  1024.00 MiB transferred (* MiB/sec) (glob)
  
  
  Throughput:
      events/s (eps): *.* (glob)
      time elapsed:                        *s (glob)
      total number of events:              262144
  
  Latency (ms):
           min:                              *.* (glob)
           avg:                              *.* (glob)
           max:                              *.* (glob)
           95th percentile:         *.* (glob)
           sum: *.* (glob)
  
  Threads fairness:
      events (avg/stddev):           */* (glob)
      execution time (avg/stddev):   */* (glob)
  
########################################################################
# Global writes
########################################################################

  $ sysbench $args --memory-oper=write run
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 2
  Initializing random number generator from current time
  
  
  Running memory speed test with the following options:
    block size: 4KiB
    total size: 1024MiB
    operation: write
    scope: global
  
  Initializing worker threads...
  
  Threads started!
  
  Total operations: 262144 (* per second) (glob)
  
  1024.00 MiB transferred (* MiB/sec) (glob)
  
  
  Throughput:
      events/s (eps): *.* (glob)
      time elapsed:                        *s (glob)
      total number of events:              262144
  
  Latency (ms):
           min:                              *.* (glob)
           avg:                              *.* (glob)
           max:                              *.* (glob)
           95th percentile:         *.* (glob)
           sum: *.* (glob)
  
  Threads fairness:
      events (avg/stddev):           */* (glob)
      execution time (avg/stddev):   */* (glob)
  
########################################################################
# Local reads
########################################################################

  $ sysbench $args --memory-scope=local --memory-oper=read run
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 2
  Initializing random number generator from current time
  
  
  Running memory speed test with the following options:
    block size: 4KiB
    total size: 1024MiB
    operation: read
    scope: local
  
  Initializing worker threads...
  
  Threads started!
  
  Total operations: 262144 (* per second) (glob)
  
  1024.00 MiB transferred (* MiB/sec) (glob)
  
  
  Throughput:
      events/s (eps): *.* (glob)
      time elapsed:                        *s (glob)
      total number of events:              262144
  
  Latency (ms):
           min:                              *.* (glob)
           avg:                              *.* (glob)
           max:                              *.* (glob)
           95th percentile:         *.* (glob)
           sum: *.* (glob)
  
  Threads fairness:
      events (avg/stddev):           */* (glob)
      execution time (avg/stddev):   */* (glob)
  
########################################################################
# Local writes
########################################################################

  $ sysbench $args --memory-scope=local --memory-oper=write run
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 2
  Initializing random number generator from current time
  
  
  Running memory speed test with the following options:
    block size: 4KiB
    total size: 1024MiB
    operation: write
    scope: local
  
  Initializing worker threads...
  
  Threads started!
  
  Total operations: 262144 (* per second) (glob)
  
  1024.00 MiB transferred (* MiB/sec) (glob)
  
  
  Throughput:
      events/s (eps): *.* (glob)
      time elapsed:                        *s (glob)
      total number of events:              262144
  
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
  
  'memory' test does not implement the 'cleanup' command.
  [1]
