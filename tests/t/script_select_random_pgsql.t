########################################################################
select_random_*.lua + PostgreSQL tests 
########################################################################

  $ . $SBTEST_INCDIR/pgsql_common.sh
  $ . $SBTEST_INCDIR/script_select_random_common.sh
  sysbench * (glob)
  
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  Creating a secondary index on 'sbtest1'...
                              Table "public.sbtest1"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest1_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_1 ON sbtest1 USING btree (k)
   CREATE UNIQUE INDEX sbtest1_pkey ON sbtest1 USING btree (id)
  
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
  
  SQL statistics:
      queries performed:
          read:                            100
          write:                           0
          other:                           0
          total:                           100
      transactions:                        100    (* per sec.) (glob)
      queries:                             100    (* per sec.) (glob)
      ignored errors:                      0      (0.00 per sec.)
      reconnects:                          0      (0.00 per sec.)
  
  General statistics:
      total time:                          *s (glob)
      total number of events:              100
  
  Latency (ms):
           min:                              *.* (glob)
           avg:                              *.* (glob)
           max:                              *.* (glob)
           95th percentile:         *.* (glob)
           sum: *.* (glob)
  
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
  Creating a secondary index on 'sbtest1'...
                              Table "public.sbtest1"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest1_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_1 ON sbtest1 USING btree (k)
   CREATE UNIQUE INDEX sbtest1_pkey ON sbtest1 USING btree (id)
  
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
  
  SQL statistics:
      queries performed:
          read:                            100
          write:                           0
          other:                           0
          total:                           100
      transactions:                        100    (* per sec.) (glob)
      queries:                             100    (* per sec.) (glob)
      ignored errors:                      0      (0.00 per sec.)
      reconnects:                          0      (0.00 per sec.)
  
  General statistics:
      total time:                          *s (glob)
      total number of events:              100
  
  Latency (ms):
           min:                              *.* (glob)
           avg:                              *.* (glob)
           max:                              *.* (glob)
           95th percentile:         *.* (glob)
           sum: *.* (glob)
  
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
