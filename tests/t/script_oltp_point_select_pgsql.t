########################################################################
oltp_point_select.lua + PostgreSQL tests 
########################################################################

  $ if [ -z "${SBTEST_PGSQL_ARGS:-}" ]
  > then
  >   exit 80
  > fi

  $ DB_DRIVER_ARGS="--db-driver=pgsql $SBTEST_PGSQL_ARGS"

  $ function db_show_table() {
  >  psql -c "\d+ $1" sbtest
  > }

  $ . $SBTEST_INCDIR/script_oltp_point_select_common.sh
  sysbench *.* * (glob)
  
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
  
  Running the test with following options:
  Number of threads: 2
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  Threads started!
  
  OLTP test statistics:
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
      total time taken by event execution: *s (glob)
  
  Latency statistics:
           min:                              *.*ms (glob)
           avg:                              *.*ms (glob)
           max:                              *.*ms (glob)
           approx.  95th percentile:         *.*ms (glob)
  
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
