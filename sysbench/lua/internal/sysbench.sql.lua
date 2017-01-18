-- Copyright (C) 2017 Alexey Kopytov <akopytov@gmail.com>

-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.

-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.

-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

-- ----------------------------------------------------------------------
-- SQL API
-- ----------------------------------------------------------------------

ffi = require("ffi")

sysbench.sql = {}

ffi.cdef[[
/*
  The following definitions have been copied with modifications from db_driver.h
*/

typedef enum
{
  DB_ERROR_NONE,                /* no error(s) */
  DB_ERROR_IGNORABLE,           /* error should be ignored as defined by command
                                line arguments or a custom error handler */
  DB_ERROR_FATAL                /* non-ignorable error */
} sql_error_t;

typedef struct
{
  const char      *sname;    /* short name */
  const char      *lname;    /* long name */

  const char      opaque[?];
} sql_driver;

typedef struct {
  uint32_t        len;         /* Value length */
  const char      *ptr;        /* Value string */
} sql_value;

/* Result set row definition */

typedef struct
{
  void            *ptr;        /* Driver-specific row data */
  sql_value       *values;     /* Array of column values */
} sql_row;

/* Query type for statistics */

typedef enum
{
  DB_STAT_OTHER,
  DB_STAT_READ,
  DB_STAT_WRITE,
  DB_STAT_TRX,
  DB_STAT_ERROR,
  DB_STAT_RECONNECT,
  DB_STAT_MAX
} sql_stat_type;

typedef struct db_conn sql_connection;
typedef struct db_stmt sql_statement;

/* Result set definition */

typedef struct
{
  sql_stat_type  stat_type;     /* Statistical counter type */
  uint32_t       nrows;         /* Number of affected rows */
  uint32_t       nfields;       /* Number of fields */
  sql_statement  *statement;    /* Pointer to prepared statement (if used) */
  void           *ptr;          /* Pointer to driver-specific data */
  sql_row        row;           /* Last fetched row */
} sql_result;

typedef enum
{
  SQL_TYPE_NONE,
  SQL_TYPE_TINYINT,
  SQL_TYPE_SMALLINT,
  SQL_TYPE_INT,
  SQL_TYPE_BIGINT,
  SQL_TYPE_FLOAT,
  SQL_TYPE_DOUBLE,
  SQL_TYPE_TIME,
  SQL_TYPE_DATE,
  SQL_TYPE_DATETIME,
  SQL_TYPE_TIMESTAMP,
  SQL_TYPE_CHAR,
  SQL_TYPE_VARCHAR
} sql_bind_type_t;

typedef struct
{
  sql_bind_type_t   type;
  void             *buffer;
  unsigned long    *data_len;
  unsigned long    max_len;
  char             *is_null;
} sql_bind;

sql_driver *db_create(const char *);
int db_destroy(sql_driver *drv);

sql_connection *db_connection_create(sql_driver * drv);
int db_connection_close(sql_connection *con);
void db_connection_free(sql_connection *con);

int db_bulk_insert_init(sql_connection *, const char *, size_t);
int db_bulk_insert_next(sql_connection *, const char *, size_t);
void db_bulk_insert_done(sql_connection *);

sql_result *db_query(sql_connection *con, const char *query, size_t len);

sql_row *db_fetch_row(sql_result *rs);

sql_statement *db_prepare(sql_connection *con, const char *query, size_t len);
int db_bind_param(sql_statement *stmt, sql_bind *params, size_t len);
int db_bind_result(sql_statement *stmt, sql_bind *results, size_t len);
sql_result *db_execute(sql_statement *stmt);
int db_close(sql_statement *stmt);

int db_free_results(sql_result *);
]]

local sql_driver = ffi.typeof('sql_driver *')
local sql_connection = ffi.typeof('sql_connection *')
local sql_statement = ffi.typeof('sql_statement *')
local sql_bind = ffi.typeof('sql_bind');
local sql_result = ffi.typeof('sql_result');
local sql_value = ffi.typeof('sql_value');
local sql_row = ffi.typeof('sql_row');

