# Copyright (c) 2016 University of Helsinki
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
# See the file COPYING included with this distribution for more
# information.

SWIG_INTERFACE = libhfst.i

# The Python bindings are built from the unified pyproject.toml/setup.py at the
# top of the tree, in "shared" mode: the extension links against the libhfst
# just built by this same tree (../libhfst/src/.libs) instead of recompiling
# all of libhfst. This is the same build the PyPI wheels use (in static mode),
# so PyPI and distribution (e.g. Debian python3-libhfst) packaging share one
# configuration.

TOP = $(abs_top_srcdir)
HFST_LIBS = $(abs_top_builddir)/libhfst/src/.libs

BUILT_SOURCES = build-python-bindings

build-python-bindings: $(SWIG_INTERFACE) $(TOP)/setup.py $(TOP)/pyproject.toml
	cd $(TOP) && HFST_USE_SHARED_LIB=1 HFST_LIBRARY_DIR=$(HFST_LIBS) \
	    $(PYTHON) setup.py build

install-exec-local:
	cd $(TOP) && HFST_USE_SHARED_LIB=1 HFST_LIBRARY_DIR=$(HFST_LIBS) \
	    $(PYTHON) setup.py install $(PYTHON_INSTALL_PARAMS)

.PHONY: build-python-bindings

SUBDIRS = test
EXTRA_DIST = libhfst.i docstrings.i README \
hfst_extensions.cpp hfst_file_extensions.cpp hfst_lexc_extensions.cpp hfst_sfst_extensions.cpp \
hfst_lookup_extensions.cpp hfst_pmatch_extensions.cpp hfst_prolog_extensions.cpp \
hfst_regex_extensions.cpp hfst_rules_extensions.cpp hfst_xfst_extensions.cpp \
hfst_pmatch_tokenize_extensions.cpp \
hfst/__init__.py hfst/exceptions/__init__.py hfst/sfst_rules/__init__.py \
hfst/xerox_rules/__init__.py

AUTOMAKE_OPTIONS = subdir-objects
