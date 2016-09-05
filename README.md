[![Coverage Status](https://coveralls.io/repos/github/akopytov/sysbench/badge.svg?branch=0.5)](https://coveralls.io/github/akopytov/sysbench?branch=0.5)
[![Build Status](https://travis-ci.org/akopytov/sysbench.svg?branch=0.5)](https://travis-ci.org/akopytov/sysbench)

About
=====

SysBench is a modular, cross-platform and multi-threaded benchmark tool
for evaluating OS parameters that are important for a system running a
database under intensive load.

The idea of this benchmark suite is to quickly get an impression about
system performance without setting up complex database benchmarks or
even without installing a database at all.

Features
--------

Current features allow to test the following system parameters:

-   file I/O performance

-   scheduler performance

-   memory allocation and transfer speed

-   POSIX threads implementation performance

-   database server performance

Installation
------------

	./autogen.sh
	./configure
	make

The above will build SysBench with MySQL support by default. If you have MySQL headers and libraries in non-standard locations (and no `mysql_config` can be found in the `PATH`), you can specify them explicitly with `--with-mysql-includes` and `--with-mysql-libs` options to `./configure`.

To compile SysBench without MySQL support, use `--without-mysql`. In
this case all database-related tests will not work, but other tests will
be functional.

Usage
=====

General syntax
--------------

The general syntax for SysBench is as follows:

		  sysbench [common-options] --test=name [test-options] command


See [General command line options](README.md#general-command-line-options) for a description of common options and documentation for particular test mode for a list of test-specific options.

Below is a brief description of available commands and their purpose:

+ `prepare`: performs preparative actions for those tests which need
them, e.g. creating the necessary files on disk for the `fileio` test,
or filling the test database for OLTP tests.
+ `run`: runs the actual test specified with the `--test` option.
+ `cleanup`: removes temporary data after the test run in those tests which create one.
+ `help`: displays usage information for a test specified with the
  `--test` option.

Also you can use `sysbench help` (without `--test`) to display the brief usage summary and the list of available test modes.

General command line options
----------------------------

The table below lists the supported common options, their descriptions and default values:

*Option*              | *Description* | *Default value*
----------------------|---------------|----------------
| `--num-threads`       | The total number of worker threads to create                                                                                                                                                                                                                                                                                                                                                                                                                            | 1               |
| `--max-requests`      | Limit for total number of requests. 0 means unlimited                                                                                                                                                                                                                                                                                                                                                                                                                   | 10000           |
| `--max-time`          | Limit for total execution time in seconds. 0 (default) means unlimited                                                                                                                                                                                                                                                                                                                                                                                                  | 0               |
| `--thread-stack-size` | Size of stack for each thread                                                                                                                                                                                                                                                                                                                                                                                                                                           | 32K             |
| `--init-rng`          | Specifies if random numbers generator should be initialized from timer before the test start                                                                                                                                                                                                                                                                                                                                                                            | off             |
| `--report-interval`   | Periodically report intermediate statistics with a specified interval in seconds. Note that statistics produced by this option is per-interval rather than cumulative. 0 disables intermediate reports                                                                                                                                                                                                                                                                  | 0               |
| `--test`              | Name of the test mode to run                                                                                                                                                                                                                                                                                                                                                                                                                                            | *Required*      |
| `--debug`             | Print more debug info                                                                                                                                                                                                                                                                                                                                                                                                                                                   | off             |
| `--validate`          | Perform validation of test results where possible                                                                                                                                                                                                                                                                                                                                                                                                                       | off             |
| `--help`              | Print help on general syntax or on a test mode specified with --test, and exit                                                                                                                                                                                                                                                                                                                                                                                          | off             |
| `--verbosity`         | Verbosity level (0 - only critical messages, 5 - debug)                                                                                                                                                                                                                                                                                                                                                                                                                 | 4               |
| `--percentile`        | SysBench measures execution times for all processed requests to display statistical information like minimal, average and maximum execution time. For most benchmarks it is also useful to know a request execution time value matching some percentile (e.g. 95% percentile means we should drop 5% of the most long requests and choose the maximal value from the remaining ones). This option allows to specify a percentile rank of query execution times to count | 95              |

Note that numerical values for all *size* options (like `--thread-stack-size` in this table) may be specified by appending the corresponding multiplicative suffix (K for kilobytes, M for megabytes, G for gigabytes and T for terabytes).