sysbench.sql.type =
   {
      NONE = ffi.C.SQL_TYPE_NONE,
      TINYINT = ffi.C.SQL_TYPE_TINYINT,
      SMALLINT = ffi.C.SQL_TYPE_SMALLINT,
      INT = ffi.C.SQL_TYPE_INT,
      BIGINT = ffi.C.SQL_TYPE_BIGINT,
      FLOAT = ffi.C.SQL_TYPE_FLOAT,
      DOUBLE = ffi.C.SQL_TYPE_DOUBLE,
      TIME = ffi.C.SQL_TYPE_TIME,
      DATE = ffi.C.SQL_TYPE_DATE,
      DATETIME = ffi.C.SQL_TYPE_DATETIME,
      TIMESTAMP = ffi.C.SQL_TYPE_TIMESTAMP,
      CHAR = ffi.C.SQL_TYPE_CHAR,
      VARCHAR = ffi.C.SQL_TYPE_VARCHAR
   }

local function check_type(vtype, var, func)
   if var == nil or not ffi.istype(vtype, var) then
      error(string.format("bad argument '%s' to %s() where a '%s' was expected",
                          var, func, vtype),
            3)
   end
end

-- Initialize a given SQL driver and return a handle to it to create
-- connections. A nil driver name (i.e. no function argument) initializes the
-- default driver, i.e. the one specified with --db-driver on the command line.
function sysbench.sql.driver(driver_name)
   local drv = ffi.C.db_create(driver_name)
   if (drv == nil) then
      error("failed to initialize the DB driver")
   end
   return ffi.gc(drv, ffi.C.db_destroy)
end

function sysbench.sql.connect(driver)
   check_type(sql_driver, driver, 'sysbench.sql.connect')
   local con = ffi.C.db_connection_create(driver)
   if con == nil then
      error('connection creation failed')
   end
   return ffi.gc(con, ffi.C.db_connection_free)
end

function sysbench.sql.disconnect(con)
   check_type(sql_connection, con, 'sysbench.sql.disconnect')
   return ffi.C.db_connection_close(con) == 0
end

