# Copyright (C) 2016 Alexey Kopytov <akopytov@gmail.com>
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

EXTRA_DIST = luajit

all-local: $(builddir)/lib/libluajit-5.1.a

# LuaJIT does not support VPATH builds
$(builddir)/lib/libluajit-5.1.a:
	$(MAKE) -C $(srcdir)/luajit clean
	rm -rf tmp
	mkdir tmp
	tar -C $(srcdir) -cf - luajit | tar -xf - -C tmp/
	chmod -R u+w tmp
	$(MAKE) -C tmp/luajit \
                PREFIX=$(abs_top_builddir)/third_party/luajit \
                INSTALL_INC=$(abs_top_builddir)/third_party/luajit/inc \
                install

clean-local:
	rm -rf tmp bin inc lib share
