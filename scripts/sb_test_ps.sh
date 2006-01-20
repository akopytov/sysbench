#!/bin/bash
# File: sb_test_ps.sh
#
# Copyright (C) 2004 Alexey Kopytov & MySQL AB

NUM_THREADS=4


progname="$0"
curdir=`dirname "$progname"`
sbrun="$curdir/sb_run.sh"

clean_vars()
{
    point_selects=0
    simple_ranges=0
    sum_ranges=0
    order_ranges=0
    distinct_ranges=0
    index_updates=0
    non_index_updates=0
}

run_test()
{
    tname="$1"
    sbargs="--num-threads=$NUM_THREADS --max-requests=0 --max-time=60 \
--test=oltp --oltp-table-size=100000 --mysql-table-type=myisam \
--oltp-point-selects=$point_selects \
--oltp-simple-ranges=$simple_ranges
--oltp-sum-ranges=$sum_ranges
--oltp-order-ranges=$order_ranges
--oltp-distinct-ranges=$distinct_ranges
--oltp-index-updates=$index_updates
--oltp-non-index-updates=$non_index_updates"

    echo -e "\n*** Running test for $tname (client-side PS) ***\n"
    sysbench $sbargs prepare >/dev/null
    sb_run.sh -c "$tname (client-side PS)" $sbargs --db-ps-mode=disable run
    sysbench $sbargs cleanup >/dev/null

    echo -e "\n*** Running test for $tname (server-side PS)... ***\n"
    sysbench $sbargs prepare >/dev/null
    sb_run.sh -c "$tname (server-side PS)" $sbargs run
    sysbench $sbargs cleanup >/dev/null
}

collect_results()
{
    for dir in output/*; do
	trans=`grep "transactions:" $dir/sysbench.out | sed -e 's/.*(\([0-9.]\+\).*)/\1/g'`
	comment=`grep "comment:" $dir/desc.txt | sed -e 's/comment: //'`
	echo "$comment: $trans"
    done
}


# Run point selects test
clean_vars
point_selects=1
run_test "point_selects"

# Run simple ranges test
clean_vars
simple_ranges=1
run_test "simple_ranges"

# Run sum ranges test
clean_vars
sum_ranges=1
run_test "sum_ranges"

# Run order order ranges test
clean_vars
order_ranges=1
run_test "order_ranges"

# Run distinct ranges test
clean_vars
distinct_ranges=1
run_test "distinct_ranges"

# Run index updates test
clean_vars
index_updates=1
run_test "index_updates"

# Run non-index updates test
clean_vars
non_index_updates=1
run_test "non_index_updates"

# Run complex test
clean_vars
point_selects=10
simple_ranges=1
sum_ranges=1
order_ranges=1
distinct_ranges=1
index_updates=1
non_index_updates=1
run_test "complex"

# Collect the results
collect_results | sort
