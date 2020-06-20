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

SUBDIRS=hfst-proc hfst-twolc hfst-tagger parsers
AUTOMAKE_OPTIONS=std-options subdir-objects
LDADD = $(top_builddir)/libhfst/src/libhfst.la $(ICU_LIBS)
AM_CPPFLAGS = -I${top_srcdir}/libhfst/src -I${top_srcdir}/libhfst/src/parsers -I${top_srcdir}/tools/src/parsers -Wno-sign-compare ${GLIB_CPPFLAGS} ${ICU_CPPFLAGS}
AM_CXXFLAGS = -Wno-deprecated -Wno-sign-compare

# sort alphabetically:
if WANT_AFFIX_GUESSIFY
MAYBE_AFFIX_GUESSIFY=hfst-affix-guessify$(EXEEXT)
endif
if WANT_COMPARE
MAYBE_COMPARE=hfst-compare$(EXEEXT)
endif
if WANT_COMPOSE
MAYBE_COMPOSE=hfst-compose$(EXEEXT)
endif
if WANT_COMPOSE_INTERSECT
MAYBE_COMPOSE_INTERSECT=hfst-compose-intersect$(EXEEXT)
endif
if WANT_CONCATENATE
MAYBE_CONCATENATE=hfst-concatenate$(EXEEXT)
endif
if WANT_CONJUNCT
MAYBE_CONJUNCT=hfst-conjunct$(EXEEXT)
endif
if WANT_DETERMINIZE
MAYBE_DETERMINIZE=hfst-determinize$(EXEEXT)
endif
if WANT_DISJUNCT
MAYBE_DISJUNCT=hfst-disjunct$(EXEEXT)
endif
if WANT_MULTIPLY
MAYBE_MULTIPLY=hfst-multiply$(EXEEXT)
endif
if WANT_EXPAND_EQUIVALENCES
MAYBE_EXPAND_EQUIVALENCES=hfst-expand-equivalences$(EXEEXT)
endif
if WANT_EDIT_METADATA
MAYBE_EDIT_METADATA=hfst-edit-metadata$(EXEEXT)
endif
if WANT_ELIMINATE_FLAGS
MAYBE_ELIMINATE_FLAGS=hfst-eliminate-flags$(EXEEXT)
endif
if WANT_FORMAT
MAYBE_FORMAT=hfst-format$(EXEEXT)
endif
if WANT_FST2FST
MAYBE_FST2FST=hfst-fst2fst$(EXEEXT)
endif
if WANT_FST2STRINGS
MAYBE_FST2STRINGS=hfst-fst2strings$(EXEEXT)
endif
if WANT_FST2TXT
MAYBE_FST2TXT=hfst-fst2txt$(EXEEXT)
endif
if WANT_GREP
MAYBE_GREP=hfst-grep$(EXEEXT)
endif
if WANT_GUESS
MAYBE_GUESS=hfst-guess$(EXEEXT)
endif
if WANT_GUESSIFY
MAYBE_GUESSIFY=hfst-guessify$(EXEEXT)
endif
if WANT_HEAD
MAYBE_HEAD=hfst-head$(EXEEXT)
endif
if WANT_INFO
MAYBE_INFO=hfst-info$(EXEEXT)
endif
if WANT_INVERT
MAYBE_INVERT=hfst-invert$(EXEEXT)
endif
if GENERATE_LEXC_WRAPPER
MAYBE_LEXC_WRAPPER=hfst-lexc-wrapper$(EXEEXT)
endif
if WANT_LEXC
MAYBE_LEXC=hfst-lexc$(EXEEXT)
endif
if WANT_LOOKUP
MAYBE_LOOKUP=hfst-lookup$(EXEEXT)
endif
if WANT_FLOOKUP
MAYBE_FLOOKUP=hfst-flookup$(EXEEXT)
endif
if WANT_PAIR_TEST
MAYBE_PAIR_TEST=hfst-pair-test$(EXEEXT)
endif
if WANT_MINIMIZE
MAYBE_MINIMIZE=hfst-minimize$(EXEEXT)
endif
if WANT_NAME
MAYBE_NAME=hfst-name$(EXEEXT)
endif
if WANT_OPTIMIZED_LOOKUP
MAYBE_OPTIMIZED_LOOKUP=hfst-optimized-lookup$(EXEEXT)
endif
if WANT_PMATCH
MAYBE_PMATCH=hfst-pmatch$(EXEEXT)
endif
if WANT_PMATCH2FST
MAYBE_PMATCH2FST=hfst-pmatch2fst$(EXEEXT)
endif
if WANT_TOKENIZE
MAYBE_TOKENIZE=hfst-tokenize$(EXEEXT)
endif
if WANT_PROJECT
MAYBE_PROJECT=hfst-project$(EXEEXT)
endif
if WANT_PRUNE_ALPHABET
MAYBE_PRUNE_ALPHABET=hfst-prune-alphabet$(EXEEXT)
endif
if WANT_PUSH_LABELS
MAYBE_PUSH_LABELS=hfst-push-labels$(EXEEXT)
endif
if WANT_PUSH_WEIGHTS
MAYBE_PUSH_WEIGHTS=hfst-push-weights$(EXEEXT)
endif
if WANT_REALIGN
MAYBE_REALIGN=hfst-realign$(EXEEXT)
endif
if WANT_REGEXP2FST
MAYBE_REGEXP2FST=hfst-regexp2fst$(EXEEXT)
endif
if WANT_REMOVE_EPSILONS
MAYBE_REMOVE_EPSILONS=hfst-remove-epsilons$(EXEEXT)
endif
if WANT_REPEAT
MAYBE_REPEAT=hfst-repeat$(EXEEXT)
endif
if WANT_REVERSE
MAYBE_REVERSE=hfst-reverse$(EXEEXT)
endif
if WANT_REWEIGHT
MAYBE_REWEIGHT=hfst-reweight$(EXEEXT)
endif
if WANT_SHUFFLE
MAYBE_SHUFFLE=hfst-shuffle$(EXEEXT)
endif
if WANT_SPLIT
MAYBE_SPLIT=hfst-split$(EXEEXT)
endif
if WANT_STRINGS2FST
MAYBE_STRINGS2FST=hfst-strings2fst$(EXEEXT)
endif
if WANT_SUBTRACT
MAYBE_SUBTRACT=hfst-subtract$(EXEEXT)
endif
if WANT_SUMMARIZE
MAYBE_SUMMARIZE=hfst-summarize$(EXEEXT)
endif
if WANT_TAIL
MAYBE_TAIL=hfst-tail$(EXEEXT)
endif
if WANT_TRAVERSE
MAYBE_TRAVERSE=hfst-traverse$(EXEEXT)
endif
if WANT_TXT2FST
MAYBE_TXT2FST=hfst-txt2fst$(EXEEXT)
endif
if WANT_CALCULATE
MAYBE_CALCULATE=hfst-calculate$(EXEEXT)
endif
if WANT_SUBSTITUTE
MAYBE_SUBSTITUTE=hfst-substitute$(EXEEXT)
endif

