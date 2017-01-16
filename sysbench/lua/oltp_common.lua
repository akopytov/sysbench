-- Copyright (C) 2006-2017 Alexey Kopytov <akopytov@gmail.com>

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
-- Common code for OLTP benchmarks
-- ----------------------------------------------------------------------

-- Generate strings of random digits with 11-digit groups separated by dashes
local function get_c_value()
   -- 10 groups, 119 characters
   return sysbench.rand.string("###########-###########-###########-" ..
                               "###########-###########-###########-" ..
                               "###########-###########-###########-" ..
                               "###########")
end

local function get_pad_value()
   -- 5 groups, 59 characters
   return sysbench.rand.string("###########-###########-###########-" ..
                               "###########-###########")
end

local function create_table(drv, con, table_num)
   local id_index_def, id_def
   local engine_def = ""
   local extra_table_options = ""
   local query

   if oltp_secondary then
     id_index_def = "KEY xid"
   else
     id_index_def = "PRIMARY KEY"
   end

   if drv:name() == "mysql" or drv:name() == "attachsql" or
      drv:name() == "drizzle"
   then
      if oltp_auto_inc then
         id_def = "INTEGER NOT NULL AUTO_INCREMENT"
      else
         id_def = "INTEGER NOT NULL"
      end
      engine_def = "/*! ENGINE = " .. mysql_table_engine .. " */"
      extra_table_options = mysql_table_options or ""
   elseif drv:name() == "pgsql"
   then
      if not oltp_auto_inc then
         id_def = "INTEGER NOT NULL"
      elseif pgsql_variant == 'redshift' then
        id_def = "INTEGER IDENTITY(1,1)"
      else
        id_def = "SERIAL"
      end
   else
      error("Unsupported database driver:" .. drv:name())
   end

   print(string.format("Creating table 'sbtest%d'...", table_num))

   query = string.format([[
CREATE TABLE sbtest%d(
  id %s,
  k INTEGER DEFAULT '0' NOT NULL,
  c CHAR(120) DEFAULT '' NOT NULL,
  pad CHAR(60) DEFAULT '' NOT NULL,
  %s (id)
) %s %s]],
      table_num, id_def, id_index_def, engine_def, extra_table_options)

   con:query(query)

   print(string.format("Inserting %d records into 'sbtest%d'",
                       oltp_table_size, table_num))

   if oltp_auto_inc then
      query = "INSERT INTO sbtest" .. table_num .. "(k, c, pad) VALUES"
   else
      query = "INSERT INTO sbtest" .. table_num .. "(id, k, c, pad) VALUES"
   end

   con:bulk_insert_init(query)

   local c_val
   local pad_val
   local i

   for i = 1,oltp_table_size do

      c_val = get_c_value()
      pad_val = get_pad_value()

      if (oltp_auto_inc) then
         query = string.format("(%d, '%s', '%s')",
                               sb_rand(1, oltp_table_size), c_val, pad_val)
      else
         query = string.format("(%d, %d, '%s', '%s')",
                               i, sb_rand(1, oltp_table_size), c_val, pad_val)
      end

      con:bulk_insert_next(query)
   end

   con:bulk_insert_done()

   if oltp_create_secondary then
      print(string.format("Creating secondary indexes on 'sbtest%d'...",
                          table_num))
      con:query(string.format("CREATE INDEX k_%d ON sbtest%d(k)",
                              table_num, table_num))
   end
end

function thread_init()
   set_vars()

   drv = sysbench.sql.driver()
   con = drv:connect()
end

function prepare()
   set_vars()

   local drv = sysbench.sql.driver()
   local con = drv:connect()
   local i

   for i = 1,oltp_tables_count do
     create_table(drv, con, i)
   end
end

function cleanup()
   set_vars()

   local drv = sysbench.sql.driver()
   local con = drv:connect()
   local i

   for i = 1,oltp_tables_count do
      print(string.format("Dropping table 'sbtest%d'...", i))
      con:query("DROP TABLE IF EXISTS sbtest" .. i )
   end
