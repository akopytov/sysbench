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
`tests` directory like this:

``` {.example}
./test_run.sh [test_name]...
```

Each `test_name` argument is name of a test case file. Functional and
regression tests are located in the `t` subdirectory in files with the
`.t` suffix.

If no tests are named on the `test_run.sh` command line, it will execute
all files with the `.t` suffix in the `t` subdirectory.
