  $ sysbench prepare
  sysbench * (glob)
  
  Cannot find script prepare: No such file or directory
  [1]

  $ cat >cmd_prepare.lua <<EOF
  > function prepare()
  >   print('function prepare()')
  > end
  > EOF
  $ sysbench cmd_prepare.lua prepare
  sysbench * (glob)
  
  function prepare()
