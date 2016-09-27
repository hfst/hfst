## Process this file with automake to produce Makefile.in

# Copyright (c) 2016 University of Helsinki
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
# See the file COPYING included with this distribution for more
# information.

SUBDIRS=implementations parsers
AUTOMAKE_OPTIONS=subdir-objects
lib_LTLIBRARIES = libhfst.la

AM_CXXFLAGS=-Wno-deprecated -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -g
AM_CPPFLAGS = -I${top_srcdir}/libhfst/src

# HFST bridge specific stuff
HFST_SRCS=HfstApply.cc HfstInputStream.cc HfstTransducer.cc HfstOutputStream.cc\
		  HfstRules.cc HfstXeroxRules.cc HfstDataTypes.cc \
		  HfstSymbolDefs.cc HfstTokenizer.cc \
		  HfstFlagDiacritics.cc HfstExceptionDefs.cc \
		  HarmonizeUnknownAndIdentitySymbols.cc \
		  HfstLookupFlagDiacritics.cc \
		  HfstEpsilonHandler.cc HfstStrings2FstTokenizer.cc \
		  HfstPrintDot.cc HfstPrintPCKimmo.cc hfst-string-conversions.cc \
		  string-utils.cc

# libtool takes over
libhfst_la_SOURCES = $(HFST_SRCS)
libhfst_la_LIBADD = implementations/libhfstimplementations.la \
			parsers/libhfstparsers.la

AM_CPPFLAGS += -I${top_srcdir}/back-ends/foma -I${top_srcdir}/back-ends

if WANT_SFST
  libhfst_la_LIBADD += $(top_builddir)/back-ends/sfst/libsfst.la
endif

if WANT_FOMA
  libhfst_la_LIBADD +=$(top_builddir)/back-ends/foma/libfoma.la
endif

if WANT_MINGW
  AM_CPPFLAGS += -I${top_srcdir}/back-ends/openfstwin/src/include \
			-I${top_srcdir}/back-ends/dlfcn -DWINDOWS
  libhfst_la_LIBADD += $(top_builddir)/back-ends/dlfcn/libdl.la \
			$(top_builddir)/back-ends/openfstwin/src/lib/libfst.la
else
  AM_CPPFLAGS += -I${top_srcdir}/back-ends/openfst/src/include
  libhfst_la_LIBADD += $(top_builddir)/back-ends/openfst/src/lib/libfst.la
endif

if WINDOWS
  libhfst_la_LIBADD += -ldl
endif


# we only have one library target but would like a particular includedir
# structure for various headers, so we do this
hfstincludedir = $(includedir)/hfst

HFST_HDRS = \
	hfst.h \
	HfstTransducer.h \
	hfst_apply_schemas.h \
	HfstDataTypes.h \
	HfstSymbolDefs.h \
	FormatSpecifiers.h \
	implementations/SfstTransducer.h \
	implementations/TropicalWeightTransducer.h \
	implementations/LogWeightTransducer.h \
	implementations/FomaTransducer.h \
	implementations/HfstOlTransducer.h \
	HfstTokenizer.h \
	implementations/ConvertTransducerFormat.h \
	implementations/HfstTransitionGraph.h \
	implementations/HfstBasicTransducer.h \
	implementations/HfstTransition.h \
	implementations/HfstBasicTransition.h \
	implementations/HfstTropicalTransducerTransitionData.h \
	implementations/compose_intersect/ComposeIntersectRulePair.h \
	implementations/compose_intersect/ComposeIntersectLexicon.h \
	implementations/compose_intersect/ComposeIntersectRule.h \
	implementations/compose_intersect/ComposeIntersectFst.h \
	HarmonizeUnknownAndIdentitySymbols.h \
	HfstExceptionDefs.h \
	HfstExtractStrings.h \
	HfstFlagDiacritics.h \
	HfstEpsilonHandler.h \
	HfstInputStream.h \
	HfstOutputStream.h \
	HfstXeroxRules.h \
	HfstLookupFlagDiacritics.h \
	HfstStrings2FstTokenizer.h \
	HfstPrintDot.h \
	HfstPrintPCKimmo.h \
	string-utils.h \
	hfst-string-conversions.h \
	parsers/LexcCompiler.h parsers/XreCompiler.h parsers/PmatchCompiler.h \
	hfstdll.h
### Add your library here ###
#	implementations/MyTransducerLibraryTransducer.h

hfstinclude_HEADERS = $(HFST_HDRS)

libhfst_la_LDFLAGS = -no-undefined -version-info 46:0:0

LIBHFST_TSTS=HfstApply HfstInputStream HfstTransducer \
		HfstOutputStream HfstXeroxRules HfstRules HfstSymbolDefs \
		HfstTokenizer HfstFlagDiacritics \
		HarmonizeUnknownAndIdentitySymbols

check_PROGRAMS=$(LIBHFST_TSTS)

HfstApply_SOURCES=HfstApply.cc
HfstApply_CXXFLAGS=-DMAIN_TEST
HfstApply_LDADD=libhfst.la
HfstInputStream_SOURCES=HfstInputStream.cc
HfstInputStream_CXXFLAGS=-DMAIN_TEST
HfstInputStream_LDADD=libhfst.la
HfstTransducer_SOURCES=HfstTransducer.cc
HfstTransducer_CXXFLAGS=-DMAIN_TEST
HfstTransducer_LDADD=libhfst.la
HfstOutputStream_SOURCES=HfstOutputStream.cc
HfstOutputStream_CXXFLAGS=-DMAIN_TEST
HfstOutputStream_LDADD=libhfst.la
HfstXeroxRules_SOURCES=HfstXeroxRules.cc
HfstXeroxRules_CXXFLAGS=-DMAIN_TEST
HfstXeroxRules_LDADD=libhfst.la
HfstRules_SOURCES=HfstRules.cc
HfstRules_CXXFLAGS=-DMAIN_TEST
HfstRules_LDADD=libhfst.la
HfstSymbolDefs_SOURCES=HfstSymbolDefs.cc
HfstSymbolDefs_CXXFLAGS=-DMAIN_TEST
HfstSymbolDefs_LDADD=libhfst.la
HfstTokenizer_SOURCES=HfstTokenizer.cc
HfstTokenizer_CXXFLAGS=-DMAIN_TEST
HfstTokenizer_LDADD=libhfst.la
HfstFlagDiacritics_SOURCES=HfstFlagDiacritics.cc
HfstFlagDiacritics_CXXFLAGS=-DMAIN_TEST
HfstFlagDiacritics_LDADD=libhfst.la
HarmonizeUnknownAndIdentitySymbols_SOURCES=HarmonizeUnknownAndIdentitySymbols.cc
HarmonizeUnknownAndIdentitySymbols_CXXFLAGS=-DMAIN_TEST
HarmonizeUnknownAndIdentitySymbols_LDADD=libhfst.la

TESTS=$(LIBHFST_TSTS)

valgrind: valgrind-unit-tests.sh $(LIBHFST_TSTS)
	$(SH) $(srcdir)/valgrind-unit-tests.sh $^

EXTRA_DIST=HfstXeroxRulesTest.cc
