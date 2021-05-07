########################################################################
# Common code for SQL API tests
#
# Expects the following variables and callback functions to be defined by the
# caller:
#
#   DB_DRIVER_ARGS -- extra driver-specific arguments to pass to sysbench
#
########################################################################

set -eu

SB_ARGS="--verbosity=1 --events=1 $DB_DRIVER_ARGS $CRAMTMP/api_sql.lua"
cat >$CRAMTMP/api_sql.lua <<EOF
function thread_init()
  drv = sysbench.sql.driver()
  con = drv:connect()
end

function event()
  local e, m
  local inspect = require("inspect")

  print("drv:name() = " .. drv:name())
  print("SQL types:")
  print(inspect(sysbench.sql.type))
  print('--')

  print("SQL error codes:")
  print(inspect(sysbench.sql.error))
  print('--')

  e, m = pcall(sysbench.sql.driver, "non-existing")
  print(m)

  con:query("DROP TABLE IF EXISTS t")
  con:query("CREATE TABLE t(a INT)")
  con:bulk_insert_init("INSERT INTO t VALUES")
  for i = 1,100 do
    con:bulk_insert_next(string.format("(%d)", i))
  end
  con:bulk_insert_done()
  print(con:query_row("SELECT COUNT(DISTINCT a) FROM t"))
  print('--')
  con:query("DROP TABLE IF EXISTS t2")
  con:query("CREATE TABLE t2(a INT, b VARCHAR(120), c FLOAT)")
  con:query("INSERT INTO t2 VALUES (1, 'foo', 0.4)")
  con:query("INSERT INTO t2 VALUES (NULL, 'bar', 0.2)")
  con:query("INSERT INTO t2 VALUES (2, NULL, 0.3)")
  con:query("INSERT INTO t2 VALUES (NULL, NULL, 0.1)")

  print('--')
  local rs = con:query("SELECT * FROM t2 ORDER BY a")
  for i = 1, rs.nrows do
     print(string.format("%s %s %s", unpack(rs:fetch_row(), 1, rs.nfields)))
  end
  print('--')

  print(string.format("%s %s", con:query_row("SELECT b, a FROM t2 ORDER BY b")))
  print('--')
  con:query("DROP TABLE t2")
  con:query("ALTER TABLE t ADD COLUMN b CHAR(10), ADD COLUMN c FLOAT")

  e, m = pcall(con.prepare, con, "SELECT * FROM nonexisting")
  print(m)
  print('--')

  local stmt = con:prepare("UPDATE t SET a = a + ?, b = ?, c = ?")
  local a = stmt:bind_create(sysbench.sql.type.INT)
  local b = stmt:bind_create(sysbench.sql.type.CHAR, 10)
  local c = stmt:bind_create(sysbench.sql.type.FLOAT)

  print(a)
  print(b)
  print(c)

  e, m = pcall(stmt.bind_create, stmt, sysbench.sql.type.DATE)
  print(m)

  print(stmt:bind_param())

  stmt:bind_param(a, b, c)
  a:set(100)
  rs1 = stmt:execute()
  print(rs1)
  a:set(200)
  b:set("01234567890")
  rs2 = stmt:execute()
  print(rs2)
  a:set(300)
  b:set("09876543210")
  c:set(0.9)
  rs3 = stmt:execute()
  print(rs3)

  e, m = pcall(rs3.free, rs)
  print(m)

  e, m = pcall(rs2.free, rs)
  print(m)

  e, m = pcall(rs1.free, rs)
  print(m)

  rs = con:query("SELECT a, b, c FROM t")
  print(unpack(rs:fetch_row(), 1, rs.nfields))

  stmt:close()

  print('--')
  con:disconnect()
  e, m = pcall(con.query, con, "SELECT 1")
  print(m)

  con:disconnect()

  print('--')
  con = drv:connect()
  rs = con:query("SELECT MIN(a), MAX(a), MIN(b), MAX(b) FROM t")
  print(rs.nfields)
  for i = 1, rs.nrows do
     print(string.format("%s %s %s %s", unpack(rs:fetch_row(), 1, rs.nfields)))
  end
  con:query("DROP TABLE t")
  print('--')
