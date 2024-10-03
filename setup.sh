#!/bin/bash -x

export HOST='competitiveperf-test.database.windows.net'
# Authentication type - ms_entra/sql
# for ms_entra - Populate OBJECTID and CLIENTID
# for sql - Populate SQLUSER and SQLSECRET
export AUTH=ms_entra
if [[ ($AUTH == "ms_entra") ]]; then
  export OBJECTID=73347005-410c-4938-8edb-9eb6aa7d372f # This is from compperf_managed_identity
  export CLIENTID=2e72ae2f-1187-453f-9d3b-6e87357479e4 # This is from compperf_managed_identity
  export SQLUSER="ms_entra"  # Use a placeholder for user
  export SQLPASSWORD=73347005-410c-4938-8edb-9eb6aa7d372f # Use a placeholder for password
else
  export SQLUSER='cloudsa'
  export SQLSECRET='1_SQLPerf'
fi
