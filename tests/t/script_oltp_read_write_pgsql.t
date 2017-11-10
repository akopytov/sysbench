########################################################################
oltp_read_write.lua + PostgreSQL tests 
########################################################################

  $ . $SBTEST_INCDIR/pgsql_common.sh
  $ OLTP_SCRIPT_PATH=${SBTEST_SCRIPTDIR}/oltp_read_write.lua
# Override --threads to run read/write tests with a single thread for
# deterministic results
  $ SB_EXTRA_ARGS="--threads=1"
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
   CREATE INDEX k_1 ON sbtest1 USING btree (k)
   CREATE UNIQUE INDEX sbtest1_pkey ON sbtest1 USING btree (id)
  
                              Table "public.sbtest2"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest2_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_2 ON sbtest2 USING btree (k)
   CREATE UNIQUE INDEX sbtest2_pkey ON sbtest2 USING btree (id)
  
                              Table "public.sbtest3"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest3_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_3 ON sbtest3 USING btree (k)
   CREATE UNIQUE INDEX sbtest3_pkey ON sbtest3 USING btree (id)
  
                              Table "public.sbtest4"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest4_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_4 ON sbtest4 USING btree (k)
   CREATE UNIQUE INDEX sbtest4_pkey ON sbtest4 USING btree (id)
  
                              Table "public.sbtest5"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest5_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_5 ON sbtest5 USING btree (k)
   CREATE UNIQUE INDEX sbtest5_pkey ON sbtest5 USING btree (id)
  
                              Table "public.sbtest6"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest6_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_6 ON sbtest6 USING btree (k)
   CREATE UNIQUE INDEX sbtest6_pkey ON sbtest6 USING btree (id)
  
                              Table "public.sbtest7"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest7_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_7 ON sbtest7 USING btree (k)
   CREATE UNIQUE INDEX sbtest7_pkey ON sbtest7 USING btree (id)
  
                              Table "public.sbtest8"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest8_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_8 ON sbtest8 USING btree (k)
   CREATE UNIQUE INDEX sbtest8_pkey ON sbtest8 USING btree (id)
  
  Did not find any relation named "sbtest9".
  sysbench *.* * (glob)
  
  FATAL: *: warmup is currently MySQL only (glob)
  sysbench *.* * (glob)
  
  Dropping table 'sbtest1'...
  Dropping table 'sbtest2'...
  Dropping table 'sbtest3'...
  Dropping table 'sbtest4'...
  Dropping table 'sbtest5'...
  Dropping table 'sbtest6'...
  Dropping table 'sbtest7'...
  Dropping table 'sbtest8'...
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
  
                              Table "public.sbtest2"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest2_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_2 ON sbtest2 USING btree (k)
   CREATE UNIQUE INDEX sbtest2_pkey ON sbtest2 USING btree (id)
  
                              Table "public.sbtest3"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest3_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_3 ON sbtest3 USING btree (k)
   CREATE UNIQUE INDEX sbtest3_pkey ON sbtest3 USING btree (id)
  
                              Table "public.sbtest4"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest4_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_4 ON sbtest4 USING btree (k)
   CREATE UNIQUE INDEX sbtest4_pkey ON sbtest4 USING btree (id)
  
                              Table "public.sbtest5"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest5_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_5 ON sbtest5 USING btree (k)
   CREATE UNIQUE INDEX sbtest5_pkey ON sbtest5 USING btree (id)
  
                              Table "public.sbtest6"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest6_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_6 ON sbtest6 USING btree (k)
   CREATE UNIQUE INDEX sbtest6_pkey ON sbtest6 USING btree (id)
  
                              Table "public.sbtest7"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest7_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_7 ON sbtest7 USING btree (k)
   CREATE UNIQUE INDEX sbtest7_pkey ON sbtest7 USING btree (id)
  
                              Table "public.sbtest8"
   Column |      Type      |                      Modifiers                       | Storage  | Stats target | Description 
  --------+----------------+------------------------------------------------------+----------+--------------+-------------
   id     | integer        | not null default nextval('sbtest8_id_seq'::regclass) | plain    |              | 
   k      | integer        | not null default 0                                   | plain    |              | 
   c      | character(120) | not null default ''::bpchar                          | extended |              | 
   pad    | character(60)  | not null default ''::bpchar                          | extended |              | 
  
  Indexes:
   CREATE INDEX k_8 ON sbtest8 USING btree (k)
   CREATE UNIQUE INDEX sbtest8_pkey ON sbtest8 USING btree (id)
  
  Did not find any relation named "sbtest9".
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 1
  Initializing random number generator from current time
  
  
  Initializing worker threads...
  
  Threads started!
  
  SQL statistics:
      queries performed:
          read:                            1400
          write:                           400
          other:                           200
          total:                           2000
      transactions:                        100    (* per sec.) (glob)
      queries:                             2000   (* per sec.) (glob)
      ignored errors:                      0      (* per sec.) (glob)
      reconnects:                          0      (* per sec.) (glob)
  
  Throughput:
      events/s (eps): *.* (glob)
      time elapsed:                        *s (glob)
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
  
  Did not find any relation named "sbtest1".
  Did not find any relation named "sbtest2".
  Did not find any relation named "sbtest3".
  Did not find any relation named "sbtest4".
  Did not find any relation named "sbtest5".
  Did not find any relation named "sbtest6".
  Did not find any relation named "sbtest7".
  Did not find any relation named "sbtest8".
  # Test --create-secondary=off
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
   CREATE UNIQUE INDEX sbtest1_pkey ON sbtest1 USING btree (id)
  
  sysbench * (glob)
  
  Dropping table 'sbtest1'...
  # Test --auto-inc=off
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  Creating a secondary index on 'sbtest1'...
  Dropping table 'sbtest1'...