end
EOF
sysbench $SB_ARGS run

# Reconnect
cat >$CRAMTMP/api_sql.lua <<EOF
function sysbench.hooks.report_cumulative(stat)
  print("reconnects = " .. stat.reconnects)
end
function thread_init()
  drv = sysbench.sql.driver()
  con = drv:connect()
end
function event()
  print(con:query_row("SELECT 1"))
  con:reconnect()
  print(con:query_row("SELECT 2"))
  print('--')
end
EOF
sysbench $SB_ARGS run

# Failed connection handling

cat >$CRAMTMP/api_sql.lua <<EOF
function event()
  local drv = sysbench.sql.driver()
  local e,m = pcall(drv.connect, drv)
  print(m)
end
EOF
# Reset --mysql-socket if it's specified on the command line, otherwise sysbench
# will assume --mysql-host=localhost
sysbench $SB_ARGS $DB_NON_EXISTING run

# Error hooks
cat >$CRAMTMP/api_sql.lua <<EOF
function sysbench.hooks.sql_error_ignorable(e)
  print("Got an error descriptor:")
  local inspect = require("inspect")
  print(inspect(e))
end

function event()
  local drv = sysbench.sql.driver()
  local con = drv:connect()
  local queries = {
    "CREATE TABLE t(a CHAR(1) NOT NULL)",
    "INSERT INTO t VALUES (1)",
    "INSERT INTO t VALUES (NULL)",
    [[INSERT INTO t VALUES ('test')]],
    "DROP TABLE t",
    "DROP TABLE t"
  }
  print('--')
  con:query("DROP TABLE IF EXISTS t")
  if (drv:name() == 'mysql') then
    con:query("SET sql_mode='STRICT_TRANS_TABLES'")
  end
  for i = 1, #queries do
    local e, m = pcall(function () con:query(queries[i]) end)
    if not e then print(m) end
  end
  print('--')
end
EOF
sysbench $SB_ARGS run


cat <<EOF
########################################################################
# Multiple connections test
########################################################################
EOF
cat >$CRAMTMP/api_sql.lua <<EOF
function thread_init()
  drv = sysbench.sql.driver()
  con = {}
  for i=1,10 do
    con[i] = drv:connect()
  end
end

function event()
  con[1]:query("DROP TABLE IF EXISTS t")
  con[2]:query("CREATE TABLE t(a INT)")
  for i=1,10 do
    con[i]:query("INSERT INTO t VALUES (" .. i .. ")")
  end
  rs = con[1]:query("SELECT * FROM t")
  for i = 1, rs.nrows do
     print(string.format("%s", unpack(rs:fetch_row(), 1, rs.nfields)))
  end
  con[1]:query("DROP TABLE t")
end
EOF

sysbench $SB_ARGS run

cat <<EOF
########################################################################
# Incorrect bulk API usage
########################################################################
EOF
cat >$CRAMTMP/api_sql.lua <<EOF
c = sysbench.sql.driver():connect()
c:query("CREATE TABLE t1(a INT)")
c:bulk_insert_init("INSERT INTO t1 VALUES")
c:bulk_insert_next("(1)")
c:bulk_insert_done()
e,m = pcall(function () c:bulk_insert_next("(2)") end)
print(m)
c:bulk_insert_done()
c:query("DROP TABLE t1")
EOF

sysbench $SB_ARGS

cat <<EOF
########################################################################
# query_row() with an empty result set
########################################################################
EOF
cat >$CRAMTMP/api_sql.lua <<EOF
c = sysbench.sql.driver():connect()
c:query("CREATE TABLE t1(a INT)")
print(c:query_row("SELECT * FROM t1"))
c:query("DROP TABLE t1")
EOF

sysbench $SB_ARGS

cat <<EOF
########################################################################
# GH-282: Mysql's fetch_row() is broken
########################################################################
EOF
cat >$CRAMTMP/api_sql.lua <<EOF
connection = sysbench.sql.driver():connect()
rows = connection:query("select 1 union select 2")

r = rows:fetch_row();
while ( r ) do
  print( r[ 1 ] )
  r = rows:fetch_row()
end
EOF

sysbench $SB_ARGS
