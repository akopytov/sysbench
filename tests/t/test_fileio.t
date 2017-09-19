########################################################################
fileio benchmark tests
########################################################################

  $ fileio_args="fileio --file-num=4 --file-total-size=32M"

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

  $ sysbench $fileio_args --events=150 --file-test-mode=rndrw run
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
  
  
  Throughput:
           read:  IOPS=*.* *.* MiB/s (*.* MB/s) (glob)
           write: IOPS=*.* *.* MiB/s (*.* MB/s) (glob)
           fsync: IOPS=*.* (glob)
  
  Latency (ms):
           min:                              *.* (glob)
           avg:                              *.* (glob)
           max:                              *.* (glob)
           95th percentile:         *.* (glob)
           sum: *.* (glob)
  
  $ sysbench $fileio_args --events=150 --file-test-mode=rndrd run
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
  
  
  Throughput:
           read:  IOPS=*.* *.* MiB/s (*.* MB/s) (glob)
           write: IOPS=*.* *.* MiB/s (*.* MB/s) (glob)
           fsync: IOPS=*.* (glob)
  
  Latency (ms):
           min:                              *.* (glob)
           avg:                              *.* (glob)
           max:                              *.* (glob)
           95th percentile:         *.* (glob)
           sum: *.* (glob)
  

  $ sysbench $fileio_args --events=150 --file-test-mode=seqrd run
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
  
  
  Throughput:
           read:  IOPS=*.* *.* MiB/s (*.* MB/s) (glob)
           write: IOPS=*.* *.* MiB/s (*.* MB/s) (glob)
           fsync: IOPS=*.* (glob)
  
  Latency (ms):
           min:                              *.* (glob)
           avg:                              *.* (glob)
           max:                              *.* (glob)
           95th percentile:         *.* (glob)
           sum: *.* (glob)
  

  $ sysbench $fileio_args --events=150 --file-test-mode=rndwr run
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
  
  
  Throughput:
           read:  IOPS=*.* *.* MiB/s (*.* MB/s) (glob)
           write: IOPS=*.* *.* MiB/s (*.* MB/s) (glob)
           fsync: IOPS=*.* (glob)
  
  Latency (ms):
           min:                              *.* (glob)
           avg:                              *.* (glob)
           max:                              *.* (glob)
           95th percentile:         *.* (glob)
           sum: *.* (glob)
  

  $ sysbench $fileio_args --events=150 --file-test-mode=foo run
  sysbench *.* * (glob)
  
  FATAL: Invalid IO operations mode: foo.
  [1]
  $ sysbench $fileio_args cleanup
  sysbench *.* * (glob)
  
  Removing test files...
  $ ls

  $ sysbench $fileio_args --file-test-mode=rndrw --verbosity=2 run
  FATAL: Cannot open file 'test_file.0' errno = 2 (No such file or directory)
  WARNING: Did you forget to run the prepare step?
  [1]
