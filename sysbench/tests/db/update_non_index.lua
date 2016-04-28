pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function thread_init(thread_id)
   set_vars()
end

function event(thread_id)
   local c_val
   local query
   c_val = sb_rand_str("###########-###########-###########-###########-###########-###########-###########-###########-###########-###########")
   query = string.format("UPDATE sbtest%d SET c='%s' WHERE id=%d", sb_rand_uniform(1, oltp_tables_count), c_val, sb_rand(1, oltp_table_size))
   rs = db_query(query)
end
