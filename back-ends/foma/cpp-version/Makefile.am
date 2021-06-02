AUTOMAKE_OPTIONS=std-options

.NOTPARALLEL:

AM_CPPFLAGS= -Wno-deprecated -D_XOPEN_SOURCE=500 -fpermissive -std=c++11

if WANT_MINGW
AM_CPPFLAGS += -D__NO_MINGW_LFS
endif

if WANT_FOMA
noinst_LTLIBRARIES = libfoma.la
endif

libfoma_la_SOURCES = int_stack.cc define.cc determinize.cc apply.cc \
        rewrite.cc lexcread.cc topsort.cc flags.cc minimize.cc reverse.cc \
        extract.cc sigma.cc structures.cc constructions.cc \
        coaccessible.cc io.cc utf8.cc spelling.cc dynarray.cc mem.cc stringhash.cc \
        trie.cc lex.lexc.cc lex.yy.cc regex.cc

EXTRA_DIST = foma.h fomalibconf.h fomalib.h lexc.h regex.h
