-- -------------------------------------------------------------------------- --
-- Bulk insert tests                                                          --
-- -------------------------------------------------------------------------- --

cursize=0

function prepare()
   local i

   db_connect()

   for i = 0,num_threads-1 do

   if (db_driver == "pgsql") then

      db_query([[
CREATE TABLE IF NOT EXISTS sbtest]] .. i .. [[ (
id INTEGER NOT NULL,
k INTEGER DEFAULT '0' NOT NULL,
PRIMARY KEY (id)
)]])

   else

      db_query([[
CREATE TABLE IF NOT EXISTS sbtest]] .. i .. [[ (
id INTEGER UNSIGNED NOT NULL,
k INTEGER UNSIGNED DEFAULT '0' NOT NULL,
PRIMARY KEY (id)
) ENGINE = InnoDB
]])

   end

   end --for
end

function event()
   local i

   if (cursize == 0) then
      db_bulk_insert_init("INSERT INTO sbtest" .. thread_id .. " VALUES")
   end

   cursize = cursize + 1

   db_bulk_insert_next("(" .. cursize .. "," .. cursize .. ")")
end

function thread_done(thread_9d)
   db_bulk_insert_done()
end

function cleanup()
   local i

   for i = 0,num_threads-1 do
      print("Dropping table 'sbtest" .. i .. "'...")
      db_query("DROP TABLE IF EXISTS sbtest".. i )
   end
end
