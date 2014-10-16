# Copyright (C) 2004 MySQL AB
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

SUBDIRS = drivers tests scripting .

bin_PROGRAMS = sysbench

EXTRA_LDFLAGS = @EXTRA_LDFLAGS@

# The following check will be extended as new database drivers will be added
if USE_MYSQL
mysql_ldadd = drivers/mysql/libsbmysql.a $(MYSQL_LIBS)
endif

if USE_DRIZZLE
drizzle_ldadd = drivers/drizzle/libsbdrizzle.a $(LIBDRIZZLE)
endif

if USE_ATTACHSQL
attachsql_ldadd = drivers/attachsql/libsbattachsql.a $(LIBATTACHSQL)
endif

if USE_ORACLE
ora_ldadd = drivers/oracle/libsboracle.a $(ORA_LIBS)
endif

if USE_PGSQL
pgsql_ldadd = drivers/pgsql/libsbpgsql.a $(PGSQL_LIBS)
endif

if USE_LUA
#lua_ldadd = scripting/lua/src/liblua.a -lm @LIBADD_DLOPEN@
lua_ldadd = scripting/lua/src/liblua.a -lm -ldl
endif

sysbench_SOURCES = sysbench.c sysbench.h sb_timer.c sb_timer.h \
sb_options.c sb_options.h sb_logger.c sb_logger.h sb_list.h db_driver.h \
db_driver.c sb_percentile.c sb_percentile.h

sysbench_LDADD = tests/fileio/libsbfileio.a tests/threads/libsbthreads.a \
    tests/memory/libsbmemory.a tests/cpu/libsbcpu.a \
    tests/mutex/libsbmutex.a scripting/libsbscript.a \
    $(mysql_ldadd) $(drizzle_ldadd) $(attachsql_ldadd) $(pgsql_ldadd) $(ora_ldadd) $(lua_ldadd)

sysbench_LDFLAGS = $(EXTRA_LDFLAGS) $(mysql_ldflags) $(attachsql_ldflags) $(pgsql_ldflags) $(ora_ldflags) $(lua_ldflags)