function sysbench.sql.query(con, query)
   check_type(sql_connection, con, 'sysbench.sql.query')
   return ffi.C.db_query(con, query, #query)
end

function sysbench.sql.bulk_insert_init(con, query)
   check_type(sql_connection, con, 'sysbench.sql.bulk_insert_init')
   return ffi.C.db_bulk_insert_init(con, query, #query)
end

function sysbench.sql.bulk_insert_next(con, val)
   check_type(sql_connection, con, 'sysbench.sql.bulk_insert_next')
   return ffi.C.db_bulk_insert_next(con, val, #val)
end

function sysbench.sql.bulk_insert_done(con)
   check_type(sql_connection, con, 'sysbench.sql.bulk_insert_done')
   return ffi.C.db_bulk_insert_done(con)
end

function sysbench.sql.prepare(con, query)
   check_type(sql_connection, con, 'sysbench.sql.prepare')
   local stmt = ffi.C.db_prepare(con, query, #query)
   return stmt
end

function sysbench.sql.bind_create(stmt, btype, maxlen)
   local sql_type = sysbench.sql.type
   local buf, buflen, datalen, isnull

   check_type(sql_statement, stmt, 'sysbench.sql.bind_create')

   if btype == sql_type.TINYINT or
      btype == sql_type.SMALLINT or
      btype == sql_type.INT or
      btype == sql_type.BIGINT
   then
      btype = sql_type.BIGINT
      buf = ffi.new('int64_t[1]')
      buflen = 8
   elseif btype == sql_type.FLOAT or
      btype == sql_type.DOUBLE
   then
      btype = sql_type.DOUBLE
      buf = ffi.new('double[1]')
      buflen = 8
   elseif btype == sql_type.CHAR or
      btype == sql_type.VARCHAR
   then
      btype = sql_type.VARCHAR
      buf = ffi.new('char[?]', maxlen)
      buflen = maxlen
   else
      error("Unsupported argument type: " .. btype)
   end

   datalen = ffi.new('unsigned long[1]')
   isnull = ffi.new('char[1]')

   return ffi.new(sql_bind, btype, buf, datalen, buflen, isnull)
end

function sysbench.sql.bind_set(bind, value)
   local sql_type = sysbench.sql.type
   local btype = bind.type

   check_type(sql_bind, bind, 'sysbench.sql.bind_set')

   if (value == nil) then
      bind.is_null[0] = true
      return
   end

   bind.is_null[0] = false

   if btype == sql_type.TINYINT or
      btype == sql_type.SMALLINT or
      btype == sql_type.INT or
      btype == sql_type.BIGINT
   then
      ffi.copy(bind.buffer, ffi.new('int64_t[1]', value), 8)
   elseif btype == sql_type.FLOAT or
      btype == sql_type.DOUBLE
   then
      ffi.copy(bind.buffer, ffi.new('double[1]', value), 8)
   elseif btype == sql_type.CHAR or
      btype == sql_type.VARCHAR
   then
      local len = #value
      len = bind.max_len < len and bind.max_len or len
      ffi.copy(bind.buffer, value, len)
      bind.data_len[0] = len
   else
      error("Unsupported argument type: " .. btype)
   end
end

function sysbench.sql.bind_destroy(bind)
   check_type(sql_bind, bind, 'sysbench.sql.bind_destroy')
end

function sysbench.sql.bind_param(stmt, ...)
   local len = #{...}
   local i

   check_type(sql_statement, stmt, 'sysbench.sql.bind_param')

   return ffi.C.db_bind_param(stmt,
                              ffi.new("sql_bind[?]", len, {...}),
                              len)
end

function sysbench.sql.execute(stmt)
   check_type(sql_statement, stmt, 'sysbench.sql.execute')
   return ffi.C.db_execute(stmt)
end

function sysbench.sql.close(stmt)
   check_type(sql_statement, stmt, 'sysbench.sql.close')
   return ffi.C.db_close(stmt)
end

-- Returns the next row of values from a result set, or nil if there are no more
-- rows to fetch. Values are returned as an array, i.e. a table with numeric
-- indexes starting from 1. The total number of values (i.e. fields in a result
-- set) can be obtained from sql_result.nfields.
function sysbench.sql.fetch_row(rs)
   check_type(sql_result, rs, 'sysbench.sql.fetch_row')

   local res = {}
   local row = ffi.C.db_fetch_row(rs)

   if row == nil then
      return nil
   end

   local i
   for i = 0, rs.nfields-1 do
      if row.values[i].ptr ~= nil then -- not a NULL value
         res[i+1] = ffi.string(row.values[i].ptr, tonumber(row.values[i].len))
      end
   end

   return res
end

function sysbench.sql.query_row(con, query)
   check_type(sql_connection, con, 'sysbench.sql.query_row')

   local rs = con:query(query)
   if rs == nil then
      return nil
   end

   return unpack(rs:fetch_row(), 1, rs.nfields)
end

function sysbench.sql.free_results(result)
   check_type(sql_result, result, 'sysbench.sql.free_results')
   return ffi.C.db_free_results(result)
end

function sysbench.sql.driver_name(driver)
   return ffi.string(driver.sname)
end

-- sql_driver metatable
local driver_mt = {
   __index = {
      connect = sysbench.sql.connect,
      name = sysbench.sql.driver_name,
   },
   __gc = ffi.C.db_destroy,
   __tostring = function() return '<sql_driver>' end,
}
ffi.metatype("sql_driver", driver_mt)

-- sql_connection metatable
local connection_mt = {
   __index = {
      disconnect = sysbench.sql.disconnect,
      query = sysbench.sql.query,
      query_row = sysbench.sql.query_row,
      bulk_insert_init = sysbench.sql.bulk_insert_init,
      bulk_insert_next = sysbench.sql.bulk_insert_next,
      bulk_insert_done = sysbench.sql.bulk_insert_done,
      prepare = sysbench.sql.prepare,
   },
   __tostring = function() return '<sql_connection>' end,
   __gc = ffi.C.db_connection_free,
}
ffi.metatype("struct db_conn", connection_mt)

-- sql_statement metatable
local statement_mt = {
   __index = {
      bind_param = sysbench.sql.bind_param,
      bind_create = sysbench.sql.bind_create,
      execute = sysbench.sql.execute,
      close = sysbench.sql.close
   },
   __tostring = function() return '<sql_statement>' end,
   __gc = sysbench.sql.close,
}
ffi.metatype("struct db_stmt", statement_mt)

-- sql_bind metatable
local bind_mt = {
   __index = {
      set = sysbench.sql.bind_set
   },
   __tostring = function() return '<sql_bind>' end,
   __gc = sysbench.sql.bind_destroy
}
ffi.metatype("sql_bind", bind_mt)

-- sql_results metatable
local result_mt = {
   __index = {
      fetch_row = sysbench.sql.fetch_row,
      free = sysbench.sql.free_results,
   },
   __tostring = function() return '<sql_result>' end,
   __gc = sysbench.sql.free_results
}
ffi.metatype("sql_result", result_mt)


-- error codes
sysbench.sql.ERROR_NONE = ffi.C.DB_ERROR_NONE
sysbench.sql.ERROR_IGNORABLE = ffi.C.DB_ERROR_IGNORABLE
sysbench.sql.ERROR_FATAL = ffi.C.DB_ERROR_FATAL
