########################################################################
select_random_*.lua + PostgreSQL tests 
########################################################################

  $ if [ -z "${SBTEST_PGSQL_ARGS:-}" ]
  > then
  >   exit 80
  > fi

  $ function db_show_table() {
  >  psql -c "\d+ $1" sbtest
  > }

  $ DB_DRIVER_ARGS="--db-driver=pgsql $SBTEST_PGSQL_ARGS"
  $ . $SBTEST_INCDIR/script_select_random_common.sh
  sysbench * (glob)
  
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  Creating secondary indexes on 'sbtest1'...
                                                   Table "public.sbtest1"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest1_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  Indexes:
      "sbtest1_pkey" PRIMARY KEY, btree (id)
      "k_1" btree (k)
  
  Did not find any relation named "sbtest2".
  Did not find any relation named "sbtest3".
  Did not find any relation named "sbtest4".
  Did not find any relation named "sbtest5".
  Did not find any relation named "sbtest6".
  Did not find any relation named "sbtest7".
  Did not find any relation named "sbtest8".
  sysbench * (glob)
  
  Running the test with following options:
  Number of threads: 1
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  Threads started!
  
  OLTP test statistics:
      queries performed:
          read:                            100
          write:                           0
          other:                           0
          total:                           100
      transactions:                        0      (0.00 per sec.)
      read/write requests:                 100    (* per sec.) (glob)
      other operations:                    0      (0.00 per sec.)
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
  
  sysbench * (glob)
  
  Dropping table 'sbtest1'...
  Did not find any relation named "sbtest1".
  Did not find any relation named "sbtest2".
  Did not find any relation named "sbtest3".
  Did not find any relation named "sbtest4".
  Did not find any relation named "sbtest5".
  Did not find any relation named "sbtest6".
  Did not find any relation named "sbtest7".
  Did not find any relation named "sbtest8".
  sysbench * (glob)
  
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  Creating secondary indexes on 'sbtest1'...
                                                   Table "public.sbtest1"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest1_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  Indexes:
      "sbtest1_pkey" PRIMARY KEY, btree (id)
      "k_1" btree (k)
  
  Did not find any relation named "sbtest2".
  Did not find any relation named "sbtest3".
  Did not find any relation named "sbtest4".
  Did not find any relation named "sbtest5".
  Did not find any relation named "sbtest6".
  Did not find any relation named "sbtest7".
  Did not find any relation named "sbtest8".
  sysbench * (glob)
  
  Running the test with following options:
  Number of threads: 1
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  Threads started!
  
  OLTP test statistics:
      queries performed:
          read:                            100
          write:                           0
          other:                           0
          total:                           100
      transactions:                        0      (0.00 per sec.)
      read/write requests:                 100    (* per sec.) (glob)
      other operations:                    0      (0.00 per sec.)
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
  
  sysbench * (glob)
  
  Dropping table 'sbtest1'...
  Did not find any relation named "sbtest1".
  Did not find any relation named "sbtest2".
  Did not find any relation named "sbtest3".
  Did not find any relation named "sbtest4".
  Did not find any relation named "sbtest5".
  Did not find any relation named "sbtest6".
  Did not find any relation named "sbtest7".
  Did not find any relation named "sbtest8".
