Skip test if the PostgreSQL driver is not available.

  $ if ! sysbench help | sed -n '/Compiled-in database drivers:/,/^$/p' | grep pgsql > /dev/null 2>&1
  > then
  >   exit 80
  > fi

  $ sysbench help | sed -n '/pgsql options:/,/^$/p'
  pgsql options:
    --pgsql-host=STRING     PostgreSQL server host [localhost]
    --pgsql-port=N          PostgreSQL server port [5432]
    --pgsql-user=STRING     PostgreSQL user [sbtest]
    --pgsql-password=STRING PostgreSQL password []
    --pgsql-db=STRING       PostgreSQL database name [sbtest]
  
