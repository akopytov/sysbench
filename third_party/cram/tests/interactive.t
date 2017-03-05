Set up cram alias and example tests:

  $ . "$TESTDIR"/setup.sh

Interactive mode (don't merge):

  $ cram -n -i examples/fail.t
  !
  --- examples/fail.t
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
  Accept this change? [yN] n
  
  # Ran 1 tests, 0 skipped, 1 failed.
  [1]
  $ md5 examples/fail.t examples/fail.t.err
  .*\b0f598c2b7b8ca5bcb8880e492ff6b452\b.* (re)
  .*\b7a23dfa85773c77648f619ad0f9df554\b.* (re)

Interactive mode (merge):

  $ cp examples/fail.t examples/fail.t.orig
  $ cram -y -i examples/fail.t
  !
  --- examples/fail.t
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
  Accept this change? [yN] y
  patching file examples/fail.t
  
  # Ran 1 tests, 0 skipped, 1 failed.
  [1]
  $ md5 examples/fail.t
  .*\b1d9e5b527f01fbf2d9b1c121d005108c\b.* (re)
  $ mv examples/fail.t.orig examples/fail.t

Verbose interactive mode (answer manually and don't merge):

  $ printf 'bad\nn\n' | cram -v -i examples/fail.t
  examples/fail.t: failed
  --- examples/fail.t
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
  Accept this change? [yN] Accept this change? [yN] # Ran 1 tests, 0 skipped, 1 failed.
  [1]
  $ md5 examples/fail.t examples/fail.t.err
  .*\b0f598c2b7b8ca5bcb8880e492ff6b452\b.* (re)
  .*\b7a23dfa85773c77648f619ad0f9df554\b.* (re)
  $ printf 'bad\n\n' | cram -v -i examples/fail.t
  examples/fail.t: failed
  --- examples/fail.t
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
  Accept this change? [yN] Accept this change? [yN] # Ran 1 tests, 0 skipped, 1 failed.
  [1]
  $ md5 examples/fail.t examples/fail.t.err
  .*\b0f598c2b7b8ca5bcb8880e492ff6b452\b.* (re)
  .*\b7a23dfa85773c77648f619ad0f9df554\b.* (re)

Verbose interactive mode (answer manually and merge):

  $ cp examples/fail.t examples/fail.t.orig
  $ printf 'bad\ny\n' | cram -v -i examples/fail.t
  examples/fail.t: failed
  --- examples/fail.t
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
  Accept this change? [yN] Accept this change? [yN] patching file examples/fail.t
  examples/fail.t: merged output
  # Ran 1 tests, 0 skipped, 1 failed.
  [1]
  $ md5 examples/fail.t
  .*\b1d9e5b527f01fbf2d9b1c121d005108c\b.* (re)
  $ mv examples/fail.t.orig examples/fail.t

Test missing patch(1) and patch(1) error:

  $ PATH=. cram -i examples/fail.t
  patch(1) required for -i
  [2]
  $ cat > patch <<EOF
  > #!/bin/sh
  > echo "patch failed" 1>&2
  > exit 1
  > EOF
  $ chmod +x patch
  $ PATH=. cram -y -i examples/fail.t
  !
  --- examples/fail.t
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
  Accept this change? [yN] y
  patch failed
  examples/fail.t: merge failed
  
  # Ran 1 tests, 0 skipped, 1 failed.
  [1]
  $ md5 examples/fail.t examples/fail.t.err
  .*\b0f598c2b7b8ca5bcb8880e492ff6b452\b.* (re)
  .*\b7a23dfa85773c77648f619ad0f9df554\b.* (re)
  $ rm patch examples/fail.t.err
