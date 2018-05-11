########################################################################
OLTP usage information test
########################################################################

  $ sysbench $SBTEST_SCRIPTDIR/oltp_read_write.lua help
  sysbench * (glob)
  
  oltp_read_write.lua options:
    --auto_inc[=on|off]           Use AUTO_INCREMENT column as Primary Key (for MySQL), or its alternatives in other DBMS. When disabled, use client-generated IDs [on]
    --create_secondary[=on|off]   Create a secondary index in addition to the PRIMARY KEY [on]
    --delete_inserts=N            Number of DELETE/INSERT combinations per transaction [1]
    --distinct_ranges=N           Number of SELECT DISTINCT queries per transaction [1]
    --index_updates=N             Number of UPDATE index queries per transaction [1]
    --mysql_storage_engine=STRING Storage engine, if MySQL is used [innodb]
    --non_index_updates=N         Number of UPDATE non-index queries per transaction [1]
    --order_ranges=N              Number of SELECT ORDER BY queries per transaction [1]
    --pgsql_variant=STRING        Use this PostgreSQL variant when running with the PostgreSQL driver. The only currently supported variant is 'redshift'. When enabled, create_secondary is automatically disabled, and delete_inserts is set to 0
    --point_selects=N             Number of point SELECT queries per transaction [10]
    --range_selects[=on|off]      Enable/disable all range SELECT queries [on]
    --range_size=N                Range size for range SELECT queries [100]
    --secondary[=on|off]          Use a secondary index in place of the PRIMARY KEY [off]
    --simple_ranges=N             Number of simple range SELECT queries per transaction [1]
    --skip_trx[=on|off]           Don't start explicit transactions and execute all queries in the AUTOCOMMIT mode [off]
    --sum_ranges=N                Number of SELECT SUM() queries per transaction [1]
    --table_size=N                Number of rows per table [10000]
    --tables=N                    Number of tables [1]
  
