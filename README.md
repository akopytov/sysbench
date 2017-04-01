[![Latest Release][release-badge]][release-url]
[![Build Status][travis-badge]][travis-url]
[![Coverage Status][coveralls-badge]][coveralls-url]
[![License][license-badge]][license-url]

# About

sysbench is a modular, cross-platform and multi-threaded benchmark tool
for evaluating OS parameters that are important for a system running a
database under intensive load.

The idea of this benchmark suite is to quickly get an impression about
system performance without setting up complex database benchmarks or
even without installing a database at all.

## Features

Current features allow to test the following system parameters:

-   file I/O performance

-   scheduler performance

-   memory allocation and transfer speed

-   POSIX threads implementation performance

-   database server performance

# Building and Installing From Source

## Build Requirements

### Debian/Ubuntu
``` shell
    apt -y install make automake libtool pkg-config vim-common
    # For MySQL support
    apt -y install libmysqlclient-dev
    # For PostgreSQL support
    apt -y install libpq-dev
```

### RedHat/CentOS
``` shell
    yum -y install make automake libtool pkg-config vim-common
    # For MySQL support
    yum -y install mysql-devel
    # For PostgreSQL support
    yum -y install postgresql-devel
```

### macOS

Assuming you have Xcode (or Xcode Commane Line Tools) and Homebrew installed:
``` shell
    brew install automake pkg-config
    # For MySQL support
    brew install mysql
    # For PostgreSQL support
    brew install postgresql
```

## Build and Install
``` shell
    ./autogen.sh
    # Add --with-pgsql to build with PostgreSQL support
    ./configure
    make
    make install
```

The above will build sysbench with MySQL support by default. If you have
MySQL headers and libraries in non-standard locations (and no
`mysql_config` can be found in the `PATH`), you can specify them
explicitly with `--with-mysql-includes` and `--with-mysql-libs` options
to `./configure`.

To compile sysbench without MySQL support, use `--without-mysql`. If no
database drivers are available database-related scripts will not work,
but other benchmarks will be functional.

See [README-WIN.txt](README-WIN.txt) for instructions on Windows builds.

See [README-Oracle.md](README-Oracle.md) for instructions on building
with Oracle client libraries.

# Usage

## General syntax

The general command line syntax for sysbench is:

		  sysbench [options]... [testname] [command] 

- *testname* is an optional name of a built-in test (e.g. `fileio`,
  `memory`, `cpu`, etc.), or a name of one of the bundled Lua scripts
  (e.g. `oltp_read_only`), or a *path* to a custom Lua script. If no
  test name is specified on the command line (and thus, there is no
  *command* too, as in that case it would be parsed as a *testname*), or
  the test name is a dash ("`-`"), then sysbench expects a Lua script to
  execute on its standard input.

- *command* is an optional argument that will be passed by sysbench to
  the built-in test or script specified with *testname*. *command*
  defines the *action* that must be performed by the test. The list of
  available commands depends on a particular test. Some tests also
  implement their own custom commands.

  Below is a description of typical test commands and their purpose:

	+ `prepare`: performs preparative actions for those tests which need
	them, e.g. creating the necessary files on disk for the `fileio`
	test, or filling the test database for database benchmarks.
	+ `run`: runs the actual test specified with the *testname*
    argument. This command is provided by all tests.
	+ `cleanup`: removes temporary data after the test run in those
    tests which create one.
	+ `help`: displays usage information for the test specified with the
	*testname* argument. This includes the full list of commands
	provided by the test, so it should be used to get the available
	commands.

- *options* is a list of zero or more command line options starting with
	`'--'`. As with commands, the `sysbench testname help` command
	should be used to describe available options provided by a
	particular test.

	See [General command line options](README.md#general-command-line-options)
	for a description of general options provided by sysbench itself.


You can use `sysbench --help` to display the general command line syntax
and options.

## General command line options

The table below lists the supported common options, their descriptions and default values:

*Option*              | *Description* | *Default value*
----------------------|---------------|----------------
| `--threads`           | The total number of worker threads to create                                                                                                                                                                                                                                                                                                                                                                                                                            | 1               |
| `--events`            | Limit for total number of requests. 0 (the default) means no limit                                                                                                                                                                                                                                                                                                                                                                                                      | 0               |
| `--time`              | Limit for total execution time in seconds. 0 means no limit                                                                                                                                                                                                                                                                                                                                                                                                             | 10              |
| `--warmup-time`       | Execute events for this many seconds with statistics disabled before the actual benchmark run with statistics enabled. This is useful when you want to exclude the initial period of a benchmark run from statistics. In many benchmarks, the initial period is not representative because CPU/database/page and other caches need some time to warm up                                                                                                                                                                                                                                                                                                  | 0               |
| `--rate`              | Average transactions rate. The number specifies how many events (transactions) per seconds should be executed by all threads on average. 0 (default) means unlimited rate, i.e. events are executed as fast as possible                                                                                                                                                                                                                                                                 | 0               |
| `--thread-stack-size` | Size of stack for each thread                                                                                                                                                                                                                                                                                                                                                                                                                                           | 32K             |
| `--report-interval`   | Periodically report intermediate statistics with a specified interval in seconds. Note that statistics produced by this option is per-interval rather than cumulative. 0 disables intermediate reports                                                                                                                                                                                                                                                                  | 0               |
| `--debug`             | Print more debug info                                                                                                                                                                                                                                                                                                                                                                                                                                                   | off             |
| `--validate`          | Perform validation of test results where possible                                                                                                                                                                                                                                                                                                                                                                                                                       | off             |
| `--help`              | Print help on general syntax or on a specified test, and exit                                                                                                                                                                                                                                                                                                                                                                                                           | off             |
| `--verbosity`         | Verbosity level (0 - only critical messages, 5 - debug)                                                                                                                                                                                                                                                                                                                                                                                                                 | 4               |
| `--percentile`        | sysbench measures execution times for all processed requests to display statistical information like minimal, average and maximum execution time. For most benchmarks it is also useful to know a request execution time value matching some percentile (e.g. 95% percentile means we should drop 5% of the most long requests and choose the maximal value from the remaining ones). This option allows to specify a percentile rank of query execution times to count | 95              |
| `--luajit-cmd`        | perform a LuaJIT control command. This option is equivalent to `luajit -j`. See [LuaJIT documentation](http://luajit.org/running.html) for more information                                                                                                                                                                                                                                                                                                             |               |

Note that numerical values for all *size* options (like `--thread-stack-size` in this table) may be specified by appending the corresponding multiplicative suffix (K for kilobytes, M for megabytes, G for gigabytes and T for terabytes).

[coveralls-badge]: https://coveralls.io/repos/github/akopytov/sysbench/badge.svg?branch=master
[coveralls-url]: https://coveralls.io/github/akopytov/sysbench?branch=master
[travis-badge]: https://travis-ci.org/akopytov/sysbench.svg?branch=master
[travis-url]: https://travis-ci.org/akopytov/sysbench?branch=master
[license-badge]: https://img.shields.io/badge/license-GPLv2-blue.svg
[license-url]: COPYING
[release-badge]: https://img.shields.io/github/release/akopytov/sysbench.svg
[release-url]: https://github.com/akopytov/sysbench/releases/latest
