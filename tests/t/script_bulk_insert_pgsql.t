########################################################################
bulk_insert.lua + PostgreSQL tests
########################################################################

  $ if [ -z "${SBTEST_PGSQL_ARGS:-}" ]
  > then
  >   exit 80
  > fi

  $ function db_show_table() {
  >  psql -c "\d+ $1" sbtest
  > }

  $ DB_DRIVER_ARGS="--db-driver=pgsql $SBTEST_PGSQL_ARGS"
  $ . $SBTEST_INCDIR/script_bulk_insert_common.sh
  Creating table 'sbtest1'...
  Creating table 'sbtest2'...
                              Table "public.sbtest1"
   Column |  Type   |     Modifiers      | Storage | Stats target | Description 
  --------+---------+--------------------+---------+--------------+-------------
   id     | integer | not null           | plain   |              | 
   k      | integer | not null default 0 | plain   |              | 
  Indexes:
      "sbtest1_pkey" PRIMARY KEY, btree (id)
  
                              Table "public.sbtest2"
   Column |  Type   |     Modifiers      | Storage | Stats target | Description 
  --------+---------+--------------------+---------+--------------+-------------
   id     | integer | not null           | plain   |              | 
   k      | integer | not null default 0 | plain   |              | 
  Indexes:
      "sbtest2_pkey" PRIMARY KEY, btree (id)
  
  Did not find any relation named "sbtest3".
  sysbench * (glob)
  
  Running the test with following options:
  Number of threads: 2
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  Threads started!
  
  OLTP test statistics:
      queries performed:
          read:                            0
          write:                           2
          other:                           0
          total:                           2
      transactions:                        100    (* per sec.) (glob)
      queries:                             2      (* per sec.) (glob)
      ignored errors:                      0      (0.00 per sec.)
      reconnects:                          0      (0.00 per sec.)
  
  General statistics:
      total time:                          *s (glob)
      total number of events:              100
      total time taken by event execution: *s (glob)
  
  Latency statistics:
           min:                              *.*ms (glob)
           avg:                              *.*ms (glob)
           max:                              *.*ms (glob)
           approx.  95th percentile:         *.*ms (glob)
  
  Threads fairness:
      events (avg/stddev):* (glob)
      execution time (avg/stddev):* (glob)
  
  Dropping table 'sbtest1'...
  Dropping table 'sbtest2'...
  Did not find any relation named "sbtest1".
  Did not find any relation named "sbtest2".
  Did not find any relation named "sbtest3".
