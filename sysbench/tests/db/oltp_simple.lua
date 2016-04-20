pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function thread_init(thread_id)
   set_vars()

end

function event(thread_id)
   rs = db_query(string.format("SELECT c FROM sbtest%d WHERE id=%d", sb_rand_uniform(1, oltp_tables_count), sb_rand(1, oltp_table_size)))
end
