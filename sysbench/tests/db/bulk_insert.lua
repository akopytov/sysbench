-- -------------------------------------------------------------------------- --
-- Bulk insert tests                                                          --
-- -------------------------------------------------------------------------- --

function thread_init(thread_id)
   table_size = table_size or 10000
end

function event(thread_id)
   local i
   local table_id

   table_id = thread_id

   db_query("DROP TABLE IF EXISTS sbtest" .. table_id)

   db_query([[
CREATE TABLE IF NOT EXISTS sbtest]] .. table_id .. [[ (
id INTEGER UNSIGNED NOT NULL,
k INTEGER UNSIGNED DEFAULT '0' NOT NULL,
PRIMARY KEY (id)
) ENGINE = InnoDB
]])

      db_bulk_insert_init("INSERT INTO sbtest" .. table_id .. " VALUES")

   for i = 1,table_size do
      db_bulk_insert_next("(" .. i .. "," .. i .. ")")
   end

   db_bulk_insert_done()
end

function cleanup()
   local i

   table_size = table_size or 10000

   for i = 1,num_threads do
      print("Dropping table 'sbtest" .. i .. "'...")
      db_query("DROP TABLE sbtest".. i )
   end
end
