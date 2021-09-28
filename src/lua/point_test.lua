#!/usr/bin/env sysbench

-- 1. 设置参数默认值
sysbench.cmdline.options = {
    table_size = { "一张表多个条数据，默认值为 1 万", 10000 },
    length = { "分片的长度，默认为2", 2 },
    start = { "开始的编号，默认为1", 1 },
    type = { "运行类型：insert/delete/update/select/all", "all" }
}

function thread_init()
    drv = sysbench.sql.driver()
    con = drv:connect()
end

-- 3. 连接数据库，并且根据配置批量创建数据
function cmd_prepare()
    local drv = sysbench.sql.driver()
    local con = drv:connect()
    -- 批量创建
    for i = sysbench.tid % sysbench.opt.threads + 1, 1, sysbench.opt.threads do
        create_table(drv, con, i)
    end
end

-- 2. 定义 prepare 命令触发的脚本
sysbench.cmdline.commands = {
    -- 执行批量导入数据函数
    prepare = { cmd_prepare, sysbench.cmdline.PARALLEL_COMMAND }
}


-- 4. 创建数据表、数据
function create_table(drv, con, table_num)
    print(string.format("Creating table 'sbtest%d'...", table_num))

    -- 1 编写SQL 建表语句
    local query = string.format(string.format([[
CREATE TABLE sbtest%d(
  id int(10) NOT NULL,
  k int(10) unsigned NOT NULL DEFAULT '0',
  c CHAR(120) DEFAULT '' NOT NULL,
  pad CHAR(60) DEFAULT '' NOT NULL,
  PRIMARY KEY (`id`)
)]], table_num))
    -- 执行SQL
    con:query(query)

    if (sysbench.opt.table_size > 0) then
        print(string.format("Inserting %d records into 'table_%d'", sysbench.opt.table_size, table_num))
    end


    -- 2 创建较大的数据
    local value = "我是大数据"

    -- 3 编写SQL创建数据
    query = "INSERT INTO sbtest" .. table_num .. "(id, k, c, pad) VALUES"
    -- 编写插入语句前半部分
    con:bulk_insert_init(query)

    -- 编写多个VALUES

    for i = sysbench.opt.start, sysbench.opt.table_size do
        local k_val = sysbench.rand.default(1, sysbench.opt.table_size)
        query = string.format("(%d, %d, '%s', '%s')", -i, k_val, value, value)
        con:bulk_insert_next(query)
    end

    -- 编写语句结束
    con:bulk_insert_done()

end


-- 执行 cleanup 命令时触发的函数。(清空数据)
function cleanup()
    local drv = sysbench.sql.driver()
    local con = drv:connect()

    print(string.format("Dropping table 'sbtest%d'...", 1))
    con:query("DROP TABLE IF EXISTS sbtest" .. 1)
end


-- 执行 run 命令 触发的函数(开始压测)，也就是真正开启事务进行压测的函数
function event()
    local table_name = "sbtest1"
    local value = "test"
    local k_val = sysbench.rand.default(1, sysbench.opt.table_size)
    local type = string.upper(sysbench.opt.type)
    local incr = sysbench.rand.sb_global_unique_id();

    if (type == "INSERT") then
        con:query(string.format("INSERT INTO %s (id, k, c, pad) VALUES (%d, %d, '%s', '%s')", table_name, incr, k_val, value, value))
    elseif (type == "DELETE") then
        con:query("DELETE FROM " .. table_name .. " WHERE id=" .. incr)
    elseif (type == "UPDATE") then
        con:query("UPDATE " .. table_name .. " SET k=k+1 WHERE id=" .. incr)
    elseif (type == "SELECT") then
        con:query("SELECT c FROM " .. table_name .. " WHERE id=" .. incr)
    elseif (type == "ALL") then
        con:query(string.format("INSERT INTO %s (id, k, c, pad) VALUES (%d, %d, '%s', '%s')", table_name, incr, k_val, value, value))
        con:query("SELECT c FROM " .. table_name .. " WHERE id=" .. incr)
        con:query("UPDATE " .. table_name .. " SET k=k+1 WHERE id=" .. incr)
        con:query("DELETE FROM " .. table_name .. " WHERE id=" .. incr)
    else
        print("type值只支持：insert/delete/update/select/all")
        os.exit();
    end

end

function thread_done()
    con:disconnect()
end

