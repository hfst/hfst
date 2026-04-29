## Process this file with automake to produce Makefile.in

# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
# See the file COPYING included with this distribution for more
# information.

lib_LTLIBRARIES=libhfst_c.la

AM_CPPFLAGS=-I${top_srcdir}/libhfst/src

libhfst_c_la_SOURCES=libhfst_c.cpp

libhfst_c_la_LDFLAGS=-no-undefined -version-info 56:1:0
libhfst_c_la_LIBADD = $(top_builddir)/libhfst/src/libhfst.la $(ICU_LIBS)

hfstincludedir=$(includedir)/hfst
hfstinclude_HEADERS=libhfst_c.h
