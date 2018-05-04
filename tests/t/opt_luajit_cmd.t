########################################################################
--luajit-cmd tests
########################################################################

# Skip the test on ppc64le, because LuaJIT does not yet support JIT there
  $ if [ $(uname -m) = "ppc64le" ]
  > then
  >   exit 80
  > fi

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
