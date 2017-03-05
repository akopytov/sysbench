Set up cram alias and example tests:

  $ . "$TESTDIR"/setup.sh

xUnit XML output:

  $ cram -q -v --xunit-file=cram.xml examples
  examples/bare.t: passed
  examples/empty.t: empty
  examples/env.t: passed
  examples/fail.t: failed
  examples/missingeol.t: passed
  examples/skip.t: skipped
  examples/test.t: passed
  # Ran 7 tests, 2 skipped, 1 failed.
  [1]
  $ cat cram.xml
  <?xml version="1.0" encoding="utf-8"?>
  <testsuite name="cram"
             tests="7"
             failures="1"
             skipped="2"
             timestamp="\d+-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\+\d{2}:\d{2}" (re)
             hostname="[^"]+" (re)
             time="\d+\.\d{6}"> (re)
    <testcase classname="examples/bare.t"
              name="bare.t"
              time="\d+\.\d{6}"/> (re)
    <testcase classname="examples/empty.t"
              name="empty.t"
              time="\d+\.\d{6}"> (re)
      <skipped/>
    </testcase>
    <testcase classname="examples/env.t"
              name="env.t"
              time="\d+\.\d{6}"/> (re)
    <testcase classname="examples/fail.t"
              name="fail.t"
              time="\d+\.\d{6}"> (re)
      <failure><![CDATA[--- examples/fail.t
  +++ examples/fail.t.err
  @@ -1,18 +1,18 @@
   Output needing escaping:
   
     $ printf '\00\01\02\03\04\05\06\07\010\011\013\014\016\017\020\021\022\n'
  -  foo
  +  \x00\x01\x02\x03\x04\x05\x06\x07\x08\t\x0b\x0c\x0e\x0f\x10\x11\x12 (esc)
     $ printf '\023\024\025\026\027\030\031\032\033\034\035\036\037\040\047\n'
  -  bar
  +  \x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f ' (esc)
   
   Wrong output and bad regexes:
   
     $ echo 1
  -  2
  +  1
     $ printf '1\nfoo\n1\n'
  -  +++ (re)
  -  foo\ (re)
  -   (re)
  +  1
  +  foo
  +  1
   
   Filler to force a second diff hunk:
   
  @@ -20,5 +20,6 @@
   Offset regular expression:
   
     $ printf 'foo\n\n1\n'
  +  foo
     
     \d (re)
  ]]></failure>
    </testcase>
    <testcase classname="examples/missingeol.t"
              name="missingeol.t"
              time="\d+\.\d{6}"/> (re)
    <testcase classname="examples/skip.t"
              name="skip.t"
              time="\d+\.\d{6}"> (re)
      <skipped/>
    </testcase>
    <testcase classname="examples/test.t"
              name="test.t"
              time="\d+\.\d{6}"/> (re)
  </testsuite>
  $ rm cram.xml examples/fail.t.err
