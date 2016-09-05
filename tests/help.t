  $ sysbench help
  sysbench 0.5:  multi-threaded system evaluation benchmark
  
  Usage:
    sysbench --test=<test-name> [options]... <command>
  
  Commands: prepare run cleanup help version
  
  General options:
    --num-threads=N             number of threads to use [1]
    --max-requests=N            limit for total number of requests [10000]
    --max-time=N                limit for total execution time in seconds [0]
    --forced-shutdown=STRING    amount of time to wait after --max-time before forcing shutdown [off]
    --thread-stack-size=SIZE    size of stack per thread [64K]
    --tx-rate=N                 target transaction rate (tps) [0]
    --report-interval=N         periodically report intermediate statistics with a specified interval in seconds. 0 disables intermediate reports [0]
    --report-checkpoints=[LIST,...]dump full statistics and reset all counters at specified points in time. The argument is a list of comma-separated values representing the amount of time in seconds elapsed from start of test when report checkpoint(s) must be performed. Report checkpoints are off by default. []
    --test=STRING               test to run
    --debug=[on|off]            print more debugging info [off]
    --validate=[on|off]         perform validation checks where possible [off]
    --help=[on|off]             print help and exit
    --version=[on|off]          print version and exit [off]
    --rand-init=[on|off]        initialize random number generator [off]
    --rand-type=STRING          random numbers distribution {uniform,gaussian,special,pareto} [special]
    --rand-spec-iter=N          number of iterations used for numbers generation [12]
    --rand-spec-pct=N           percentage of values to be treated as 'special' (for special distribution) [1]
    --rand-spec-res=N           percentage of 'special' values to use (for special distribution) [75]
    --rand-seed=N               seed for random number generator, ignored when 0 [0]
    --rand-pareto-h=N           parameter h for pareto distibution [0.2]
    --config-file=FILENAME      File containing command line options
  
  Log options:
    --verbosity=N      verbosity level {5 - debug, 0 - only critical messages} [3]
  
    --percentile=N      percentile rank of query response times to count [95]
  
  General database options:
  
    --db-driver=STRING  specifies database driver to use ('help' to get list of available drivers)
    --db-ps-mode=STRING prepared statements usage mode {auto, disable} [auto]
    --db-debug=[on|off] print database-specific debug information [off]
  
  
  Compiled-in database drivers:
    mysql - MySQL driver
  
  mysql options:
    --mysql-host=[LIST,...]      MySQL server host [localhost]
    --mysql-port=N               MySQL server port [3306]
    --mysql-socket=[LIST,...]    MySQL socket
    --mysql-user=STRING          MySQL user [sbtest]
    --mysql-password=STRING      MySQL password []
    --mysql-db=STRING            MySQL database name [sbtest]
    --mysql-table-engine=STRING  storage engine to use for the test table {myisam,innodb,bdb,heap,ndbcluster,federated} [innodb]
    --mysql-engine-trx=STRING    whether storage engine used is transactional or not {yes,no,auto} [auto]
    --mysql-ssl=[on|off]         use SSL connections, if available in the client library [off]
    --mysql-compression=[on|off] use compression, if available in the client library [off]
    --myisam-max-rows=N          max-rows parameter for MyISAM tables [1000000]
    --mysql-debug=[on|off]       dump all client library calls [off]
    --mysql-ignore-errors=[LIST,...]list of errors to ignore, or "all" [1213,1020,1205]
    --mysql-dry-run=[on|off]     Dry run, pretent that all MySQL client API calls are successful without executing them [off]
  
  Compiled-in tests:
    fileio - File I/O test
    cpu - CPU performance test
    memory - Memory functions speed test
    threads - Threads subsystem performance test
    mutex - Mutex performance test
  
  See 'sysbench --test=<name> help' for a list of options for each test.
  
