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
  SB_CNT_OTHER,
  SB_CNT_READ,
  SB_CNT_WRITE,
  SB_CNT_TRX,
  SB_CNT_ERROR,
  SB_CNT_RECONNECT,
  SB_CNT_MAX
} sb_counter_type;

typedef struct
{
  sql_error_t     error;             /* Driver-independent error code */
  int             sql_errno;         /* Driver-specific error code */
  const char      *sql_state;        /* Database-specific SQL state */
  const char      *sql_errmsg;       /* Database-specific error message */
  sql_driver      *driver;           /* DB driver for this connection */

  const char      opaque[?];
} sql_connection;

typedef struct
{
  sql_connection  *connection;

  const char      opaque[?];
} sql_statement;

/* Result set definition */

typedef struct
{
  sb_counter_type   counter;     /* Statistical counter type */
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
int db_connection_reconnect(sql_connection *con);
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

-- Initialize a given SQL driver and return a handle to it to create
-- connections. A nil driver name (i.e. no function argument) initializes the
-- default driver, i.e. the one specified with --db-driver on the command line.
function sysbench.sql.driver(driver_name)
   local drv = ffi.C.db_create(driver_name)
   if (drv == nil) then
      error("failed to initialize the DB driver", 2)
   end
   return ffi.gc(drv, ffi.C.db_destroy)
end

-- sql_driver methods
local driver_methods = {}

function driver_methods.connect(self)
   local con = ffi.C.db_connection_create(self)
   if con == nil then
      error("connection creation failed", 2)
   end
   return ffi.gc(con, ffi.C.db_connection_free)
end

function driver_methods.name(self)
   return ffi.string(self.sname)
end

-- sql_driver metatable
local driver_mt = {
   __index = driver_methods,
   __gc = ffi.C.db_destroy,
   __tostring = function() return '<sql_driver>' end,
}
ffi.metatype("sql_driver", driver_mt)

-- sql_connection methods
local connection_methods = {}

function connection_methods.disconnect(self)
   return assert(ffi.C.db_connection_close(self) == 0)
end

function connection_methods.reconnect(self)
   return assert(ffi.C.db_connection_reconnect(self) == 0)
end

function connection_methods.check_error(self, rs, query)
   if rs ~= nil or self.error == sysbench.sql.error.NONE then
      return rs
   end

   if self.sql_state == nil or self.sql_errmsg == nil then
      -- It must be an API error, don't bother trying to downgrade it an
      -- ignorable error
      error("SQL API error", 3)
   end

   local sql_state = ffi.string(self.sql_state)
   local sql_errmsg = ffi.string(self.sql_errmsg)

   -- Create an error descriptor containing connection, failed query, SQL error
   -- number, state and error message provided by the SQL driver
   errdesc = {
      connection = self,
      query = query,
      sql_errno = self.sql_errno,
      sql_state = sql_state,
      sql_errmsg = sql_errmsg
   }

   -- Check if the error has already been marked as ignorable by the driver, or
   -- there is an error hook that allows downgrading it to IGNORABLE
   if (self.error == sysbench.sql.error.FATAL and
          type(sysbench.hooks.sql_error_ignorable) == "function" and
          sysbench.hooks.sql_error_ignorable(errdesc)) or
      self.error == sysbench.sql.error.IGNORABLE
   then
      -- Throw a 'restart event' exception that can be caught by the user script
      -- to do some extra steps to restart a transaction (e.g. reprepare
      -- statements after a reconnect). Otherwise it will be caught by
      -- thread_run() in sysbench.lua, in which case the entire current event
      -- will be restarted without extra processing.
      errdesc.errcode = sysbench.error.RESTART_EVENT
      error(errdesc, 3)
   end

   -- Just throw a regular error message on a fatal error
   error(string.format("SQL error, errno = %d, state = '%s': %s",
                       self.sql_errno, sql_state, sql_errmsg), 2)
end

function connection_methods.query(self, query)
   local rs = ffi.C.db_query(self, query, #query)
   return self:check_error(rs, query)
end

function connection_methods.bulk_insert_init(self, query)
   return ffi.C.db_bulk_insert_init(self, query, #query)
end

function connection_methods.bulk_insert_next(self, val)
   return ffi.C.db_bulk_insert_next(self, val, #val)
end

function connection_methods.bulk_insert_done(self)
   return ffi.C.db_bulk_insert_done(self)
end

function connection_methods.prepare(self, query)
   local stmt = ffi.C.db_prepare(self, query, #query)
   if stmt == nil then
      self:check_error(nil, query)
   end
   return stmt
end

-- A convenience wrapper around sql_connection:query() and
-- sql_result:fetch_row(). Executes the specified query and returns the first
-- row from the result set, if available, or nil otherwise
function connection_methods.query_row(self, query)
   local rs = self:query(query)

   if rs == nil then
      return nil
   end

   return unpack(rs:fetch_row(), 1, rs.nfields)
end

-- sql_connection metatable
local connection_mt = {
   __index = connection_methods,
   __tostring = function() return '<sql_connection>' end,
   __gc = ffi.C.db_connection_free,
}
ffi.metatype("sql_connection", connection_mt)

-- sql_param
local sql_param = {}
function sql_param.set(self, value)
   local sql_type = sysbench.sql.type
   local btype = self.type

   if (value == nil) then
      self.is_null[0] = true
      return
   end

   self.is_null[0] = false

   if btype == sql_type.TINYINT or
      btype == sql_type.SMALLINT or
      btype == sql_type.INT or
      btype == sql_type.BIGINT
   then
      self.buffer[0] = value
   elseif btype == sql_type.FLOAT or
      btype == sql_type.DOUBLE
   then
      self.buffer[1] = value
   elseif btype == sql_type.CHAR or
      btype == sql_type.VARCHAR
   then
      local len = #value
      len = self.max_len < len and self.max_len or len
      ffi.copy(self.buffer, value, len)
      self.data_len[0] = len
   else
      error("Unsupported argument type: " .. btype, 2)
   end
end

function sql_param.set_rand_str(self, fmt)
   local sql_type = sysbench.sql.type
   local btype = self.type

   self.is_null[0] = false

   if btype == sql_type.CHAR or
      btype == sql_type.VARCHAR
   then
      local len = #fmt
      len = self.max_len < len and self.max_len or len
      ffi.C.sb_rand_str(fmt, self.buffer)
      self.data_len[0] = len
   else
      error("Unsupported argument type: " .. btype, 2)
   end
end

sql_param.__index = sql_param
sql_param.__tostring = function () return '<sql_param>' end

-- sql_statement methods
local statement_methods = {}

function statement_methods.bind_create(self, btype, max_len)
   local sql_type = sysbench.sql.type

   local param = setmetatable({}, sql_param)

   if btype == sql_type.TINYINT or
      btype == sql_type.SMALLINT or
      btype == sql_type.INT or
      btype == sql_type.BIGINT
   then
      param.type = sql_type.BIGINT
      param.buffer = ffi.new('int64_t[1]')
      param.max_len = 8
   elseif btype == sql_type.FLOAT or
      btype == sql_type.DOUBLE
   then
      param.type = sql_type.DOUBLE
      param.buffer = ffi.new('double[1]')
      param.max_len = 8
   elseif btype == sql_type.CHAR or
      btype == sql_type.VARCHAR
   then
      param.type = sql_type.VARCHAR
      param.buffer = ffi.new('char[?]', max_len)
      param.max_len = max_len
   else
      error("Unsupported argument type: " .. btype, 2)
   end

   param.data_len = ffi.new('unsigned long[1]')
   param.is_null = ffi.new('char[1]')

   return param
end

function statement_methods.bind_param(self, ...)
   local len = select('#', ...)
   if len  < 1 then return nil end

   local binds = ffi.new("sql_bind[?]", len)

   local i, param

   for i, param in ipairs({...}) do
      binds[i-1].type = param.type
      binds[i-1].buffer = param.buffer
      binds[i-1].data_len = param.data_len
      binds[i-1].max_len = param.max_len
      binds[i-1].is_null = param.is_null
   end
   return ffi.C.db_bind_param(self, binds, len)
end

function statement_methods.execute(self)
   local rs = ffi.C.db_execute(self)
   return self.connection:check_error(rs, '<prepared statement>')
end

function statement_methods.close(self)
   return ffi.C.db_close(self)
end

-- sql_statement metatable
local statement_mt = {
   __index = statement_methods,
   __tostring = function() return '<sql_statement>' end,
   __gc = ffi.C.db_close,
}
ffi.metatype("sql_statement", statement_mt)

local bind_mt = {
   __tostring = function() return '<sql_bind>' end,
}
ffi.metatype("sql_bind", bind_mt)

-- sql_result methods
local result_methods = {}

-- Returns the next row of values from a result set, or nil if there are no more
-- rows to fetch. Values are returned as an array, i.e. a table with numeric
-- indexes starting from 1. The total number of values (i.e. fields in a result
-- set) can be obtained from sql_result.nfields.
function result_methods.fetch_row(self)
   local res = {}
   local row = ffi.C.db_fetch_row(self)

   if row == nil then
      return nil
   end

   local i
   for i = 0, self.nfields-1 do
      if row.values[i].ptr ~= nil then -- not a NULL value
         res[i+1] = ffi.string(row.values[i].ptr, tonumber(row.values[i].len))
      end
   end

   return res
end

function result_methods.free(self)
   return assert(ffi.C.db_free_results(self) == 0, "db_free_results() failed")
end

-- sql_results metatable
local result_mt = {
   __index = result_methods,
   __tostring = function() return '<sql_result>' end,
   __gc = ffi.C.db_free_results
}
ffi.metatype("sql_result", result_mt)

-- error codes
sysbench.sql.error = {}
sysbench.sql.error.NONE = ffi.C.DB_ERROR_NONE
sysbench.sql.error.IGNORABLE = ffi.C.DB_ERROR_IGNORABLE
sysbench.sql.error.FATAL = ffi.C.DB_ERROR_FATAL
