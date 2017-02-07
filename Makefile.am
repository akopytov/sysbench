# Copyright (C) 2004 MySQL AB
# Copyright (C) 2004-2016 Alexey Kopytov <akopytov@gmail.com>
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

ACLOCAL_AMFLAGS = -I m4
AM_DISTCHECK_CONFIGURE_FLAGS = --without-mysql

if USE_BUNDLED_LUAJIT
  LUAJIT_DIR = third_party/luajit
endif

if USE_BUNDLED_CK
  CK_DIR = third_party/concurrency_kit
endif

SUBDIRS = doc $(LUAJIT_DIR) $(CK_DIR) src tests

EXTRA_DIST = autogen.sh README.md README-WIN.txt README-Oracle.md ChangeLog \
             snap/snapcraft.yaml.in

test:
	cd tests && $(MAKE) test
