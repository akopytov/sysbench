# Copyright (C) 2004 MySQL AB
# Copyright (C) 2004-2014 Alexey Kopytov <akopytov@gmail.com>
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

if USE_MYSQL
MYSQL_DIR = mysql
endif

if USE_DRIZZLE
DRIZZLE_DIR = drizzle
endif

if USE_ATTACHSQL
ATTACHSQL_DIR = attachsql
endif

if USE_ORACLE
ORACLE_DIR = oracle
endif

if USE_PGSQL
PGSQL_DIR = pgsql
endif

SUBDIRS = $(MYSQL_DIR) $(ORACLE_DIR) $(PGSQL_DIR) $(DRIZZLE_DIR) $(ATTACHSQL_DIR)
