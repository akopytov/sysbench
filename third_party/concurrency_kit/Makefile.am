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

EXTRA_DIST = ck

all-local: $(builddir)/lib/libck.a

# Concurrency Kit does not support VPATH builds
$(builddir)/lib/libck.a:
	rm -rf tmp
	mkdir tmp
	tar -C $(srcdir) -cf - ck | tar -xf - -C tmp/
	chmod -R u+w tmp
	cd tmp/ck &&                        \
	CC="$(CC)"                          \
	CFLAGS="$(CFLAGS) $(CPPFLAGS)"      \
	LDFLAGS="$(LDFLAGS)"                \
	  ./configure ${CK_CONFIGURE_FLAGS} \
	  --prefix=$(abs_top_builddir)/third_party/concurrency_kit && \
	$(MAKE) && \
	$(MAKE) install

clean-local:
	rm -rf tmp include lib share
