pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function thread_init(thread_id)
   set_vars()
end

function event(thread_id)
   local table_name
   local i
   local c_val
   local k_val
   local pad_val

   table_name = string.format("sbtest%d", sb_rand_uniform(1, oltp_tables_count))

   k_val = sb_rand(1, oltp_table_size)
   c_val = sb_rand_str([[
###########-###########-###########-###########-###########-###########-###########-###########-###########-###########]])
   pad_val = sb_rand_str([[
###########-###########-###########-###########-###########]])

   if (db_driver == "pgsql" and oltp_auto_inc) then
      rs = db_query("INSERT INTO " .. table_name .. " (k, c, pad) VALUES " ..
                       string.format("(%d, '%s', '%s')", k_val, c_val, pad_val))
   else
      if (oltp_auto_inc) then
         i = 0
      else
         i = sb_rand_uniq(1, oltp_table_size)
      end
      rs = db_query("INSERT INTO " .. table_name ..
                       " (id, k, c, pad) VALUES " ..
                       string.format("(%d, %d, '%s', '%s')", i, k_val, c_val,
                                     pad_val))
   end
end
