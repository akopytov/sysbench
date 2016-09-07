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

TESTS = test_run.sh

test_SCRIPTS = test_run.sh

EXTRA_DIST = $(test_SCRIPTS) \
             README.md

testroot = 	$(datadir)
testdir = 	$(testroot)/sysbench/tests
test_dirs= 	t

# Used by dist-hook and install-data-local to copy all
# test files into either dist or install directory
install_test_files:
	@if test -z "$(INSTALL_TO_DIR)"; then \
	  echo "Set INSTALL_TO_DIR!" && exit 1; \
	fi
	@for dir in $(test_dirs); do \
	  from_dir="$(srcdir)/$$dir"; \
	  to_dir="$(INSTALL_TO_DIR)/$$dir"; \
	  $(mkinstalldirs) "$$to_dir"; \
	  for f in `(cd $$from_dir && ls *.t)`; do \
	    if test -f "$$from_dir/$$f"; then \
	      $(INSTALL_DATA) "$$from_dir/$$f" "$$to_dir/$$f" ; \
	    fi; \
	  done \
	done

dist-hook:
	$(MAKE)	INSTALL_TO_DIR="$(distdir)" install_test_files

install-data-local:
	$(MAKE)	INSTALL_TO_DIR="$(DESTDIR)$(testdir)" install_test_files

uninstall-local:
	rm -f -r $(DESTDIR)$(testdir)

test:
	./test_run.sh
