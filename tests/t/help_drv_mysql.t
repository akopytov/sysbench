Skip test if the MySQL driver is not available.

  $ if [ -z "$SBTEST_HAS_MYSQL" ]
  > then
  >   exit 80
  > fi

  $ sysbench --help | sed -n '/mysql options:/,/^$/p'
  mysql options:
    --mysql-host=[LIST,...]          MySQL server host [localhost]
    --mysql-port=[LIST,...]          MySQL server port [3306]
    --mysql-socket=[LIST,...]        MySQL socket
    --mysql-user=STRING              MySQL user [sbtest]
    --mysql-password=STRING          MySQL password []
    --mysql-db=STRING                MySQL database name [sbtest]
    --mysql-ssl[=on|off]             use SSL connections, if available in the client library [off]
    --mysql-ssl-cipher=STRING        use specific cipher for SSL connections []
    --mysql-compression[=on|off]     use compression, if available in the client library [off]
    --mysql-debug[=on|off]           trace all client library calls [off]
    --mysql-ignore-errors=[LIST,...] list of errors to ignore, or "all" [1213,1020,1205]
    --mysql-dry-run[=on|off]         Dry run, pretend that all MySQL client API calls are successful without executing them [off]
  