noinst_HEADERS= \
	calculate_functions.h hfst-commandline.h hfst-program-options.h \
	inc/check-params-binary.h inc/check-params-common.h \
	inc/check-params-unary.h  inc/getopt-cases-binary.h \
	inc/getopt-cases-common.h inc/getopt-cases-error.h  \
	inc/getopt-cases-unary.h  inc/globals-binary.h      \
	inc/globals-common.h      inc/globals-unary.h \
	hfst-file-to-mem.h \
	hfst-tool-metadata.h hfst-optimized-lookup.h \
	guessify_fst.h generate_model_forms.h
# sort in abc order
# finish with xyz
# see <http://en.wikipedia.org/wiki/Alphabets#Alphabetic_order> for details
bin_PROGRAMS= \
			 $(MAYBE_AFFIX_GUESSIFY)              \
			 $(MAYBE_COMPARE) $(MAYBE_COMPOSE) $(MAYBE_CONCATENATE)     \
			 $(MAYBE_CONJUNCT) $(MAYBE_DETERMINIZE) $(MAYBE_DISJUNCT)   \
			 $(MAYBE_MULTIPLY) $(MAYBE_EDIT_METADATA)                  \
			 $(MAYBE_FORMAT) $(MAYBE_ELIMINATE_FLAGS) $(MAYBE_FST2FST) \
			 $(MAYBE_FST2STRINGS)      \
			 $(MAYBE_FST2TXT) $(MAYBE_GREP) $(MAYBE_HEAD)               \
			 $(MAYBE_INVERT)                                            \
			 $(MAYBE_LOOKUP) $(MAYBE_FLOOKUP) $(MAYBE_MINIMIZE) $(MAYBE_NAME)            \
			 $(MAYBE_OPTIMIZED_LOOKUP) $(MAYBE_PMATCH)                  \
			 $(MAYBE_PMATCH2FST) $(MAYBE_TOKENIZE)                         \
			 $(MAYBE_PROJECT) $(MAYBE_PRUNE_ALPHABET)                   \
			 $(MAYBE_PUSH_LABELS) $(MAYBE_PUSH_WEIGHTS) $(MAYBE_REALIGN) \
			 $(MAYBE_REGEXP2FST)                  \
			 $(MAYBE_REMOVE_EPSILONS) $(MAYBE_REPEAT) $(MAYBE_REVERSE)  \
			 $(MAYBE_REWEIGHT) $(MAYBE_SHUFFLE)                         \
			 $(MAYBE_SPLIT) $(MAYBE_STRINGS2FST) $(MAYBE_STRIP_HEADER)  \
			 $(MAYBE_SUBTRACT) $(MAYBE_SUMMARIZE) $(MAYBE_TAIL)         \
			 $(MAYBE_TXT2FST) $(MAYBE_CALCULATE) $(MAYBE_SUBSTITUTE)    \
			 $(MAYBE_LEXC) $(MAYBE_LEXC_WRAPPER)   \
			 $(MAYBE_FORMAT)                     \
			 $(MAYBE_COMPOSE_INTERSECT) $(MAYBE_NAME) $(MAYBE_PAIR_TEST)\
			 $(MAYBE_TRAVERSE) $(MAYBE_GUESSIFY) $(MAYBE_GUESS) \
			 $(MAYBE_INFO) $(MAYBE_EXPAND_EQUIVALENCES)

