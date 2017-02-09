########################################################################
oltp_point_select.lua + PostgreSQL tests 
########################################################################

  $ . $SBTEST_INCDIR/pgsql_common.sh
  $ OLTP_SCRIPT_PATH=${SBTEST_SCRIPTDIR}/oltp_point_select.lua
  $ . $SBTEST_INCDIR/script_oltp_common.sh
  sysbench *.* * (glob)
  
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  Creating a secondary index on 'sbtest1'...
  Creating table 'sbtest2'...
  Inserting 10000 records into 'sbtest2'
  Creating a secondary index on 'sbtest2'...
  Creating table 'sbtest3'...
  Inserting 10000 records into 'sbtest3'
  Creating a secondary index on 'sbtest3'...
  Creating table 'sbtest4'...
  Inserting 10000 records into 'sbtest4'
  Creating a secondary index on 'sbtest4'...
  Creating table 'sbtest5'...
  Inserting 10000 records into 'sbtest5'
  Creating a secondary index on 'sbtest5'...
  Creating table 'sbtest6'...
  Inserting 10000 records into 'sbtest6'
  Creating a secondary index on 'sbtest6'...
  Creating table 'sbtest7'...
  Inserting 10000 records into 'sbtest7'
  Creating a secondary index on 'sbtest7'...
  Creating table 'sbtest8'...
  Inserting 10000 records into 'sbtest8'
  Creating a secondary index on 'sbtest8'...
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
  
                                                   Table "public.sbtest2"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest2_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  Indexes:
      "sbtest2_pkey" PRIMARY KEY, btree (id)
      "k_2" btree (k)
  
                                                   Table "public.sbtest3"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest3_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  Indexes:
      "sbtest3_pkey" PRIMARY KEY, btree (id)
      "k_3" btree (k)
  
                                                   Table "public.sbtest4"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest4_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  Indexes:
      "sbtest4_pkey" PRIMARY KEY, btree (id)
      "k_4" btree (k)
  
                                                   Table "public.sbtest5"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest5_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  Indexes:
      "sbtest5_pkey" PRIMARY KEY, btree (id)
      "k_5" btree (k)
  
                                                   Table "public.sbtest6"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest6_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  Indexes:
      "sbtest6_pkey" PRIMARY KEY, btree (id)
      "k_6" btree (k)
  
                                                   Table "public.sbtest7"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest7_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  Indexes:
      "sbtest7_pkey" PRIMARY KEY, btree (id)
      "k_7" btree (k)
  
                                                   Table "public.sbtest8"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest8_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  Indexes:
      "sbtest8_pkey" PRIMARY KEY, btree (id)
      "k_8" btree (k)
  
  Did not find any relation named "sbtest9".
  sysbench *.* * (glob)
  
  FATAL: *: prewarm is currently MySQL only (glob)
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 2
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
      ignored errors:                      0      (* per sec.) (glob)
      reconnects:                          0      (* per sec.) (glob)
  
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
      events (avg/stddev):           */* (glob)
      execution time (avg/stddev):   */* (glob)
  
  sysbench *.* * (glob)
  
  Dropping table 'sbtest1'...
  Dropping table 'sbtest2'...
  Dropping table 'sbtest3'...
  Dropping table 'sbtest4'...
  Dropping table 'sbtest5'...
  Dropping table 'sbtest6'...
  Dropping table 'sbtest7'...
  Dropping table 'sbtest8'...
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
                                                   Table "public.sbtest1"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest1_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  Indexes:
      "sbtest1_pkey" PRIMARY KEY, btree (id)
  
  sysbench * (glob)
  
  Dropping table 'sbtest1'...
