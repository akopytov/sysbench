export HOST='competitiveperf-test.database.windows.net'
export OBJECTID=73347005-410c-4938-8edb-9eb6aa7d372f
export CLIENTID=2e72ae2f-1187-453f-9d3b-6e87357479e4
export SQLCMD_CMD="sqlcmd -S $HOST --authentication-method=ActiveDirectoryManagedIdentity -U $CLIENTID"
alias c2s="$SQLCMD_CMD"
