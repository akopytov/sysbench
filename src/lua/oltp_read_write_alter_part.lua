#!/usr/bin/env sysbench
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
-- Read/Write OLTP benchmark
-- ----------------------------------------------------------------------

require("oltp_common")

sysbench.cmdline.options.ddl_enabled =
   {"Enable/disable concurrent DDL", true}

sysbench.cmdline.options.read_only =
   {"Test SELECT command only", false}

sysbench.cmdline.options.hybrid =
   {"Whether contains hybrid DML requests (explicitly indicate PARTITION or not)", false}

local part_sqls = {
   point_selects =
      "SELECT c FROM sbtest%u PARTITION(%s) WHERE id=%d",
   simple_ranges =
      "SELECT c FROM sbtest%u PARTITION(%s) WHERE id BETWEEN %d AND %d",
   sum_ranges =
      "SELECT SUM(k) FROM sbtest%u PARTITION(%s) WHERE id BETWEEN %d AND %d",
   order_ranges =
      "SELECT c FROM sbtest%u PARTITION(%s) WHERE id BETWEEN %d AND %d ORDER BY c",
   distinct_ranges =
      "SELECT DISTINCT c FROM sbtest%u PARTITION(%s) WHERE id BETWEEN %d AND %d ORDER BY c",
   index_updates =
      "UPDATE sbtest%u PARTITION(%s) SET k=k+1 WHERE id=%d",
   non_index_updates =
      "UPDATE sbtest%u PARTITION(%s) SET c='%s' WHERE id=%d",
   deletes =
      "DELETE FROM sbtest%u PARTITION(%s) WHERE id=%d",
   inserts =
      "INSERT INTO sbtest%u PARTITION(%s) (id, k, c, pad) VALUES (%d, %d, '%s', '%s')"
}

local sqls = {
   point_selects =
      "SELECT c FROM sbtest%u WHERE id=%d",
   simple_ranges =
      "SELECT c FROM sbtest%u WHERE id BETWEEN %d AND %d",
   sum_ranges =
      "SELECT SUM(k) FROM sbtest%u WHERE id BETWEEN %d AND %d",
   order_ranges =
      "SELECT c FROM sbtest%u WHERE id BETWEEN %d AND %d ORDER BY c",
   distinct_ranges =
      "SELECT DISTINCT c FROM sbtest%u WHERE id BETWEEN %d AND %d ORDER BY c",
   index_updates =
      "UPDATE sbtest%u SET k=k+1 WHERE id=%d",
   non_index_updates =
      "UPDATE sbtest%u SET c='%s' WHERE id=%d",
   deletes =
      "DELETE FROM sbtest%u WHERE id=%d",
   inserts =
      "INSERT INTO sbtest%u (id, k, c, pad) VALUES (%d, %d, '%s', '%s')"
}

local alter_part_ddls = {
  drop_part =
     "ALTER TABLE sbtest%u DROP PARTITION %s",
  add_part =
     "ALTER TABLE sbtest%u ADD PARTITION %s"
}

local part_defs = {}
local part_value_index = 1
local part_def_index = 2
local part_status_index = 3

-- Initialize all partitions' definitions and status
local function initialize()
   for i = 1, sysbench.opt.partition_num do
      part_name = string.format("p%d", i - 1)
      boundary_value = math.floor(sysbench.opt.table_size * i / sysbench.opt.partition_num) + 1
      cur_part_def = string.format("PARTITION %s VALUES LESS THAN (%d)", part_name, boundary_value)

      part_defs[part_name] = {boundary_value, cur_part_def, true}
   end
end

-- Get partition name by index
local function get_part_name(i)
   return string.format("p%d", i)
end

function prepare_statements()
   if not sysbench.opt.skip_trx then
      prepare_begin()
      prepare_commit()
   end

   prepare_point_selects()

   if sysbench.opt.range_selects then
      prepare_simple_ranges()
      prepare_sum_ranges()
      prepare_order_ranges()
      prepare_distinct_ranges()
   end

   prepare_index_updates()
   prepare_non_index_updates()
   prepare_delete_inserts()

   initialize()
end

-- Get table definition
local function get_partition_status(show)
   local tnum = get_table_num()
   local rs = con:query(string.format("SHOW CREATE TABLE sbtest%d", tnum))
   local row = rs:fetch_row()

   if (show) then
     print(string.format("%s", row[2]))
   end

   for i = 1, sysbench.opt.partition_num do
      local part_name = get_part_name(i - 1)
      if string.find(row[2], part_name) then
         part_defs[part_name][part_status_index] = true
      else
         part_defs[part_name][part_status_index] = false
      end
   end

   local max_exist_part = -1
   for i = sysbench.opt.partition_num - 1, 0, -1 do
      if part_defs[get_part_name(i)][part_status_index] then
         max_exist_part = i
         break
      end
   end

   sysbench.opt.table_size = part_defs[get_part_name(max_exist_part)][part_value_index] - 1
end

-- Get partition in which the record with given id is
local function get_part_by_id(id)
   for i = 1, sysbench.opt.partition_num do
      local part_name = get_part_name(i - 1)
      if part_defs[part_name][part_status_index] and (id < part_defs[part_name][part_value_index]) then
         return part_name
      end
   end
end

