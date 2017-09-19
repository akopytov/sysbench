########################################################################
Legacy SQL Lua API tests
########################################################################

  $ if [ -z "${SBTEST_MYSQL_ARGS:-}" ]
  > then
  >   exit 80
  > fi

  $ SB_ARGS="--verbosity=1 --max-requests=1 --db-driver=mysql $SBTEST_MYSQL_ARGS --test=$CRAMTMP/api_legacy_sql.lua"
  $ cat >$CRAMTMP/api_legacy_sql.lua <<EOF
  > function event(thread_id)
  >   db_query("CREATE TABLE t(a INT)")
  > 
  >   db_bulk_insert_init("INSERT INTO t VALUES")
  >   for i = 1,100 do
  >     db_bulk_insert_next(string.format("(%d)", i))
  >   end
  >   db_bulk_insert_done()
  > 
  >   db_connect()
  >   db_query("SELECT 1")
  >   db_disconnect()
  >   db_query("SELECT 1")
  >   db_connect()
  > 
  >   local stmt = db_prepare("UPDATE t SET a = a + ?")
  >   db_bind_param(stmt, {100})
  >   rs = db_execute(stmt)
  >   db_store_results(rs)
  >   db_free_results(rs)
  >   db_close(stmt)
  > 
  >   print("DB_ERROR_NONE = " .. DB_ERROR_NONE)
  >   print("DB_ERROR_RESTART_TRANSACTION = " .. DB_ERROR_RESTART_TRANSACTION)
  >   print("DB_ERROR_FAILED = " .. DB_ERROR_FAILED)
  > end
  > EOF

  $ mysql -uroot sbtest -Nse "DROP TABLE IF EXISTS t"

  $ sysbench $SB_ARGS run
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
  WARNING: --max-requests is deprecated, use --events instead
  DB_ERROR_NONE = [0-9] (re)
  DB_ERROR_RESTART_TRANSACTION = [0-9] (re)
  DB_ERROR_FAILED = [0-9] (re)

  $ mysql -uroot sbtest -Nse "SHOW CREATE TABLE t\G"
  *************************** 1. row ***************************
  t
  CREATE TABLE `t` (
    `a` int(11) DEFAULT NULL
  ) * (glob)

  $ mysql -uroot sbtest -Nse "SELECT COUNT(DISTINCT a) FROM t"
  100

  $ mysql -uroot sbtest -Nse "SELECT MIN(a), MAX(a) FROM t\G"
  *************************** 1. row ***************************
  101
  200

  $ mysql -uroot sbtest -Nse "DROP TABLE t"

  $ function db_show_table() {
  >   mysql -uroot sbtest -Nse "SHOW CREATE TABLE $1\G"
  > }

  $ DB_DRIVER_ARGS="--db-driver=mysql --mysql-table-engine=myisam $SBTEST_MYSQL_ARGS"
  $ . $SBTEST_INCDIR/script_oltp_legacy_common.sh
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
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
  *************************** 1. row ***************************
  sbtest1
  CREATE TABLE `sbtest1` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_1` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  *************************** 1. row ***************************
  sbtest2
  CREATE TABLE `sbtest2` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_2` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  *************************** 1. row ***************************
  sbtest3
  CREATE TABLE `sbtest3` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_3` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  *************************** 1. row ***************************
  sbtest4
  CREATE TABLE `sbtest4` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_4` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  *************************** 1. row ***************************
  sbtest5
  CREATE TABLE `sbtest5` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_5` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  *************************** 1. row ***************************
  sbtest6
  CREATE TABLE `sbtest6` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_6` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  *************************** 1. row ***************************
  sbtest7
  CREATE TABLE `sbtest7` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_7` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  *************************** 1. row ***************************
  sbtest8
  CREATE TABLE `sbtest8` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_8` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest9' doesn't exist
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
  WARNING: --num-threads is deprecated, use --threads instead
  WARNING: --max-requests is deprecated, use --events instead
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 2
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
  
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
  sysbench *.* * (glob)
  
  Dropping table 'sbtest1'...
  Dropping table 'sbtest2'...
  Dropping table 'sbtest3'...
  Dropping table 'sbtest4'...
  Dropping table 'sbtest5'...
  Dropping table 'sbtest6'...
  Dropping table 'sbtest7'...
  Dropping table 'sbtest8'...
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest1' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest2' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest3' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest4' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest5' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest6' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest7' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest8' doesn't exist
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
  sysbench * (glob)
  
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  *************************** 1. row ***************************
  sbtest1
  CREATE TABLE `sbtest1` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
  sysbench * (glob)
  
  Dropping table 'sbtest1'...

  $ DB_DRIVER_ARGS="--db-driver=mysql --mysql-table-engine=innodb $SBTEST_MYSQL_ARGS"
  $ . $SBTEST_INCDIR/script_oltp_legacy_common.sh
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
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
  *************************** 1. row ***************************
  sbtest1
  CREATE TABLE `sbtest1` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_1` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  *************************** 1. row ***************************
  sbtest2
  CREATE TABLE `sbtest2` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_2` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  *************************** 1. row ***************************
  sbtest3
  CREATE TABLE `sbtest3` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_3` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  *************************** 1. row ***************************
  sbtest4
  CREATE TABLE `sbtest4` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_4` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  *************************** 1. row ***************************
  sbtest5
  CREATE TABLE `sbtest5` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_5` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  *************************** 1. row ***************************
  sbtest6
  CREATE TABLE `sbtest6` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_6` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  *************************** 1. row ***************************
  sbtest7
  CREATE TABLE `sbtest7` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_7` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  *************************** 1. row ***************************
  sbtest8
  CREATE TABLE `sbtest8` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_8` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest9' doesn't exist
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
  WARNING: --num-threads is deprecated, use --threads instead
  WARNING: --max-requests is deprecated, use --events instead
  sysbench *.* * (glob)
  
  Running the test with following options:
  Number of threads: 2
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
  
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
  sysbench *.* * (glob)
  
  Dropping table 'sbtest1'...
  Dropping table 'sbtest2'...
  Dropping table 'sbtest3'...
  Dropping table 'sbtest4'...
  Dropping table 'sbtest5'...
  Dropping table 'sbtest6'...
  Dropping table 'sbtest7'...
  Dropping table 'sbtest8'...
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest1' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest2' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest3' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest4' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest5' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest6' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest7' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest8' doesn't exist
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
  sysbench * (glob)
  
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  *************************** 1. row ***************************
  sbtest1
  CREATE TABLE `sbtest1` (
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `k` int(10) unsigned NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* MAX_ROWS=1000000 (glob)
  WARNING: the --test option is deprecated. You can pass a script name or path on the command line without any options.
  sysbench * (glob)
  
  Dropping table 'sbtest1'...

