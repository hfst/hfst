## Process this file with automake to produce Makefile.in

## Copyright (C) 2010 University of Helsinki

## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = .
if EMSCRIPTEN
SUBDIRS += libhfst
else
SUBDIRS += back-ends libhfst tools test doc man scripts
endif

if HAVE_PYTHON_BINDINGS
SUBDIRS += python
endif

# for external projects to use HFST from auto*
m4dir=$(datadir)/aclocal
m4_DATA=hfst.m4

EXTRA_DIST=hfst.m4

if CAN_DOXYGEN
doxygen:
	doxygen Doxyfile
endif

valgrind:
	$(MAKE) -C libhfst/src/ valgrind
	$(MAKE) -C test/tools/ valgrind