end

local function get_range_str()
   local start = sb_rand(1, oltp_table_size)
   return string.format("WHERE id BETWEEN %u AND %u",
                        start, start + oltp_range_size - 1)
end

function execute_point_selects(con, table_name)
   for i=1, oltp_point_selects do
      con:query(string.format("SELECT c FROM %s WHERE id=%u",
                              table_name, sb_rand(1, oltp_table_size)))
   end
end

function execute_simple_ranges(con, table_name)
   for i=1, oltp_simple_ranges do
      con:query(string.format("SELECT c FROM %s %s",
                              table_name, get_range_str()))
   end
end

function execute_sum_ranges(con, table_name)
   for i=1, oltp_sum_ranges do
      con:query(string.format("SELECT SUM(k) FROM %s %s",
                              table_name, get_range_str()))
   end
end

function execute_order_ranges(con, table_name)
   for i=1, oltp_order_ranges do
      con:query(string.format("SELECT c FROM %s %s ORDER BY c",
                              table_name, get_range_str()))
   end
end

function execute_distinct_ranges(con, table_name)
   for i=1, oltp_distinct_ranges do
      con:query(string.format("SELECT DISTINCT c FROM %s %s ORDER BY c",
                              table_name, get_range_str()))
   end
end

function execute_index_updates(con, table_name)
   for i=1, oltp_index_updates do
      con:query(string.format("UPDATE %s SET k=k+1 WHERE id=%u",
                              table_name, sb_rand(1, oltp_table_size)))
   end
end

function execute_non_index_updates(con, table_name)
   for i=1, oltp_non_index_updates do
      con:query(string.format("UPDATE %s SET c='%s' WHERE id=%u",
                              table_name, get_c_value(),
                              sb_rand(1, oltp_table_size)))
   end
end

function execute_delete_inserts(con, table_name)
   for i=1, oltp_delete_inserts do
      local id = sb_rand(1, oltp_table_size)
      local k = sb_rand(1, oltp_table_size)

      con:query(string.format("DELETE FROM %s WHERE id=%u", table_name, id))
      con:query(string.format("INSERT INTO %s (id, k, c, pad) VALUES " ..
                                 "(%d, %d, '%s', '%s')",
                              table_name, id, k,
                              get_c_value(), get_pad_value()))
   end
end

function set_vars()
   oltp_table_size = tonumber(oltp_table_size) or 10000
   oltp_range_size = tonumber(oltp_range_size) or 100
   oltp_tables_count = tonumber(oltp_tables_count) or 1
   oltp_point_selects = tonumber(oltp_point_selects) or 10
   oltp_simple_ranges = tonumber(oltp_simple_ranges) or 1
   oltp_sum_ranges = tonumber(oltp_sum_ranges) or 1
   oltp_order_ranges = tonumber(oltp_order_ranges) or 1
   oltp_distinct_ranges = tonumber(oltp_distinct_ranges) or 1
   oltp_index_updates = tonumber(oltp_index_updates) or 1
   oltp_non_index_updates = tonumber(oltp_non_index_updates) or 1
   oltp_delete_inserts = tonumber(oltp_delete_inserts) or 1

   if (oltp_range_selects == 'off') then
      oltp_range_selects = false
   else
      oltp_range_selects = true
   end

   if (oltp_auto_inc == 'off') then
      oltp_auto_inc = false
   else
      oltp_auto_inc = true
   end

   if (oltp_skip_trx == 'on') then
      oltp_skip_trx = true
   else
      oltp_skip_trx = false
   end

   if (oltp_create_secondary == 'off') then
      oltp_create_secondary = false
   else
      oltp_create_secondary = true
   end

   if (pgsql_variant == 'redshift') then
      oltp_create_secondary = false
      oltp_delete_inserts = 0
   end

end