HFST_COMMON_SRC=hfst-program-options.cc hfst-commandline.cc hfst-tool-metadata.cc

# sort alphabetically
hfst_affix_guessify_SOURCES=hfst-affix-guessify.cc $(HFST_COMMON_SRC)
hfst_calculate_SOURCES=sfst-main.cc $(HFST_COMMON_SRC)
hfst_compare_SOURCES=hfst-compare.cc $(HFST_COMMON_SRC)
hfst_compose_intersect_SOURCES=hfst-compose-intersect.cc $(HFST_COMMON_SRC)
hfst_compose_SOURCES=hfst-compose.cc $(HFST_COMMON_SRC)
hfst_concatenate_SOURCES=hfst-concatenate.cc $(HFST_COMMON_SRC)
hfst_conjunct_SOURCES=hfst-conjunct.cc $(HFST_COMMON_SRC)
hfst_determinize_SOURCES=hfst-determinize.cc $(HFST_COMMON_SRC)
hfst_disjunct_SOURCES=hfst-disjunct.cc $(HFST_COMMON_SRC)
hfst_multiply_SOURCES=hfst-multiply.cc $(HFST_COMMON_SRC)
hfst_expand_equivalences_SOURCES=hfst-expand-equivalences.cc $(HFST_COMMON_SRC)
hfst_edit_metadata_SOURCES=hfst-edit-metadata.cc $(HFST_COMMON_SRC)
hfst_eliminate_flags_SOURCES=hfst-eliminate-flags.cc $(HFST_COMMON_SRC)
hfst_format_SOURCES=hfst-format.cc $(HFST_COMMON_SRC)
hfst_fst2fst_SOURCES=hfst-fst2fst.cc $(HFST_COMMON_SRC)
hfst_fst2strings_SOURCES=hfst-fst2strings.cc $(HFST_COMMON_SRC)
hfst_fst2txt_SOURCES=hfst-fst2txt.cc $(HFST_COMMON_SRC)
hfst_grep_SOURCES=hfst-grep.cc $(HFST_COMMON_SRC)
hfst_guess_SOURCES=hfst-guess.cc generate_model_forms.cc guessify_fst.cc $(HFST_COMMON_SRC)
hfst_guessify_SOURCES=hfst-guessify.cc guessify_fst.cc $(HFST_COMMON_SRC)
hfst_head_SOURCES=hfst-head.cc $(HFST_COMMON_SRC)
hfst_info_SOURCES=hfst-info.cc $(HFST_COMMON_SRC)
hfst_invert_SOURCES=hfst-invert.cc $(HFST_COMMON_SRC)
hfst_lexc_wrapper_SOURCES=hfst-lexc-wrapper.cc lexc-readline-ui.cc $(HFST_COMMON_SRC)
hfst_lexc_SOURCES=hfst-lexc-compiler.cc $(HFST_COMMON_SRC)
hfst_lookup_SOURCES=hfst-lookup.cc $(HFST_COMMON_SRC)
hfst_flookup_SOURCES=hfst-flookup.cc $(HFST_COMMON_SRC)
hfst_pair_test_SOURCES=hfst-pair-test.cc $(HFST_COMMON_SRC)
hfst_minimize_SOURCES=hfst-minimize.cc $(HFST_COMMON_SRC)
hfst_name_SOURCES=hfst-name.cc $(HFST_COMMON_SRC)
hfst_optimized_lookup_SOURCES=hfst-optimized-lookup.cc
hfst_pmatch_SOURCES=hfst-pmatch.cc $(HFST_COMMON_SRC)
hfst_tokenize_SOURCES=hfst-tokenize.cc $(HFST_COMMON_SRC)
hfst_project_SOURCES=hfst-project.cc $(HFST_COMMON_SRC)
hfst_prune_alphabet_SOURCES=hfst-prune-alphabet.cc $(HFST_COMMON_SRC)
hfst_push_labels_SOURCES=hfst-push-labels.cc $(HFST_COMMON_SRC)
hfst_push_weights_SOURCES=hfst-push-weights.cc $(HFST_COMMON_SRC)
hfst_realign_SOURCES=hfst-realign.cc $(HFST_COMMON_SRC)
hfst_regexp2fst_SOURCES=hfst-regexp2fst.cc hfst-file-to-mem.cc $(HFST_COMMON_SRC)
hfst_pmatch2fst_SOURCES=hfst-pmatch2fst.cc $(HFST_COMMON_SRC)
hfst_remove_epsilons_SOURCES=hfst-remove-epsilons.cc $(HFST_COMMON_SRC)
hfst_repeat_SOURCES=hfst-repeat.cc $(HFST_COMMON_SRC)
hfst_reverse_SOURCES=hfst-reverse.cc $(HFST_COMMON_SRC)
hfst_reweight_SOURCES=hfst-reweight.cc $(HFST_COMMON_SRC)
hfst_shuffle_SOURCES=hfst-shuffle.cc $(HFST_COMMON_SRC)
hfst_split_SOURCES=hfst-split.cc $(HFST_COMMON_SRC)
hfst_strings2fst_SOURCES=hfst-strings2fst.cc $(HFST_COMMON_SRC)
hfst_substitute_SOURCES=hfst-substitute.cc $(HFST_COMMON_SRC)
hfst_subtract_SOURCES=hfst-subtract.cc $(HFST_COMMON_SRC)
hfst_summarize_SOURCES=hfst-summarize.cc $(HFST_COMMON_SRC)
hfst_tail_SOURCES=hfst-tail.cc $(HFST_COMMON_SRC)
hfst_traverse_SOURCES=hfst-traverse.cc $(HFST_COMMON_SRC)
hfst_txt2fst_SOURCES=hfst-txt2fst.cc $(HFST_COMMON_SRC)

