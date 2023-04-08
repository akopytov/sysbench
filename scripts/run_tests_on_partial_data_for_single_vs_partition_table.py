#!/usr/bin/python

'''
Test partial data access performance for single table and partitioned
table. This requires the data has already been prepared in databases
sbtest_`table_size` and sbtest_`table_size`_part.
'''

import subprocess

# Time 600 Report-interval 10
template = '''sysbench --mysql-ignore-errors=1062,1213 --time={} \
--report-interval=5 --auto_inc=false --table_size={} --access_table_ratio={} \
--rand-type=uniform --partitioned={} --partition_num=100 --threads=64 \
--without_data=true --create_secondary=false \
--mysql-user=test --mysql-host=127.0.0.1 --mysql-port=3306 \
--mysql-password=123456 --mysql-db={} {} {}'''

action = "run"
test_sets = ["oltp_point_select.lua", "oltp_read_only.lua",
             "oltp_read_write.lua", "oltp_write_only.lua",
             "oltp_update_index.lua", "oltp_update_non_index.lua"]

table_size = 2000000000
access_ratios = [0.01, 0.05, 0.1, 0.2, 0.4, 0.8, 1]

qps = {}
part_qps = {}
warm_up_time = 100
run_time = 80

for partitioned in ["false", "true"]:
    if partitioned == "false":
        qps_ref = qps
    else:
        qps_ref = part_qps

    db_name = "sbtest_" + str(table_size) + \
        ("" if partitioned == "false" else "_part")
    for test_set in test_sets:
        qps_ref[test_set] = []
        for ratio in access_ratios:
            warm_up = template.format(warm_up_time, table_size, ratio,
                                      partitioned, db_name, test_set, action)

            print("Warm up: " + warm_up)
            popen = subprocess.Popen(warm_up, stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE, shell=True)
            while popen.poll() is None:
                r = popen.stdout.readline()
                print(r.strip('\n'))

            command = template.format(run_time, table_size, ratio, partitioned,
                                      db_name, test_set, action)
            print(command)
            popen = subprocess.Popen(command, stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE, shell=True)
            while popen.poll() is None:
                r = popen.stdout.readline()
                print(r.strip('\n'))
                if r.find("queries:") != -1:
                    begin = r.find("(")
                    end = r.find("per")
                    qps_ref[test_set].append(float(r[begin+1:end-1]))

            print(qps_ref)

print("Single table:")
print(qps)
print("Partitioned table:")
print(part_qps)
