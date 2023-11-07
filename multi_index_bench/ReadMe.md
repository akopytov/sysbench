## 使用方法
把该文件夹的两个文件放在执行sysbench命令的当前目录，sysbench会自动识别替换安装内容。

## 主要修改内容：
1、在数据表中添加索引数据项，用于测试带有索引的操作内容。

2、select_random_ranges 可以测试多个库，同时完成随机选择数据库和索引。

## 添加参数：
```
index_num =
      {"create table index numbers", 0},
table_with_index =
      {"if index_nums > 0 create table with index", false},
db_prefix =
      {"manual create database prefix, this will disable mysql-db param", "sysbench"},
db_num =
      {"manual create database number, this will disable mysql-db param", 20}
```

## 用法：
1、设置 index_num 为需要创建索引项的个数。table_with_index 表示是否在创建表是包含索引。

2、select_random_ranges 需要使用的多个数据库需要通过 1，手动完成创建。（当前默认数据名是 sysbench1, sysbench2, sysbench3 ... ）


