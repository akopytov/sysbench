As of sysbench 1.0 support for native Windows builds was dropped. It may
be re-introduced in later versions.

Currently, the recommended way to build sysbench on Windows is using
Windows Subsystem for Linux available in Windows 10:
https://msdn.microsoft.com/en-us/commandline/wsl/about

After installing WSL and getting a bash prompt, following Debian/Ubuntu
build instructions is sufficient.

Alternatively, one can build and use sysbench 0.5 natively for Windows.
