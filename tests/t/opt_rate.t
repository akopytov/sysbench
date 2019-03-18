########################################################################
Tests for the --rate option (aka the limited rate mode)
########################################################################

# Failing to deliver the requested rate should result in a non-zero exit code
  $ sysbench --rate=2000000000 cpu run --verbosity=1
  FATAL: The event queue is full. This means the worker threads are unable to keep up with the specified event generation rate
  [1]
