#!/usr/bin/python

'''
Create and insert data in parallel into single table and partitioned table.
Then run tests on these two types of tables. Draw the performance curves.

This requires the data has already been prepared in databases
sbtest_`table_size` and sbtest_`table_size`_part.
'''

import subprocess
import sys
import matplotlib
# matplotlib.use('Agg')
import matplotlib.pyplot as plt

# Time 600 Report-interval 10
template = '''sysbench --mysql-ignore-errors=1062,1213 --time={} \
--report-interval=5 --auto_inc=false --table_size={} \
--rand-type=uniform --partitioned={} --partition_num=100 --threads=64 \
--without_data=true --create_secondary=false \
--mysql-user=test --mysql-host=127.0.0.1 --mysql-port=3306 \
--mysql-password=123456 --mysql-db={} {} {}'''

prepare_insert = '''sysbench --time=1 --report-interval=10 --table_size={} \
--rand-type=uniform --partitioned={} --partition_num=100 --threads=10 \
--auto_inc=false --without_data=true --create_secondary=false \
--mysql-user=test --mysql-host=127.0.0.1 --mysql-port=3306 \
--mysql-password=123456 --mysql-db={} oltp_prepare_insert.lua {}'''

actions = ["cleanup", "prepare", "run"]
test_sets = ["oltp_point_select.lua", "oltp_read_only.lua",
             "oltp_read_write.lua", "oltp_write_only.lua",
             "oltp_update_index.lua", "oltp_update_non_index.lua"]
table_sizes = [1000000, 10000000, 100000000, 500000000, 1000000000, 2000000000]

qps = {}
part_qps = {}
warm_up_time = 100
run_time = 80

for table_size in table_sizes:
    for partitioned in ["false", "true"]:
        if partitioned == "false":
            qps_ref = qps
        else:
            qps_ref = part_qps

        db_name = "sbtest_" + str(table_size) + \
            ("" if partitioned == "false" else "_part")
        for action in actions:
            if action == "prepare" or action == "cleanup":
                command = prepare_insert.format(table_size, partitioned,
                                                db_name, action)
                print(command)
                popen = subprocess.Popen(command, stdout=subprocess.PIPE,
                                         stderr=subprocess.PIPE, shell=True)
                while popen.poll() is None:
                    r = popen.stdout.readline()
                    print(r.strip('\n'))

                # Two stage prepare. The second stage is to bulk insert data in
                # multithreads.
                if action == "prepare":
                    command = prepare_insert.format(table_size, partitioned,
                                                    db_name, "run")
                    print(command)
                    popen = subprocess.Popen(command, stdout=subprocess.PIPE,
                                             stderr=subprocess.PIPE, shell=True)
                    while popen.poll() is None:
                        r = popen.stdout.readline()
                        print(r.strip('\n'))
            else:
                for test_set in test_sets:
                    if not qps_ref.has_key(test_set):
                        qps_ref[test_set] = []

                    # Warm up for a while to make results stable. Fill buffer
                    # pool.
                    warm_up = template.format(warm_up_time, table_size,
                                              partitioned, db_name, test_set,
                                              action)
                    print("Warm up: " + warm_up)
                    popen = subprocess.Popen(warm_up, stdout=subprocess.PIPE,
                                             stderr=subprocess.PIPE, shell=True)
                    while popen.poll() is None:
                        r = popen.stdout.readline()
                        print(r.strip('\n'))

                    command = template.format(rum_time, table_size, partitioned,
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

sys.exit()

# Paint curve part
plt.figure(figsize=(6,10))

plt.subplot(2, 1, 1)
plt.xscale("log")
plt.plot(table_sizes, qps["oltp_point_select.lua"], 's-', color='b',
         ms=3, label='oltp_point_select')
plt.plot(table_sizes, part_summary["oltp_point_select.lua"], 'o--', color='b',
         ms=3, label='partitioned oltp_point_select')
plt.plot(table_sizes, qps["oltp_read_only.lua"], 's-', color='g',
         ms=3, label='oltp_read_only')
plt.plot(table_sizes, part_summary["oltp_read_only.lua"], 'o--', color='g',
         ms=3, label='partitioned oltp_read_only')
plt.plot(table_sizes, qps["oltp_read_write.lua"], 's-', color='r',
         ms=3, label='oltp_read_write')
plt.plot(table_sizes, part_summary["oltp_read_write.lua"], 'o--', color='r',
         ms=3, label='partitioned oltp_read_write')
plt.ylabel("qps")
plt.legend(bbox_to_anchor=(1.05, 0), loc=3, borderaxespad=0)

plt.subplot(2, 1, 2)
plt.xscale("log")
plt.plot(table_sizes, qps["oltp_write_only.lua"], 's-', color='y', ms=3,
         label='oltp_write_only')
plt.plot(table_sizes, part_summary["oltp_write_only.lua"], 'o--', color='y',
         ms=3, label='partitioned oltp_write_only')
plt.plot(table_sizes, qps["oltp_update_index.lua"], 's-',
         color='c', ms=3, label='oltp_update_index')
plt.plot(table_sizes, part_summary["oltp_update_index.lua"], 'o--', color='c',
         ms=3, label='partitioned oltp_update_index')
plt.plot(table_sizes, qps["oltp_update_non_index.lua"], 's-',
         color='violet', ms=3, label='oltp_update_non_index')
plt.plot(table_sizes, part_qps["oltp_update_non_index.lua"], 'o--',
         color='violet', ms=3, label='partitioned oltp_update_non_index')
plt.xlabel("table size")
plt.ylabel("qps")

plt.legend(bbox_to_anchor=(1.05, 0), loc=3, borderaxespad=0)
plt.savefig('/tmp/test.png', dpi=600, bbox_inches='tight')
