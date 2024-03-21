#!/usr/bin/env sysbench
-- This test is designed for testing MariaDB's key_cache_segments for MyISAM,
-- and should work with other storage engines as well.
--
-- For details about key_cache_segments please refer to:
-- http://kb.askmonty.org/v/segmented-key-cache
--

require("oltp_common")

-- Test specific options
sysbench.cmdline.options.random_points =
   {"Number of random points in the IN() clause in generated SELECTs", 100}
sysbench.cmdline.options.hot_points =
   {"If true then use the same N adjacent values for the inlist clause", false}

function thread_init()
   drv = sysbench.sql.driver()
   con = drv:connect()

   stmt = {}
   params = {}

   for t = 1, sysbench.opt.tables do
      stmt[t] = {}
      params[t] = {}
   end

   if sysbench.opt.hot_points then
      hot_key = sysbench.opt.table_size / 2
   end

   rlen = sysbench.opt.table_size / sysbench.opt.threads
   thread_id = sysbench.tid % sysbench.opt.threads

   local points = string.rep("?, ", sysbench.opt.random_points - 1) .. "?"

   for t = 1, sysbench.opt.tables do

      stmt[t] = con:prepare(string.format([[
           SELECT c
             FROM sbtest%d
             WHERE id IN (%s)
           ]], t, points))

      for j = 1, sysbench.opt.random_points do
         params[t][j] = stmt[t]:bind_create(sysbench.sql.type.INT)
      end

      stmt[t]:bind_param(unpack(params[t]))
   end
end

function thread_done()
   for t = 1, sysbench.opt.tables do
      stmt[t]:close()
   end
   con:disconnect()
end

function event()
   local tnum = sysbench.rand.uniform(1, sysbench.opt.tables)

   if sysbench.opt.hot_points then
      for i = 1, sysbench.opt.random_points do
         params[tnum][i]:set(hot_key+i)
      end
   else
      for i = 1, sysbench.opt.random_points do
         params[tnum][i]:set(sysbench.rand.default(1, sysbench.opt.table_size))
      end
   end

   stmt[tnum]:execute()
   
   check_reconnect()
end