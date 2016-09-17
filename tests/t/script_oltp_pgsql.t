########################################################################
oltp.lua + PostgreSQL tests 
########################################################################

  $ if [ -z "${SBTEST_PGSQL_ARGS:-}" ]
  > then
  >   exit 80
  > fi

  $ DB_DRIVER_ARGS="--db-driver=pgsql $SBTEST_PGSQL_ARGS"
  $ . $SBTEST_INCDIR/script_oltp_common.sh
  sysbench *:  multi-threaded system evaluation benchmark (glob)
  
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  Creating secondary indexes on 'sbtest1'...
  Creating table 'sbtest2'...
  Inserting 10000 records into 'sbtest2'
  Creating secondary indexes on 'sbtest2'...
  Creating table 'sbtest3'...
  Inserting 10000 records into 'sbtest3'
  Creating secondary indexes on 'sbtest3'...
  Creating table 'sbtest4'...
  Inserting 10000 records into 'sbtest4'
  Creating secondary indexes on 'sbtest4'...
  Creating table 'sbtest5'...
  Inserting 10000 records into 'sbtest5'
  Creating secondary indexes on 'sbtest5'...
  Creating table 'sbtest6'...
  Inserting 10000 records into 'sbtest6'
  Creating secondary indexes on 'sbtest6'...
  Creating table 'sbtest7'...
  Inserting 10000 records into 'sbtest7'
  Creating secondary indexes on 'sbtest7'...
  Creating table 'sbtest8'...
  Inserting 10000 records into 'sbtest8'
  Creating secondary indexes on 'sbtest8'...
  sysbench *:  multi-threaded system evaluation benchmark (glob)
  
  Running the test with following options:
  Number of threads: 1
  Random number generator seed is 0 and will be ignored
  
  
  Initializing worker threads...
  
  Threads started!
  
  OLTP test statistics:
      queries performed:
          read:                            1400
          write:                           400
          other:                           200
          total:                           2000
      transactions:                        100    (* per sec.) (glob)
      read/write requests:                 1800   (* per sec.) (glob)
      other operations:                    200    (* per sec.) (glob)
      ignored errors:                      0      (* per sec.) (glob)
      reconnects:                          0      (* per sec.) (glob)
  
  General statistics:
      total time:                          *s (glob)
      total number of events:              100
      total time taken by event execution: *s (glob)
      response time:
           min:                               *ms (glob)
           avg:                               *ms (glob)
           max:                               *ms (glob)
           approx.  95 percentile:            *ms (glob)
  
  Threads fairness:
      events (avg/stddev):           */* (glob)
      execution time (avg/stddev):   */* (glob)
  
  sysbench *:  multi-threaded system evaluation benchmark (glob)
  
  Dropping table 'sbtest1'...
  Dropping table 'sbtest2'...
  Dropping table 'sbtest3'...
  Dropping table 'sbtest4'...
  Dropping table 'sbtest5'...
  Dropping table 'sbtest6'...
  Dropping table 'sbtest7'...
  Dropping table 'sbtest8'...

