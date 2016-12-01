########################################################################
fileio benchmark tests
########################################################################

  $ fileio_args="--test=fileio --file-num=4 --file-total-size=32M"

  $ sysbench $fileio_args prepare
  sysbench *.* * (glob)
  
  4 files, 8192Kb each, 32Mb total
  Creating files for the test...
  Extra file open flags: 0
  Creating file test_file.0
  Creating file test_file.1
  Creating file test_file.2
  Creating file test_file.3
  33554432 bytes written in * seconds (*.* MiB/sec). (glob)

  $ ls test_file.*
  test_file.0
  test_file.1
  test_file.2
  test_file.3
  $ for i in $(seq 0 3)
  > do
  >   echo -n "test_file.$i: "
  >   echo $(wc -c < test_file.$i)
  > done
  test_file.0: 8388608
  test_file.1: 8388608
  test_file.2: 8388608
  test_file.3: 8388608

  $ sysbench $fileio_args run | grep FATAL
  FATAL: Missing required argument: --file-test-mode

  $ sysbench $fileio_args --max-requests=150 --file-test-mode=rndrw run
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 1
  Initializing random number generator from current time
  
  
  Extra file open flags: 0
  4 files, 8MiB each
  32MiB total file size
  Block size 16KiB
  Number of IO requests: 150
  Read/Write ratio for combined random IO test: 1.50
  Periodic FSYNC enabled, calling fsync() each 100 requests.
  Calling fsync() at the end of test, Enabled.
  Using synchronous I/O mode
  Doing random r/w test
  Initializing worker threads...
  
  Threads started!
  
  
  File operations:
      reads/s:                      *.* (glob)
      writes/s:                     *.* (glob)
      fsyncs/s:                     *.* (glob)
  
  Throughput:
      read, MiB/s:                  *.* (glob)
      written, MiB/s:               *.* (glob)
  
  General statistics:
      total time:                          *.*s (glob)
      total number of events:              150
      total time taken by event execution: *.*s (glob)
      response time:
           min:                                  *.*ms (glob)
           avg:                                  *.*ms (glob)
           max:                                  *.*ms (glob)
           approx.  95th percentile:             *.*ms (glob)
  
  Threads fairness:
      events (avg/stddev):           150.0000/0.00
      execution time (avg/stddev):   *.*/0.00 (glob)
  
  $ sysbench $fileio_args --max-requests=150 --file-test-mode=rndrd run
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 1
  Initializing random number generator from current time
  
  
  Extra file open flags: 0
  4 files, 8MiB each
  32MiB total file size
  Block size 16KiB
  Number of IO requests: 150
  Read/Write ratio for combined random IO test: 1.50
  Periodic FSYNC enabled, calling fsync() each 100 requests.
  Calling fsync() at the end of test, Enabled.
  Using synchronous I/O mode
  Doing random read test
  Initializing worker threads...
  
  Threads started!
  
  
  File operations:
      reads/s:                      *.* (glob)
      writes/s:                     0.00
      fsyncs/s:                     0.00
  
  Throughput:
      read, MiB/s:                  *.* (glob)
      written, MiB/s:               0.00
  
  General statistics:
      total time:                          *.*s (glob)
      total number of events:              150
      total time taken by event execution: *.*s (glob)
      response time:
           min:                                  *.*ms (glob)
           avg:                                  *.*ms (glob)
           max:                                  *.*ms (glob)
           approx.  95th percentile:             *.*ms (glob)
  
  Threads fairness:
      events (avg/stddev):           150.0000/0.00
      execution time (avg/stddev):   *.*/0.00 (glob)
  

  $ sysbench $fileio_args --max-requests=150 --file-test-mode=seqrd run
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 1
  Initializing random number generator from current time
  
  
  Extra file open flags: 0
  4 files, 8MiB each
  32MiB total file size
  Block size 16KiB
  Periodic FSYNC enabled, calling fsync() each 100 requests.
  Calling fsync() at the end of test, Enabled.
  Using synchronous I/O mode
  Doing sequential read test
  Initializing worker threads...
  
  Threads started!
  
  
  File operations:
      reads/s:                      *.* (glob)
      writes/s:                     0.00
      fsyncs/s:                     0.00
  
  Throughput:
      read, MiB/s:                  *.* (glob)
      written, MiB/s:               0.00
  
  General statistics:
      total time:                          *.*s (glob)
      total number of events:              150
      total time taken by event execution: *.*s (glob)
      response time:
           min:                                  *.*ms (glob)
           avg:                                  *.*ms (glob)
           max:                                  *.*ms (glob)
           approx.  95th percentile:             *.*ms (glob)
  
  Threads fairness:
      events (avg/stddev):           150.0000/0.00
      execution time (avg/stddev):   *.*/0.00 (glob)
  

  $ sysbench $fileio_args --max-requests=150 --file-test-mode=rndwr run
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 1
  Initializing random number generator from current time
  
  
  Extra file open flags: 0
  4 files, 8MiB each
  32MiB total file size
  Block size 16KiB
  Number of IO requests: 150
  Read/Write ratio for combined random IO test: 1.50
  Periodic FSYNC enabled, calling fsync() each 100 requests.
  Calling fsync() at the end of test, Enabled.
  Using synchronous I/O mode
  Doing random write test
  Initializing worker threads...
  
  Threads started!
  
  
  File operations:
      reads/s:                      0.00
      writes/s:                     *.* (glob)
      fsyncs/s:                     *.* (glob)
  
  Throughput:
      read, MiB/s:                  0.00
      written, MiB/s:               *.* (glob)
  
  General statistics:
      total time:                          *.*s (glob)
      total number of events:              150
      total time taken by event execution: *.*s (glob)
      response time:
           min:                                  *.*ms (glob)
           avg:                                  *.*ms (glob)
           max:                                  *.*ms (glob)
           approx.  95th percentile:             *.*ms (glob)
  
  Threads fairness:
      events (avg/stddev):           150.0000/0.00
      execution time (avg/stddev):   *.*/0.00 (glob)
  

  $ sysbench $fileio_args --max-requests=150 --file-test-mode=foo run
  sysbench *.* * (glob)
  
  FATAL: Invalid IO operations mode: foo.
  [1]
  $ sysbench $fileio_args cleanup
  sysbench *.* * (glob)
  
  Removing test files...
  $ ls
