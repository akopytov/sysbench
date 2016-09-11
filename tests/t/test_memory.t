########################################################################
memory benchmark tests
########################################################################

  $ args="--test=memory --memory-block-size=4K --memory-total-size=1G --max-requests=100 --num-threads=2"

The --memory-hugetlb option is supported and printed by 'sysbench
help' only on Linux.

  $ if [ "$(uname -s)" = "Linux" ]
  > then
  >   sysbench $args help | grep hugetlb
  > else
  >   echo "  --memory-hugetlb=[on|off]   allocate memory from HugeTLB pool [off]"
  > fi
    --memory-hugetlb=[on|off]   allocate memory from HugeTLB pool [off]

  $ sysbench $args help | grep -v hugetlb
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  memory options:
    --memory-block-size=SIZE    size of memory block for test [1K]
    --memory-total-size=SIZE    total size of data to transfer [100G]
    --memory-scope=STRING       memory access scope {global,local} [global]
    --memory-oper=STRING        type of memory operations {read, write, none} [write]
    --memory-access-mode=STRING memory access mode {seq,rnd} [seq]
  
  $ sysbench $args prepare
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  'memory' test does not have the 'prepare' command.
  [1]
  $ sysbench $args run
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  Running the test with following options:
  Number of threads: 2
  Random number generator seed is 0 and will be ignored
  
  
  Initializing worker threads...
  
  Threads started!
  
  Operations performed: 262144 (* ops/sec) (glob)
  
  1024.00 MB transferred (* MB/sec) (glob)
  
  
  General statistics:
      total time:                          *s (glob)
      total number of events:              262144 (glob)
      total time taken by event execution: *s (glob)
      response time:
           min:                                *ms (glob)
           avg:                                *ms (glob)
           max:                                *ms (glob)
           approx.  95 percentile:             *ms (glob)
  
  Threads fairness:
      events (avg/stddev):           */* (glob)
      execution time (avg/stddev):   */* (glob)
  
  $ sysbench $args cleanup
  sysbench *.*:  multi-threaded system evaluation benchmark (glob)
  
  'memory' test does not have the 'cleanup' command.
  [1]