-- Get partitions in which the records with given range are
local function get_parts_by_range(min_id, max_id)
   local parts_name = ""
   local min_part

   for i = 1, sysbench.opt.partition_num do
      local part_name = get_part_name(i - 1)
      if part_defs[part_name][part_status_index] and (min_id < part_defs[part_name][part_value_index]) then
         min_part = i
         break
      end
   end

   for i = min_part, sysbench.opt.partition_num do
      local part_name = get_part_name(i - 1)

      if part_defs[part_name][part_status_index] then
         parts_name = parts_name .. string.format("%s, ", part_name)

         if (max_id < part_defs[part_name][part_value_index]) then
            break
         end
      end
   end

   return string.sub(parts_name, 1, string.len(parts_name) - 2)
end

local function execute_part_point_selects()
   local query_type = "point_selects"
   local tnum = get_table_num()
   local i

   for i = 1, sysbench.opt.point_selects do
      local id = get_id()
      if sysbench.opt.hybrid and sysbench.rand.uniform(0, 1) == 0 then
        con:query(string.format(sqls[query_type], tnum, id))
      else
        con:query(string.format(part_sqls[query_type], tnum, get_part_by_id(id), id))
      end
   end
end

local function execute_part_range(key)
   local tnum = get_table_num()

   for i = 1, sysbench.opt[key] do
      local id = get_id()
      local max_id = id + sysbench.rand.default(1, sysbench.opt.range_size)
      if sysbench.opt.hybrid and sysbench.rand.uniform(0, 1) == 0 then
        con:query(string.format(sqls[key], tnum, id, max_id))
      else
        con:query(string.format(part_sqls[key], tnum, get_parts_by_range(id, max_id), id, max_id))
      end
   end
end

local function execute_part_index_updates()
   local query_type = "index_updates"
   local tnum = get_table_num()

   for i = 1, sysbench.opt.index_updates do
      local id = get_id()
      if sysbench.opt.hybrid and sysbench.rand.uniform(0, 1) == 0 then
        con:query(string.format(sqls[query_type], tnum, id))
      else
        con:query(string.format(part_sqls[query_type], tnum, get_part_by_id(id), id))
      end
   end
end

local function execute_part_non_index_updates()
   local query_type = "non_index_updates"
   local tnum = get_table_num()

   for i = 1, sysbench.opt.non_index_updates do
      local id = get_id()
      if sysbench.opt.hybrid and sysbench.rand.uniform(0, 1) == 0 then
        con:query(string.format(sqls[query_type], tnum, get_c_value(), id))
      else
        con:query(string.format(part_sqls[query_type], tnum, get_part_by_id(id), get_c_value(), id))
      end
   end
end

local function execute_part_delete_inserts()
   local tnum = get_table_num()

   for i = 1, sysbench.opt.delete_inserts do
      local id = get_id()
      local k = get_id()

      if sysbench.opt.hybrid and sysbench.rand.uniform(0, 1) == 0 then
        con:query(string.format(sqls["deletes"], tnum, id))

        con:query(string.format(sqls["inserts"], tnum, id, k, get_c_value(), get_pad_value()))
      else
        con:query(string.format(part_sqls["deletes"], tnum, get_part_by_id(id), id))

        con:query(string.format(part_sqls["inserts"], tnum, get_part_by_id(id), id, k,
                              get_c_value(), get_pad_value()))
      end
   end
end

local function dml_requests()
   execute_part_point_selects()

   if sysbench.opt.range_selects then
      execute_part_range("simple_ranges")
      execute_part_range("sum_ranges")
      execute_part_range("order_ranges")
      execute_part_range("distinct_ranges")
      execute_simple_ranges()
      execute_sum_ranges()
      execute_order_ranges()
      execute_distinct_ranges()
   end

   if not sysbench.opt.read_only then
      execute_part_index_updates()
      execute_part_non_index_updates()
      execute_part_delete_inserts()
      execute_index_updates()
      execute_non_index_updates()
      execute_delete_inserts()
   end
end

function ddl_alter_single_part()
   local tnum = get_table_num()

   local max_exist_part = -1
   for i = sysbench.opt.partition_num - 1, 0, -1 do
      if part_defs[get_part_name(i)][part_status_index] then
         max_exist_part = i
         break
      end
   end

   local part_num = sysbench.rand.uniform(0, sysbench.opt.partition_num - 1)
   local ddl = ""

   -- DROP PARTITION
   if part_num <= max_exist_part then
      local part_name = get_part_name(max_exist_part)

      ddl = string.format(alter_part_ddls["drop_part"], tnum, part_name)
   else
   -- ADD PARTITION
      local alter_def = "("
      alter_def = alter_def .. string.format("%s, ", part_defs[get_part_name(max_exist_part + 1)][part_def_index])
      alter_def = string.sub(alter_def, 1, string.len(alter_def) - 2) .. ")"

      ddl = string.format(alter_part_ddls["add_part"], tnum, alter_def)
   end

   print(ddl)
   con:query(ddl)
   get_partition_status(true)
end

function event()
-- Get TABLE definition
   get_partition_status(false)

   if not sysbench.opt.skip_trx then
      begin()
   end

   local luck = sysbench.rand.uniform(1, 10000)
-- release 10 threads
--   if (luck <= 9999) then
-- release asan
--   if (luck <= 9990) then
-- debug
--   if (luck <= 9900) then
-- release 5 threads
   if (luck <= 9998) then
      dml_requests()
   elseif sysbench.opt.ddl_enabled and not sysbench.opt.read_only then
      ddl_alter_single_part()
   end

   if not sysbench.opt.skip_trx then
      commit()
   end
end
