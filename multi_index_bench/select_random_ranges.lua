#!/usr/bin/env sysbench
-- This test is designed for testing MariaDB's key_cache_segments for MyISAM,
-- and should work with other storage engines as well.
--
-- For details about key_cache_segments please refer to:
-- http://kb.askmonty.org/v/segmented-key-cache
--

require("oltp_common")

-- Add --number-of-ranges and --delta to the list of standard OLTP options
sysbench.cmdline.options.number_of_ranges =
   {"Number of random BETWEEN ranges per SELECT", 10}
sysbench.cmdline.options.delta =
   {"Size of BETWEEN ranges", 5}

-- Override standard prepare/cleanup OLTP functions, as this benchmark does not
-- support multiple tables
oltp_prepare = prepare
oltp_cleanup = cleanup

function prepare()
   assert(sysbench.opt.tables == 1, "this benchmark does not support " ..
             "--tables > 1")
   oltp_prepare()
end

function cleanup()
   assert(sysbench.opt.tables == 1, "this benchmark does not support " ..
             "--tables > 1")
   oltp_cleanup()
end

function thread_init()
   stmt = {}
   params = {}
   drv = sysbench.sql.driver()
   con = drv:connect()
   for db_id = 1, sysbench.opt.db_num do
      local db_name = string.format("%s%d",sysbench.opt.db_prefix, db_id)
      stmt[db_id] = {}
      params[db_id] = {}
      for kid = 1, sysbench.opt.index_num do
         stmt[db_id][kid] = {}
         local k_name = string.format("k%d",kid)
         local ranges = string.rep(k_name .. " BETWEEN ? AND ? OR ",
                                 sysbench.opt.number_of_ranges - 1) ..
            k_name .. " BETWEEN ? AND ?"

         if (sysbench.opt.secondary_ranges == 0) then
            stmt[db_id][kid] = con:prepare(string.format([[
               SELECT count(k)
               FROM %s.sbtest1
               WHERE %s]], db_name, ranges))
         elseif (sysbench.opt.secondary_ranges == 2) then
            -- MySQL doesn't yet support 'LIMIT & IN/ALL/ANY/SOME subquery,
            -- So we create an extra nested subquery
            stmt[db_id][kid] = con:prepare(string.format([[
               SELECT count(*), sum(length(c)) FROM %s.sbtest1 WHERE id IN
               (SELECT * FROM (SELECT id FROM %s.sbtest1 WHERE %s LIMIT %d) as t)]],
               db_name, db_name, ranges, sysbench.opt.range_size))
         elseif (sysbench.opt.secondary_ranges == 3) then
            -- MySQL does not generate MRR query plan for secondary_ranges == 2,
            -- We add secondary_ranges == 3 as the query for get range_size rows
            -- by MRR, secondary_ranges == 1 likely get more rows than range_size.
            stmt[db_id][kid] = con:prepare(string.format([[
               SELECT length(c)
               FROM %s.sbtest1
               WHERE %s LIMIT %d]], db_name, ranges, sysbench.opt.range_size))
         else
            stmt[db_id][kid] = con:prepare(string.format([[
               SELECT sum(length(c))
               FROM %s.sbtest1
               WHERE %s]], db_name, ranges))
         end
         params[db_id][kid] = {}
         for j = 1, sysbench.opt.number_of_ranges*2 do
            params[db_id][kid][j] = stmt[db_id][kid]:bind_create(sysbench.sql.type.INT)
         end

         stmt[db_id][kid]:bind_param(unpack(params[db_id][kid]))
      end
   end

   rlen = sysbench.opt.table_size / sysbench.opt.threads

   thread_id = sysbench.tid % sysbench.opt.threads
end

function thread_done()
   for db_id = 1, sysbench.opt.db_num do
      for kid = 1, sysbench.opt.index_num do
         stmt[db_id][kid]:close()
      end
   end
   con:disconnect()
end

function event()
   -- To prevent overlapping of our range queries we need to partition the whole
   -- table into 'threads' segments and then make each thread work with its
   -- own segment.
   local lkid = sysbench.rand.default(1, sysbench.opt.index_num)
   local db_id = sysbench.rand.default(1, sysbench.opt.db_num)
   for i = 1, sysbench.opt.number_of_ranges*2, 2 do
      local rmin = rlen * thread_id
      local rmax = rmin + rlen
      local val = sysbench.rand.default(rmin, rmax)
      params[db_id][lkid][i]:set(val)
      params[db_id][lkid][i+1]:set(val + sysbench.opt.delta)
   end

   stmt[db_id][lkid]:execute()

   check_reconnect()
end
