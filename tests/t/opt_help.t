########################################################################
Skip everything between "Compiled-in database drivers:" and
"Compiled-in tests:" as that part depends on available database
drivers and thus, build options. Driver-specific options are tested
separately.
########################################################################

  $ sysbench --help | sed '/Compiled-in database drivers:/,/Compiled-in tests:/d' 
  Usage:
    sysbench [options]... [testname] [command]
  
  Commands implemented by most tests: prepare run cleanup help
  
  General options:
    --threads=N                     number of threads to use [1]
    --events=N                      limit for total number of events [0]
    --time=N                        limit for total execution time in seconds [10]
    --forced-shutdown=STRING        number of seconds to wait after the --time limit before forcing shutdown, or 'off' to disable [off]
    --thread-stack-size=SIZE        size of stack per thread [64K]
    --rate=N                        average transactions rate. 0 for unlimited rate [0]
    --report-interval=N             periodically report intermediate statistics with a specified interval in seconds. 0 disables intermediate reports [0]
    --report-checkpoints=[LIST,...] dump full statistics and reset all counters at specified points in time. The argument is a list of comma-separated values representing the amount of time in seconds elapsed from start of test when report checkpoint(s) must be performed. Report checkpoints are off by default. []
    --debug[=on|off]                print more debugging info [off]
    --validate[=on|off]             perform validation checks where possible [off]
    --help[=on|off]                 print help and exit [off]
    --version[=on|off]              print version and exit [off]
    --config-file=FILENAME          File containing command line options
    --tx-rate=N                     deprecated alias for --rate [0]
    --max-requests=N                deprecated alias for --events [0]
    --max-time=N                    deprecated alias for --time [0]
    --num-threads=N                 deprecated alias for --threads [1]
  
  Pseudo-Random Numbers Generator options:
    --rand-type=STRING random numbers distribution {uniform,gaussian,special,pareto} [special]
    --rand-spec-iter=N number of iterations used for numbers generation [12]
    --rand-spec-pct=N  percentage of values to be treated as 'special' (for special distribution) [1]
    --rand-spec-res=N  percentage of 'special' values to use (for special distribution) [75]
    --rand-seed=N      seed for random number generator. When 0, the current time is used as a RNG seed. [0]
    --rand-pareto-h=N  parameter h for pareto distribution [0.2]
  
  Log options:
    --verbosity=N verbosity level {5 - debug, 0 - only critical messages} [3]
  
    --percentile=N       percentile to calculate in latency statistics (1-100). Use the special value of 0 to disable percentile calculations [95]
    --histogram[=on|off] print latency histogram in report [off]
  
  General database options:
  
    --db-driver=STRING  specifies database driver to use \('help' to get list of available drivers\)( \[mysql\])? (re)
    --db-ps-mode=STRING prepared statements usage mode {auto, disable} [auto]
    --db-debug[=on|off] print database-specific debug information [off]
  
  
    fileio - File I/O test
    cpu - CPU performance test
    memory - Memory functions speed test
    threads - Threads subsystem performance test
    mutex - Mutex performance test
  
  See 'sysbench <testname> help' for a list of options for each test.
  
########################################################################
Test driver-specific options
########################################################################
  $ drivers=$(sysbench --help | sed -n '/Compiled-in database drivers:/,/^$/p' | tail -n +2 | cut -d ' ' -f 3)
  $ for drv in $drivers
  > do
  >   if [ ! -r ${SBTEST_SUITEDIR}/help_drv_${drv}.t ]
  >   then
  >     echo "Cannot find test(s) for $drv driver options!"
  >     exit 1
  >   fi
  > done
