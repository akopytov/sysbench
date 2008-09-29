How to build on Windows


You need CMake (download from http://www.cmake.org/) and Visual Studio 2005 or 
later (free Express edition should work ok)

1.Open Visual Studio command line prompt
2.To build with MySQL support, you will need mysql.h header file and client 
library libmysqld.lib
One can get them e.g by downloading and unpacking the "zip" distribution of mysql

- Append directory where libmysql.lib is located to environment variable LIB, e.g
  set LIB=%LIB%;G:\mysql-noinstall-6.0.6-alpha-win32\mysql-6.0.6-alpha-win32\lib\opt

- Append directory where mysql.h is located to environment variable INCLUDE, e.g
  set INCLUDE=%INCLUDE%;G:\mysql-noinstall-6.0.6-alpha-win32\mysql-6.0.6-alpha-win32\include
3.In the sysbench directory, execute cmake -G "Visual Studio 9 2008" 
4.Open sysbench.sln in Explorer and build Relwithdebinfo target.
  Alternatively, from the command line, issue  
  vcbuild /useenv sysbench.sln "Relwithdebinfo|Win32"
