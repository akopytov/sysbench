  $ . $SBTEST_CONFIG

  $ sysbench --version
  sysbench [.0-9]* (re)

  $ version=$(sysbench --version | cut -d ' ' -f 2)
  $ test "$version" = "$SBTEST_VERSION"
