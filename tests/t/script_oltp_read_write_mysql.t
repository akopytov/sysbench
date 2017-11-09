########################################################################
oltp_read_write.lua + MySQL tests 
########################################################################

  $ . $SBTEST_INCDIR/mysql_common.sh
  $ DB_DRIVER_ARGS="--db-driver=mysql --mysql-storage-engine=myisam $SBTEST_MYSQL_ARGS"
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
  *************************** 1. row ***************************
  sbtest1
  CREATE TABLE `sbtest1` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_1` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest2
  CREATE TABLE `sbtest2` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_2` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest3
  CREATE TABLE `sbtest3` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_3` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest4
  CREATE TABLE `sbtest4` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_4` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest5
  CREATE TABLE `sbtest5` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_5` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest6
  CREATE TABLE `sbtest6` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_6` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest7
  CREATE TABLE `sbtest7` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_7` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest8
  CREATE TABLE `sbtest8` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_8` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest9' doesn't exist
  sysbench * (glob)
  
  Preloading table sbtest1
  Preloading table sbtest2
  Preloading table sbtest3
  Preloading table sbtest4
  Preloading table sbtest5
  Preloading table sbtest6
  Preloading table sbtest7
  Preloading table sbtest8
  sysbench *.* * (glob)
  
  Dropping table 'sbtest1'...
  Dropping table 'sbtest2'...
  Dropping table 'sbtest3'...
  Dropping table 'sbtest4'...
  Dropping table 'sbtest5'...
  Dropping table 'sbtest6'...
  Dropping table 'sbtest7'...
  Dropping table 'sbtest8'...
  *************************** 1. row ***************************
  sbtest1
  CREATE TABLE `sbtest1` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_1` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest2
  CREATE TABLE `sbtest2` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_2` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest3
  CREATE TABLE `sbtest3` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_3` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest4
  CREATE TABLE `sbtest4` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_4` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest5
  CREATE TABLE `sbtest5` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_5` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest6
  CREATE TABLE `sbtest6` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_6` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest7
  CREATE TABLE `sbtest7` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_7` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest8
  CREATE TABLE `sbtest8` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_8` (`k`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest9' doesn't exist
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
  
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest1' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest2' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest3' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest4' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest5' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest6' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest7' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest8' doesn't exist
  # Test --create-secondary=off
  sysbench * (glob)
  
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  *************************** 1. row ***************************
  sbtest1
  CREATE TABLE `sbtest1` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`)
  ) ENGINE=MyISAM AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  sysbench * (glob)
  
  Dropping table 'sbtest1'...
  # Test --auto-inc=off
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  Creating a secondary index on 'sbtest1'...
  Dropping table 'sbtest1'...


  $ DB_DRIVER_ARGS="--db-driver=mysql --mysql-storage-engine=innodb $SBTEST_MYSQL_ARGS"
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
  *************************** 1. row ***************************
  sbtest1
  CREATE TABLE `sbtest1` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_1` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest2
  CREATE TABLE `sbtest2` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_2` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest3
  CREATE TABLE `sbtest3` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_3` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest4
  CREATE TABLE `sbtest4` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_4` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest5
  CREATE TABLE `sbtest5` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_5` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest6
  CREATE TABLE `sbtest6` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_6` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest7
  CREATE TABLE `sbtest7` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_7` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest8
  CREATE TABLE `sbtest8` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_8` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest9' doesn't exist
  sysbench * (glob)
  
  Preloading table sbtest1
  Preloading table sbtest2
  Preloading table sbtest3
  Preloading table sbtest4
  Preloading table sbtest5
  Preloading table sbtest6
  Preloading table sbtest7
  Preloading table sbtest8
  sysbench *.* * (glob)
  
  Dropping table 'sbtest1'...
  Dropping table 'sbtest2'...
  Dropping table 'sbtest3'...
  Dropping table 'sbtest4'...
  Dropping table 'sbtest5'...
  Dropping table 'sbtest6'...
  Dropping table 'sbtest7'...
  Dropping table 'sbtest8'...
  *************************** 1. row ***************************
  sbtest1
  CREATE TABLE `sbtest1` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_1` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest2
  CREATE TABLE `sbtest2` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_2` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest3
  CREATE TABLE `sbtest3` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_3` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest4
  CREATE TABLE `sbtest4` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_4` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest5
  CREATE TABLE `sbtest5` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_5` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest6
  CREATE TABLE `sbtest6` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_6` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest7
  CREATE TABLE `sbtest7` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_7` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  *************************** 1. row ***************************
  sbtest8
  CREATE TABLE `sbtest8` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_8` (`k`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest9' doesn't exist
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
  
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest1' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest2' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest3' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest4' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest5' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest6' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest7' doesn't exist
  ERROR 1146 (42S02) at line 1: Table 'sbtest.sbtest8' doesn't exist
  # Test --create-secondary=off
  sysbench * (glob)
  
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  *************************** 1. row ***************************
  sbtest1
  CREATE TABLE `sbtest1` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `k` int(11) NOT NULL DEFAULT '0',
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`)
  ) ENGINE=InnoDB AUTO_INCREMENT=10001 DEFAULT CHARSET=* (glob)
  sysbench * (glob)
  
  Dropping table 'sbtest1'...
  # Test --auto-inc=off
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  Creating a secondary index on 'sbtest1'...
  Dropping table 'sbtest1'...
