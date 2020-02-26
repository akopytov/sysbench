  $ . $SBTEST_INCDIR/mysql_common.sh
  $ SB_EXTRA_ARGS=${SB_EXTRA_ARGS:-}
  $ ARGS="oltp_read_write ${DB_DRIVER_ARGS} --verbosity=1 ${SB_EXTRA_ARGS}"

  $ sysbench $ARGS --create-table-options="COMMENT='foo'" prepare
  Creating table 'sbtest1'...
  Inserting 10000 records into 'sbtest1'
  Creating a secondary index on 'sbtest1'...

  $ db_show_table sbtest1
  *************************** 1. row ***************************
  sbtest1
  CREATE TABLE `sbtest1` (
    `id` int* NOT NULL AUTO_INCREMENT, (glob)
    `k` int* NOT NULL DEFAULT '0', (glob)
    `c` char(120)* NOT NULL DEFAULT '', (glob)
    `pad` char(60)* NOT NULL DEFAULT '', (glob)
    PRIMARY KEY (`id`),
    KEY `k_1` (`k`)
  ) ENGINE=InnoDB * COMMENT='foo' (glob)

  $ sysbench $ARGS cleanup
  Dropping table 'sbtest1'...
