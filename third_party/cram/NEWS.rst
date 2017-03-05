======
 News
======

Version 0.7 (Feb. 24, 2016)
---------------------------

* Added the ``-d``/``--debug`` flag that disables diffing of
  expected/actual output and instead passes through script output to
  ``stdout``/``stderr``.

* Added the ``--shell-opts`` flag for specifying flags to invoke the
  shell with. By setting ``--shell-opts='-x'`` and ``--debug``
  together, this can be used to see shell commands as they're run and
  their output in real time which can be useful for debugging slow or
  hanging tests.

* Added xUnit XML output support (for better integration of test
  results with Bamboo and other continuous integration tools).

* Added support for using (esc) on expected out lines that aren't
  automatically escaped in actual output.

* Added the ``$TESTSHELL`` environment variable. This allows a test to
  portably check what shell it was invoked with.

* Added an error message for when no tests are found in a directory.

* Changed ``Makefile`` to install into ``/usr/local`` by default.

* Simplified the ``Makefile``'s targets. The targets available now are
  ``all``, ``build``, ``check``/``test``, ``clean``, ``dist``,
  ``install``, and ``quicktest`` (for running the test suite without
  checking test coverage).

* Fixed non-ASCII strings not being escaped with ``(esc)`` on Python 3.

* Fixed a crash on tests that don't have a trailing newline.

* Fixed a crash when using ``set -x`` with zsh.


Version 0.6 (Aug. 1, 2013)
--------------------------

* Added the long option ``--preserve-env`` for ``-E``.

* Added support for specifying options in ``.cramrc`` (configurable
  with the ``CRAMRC`` environment variable).

* Added a ``--shell`` option to change the shell tests are run
  with. Contributed by `Kamil Kisiel`_.

* Added Arch Linux package metadata (in ``contrib/``). Contributed by
  `Andrey Vlasovskikh`_.

* Fixed shell commands unintentionally inheriting Python's ``SIGPIPE``
  handler (causing commands that close pipes to print ``broken pipe``
  messages).

* Fixed ``EPIPE`` under PyPy when applying patches in
  ``--interactive`` mode.

* Added ``TESTFILE`` test environment variable (set to the name of the
  current test).

* Fixed GNU patch 2.7 compatibility by using relative paths instead of
  absolute paths. Contributed by `Douglas Creager`_.

* Fixed name clashes in temporary test directories (e.g., when running
  two tests with the same name in different folders).

* **Backwards compatibility:** Fixed improper usage of the subprocess
  library under Python 3. This fixes Python 3.3 support, but breaks
  support for Python 3.1-3.2.3 due to a bug in Python. If you're using
  Python 3.0-3.2, you must upgrade to Python 3.2.4 or newer.

.. _Kamil Kisiel: http://kamilkisiel.net/
.. _Andrey Vlasovskikh: https://twitter.com/vlasovskikh
.. _Douglas Creager: http://dcreager.net/


Version 0.5 (Jan. 8, 2011)
--------------------------

* **The test format has changed:** Matching output not ending in a
  newline now requires the ``(no-eol)`` keyword instead of ending the
  line in ``%``.

* Matching output containing unprintable characters now requires the
  ``(esc)`` keyword. Real output containing unprintable characters
  will automatically receive ``(esc)``.

* If an expected line matches its real output line exactly, special
  matching like ``(re)`` or ``(glob)`` will be ignored.

* Regular expressions ending in a trailing backslash are now
  considered invalid.

* Added an ``--indent`` option for changing the default amount of
  indentation required to specify commands and output.

* Added support for specifying command line options in the ``CRAM``
  environment variable.

* The ``--quiet`` and ``--verbose`` options can now be used together.

* When running Cram under Python 3, Unicode-specific line break
  characters will no longer be parsed as newlines.

* Tests are no longer required to end in a trailing newline.


Version 0.4 (Sep. 28, 2010)
---------------------------

* **The test format has changed:** Output lines containing regular
  expressions must now end in ``(re)`` or they'll be matched
  literally. Lines ending with keywords are matched literally first,
  however.

* Regular expressions are now matched from beginning to end. In other
  words ``\d (re)`` is matched as ``^\d$``.

* In addition to ``(re)``, ``(glob)`` has been added. It supports
  ``*``, ``?``, and escaping both characters (and backslashes) using
  ``\``.

* **Environment settings have changed:** The ``-D`` flag has been
  removed, ``$TESTDIR`` is now set to the directory containing the
  ``.t`` file, and ``$CRAMTMP`` is set to the test runner's temporary
  directory.

* ``-i``/``--interactive`` now requires ``patch(1)``. Instead of
  ``.err`` files replacing ``.t`` files during merges, diffs are
  applied using ``patch(1)``. This prevents matching regular
  expressions and globs from getting clobbered.

* Previous ``.err`` files are now removed when tests pass.

* Cram now exits with return code ``1`` if any tests failed.

* If a test exits with return code ``80``, it's considered a skipped a
  test. This is useful for intentionally disabling tests when they
  only work on certain platforms or in certain settings.

* The number of tests, the number of skipped tests, and the number of
  failed tests are now printed after all tests are finished.

* Added ``-q``/``--quiet`` to suppress diff output.

* Added `contrib/cram.vim`_ syntax file for Vim. Contributed by `Steve
  Losh`_.

.. _contrib/cram.vim: https://bitbucket.org/brodie/cram/src/default/contrib/cram.vim
.. _Steve Losh: http://stevelosh.com/


Version 0.3 (Sep. 20, 2010)
---------------------------

* Implemented resetting of common environment variables. This behavior
  can be disabled using the ``-E`` flag.

* Changed the test runner to first make its own overall random
  temporary directory, make ``tmp`` inside of it and set ``TMPDIR``,
  etc. to its path, and run each test with a random temporary working
  directory inside of that.

* Added ``--keep-tmpdir``. Temporary directories are named by test
  filename (along with a random string).

* Added ``-i``/``--interactive`` to merge actual output back to into
  tests interactively.

* Added ability to match command output not ending in a newline by
  suffixing output in the test with ``%``.


Version 0.2 (Sep. 19, 2010)
---------------------------

* Changed the test runner to run tests with a random temporary working
  directory.


Version 0.1 (Sep. 19, 2010)
---------------------------

* Initial release.
