AUTOMAKE_OPTIONS=std-options

.NOTPARALLEL:

AM_CPPFLAGS= -Wno-deprecated -std=c99 -D_XOPEN_SOURCE=500

if WANT_MINGW
AM_CPPFLAGS += -D__NO_MINGW_LFS
endif

if WANT_FOMA
noinst_LTLIBRARIES = libfoma.la
endif

lex.yy.c: regex.l regex.h
	$(LEX) -8 regex.l

lex.lexc.c: lexc.l
	$(LEX) -8 --prefix=lexc $<

lex.interface.c: interface.l
	$(LEX) -8 --prefix=interface $<

lex.cmatrix.c: cmatrix.l
	$(LEX) -8 --prefix=cmatrix $<

regex.c regex.h: regex.y constructions.o
	$(YACC) --defines=regex.h --output=regex.c $<

libfoma_la_SOURCES = int_stack.c define.c determinize.c apply.c \
        rewrite.c lexcread.c topsort.c flags.c minimize.c reverse.c \
        extract.c sigma.c structures.c constructions.c \
        coaccessible.c io.c utf8.c spelling.c dynarray.c mem.c stringhash.c \
        trie.c lex.lexc.c lex.yy.c regex.c

EXTRA_DIST = foma.h fomalibconf.h fomalib.h lexc.h regex.h \
	cmatrix.l interface.l lexc.l regex.l regex.y
