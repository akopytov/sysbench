########################################################################
Tests for custom report hooks
########################################################################

# Trigger one intermediate and one cumulative report
  $ SB_ARGS="api_reports.lua --time=3 --report-interval=2 --verbosity=1"

########################################################################
# Default human-readable format via a custom hook
########################################################################

  $ cat >api_reports.lua <<EOF
  > ffi.cdef[[int usleep(unsigned int);]]
  > 
  > function event()
  >   ffi.C.usleep(1000)
  > end
  > 
  > sysbench.hooks.report_intermediate = sysbench.report_default
  > sysbench.hooks.report_cumulative = sysbench.report_default
  > EOF

  $ sysbench $SB_ARGS run
  \[ 2s \] thds: 1 tps: [0-9]*\.[0-9]* qps: 0\.00 \(r\/w\/o: 0\.00\/0\.00\/0\.00\) lat \(ms,95%\): [1-9][0-9]*\.[0-9]* err\/s 0\.00 reconn\/s: 0\.00 (re)
  \[ 3s \] thds: 0 tps: [0-9]*\.[0-9]* qps: 0\.00 \(r\/w\/o: 0\.00\/0\.00\/0\.00\) lat \(ms,95%\): [1-9][0-9]*\.[0-9]* err\/s 0\.00 reconn\/s: 0\.00 (re)

########################################################################
# CSV format via a custom hook
########################################################################

  $ cat >api_reports.lua <<EOF
  > ffi.cdef[[int usleep(unsigned int);]]
  > 
  > function event()
  >   ffi.C.usleep(1000)
  > end
  > 
  > sysbench.hooks.report_intermediate = sysbench.report_csv
  > sysbench.hooks.report_cumulative = sysbench.report_csv
  > EOF

  $ sysbench $SB_ARGS run
  2,1,[0-9]*\.[0-9]*,0\.00,0\.00,0\.00,0\.00,[1-9][0-9]*\.[0-9]*,0\.00,0\.00 (re)
  3,0,[0-9]*\.[0-9]*,0\.00,0\.00,0\.00,0\.00,[1-9][0-9]*\.[0-9]*,0\.00,0\.00 (re)

########################################################################
# JSON format via a custom hook
########################################################################

  $ cat >api_reports.lua <<EOF
  > ffi.cdef[[int usleep(unsigned int);]]
  > 
  > function event()
  >   ffi.C.usleep(1000)
  > end
  > 
  > sysbench.hooks.report_intermediate = sysbench.report_json
  > sysbench.hooks.report_cumulative = sysbench.report_json
  > EOF

  $ sysbench $SB_ARGS run
  {
    "time":    2,
    "threads": 1,
    "tps": *.*, (glob)
    "qps": {
      "total": 0.00,
      "reads": 0.00,
      "writes": 0.00,
      "other": 0.00
    },
    "latency": [1-9][0-9]*\.[0-9]*, (re)
    "errors": 0.00,
    "reconnects": 0.00
  },
  {
    "time":    3,
    "threads": 0,
    "tps": *.*, (glob)
    "qps": {
      "total": 0.00,
      "reads": 0.00,
      "writes": 0.00,
      "other": 0.00
    },
    "latency": [1-9][0-9]*\.[0-9]*, (re)
    "errors": 0.00,
    "reconnects": 0.00
  },
