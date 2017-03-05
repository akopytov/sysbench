* Add more comments explaining how different parts of the code work.

* Add a man page.

* Implement string substitutions (e.g., --substitute=FOOPORT=123).

* Conditionals (e.g., --define=windows=1, #if windows ... #else ...
  #endif).

* Support #!/usr/bin/env cram

* Support .cramrc in test directories. Though, if I do this, what happens
  when there are multiple .cramrc files? Does the deepest one completely
  override the others? Do they merge together?

* Homebrew formula.

* Debian, Ubuntu, CentOS/RHEL repos.

* Implement a test that does stricter style guide testing.

* Write contributor guidelines.

* Get the test suite running on AppVeyor under MSYS2.

  - http://help.appveyor.com/discussions/suggestions/615-support-for-msys2
  - https://github.com/behdad/harfbuzz/pull/112/files
  - https://github.com/khaledhosny/ots/pull/67/files
  - https://github.com/appveyor/ci/issues/352#issuecomment-138149606
  - https://github.com/appveyor/ci/issues/597
  - http://www.appveyor.com

* Get the test suite fully passing with Python.org's Windows
  distribution.

* Global setup/teardown support.

* Local setup/teardown? This is technically already supported via
  sourcing scripts and using exit traps, but dedicated syntax might be
  nice (e.g., #setup ... #endsetup? or maybe just #teardown ...
  #endteardown or #finally ... #endfinally?).

* Implement -j flag for concurrency.

* Flexible indentation support (with an algorithm similar to Python's
  for detecting indentation on a per-block basis).

* Some sort of plugin system (one that doesn't require writing plugins
  in Python) that allows basic extension of Cram's functionality (and
  possibly even syntax, though perhaps limited to just "macros" like
  #foo, #bar, etc. and matchers like (baz), (quux), etc.).

* Be able to run the Mercurial test suite.

* Write cram plugins for other testing frameworks (nose, py.test,
  etc.).

* Somehow make it possible to specify tests in Python doc
  strings (and similar things in other languages like Perl, Ruby,
  etc.).

* Emacs mode.
