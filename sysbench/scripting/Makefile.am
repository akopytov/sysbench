# Copyright (C) 2006 MySQL AB
# Copyright (C) 2006-2008 Alexey Kopytov <akopytov@gmail.com>
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

if USE_LUA
SUBDIRS = lua .
lua_sources = script_lua.c script_lua.h
AM_CPPFLAGS += -I$(srcdir)/lua/src -DDATA_PATH=\"$(pkgdatadir)\"
endif

noinst_LIBRARIES = libsbscript.a

libsbscript_a_SOURCES = sb_script.c sb_script.h $(lua_sources)

libsbscript_a_CPPFLAGS = $(AM_CPPFLAGS)

