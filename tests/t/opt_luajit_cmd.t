########################################################################
--luajit-cmd tests
########################################################################

  $ cat >opt_luajit_cmd.lua <<EOF
  > print("JIT status: " .. (jit.status() and "on" or "off"))
  > EOF
  $ sysbench --luajit-cmd opt_luajit_cmd.lua
  sysbench * (glob)
  
  JIT status: on

  $ sysbench --luajit-cmd=off opt_luajit_cmd.lua
  sysbench * (glob)
  
  JIT status: off

  $ sysbench --luajit-cmd=on opt_luajit_cmd.lua
  sysbench * (glob)
  
  JIT status: on
