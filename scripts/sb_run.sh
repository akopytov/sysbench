#!/bin/bash
# File: sb_run.sh
#
# Copyright (C) 2004 Alexey Kopytov & MySQL AB

sbname=sysbench

#Install signals handlers
trap 'echo "Got SIGINT, exiting..."; \
      killall vmstat; killall iostat; killall mpstat' INT

trap 'echo "Got SIGTERM, exiting..."; \
      killall vmstat; killall iostat; killall mpstat' TERM


# First try to find SysBench in the current dir
found=""
progname="$0"
curdir=`dirname "$progname"`
sbpath="$curdir/$sbname"

# Try to find SysBench in the current source tree
if test -x "$sbpath"; then
  found=1
else
  topsrcdir="$curdir/../"
  sbpath="$topsrcdir/sysbench/$sbname"
fi

# Try to find SysBench in $PATH
if test -z "$found" -a -x "$sbpath"; then
  found=1
else
  sbpath=`which "$sbname" 2>/dev/null`
fi

if test -z "$found" -a -n "$sbpath" -a -x "$sbpath"; then
  found=1
fi

if test -z "$found"; then
  echo "$sbname binary not found, exiting..."
  echo
  exit 1
fi

UPDATES_INTERVAL=5
sbargs=""
while [ $# -gt 0 ]
do
  case "$1" in
    -u)
      UPDATES_INTERVAL=$2
      shift 2
      ;;
    -c)
      COMMENT="$2"
      shift 2
      ;;
    *)
      sbargs="$sbargs $1"
      shift 1
      ;;
  esac
done

# Determine run number for the output directory
RUN_NUMBER=1
if [ -f ".run_number" ]; then
  read RUN_NUMBER < .run_number
fi

if [ $RUN_NUMBER -eq -1 ]; then
        RUN_NUMBER=0
fi

# Create the output directory
OUTPUT_DIR=output/$RUN_NUMBER
mkdir -p $OUTPUT_DIR

# Update the run number
RUN_NUMBER=`expr $RUN_NUMBER + 1`
echo $RUN_NUMBER > .run_number

# Start vmstat
nohup vmstat -n $UPDATES_INTERVAL >$OUTPUT_DIR/vmstat.out 2>&1 &

# Start iostat
nohup iostat $UPDATES_INTERVAL >$OUTPUT_DIR/iostat.out 2>&1 &

# Start mpstat
nohup mpstat $UPDATES_INTERVAL >$OUTPUT_DIR/mpstat.out 2>&1 &

cat  >$OUTPUT_DIR/desc.txt <<EOF
$sbname command line args: $sbpath $sbargs
comment: $COMMENT
EOF


echo "Running $sbpath $sbargs"

$sbpath $sbargs >$OUTPUT_DIR/sysbench.out 2>&1

killall vmstat; killall iostat; killall mpstat
