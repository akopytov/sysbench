-- Copyright (C) 2016-2018 Alexey Kopytov <akopytov@gmail.com>

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



-- This functions queries information about
-- indexes in innoDB buffer poll and prints it to std out
function show_index_stat(table_name)
   local drv = sysbench.sql.driver()
   local con = drv:connect()

   con:query("SET SESSION sql_mode = sys.list_drop(@@SESSION.sql_mode, 'ONLY_FULL_GROUP_BY');")
   con:query("SET sql_mode = sys.list_drop(@@sql_mode, 'ONLY_FULL_GROUP_BY');")
   con:query("SET @@sql_mode = sys.list_drop(@@sql_mode, 'ONLY_FULL_GROUP_BY');")

   con:query("USE information_schema;")
   con:query("SET @page_size = @@innodb_page_size;;")
   con:query("SET @bp_pages = @@innodb_buffer_pool_size/@page_size;")

   local query =
      [[
      SELECT INDEX_TYPE, PAGES, PCT_OF_BUFFER_POOL, PCT_OF_INDEX
      FROM
      (SELECT P.TABLE_NAME, P.PAGE_TYPE,
         CASE
            WHEN P.INDEX_NAME IS NULL
               THEN NULL
            WHEN P.TABLE_NAME LIKE '`SYS_%'
               THEN P.INDEX_NAME
            WHEN P.INDEX_NAME <> 'PRIMARY'
               THEN 'SECONDARY'
            ELSE
               'PRIMARY'
        END
      AS INDEX_TYPE,

      COUNT(DISTINCT P.PAGE_NUMBER) AS PAGES,
      ROUND(100*COUNT(DISTINCT P.PAGE_NUMBER)/@bp_pages,2)
      AS PCT_OF_BUFFER_POOL,
         CASE
            WHEN P.TABLE_NAME IS NULL
               THEN NULL
            WHEN P.TABLE_NAME LIKE '`SYS_%'
               THEN NULL
            ELSE
               ROUND(100*COUNT(DISTINCT P.PAGE_NUMBER)/
                  CASE P.INDEX_NAME
                     WHEN 'PRIMARY'
                        THEN TS.DATA_LENGTH/@page_size
                     ELSE
                         TS.INDEX_LENGTH/@page_size
                     END,
               2)
         END
      AS PCT_OF_INDEX

      FROM INNODB_BUFFER_PAGE AS P
      JOIN INNODB_SYS_TABLES AS T
         ON P.SPACE = T.SPACE
      JOIN TABLES AS TS ON T.NAME = CONCAT(TS.TABLE_SCHEMA,'/',TS.TABLE_NAME)
         WHERE TS.TABLE_SCHEMA <> 'mysql' and TS.TABLE_NAME = ]] ..
         string.format("'%s'", table_name) ..
      [[

         GROUP BY TABLE_NAME, PAGE_TYPE, INDEX_TYPE) tmp

         WHERE PAGE_TYPE = 'INDEX';
      ]]

   local query_test = [[
      select * from sbtest1 limit 5;
      ]]
   local result = con:query(query)
   if result == nil
   then
      print("Error! Can't query statistic from information schema")
   else
      print("INDEX_TYPE | PAGES | PCT_OF_BUFFER_POOL | PCT_OF_INDEX")
      part_lengths = {11, 7, 20, 13}                  -- lengths of headers
      local primary_index = format_array(result:fetch_row(), part_lengths)
      print(primary_index)

      secondary_index_row = result:fetch_row()
      if secondary_index_row ~= nil
      then
         local secondary_index = format_array(secondary_index_row, part_lengths)
         print(secondary_index)
      end
   end
end


-- This function just make a string from array where each element
-- of array around with spaces to have length from part_lengths
function format_array(arr, part_lengths)
   assert(#arr > 3, "Function format_array requires more then 3 array values")
   local string = ""
   for k, v in pairs(arr)
   do
      local bit = 0
      local val = tostring(v)
      while #val < part_lengths[k]
      do
         if bit == 0
         then
            val = ' ' .. val
         else
            val = val .. ' '
         end
         bit = (bit + 1) % 2
      end
      string = string .. val
      if k ~= 4
      then
         string = string .. "|"
       end
   end
   return string
end