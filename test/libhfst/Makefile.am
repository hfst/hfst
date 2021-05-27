## Process this file with automake to produce Makefile.in
LDADD = ../../libhfst/src/libhfst.la
AM_CPPFLAGS = -I$(top_srcdir)/libhfst/src/ -I. -g

AM_CPPFLAGS += -I${top_srcdir}/back-ends/foma \
		-I${top_srcdir}/back-ends

if WANT_MINGW
  AM_CPPFLAGS += -I${top_srcdir}/back-ends/openfstwin/src/include \
		-I${top_srcdir}/back-ends/dlfcn -DWINDOWS
if WANT_OPENFST_UPSTREAM
# Skip extra include
else
  AM_CPPFLAGS += -I${top_srcdir}/back-ends/openfst/src/include
endif
endif

AM_CXXFLAGS = -Wno-deprecated

# programs to build before unit etc. testing
check_PROGRAMS=test_rules test_constructors test_streams test_tokenizer \
test_transducer_functions test_hfst_basic_transducer test_flag_diacritics \
test_examples

# sources for programs
test_rules_SOURCES=test_rules.cc
test_constructors_SOURCES=test_constructors.cc
test_streams_SOURCES=test_streams.cc
test_tokenizer_SOURCES=test_tokenizer.cc
test_transducer_functions_SOURCES=test_transducer_functions.cc
test_hfst_basic_transducer_SOURCES=test_hfst_basic_transducer.cc
test_flag_diacritics_SOURCES=test_flag_diacritics.cc
test_examples_SOURCES=test_examples.cc
noinst_HEADERS=auxiliary_functions.cc

# programs to run for unit etc. testing
TESTS=test_rules test_constructors test_streams test_tokenizer \
test_transducer_functions test_hfst_basic_transducer test_flag_diacritics \
test_examples

# files needed for test programs
EXTRA_DIST=foobar.att test_transducers.att test_lexc.lexc test_lexc_fail.lexc

clean-local:
	-rm -f *.hfst
