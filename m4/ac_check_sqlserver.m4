dnl ---------------------------------------------------------------------------
dnl Macro: AC_CHECK_SQLSERVER TODO!
dnl ---------------------------------------------------------------------------

AC_DEFUN([AC_CHECK_SQLSERVER],[
# TODO: Check for dependencies
AC_MSG_CHECKING(SQL Server ODBC 18)
AC_SEARCH_LIBS([SQLExecDirect],
      [msodbcsql18 unixodbc-dev odbc],
      [],
      AC_MSG_ERROR([cannot find ODBC 18 for SQL Server library]))
SQLSERVER_LIBS="-lodbc"
AC_MSG_RESULT($SQLSERVER_LIBS)

])


