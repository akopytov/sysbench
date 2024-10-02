#!/bin/bash -x

host="competitiveperf-test.database.windows.net"
port=1433
db="tiny96"
threads=1
runtime=600
options=" --table_size=10000 --tables=10 "

client_id=73347005-410c-4938-8edb-9eb6aa7d372f 

# Parse command-line arguments
while getopts ":r:s:t:h" OPTNAME
do
  case "${OPTNAME}" in
    r)  runtime="${OPTARG}" ;;
    s)  host="${OPTARG}" ;;
    t)  threads="${OPTARG}" ;;
  esac
done
shift $((OPTIND-1))

# Cleanup previous test tables
sysbench ./src/lua/oltp_read_only.lua --db-driver=sqlserver \
  --sqlserver-host=$host \
  --sqlserver-port=$port \
  --sqlserver-user=$client_id \
  --sqlserver-db=$db \
  cleanup

# Prepare the database with the specified number of tables
sysbench ./src/lua/oltp_read_only.lua --db-driver=sqlserver \
  --sqlserver-host=$host \
  --sqlserver-port=$port \
  --sqlserver-user=$client_id \
  --sqlserver-db=$db \
  --threads=$threads \
  --tables=10 \
  prepare

# Run sysbench with Managed Identity authentication
sysbench ./src/lua/oltp_read_only.lua --db-driver=sqlserver \
  --thread-stack-size=256 \
  --sqlserver-host=$host \
  --sqlserver-port=$port \
  --sqlserver-user=$client_id \
  --threads=$threads \
  --time=$runtime \
  --sqlserver-db=$db \
  --report-interval=10 \
  --histogram=off $options run

# Cleanup after testing
sysbench ./src/lua/oltp_read_only.lua --db-driver=sqlserver \
  --sqlserver-host=$host \
  --sqlserver-port=$port \
  --sqlserver-user=$client_id \
  --sqlserver-db=$db \
  cleanup