# aliases:
hfst-determinise$(EXEEXT): hfst-determinize$(EXEEXT)
install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
		$(LN_S) -f hfst-determinize$(EXEEXT) hfst-determinise$(EXEEXT) && \
		$(LN_S) -f hfst-fst2strings$(EXEEXT) hfst-expand$(EXEEXT) && \
		$(LN_S) -f hfst-conjunct$(EXEEXT) hfst-intersect$(EXEEXT) && \
		$(LN_S) -f hfst-minimize$(EXEEXT) hfst-minimise$(EXEEXT) && \
		$(LN_S) -f hfst-optimized-lookup$(EXEEXT) hfst-optimised-lookup$(EXEEXT) && \
		$(LN_S) -f hfst-subtract$(EXEEXT) hfst-minus$(EXEEXT) && \
		$(LN_S) -f hfst-calculate$(EXEEXT) hfst-sfstpl2fst$(EXEEXT) && \
		$(LN_S) -f hfst-summarize$(EXEEXT) hfst-summarise$(EXEEXT) && \
		$(LN_S) -f hfst-disjunct$(EXEEXT) hfst-union$(EXEEXT) && \
		$(LN_S) -f hfst-tokenize$(EXEEXT) hfst-proc2$(EXEEXT) && \
		$(LN_S) -f hfst-tokenize$(EXEEXT) hfst-tokenise$(EXEEXT)

uninstall-local:
	cd $(DESTDIR)$(bindir) && \
		rm hfst-determinise && \
		rm hfst-expand && \
		rm hfst-intersect && \
		rm hfst-minimise && \
		rm hfst-optimised-lookup && \
		rm hfst-minus && \
		rm hfst-sfstpl2fst && \
		rm hfst-summarise && \
		rm hfst-union && \
		rm hfst-tokenise && \
		rm hfst-proc2

hfst_calculate_CXXFLAGS= -Wall -Wno-error -Wno-sign-compare -Wno-deprecated
