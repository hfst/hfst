## Process this file with automake to produce Makefile.in

# Copyright (c) 2016 University of Helsinki
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
# See the file COPYING included with this distribution for more
# information.

SUBDIRS = src

if CAN_DOXYGEN
.PHONY: apidoc

apidoc: Doxyfile
	$(DOXYGEN) $<
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hfst.pc hfst_c.pc
