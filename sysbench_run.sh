#!/bin/bash -x

# Usage: sysbench_run.sh [-b <bench>] [-d <dbname>] [-r <runtime>] [-s <host>] [-t <threads>] [-u <user>] [-p <password>] [-h]
# Make sure the database exists and is clean before running.

# Default values
host=${HOST:-"competitiveperf-test.database.windows.net"}
port=${PORT:-1433}
db="sysbench_db"
user=${SQLUSER:-"ms_entra"}  
password=${SQLPASSWORD:-""}   
threads=${THREADS:-1}        # Corrected syntax for default value
runtime=${RUNTIME:-600}       # Corrected syntax for default value
bench=${BENCH:-"oltp_read_only"}
prepare=${PREPARE:-1}         # Default value for prepare
options=" --table_size=10000 --tables=10 "

function print_usage() {
  echo "$0 [-b <bench>] [-d <dbname>] [-r <runtime>] [-s <host>] [-t <threads>] [-u <user>] [-p <password>] [-p <prepare>] [-h]"
  exit 1
}

# Accept command-line parameter values to override default values
while getopts ":b:d:s:t:u:p:r:hp:" OPTNAME; do
  case "${OPTNAME}" in
    b) bench="${OPTARG}" ;;
    d) db="${OPTARG}" ;;
    r) runtime="${OPTARG}" ;;
    s) host="${OPTARG}" ;;
    t) threads="${OPTARG}" ;;
    u) user="${OPTARG}" ;;
    p) password="${OPTARG}" ;;
    h) print_usage ;;
    :) echo "`date` - FAIL: expected \"${OPTARG}\" value not found"
       print_usage ;;
    \?) echo "`date` - FAIL: unknown command-line option \"${OPTARG}\""
        print_usage ;;
  esac
done
shift $((OPTIND-1))

echo "Running workload for $bench on $host:$port using user: $user"

# Cleanup previous test tables
sysbench ./src/lua/oltp_read_only.lua --db-driver=sqlserver \
  --sqlserver-host=$host \
  --sqlserver-port=$port \
  --sqlserver-user=$user \
  --sqlserver-password=$password \
  --sqlserver-db=$db \
  --tables=10 \
  cleanup

# Prepare the database with the specified number of tables
sysbench ./src/lua/oltp_read_only.lua --db-driver=sqlserver \
  --sqlserver-host=$host \
  --sqlserver-port=$port \
  --sqlserver-user=$user \
  --sqlserver-password=$password \
  --sqlserver-db=$db \
  --threads=$threads \
  --tables=10 \
  prepare

# Run sysbench with Managed Identity authentication
sysbench ./src/lua/oltp_read_only.lua --db-driver=sqlserver \
  --thread-stack-size=256 \
  --sqlserver-host=$host \
  --sqlserver-port=$port \
  --sqlserver-user=$user \
  --sqlserver-password=$password \
  --threads=$threads \
  --time=$runtime \
  --sqlserver-db=$db \
  --report-interval=10 \
  --histogram=off $options run

# Cleanup after testing
sysbench ./src/lua/oltp_read_only.lua --db-driver=sqlserver \
  --sqlserver-host=$host \
  --sqlserver-port=$port \
  --sqlserver-user=$user \
  --sqlserver-password=$password \
  --sqlserver-db=$db \
  --tables=10 \
  cleanup
