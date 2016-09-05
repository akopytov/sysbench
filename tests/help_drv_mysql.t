Skip test if the MySQL driver is not available.

  $ if ! sysbench help | sed -n '/Compiled-in database drivers:/,/^$/p' | grep mysql > /dev/null 2>&1
  > then
  >   exit 80
  > fi

  $ sysbench help | sed -n '/mysql options:/,/^$/p'
  mysql options:
    --mysql-host=[LIST,...]      MySQL server host [localhost]
    --mysql-port=N               MySQL server port [3306]
    --mysql-socket=[LIST,...]    MySQL socket
    --mysql-user=STRING          MySQL user [sbtest]
    --mysql-password=STRING      MySQL password []
    --mysql-db=STRING            MySQL database name [sbtest]
    --mysql-table-engine=STRING  storage engine to use for the test table {myisam,innodb,bdb,heap,ndbcluster,federated} [innodb]
    --mysql-engine-trx=STRING    whether storage engine used is transactional or not {yes,no,auto} [auto]
    --mysql-ssl=[on|off]         use SSL connections, if available in the client library [off]
    --mysql-compression=[on|off] use compression, if available in the client library [off]
    --myisam-max-rows=N          max-rows parameter for MyISAM tables [1000000]
    --mysql-debug=[on|off]       dump all client library calls [off]
    --mysql-ignore-errors=[LIST,...]list of errors to ignore, or "all" [1213,1020,1205]
    --mysql-dry-run=[on|off]     Dry run, pretent that all MySQL client API calls are successful without executing them [off]
  
