sysbench Test Suite
===================

sysbench uses the [Cram](https://bitheap.org/cram/) framework for
functional and regression testing. If your system has Python 2.7.9 or
later, or Python 3.4 or later, installing Cram is as simple as executing
`pip install cram`.

If you use an older Python version, you may need to [install
pip](https://pip.pypa.io/en/latest/installing/) first:

``` {.example}
curl https://bootstrap.pypa.io/get-pip.py | python
```

To run the sysbench test suite, invoke the `test_run.sh` script in the
`tests` directory as follows:

``` {.example}
./test_run.sh [test_name]...
```

Each `test_name` argument is a name of a test case file. Functional and
regression tests are located in the `t` subdirectory in files with the
`.t` suffix.

If no tests are named on the `test_run.sh` command line, it will execute
all files with the `.t` suffix in the `t` subdirectory.

Some tests require external servers (MySQL, PostgreSQL, etc). One should
use environment variables to specify connection related arguments that
sysbench can use to connect to such external server(s). The currently
recognized variables are:

- `SBTEST_MYSQL_ARGS` -- MySQL connection options: `--mysql-host`,
  `--mysql-port`, `--mysql-socket` `--mysql-user`, `--mysql-password`
  and `--mysql-db`;

- `SBTEST_PGSQL_ARGS` -- PostgreSQL connection options: `--pgsql-host`,
  `--pgsql-port`, `--pgsql-user`, `--pgsql-password` and `--pgsql-db`.

For example:
``` {.example}
export SBTEST_MYSQL_ARGS="--mysql-host=localhost --mysql-user=sbtest --mysql-password=secret --mysql-db=sbtest"
export SBTEST_PGSQL_ARGS="--pgsql-host=localhost --pgsql-user=postgres --pgsql-password=secret --pgsql-db=sbtest"
./test_run.sh
```

sysbench assumes that server(s) are pre-configured so that the specified
database exists and the user connecting with the specified credentials
has all privileges on the database. In particular, sysbench must have
enough privileges to create/drop/read/modify tables in that database.
