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

# programs to run for unit etc. testing
# prefer alphabetical order, except for core tests others depend upon
# If compare doesn't function 99 % of other tests will fail anyways

# test fsmbook tests after establishing that simple tools work
SUBDIRS=. fsmbook-tests

if WANT_STRESSTESTS
STRESSES=lexc2fst-stress.sh lookup-stress.sh proc-stress.sh valgrind.sh
endif

TESTS=hfst-check-version.sh \
	hfst-check-help.sh \
	hfst-check-environment.sh \
	empty-input.sh \
	permutate-file-params.sh

if WANT_COMPARE
TESTS += compare-functionality.sh
endif
if WANT_COMPOSE
TESTS += compose-functionality.sh
endif
if WANT_CONCATENATE
TESTS += concatenate-functionality.sh
endif
if WANT_CONJUNCT
TESTS += conjunct-functionality.sh
endif
if WANT_DETERMINIZE
TESTS += determinize-functionality.sh
endif
if WANT_DISJUNCT
TESTS += disjunct-functionality.sh
endif
if WANT_FORMAT
TESTS += format-functionality.sh
endif
if WANT_FST2STRINGS
TESTS += fst2strings-functionality.sh
endif
if WANT_FST2TXT
TESTS += fst2txt-functionality.sh
endif
if WANT_HEAD
TESTS += head-functionality.sh
endif
if WANT_INVERT
TESTS += invert-functionality.sh
endif
if WANT_LOOKUP
TESTS += lookup-functionality.sh
endif
if WANT_FLOOKUP
TESTS += flookup-functionality.sh
endif
if WANT_OPTIMIZED_LOOKUP
TESTS += optimized-lookup-functionality.sh
endif
if WANT_MINIMIZE
TESTS += minimize-functionality.sh
endif
if WANT_PMATCH
TESTS += pmatch2fst-functionality.sh
TESTS += pmatch-functionality.sh
endif
if WANT_PROJECT
TESTS += project-functionality.sh
endif
if WANT_PRUNE_ALPHABET
TESTS += prune-alphabet-functionality.sh
endif
if WANT_REGEXP2FST
TESTS += regexp2fst-functionality.sh
endif
if WANT_REMOVE_EPSILONS
TESTS += remove-epsilons-functionality.sh
endif
if WANT_REPEAT
TESTS += repeat-functionality.sh
endif
if WANT_REVERSE
TESTS += reverse-functionality.sh
endif
if WANT_REWEIGHT
TESTS += reweight-functionality.sh
endif
if WANT_SPLIT
TESTS += split-functionality.sh
endif
if WANT_STRINGS2FST
TESTS += strings2fst-functionality.sh
endif
if WANT_SUBSTITUTE
TESTS += substitute-functionality.sh
endif
if WANT_SUBTRACT
TESTS += subtract-functionality.sh
endif
if WANT_SUMMARIZE
TESTS += summarize-functionality.sh
endif

if WANT_FORMAT
if WANT_COMPARE 
if WANT_COMPOSE 
if WANT_REGEXP2FST
if WANT_MINIMIZE
if WANT_CONCATENATE
if WANT_CONJUNCT
if WANT_DISJUNCT
if WANT_SUBTRACT
if WANT_STRINGS2FST
TESTS += symbol-harmonization-functionality.sh
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif

if WANT_TAIL
TESTS += tail-functionality.sh
endif
if WANT_TXT2FST
TESTS += txt2fst-functionality.sh
endif

if WANT_LEXC_WRAPPER
TESTS += lexc-wrapper-functionality.sh
endif
if WANT_LEXC
TESTS += lexc-compiler-functionality.sh
endif
if WANT_CALCULATE
TESTS += calculate-functionality.sh
endif
if WANT_SHUFFLE
TESTS += shuffle-functionality.sh
endif
if WANT_PROC
TESTS += proc-functionality.sh
endif
if WANT_TOKENIZE
TESTS += tokenize-functionality.sh tokenize-backtrack-functionality.sh
endif

TESTS += $(STRESSES)
TESTS += mismatching-input-streams.sh

# XFAIL_TESTS=incompatible-formats.sh latin-1-strings.sh

DEFAULT_CHECKS=0to3cats.hfst 2to4cats.hfst 4cats.hfst 4toINFcats.hfst \
		   cat2cat_or_CAT_uppercased.hfst cat2CAT_uppercased.hfst cat2dog.hfst \
		   cat+dog.hfst catdog.hfst cat_or_dog.hfst cat.hfst \
		   CAT_uppercased2cat.hfst CAT_uppercased.hfst cat_weight_final.hfst \
		   cat_weight_initial.hfst cat_weight_middle.hfst dog2cat.hfst \
		   dog.hfst infinitely_ambiguous.hfst infinitely_ambiguous_with_flags.hfst unification_flags.hfst unification_flags_fail.hfst non_minimal.hfst rule1.hfst \
		   rule2.hfst summarize_test1.hfst summarize_test2.hfst tac.hfst \
		   epsilon.hfst another_epsilon.hfst epsilon2cat.hfst cat2epsilon.hfst \
		   unknown-star.hfst identity-star.hfst unknown2a.hfst identity.hfst \
		   heavycat.hfst abid.hfst unk2unk.hfst unk_or_id_star.hfst a2b.hfst \
		   a2b_complement.hfst a2b_input_projection_complement.hfst empty.hfst \
		   walk_or_dog.hfst \
		   ab.hfst bc.hfst id.hfst aid.hfst idb.hfst \
		   ab_shuffle_bc.hfst id_shuffle_id.hfst aid_shuffle_idb.hfst \
		   prunable_alphabet.hfst non_prunable_alphabet_1.hfst non_prunable_alphabet_2.hfst id.hfst \
		   a2a_or_a2b_or_a2unk.hfst a2b_or_b2b_or_unk2b.hfst unk2unk_or_id.hfst \
		   a_or_id.hfst id_star_a_b_c.hfst pmatch_endtag.pmatch
OL_CHECKS=cat2dog.hfstol cat2dog.genhfstol cat_weight_final.hfstol cat_weight_ambig.hfstol \
			proc-caps.hfstol proc-caps.genhfstol \
			compounds.hfstol compounds2.hfstol
if WANT_SFST
SFST_CHECKS=0to3cats.sfst 2to4cats.sfst 4cats.sfst\
			4toINFcats.sfst cat2cat_or_CAT_uppercased.sfst \
			cat2CAT_uppercased.sfst cat2dog.sfst \
			cat+dog.sfst catdog.sfst cat_or_dog.sfst \
			cat.sfst CAT_uppercased2cat.sfst CAT_uppercased.sfst\
			cat_weight_final.sfst cat_weight_initial.sfst \
			cat_weight_middle.sfst dog2cat.sfst dog.sfst \
			infinitely_ambiguous.sfst infinitely_ambiguous_with_flags.sfst unification_flags.sfst unification_flags_fail.sfst non_minimal.sfst \
			rule1.sfst rule2.sfst summarize_test1.sfst \
			summarize_test2.sfst tac.sfst unknown2a.sfst\
			identity.sfst abid.sfst unk2unk.sfst unk_or_id_star.sfst \
			a2b.sfst a2b_complement.sfst a2b_input_projection_complement.sfst \
			empty.sfst \
			walk_or_dog.sfst ab.sfst bc.sfst id.sfst aid.sfst idb.sfst \
			ab_shuffle_bc.sfst id_shuffle_id.sfst aid_shuffle_idb.sfst \
			prunable_alphabet.sfst non_prunable_alphabet_1.sfst non_prunable_alphabet_2.sfst id.sfst \
			a2a_or_a2b_or_a2unk.sfst a2b_or_b2b_or_unk2b.sfst unk2unk_or_id.sfst \
			a_or_id.sfst id_star_a_b_c.sfst epsilon.sfst another_epsilon.sfst substituting_transducer.sfst substituted_transducer.sfst
endif
if WANT_OPENFST
OFST_CHECKS=0to3cats.ofst \
			2to4cats.ofst 4cats.ofst 4toINFcats.ofst \
			cat2cat_or_CAT_uppercased.ofst cat2CAT_uppercased.ofst \
			cat2dog.ofst cat+dog.ofst catdog.ofst \
			cat_or_dog.ofst cat.ofst CAT_uppercased2cat.ofst \
			CAT_uppercased.ofst cat_weight_final.ofst \
			cat_weight_initial.ofst cat_weight_middle.ofst \
			dog2cat.ofst dog.ofst infinitely_ambiguous.ofst infinitely_ambiguous_with_flags.ofst unification_flags.ofst unification_flags_fail.ofst \
			non_minimal.ofst rule1.ofst rule2.ofst \
			summarize_test1.ofst summarize_test2.ofst tac.ofst\
			unknown2a.ofst identity.ofst \
			heavycat.hfst abid.ofst unk2unk.ofst \
			cat2dog_0.3.ofst cat2dog_0.5.ofst empty.ofst unk_or_id_star.ofst \
			a2b.ofst a2b_complement.ofst a2b_input_projection_complement.ofst \
			walk_or_dog.ofst ab.ofst bc.ofst id.ofst aid.ofst idb.ofst \
			ab_shuffle_bc.ofst id_shuffle_id.ofst aid_shuffle_idb.ofst \
			prunable_alphabet.ofst non_prunable_alphabet_1.ofst non_prunable_alphabet_2.ofst id.ofst \
			a2a_or_a2b_or_a2unk.ofst a2b_or_b2b_or_unk2b.ofst unk2unk_or_id.ofst \
			a_or_id.ofst id_star_a_b_c.ofst epsilon.ofst another_epsilon.ofst substituting_transducer.ofst substituted_transducer.ofst
endif
if WANT_FOMA
FOMA_CHECKS=0to3cats.foma 2to4cats.foma 4cats.foma \
			4toINFcats.foma cat2cat_or_CAT_uppercased.foma \
			cat2CAT_uppercased.foma cat2dog.foma \
			cat+dog.foma catdog.foma cat_or_dog.foma \
			cat.foma CAT_uppercased2cat.foma \
			CAT_uppercased.foma cat_weight_final.foma \
			cat_weight_initial.foma cat_weight_middle.foma \
			dog2cat.foma dog.foma infinitely_ambiguous.foma infinitely_ambiguous_with_flags.foma unification_flags.foma unification_flags_fail.foma \
			non_minimal.foma rule1.foma rule2.foma \
			summarize_test1.foma summarize_test2.foma tac.foma\
			unknown2a.foma identity.foma abid.foma unk2unk.foma unk_or_id_star.foma \
			a2b.foma a2b_complement.foma a2b_input_projection_complement.foma \
			empty.foma \
			walk_or_dog.foma ab.foma bc.foma id.foma aid.foma idb.foma \
			ab_shuffle_bc.foma id_shuffle_id.foma aid_shuffle_idb.foma \
			prunable_alphabet.foma non_prunable_alphabet_1.foma non_prunable_alphabet_2.foma id.foma \
			a2a_or_a2b_or_a2unk.foma a2b_or_b2b_or_unk2b.foma unk2unk_or_id.foma \
			a_or_id.foma id_star_a_b_c.foma epsilon.foma another_epsilon.foma substituting_transducer.foma substituted_transducer.foma
endif

check_DATA=$(DEFAULT_CHECKS) $(SFST_CHECKS) $(OFST_CHECKS) $(FOMA_CHECKS) $(OL_CHECKS)

FST_TXTS=0to3cats.txt 2to4cats.txt 4cats.txt 4toINFcats.txt cat_weight_ambig.txt \
		 cat2cat_or_CAT_uppercased.txt cat2CAT_uppercased.txt \
		 cat2epsilon.txt another_epsilon.txt cat2dog.txt \
		 cat+dog.txt catdog.txt cat_or_dog.txt cat.txt CAT_uppercased2cat.txt \
		 CAT_uppercased.txt cat_weight_final.txt cat_weight_initial.txt \
		 cat_weight_middle.txt dog2cat.txt dog.txt\
		 epsilon.txt epsilon2cat.txt infinitely_ambiguous.txt infinitely_ambiguous_with_flags.txt unification_flags.txt unification_flags_fail.txt \
		 non_minimal.txt rule1.txt rule2.txt summarize_test1.txt \
		 summarize_test2.txt tac.txt proc-caps.txt \
		 unknown-star.txt identity-star.txt unknown2a.txt identity.txt\
		 heavycat.txt abid.txt unk2unk.txt cat2dog_0.3.txt cat2dog_0.5.txt empty.txt unk_or_id_star.txt \
		 a2b.txt a2b_complement.txt a2b_input_projection_complement.txt \
		 walk_or_dog.txt ab.txt bc.txt id.txt aid.txt idb.txt \
		 ab_shuffle_bc.txt id_shuffle_id.txt aid_shuffle_idb.txt \
		 compounds.txt compounds2.txt \
		 id_shuffle_id.txt aid_shuffle_idb.txt \
		 prunable_alphabet.txt non_prunable_alphabet_1.txt non_prunable_alphabet_2.txt id.txt unk2unk_or_id.txt \
		 a2a_or_a2b_or_a2unk.txt a2b_or_b2b_or_unk2b.txt a_or_id.txt id_star_a_b_c.txt \
		 substituting_transducer.txt substituted_transducer.txt
FST_STRINGS=cat.strings proc-caps-in.strings proc-caps-gen.strings \
			proc-caps-out1.strings proc-caps-out2.strings \
			proc-caps-out3.strings proc-caps-out4.strings \
			proc-caps-out5.strings \
			proc-cat-out.strings proc-cat-weighted-out.strings \
			proc-compounds-out.strings proc-compounds.strings \
			proc-compounds2-out.strings proc-compounds2.strings \
			utf-8.strings latin-1.strings \
			cat2dog.strings heavycat.strings \
			cat_weight_ambig_out.strings cat_weight_ambig_W_out.strings \
			proc-cat-NUL.strings cat_cat.strings cat_weight_ambig_xerox.strings \
			cat_weight_ambig_W_xerox.strings cat_weight_ambig_W1_xerox.strings
FST_PAIRS=cat2dog.pairs
FST_PAIRSTRINGS=cat2dog.pairstring
FST_SPACESTRINGS=cat2dog.spaces
SUBSTITUTE_TXTS=cat2dog.substitute
XRE_TXTS=cats_and_dogs.xre cats_and_dogs_semicolon.xre \
	 at_file_quote.openfst-tropical.xre \
	 at_file_quote.sfst.xre at_file_quote.foma.xre \
	 left-arrow-with-semicolon-comment.xre \
	 left-arrow-with-semicolon-many-comments.xre
PMATCH_TXTS=pmatch_blanks.txt pmatch_endtag.txt
PMATCHSCRIPTS=pmatch-tests.sh pmatch-tester.sh
LEXC_TXTS=basic.cat-dog-bird.lexc basic.colons.lexc basic.comments.lexc \
		  basic.empty-sides.lexc basic.end.lexc basic.escapes.lexc \
		  basic.infostrings.lexc basic.initial-lexicon-empty.lexc \
		  basic.multichar-symbols.lexc basic.multichar-symbol-with-0.lexc \
		  basic.multi-entry-lines.lexc basic.multi-file-1.lexc \
		  basic.multi-file-2.lexc basic.multi-file-3.lexc \
		  basic.multichar-escaped-zero.lexc \
		  basic.no-newline-at-end.lexc basic.no-Root.lexc \
		  basic.punctuation.lexc basic.root-loop.lexc \
		  basic.spurious-lexicon.lexc basic.string-pairs.lexc \
		  basic.two-lexicons.lexc basic.UTF-8.lexc basic.zeros-epsilons.lexc \
		  cat.lexc hfst.weights.lexc stress.random-lexicons-100.lexc \
		  xfail.bogus.lexc xfail.ISO-8859-1.lexc xfail.lexicon-semicolon.lexc \
		  xfail.sublexicon-defined-more-than-once.lexc \
		  xre.any-variations.lexc xre.at-file.lexc \
		  xre.automatic-multichar-symbols.lexc xre.basic.lexc \
		  xre.definitions.lexc xre.months.lexc xre.nested-definitions.lexc \
		  xre.numeric-star.lexc xre.sharp.lexc xre.quotations.lexc \
		  xre.star-plus-optional.lexc basic.lowercase-lexicon-end.lexc \
		  basic.multichar-flag-with-zero.lexc basic.almost-reserved-words.lexc \
		  basic.regexps.lexc no-newline-before-sublexicon.lexc \
		  warn.sublexicon-mentioned-but-not-defined.lexc \
		  warn.one-sided-flags.lexc

#XFST_TXTS=cat-regex.xfst
ALL_SRCS=$(FST_TXTS) $(FST_STRINGS) $(FST_PAIRS) $(FST_PAIRSTRINGS) \
		$(FST_SPACESTRINGS) $(SUBSTITUTE_TXTS) $(XRE_TEXTS) \
		$(PMATCH_TXTS)

# this compilation is used by check target before running the tests
#%.hfst: %.txt
.txt.hfst:
	${top_builddir}/tools/src/hfst-txt2fst -e '@0@' $< > $@

#%.sfst: %.txt
.txt.sfst:
	${top_builddir}/tools/src/hfst-txt2fst -f sfst -e '@0@' $< > $@

#%.ofst: %.txt
.txt.ofst:
	${top_builddir}/tools/src/hfst-txt2fst -f openfst-tropical -e '@0@' $< > $@

#%.foma: %.txt
.txt.foma:
	${top_builddir}/tools/src/hfst-txt2fst -f foma -e '@0@' $< > $@

#%.hfstol: %.txt
.txt.hfstol:
	${top_builddir}/tools/src/hfst-txt2fst -e '@0@' -i $< | ${top_builddir}/tools/src/hfst-fst2fst -w -o $@

#%.genhfstol: %.txt
.txt.genhfstol:
	${top_builddir}/tools/src/hfst-txt2fst -e '@0@' -i $< | ${top_builddir}/tools/src/hfst-invert | ${top_builddir}/tools/src/hfst-fst2fst -w -o $@

.txt.pmatch:
	${top_builddir}/tools/src/hfst-pmatch2fst $< > $@

EXTRA_FILES=cat2dog.substitute cat.strings cat.txt cats_and_dogs.xre utf-8.strings latin-1.strings c_a_t.strings dos.strings not-contains-a.xre not-contains-a-comment-emptyline.xre parallel-left-arrow-multicom-emptyline.xre  parallel-left-arrow.xre cat.prolog negative_epsilon_cycles.txt no_negative_epsilon_cycles.txt substituting_transducer.txt substituted_transducer.txt

TOKENIZE_FILES=tokenize-dog-gen.hfst \
tokenize-dog.hfst \
tokenize-dog-in.lexc \
tokenize-dog-out-cg.strings \
tokenize-dog-out-gtd.strings \
tokenize-dog-out.strings \
tokenize-dog-out-xerox.strings \
tokenize-dog.pmscript \
tokenize-backtrack-gen.hfst \
tokenize-backtrack.hfst \
tokenize-backtrack.lexc \
tokenize-backtrack-out-gtd.strings \
tokenize-backtrack.pmscript


RESULT_FILES=basic.cat-dog-bird.lexc.flag.result.prolog \
basic.cat-dog-bird.lexc.result.prolog \
basic.colons.lexc.flag.result.prolog \
basic.colons.lexc.result.prolog \
basic.comments.lexc.flag.result.prolog \
basic.comments.lexc.result.prolog \
basic.empty-sides.lexc.flag.result.prolog \
basic.empty-sides.lexc.result.prolog \
basic.escapes.lexc.flag.result.prolog \
basic.escapes.lexc.result.prolog \
basic.infostrings.lexc.flag.result.prolog \
basic.infostrings.lexc.result.prolog \
basic.initial-lexicon-empty.lexc.flag.result.prolog \
basic.initial-lexicon-empty.lexc.result.prolog \
basic.lowercase-lexicon-end.lexc.flag.result.prolog \
basic.lowercase-lexicon-end.lexc.result.prolog \
basic.multichar-symbols.lexc.flag.result.prolog \
basic.multichar-symbols.lexc.result.prolog \
basic.multi-entry-lines.lexc.flag.result.prolog \
basic.multi-entry-lines.lexc.result.prolog \
basic.multichar-escaped-zero.lexc.flag.result.prolog \
basic.multichar-escaped-zero.lexc.result.prolog \
basic.no-newline-at-end.lexc.flag.result.prolog \
basic.no-newline-at-end.lexc.result.prolog \
basic.no-Root.lexc.flag.result.prolog \
basic.no-Root.lexc.result.prolog \
basic.punctuation.lexc.flag.result.prolog \
basic.punctuation.lexc.result.prolog \
basic.almost-reserved-words.lexc.flag.result.prolog \
basic.almost-reserved-words.lexc.result.prolog \
basic.regexps.lexc.flag.result.prolog \
basic.regexps.lexc.result.prolog \
basic.root-loop.lexc.flag.result.prolog \
basic.root-loop.lexc.result.prolog \
basic.spurious-lexicon.lexc.flag.result.prolog \
basic.spurious-lexicon.lexc.result.prolog \
basic.string-pairs.lexc.flag.result.prolog \
basic.string-pairs.lexc.result.prolog \
basic.two-lexicons.lexc.flag.result.prolog \
basic.two-lexicons.lexc.result.prolog \
basic.UTF-8.lexc.flag.result.prolog \
basic.UTF-8.lexc.result.prolog \
basic.zeros-epsilons.lexc.flag.result.prolog \
basic.zeros-epsilons.lexc.result.prolog \
hfst.weights.lexc.flag.result.prolog \
hfst.weights.lexc.result.prolog \
xre.any-variations.lexc.flag.result.prolog \
xre.any-variations.lexc.result.prolog \
xre.automatic-multichar-symbols.lexc.flag.result.prolog \
xre.automatic-multichar-symbols.lexc.result.prolog \
xre.basic.lexc.flag.result.prolog \
xre.basic.lexc.result.prolog \
xre.definitions.lexc.flag.result.prolog \
xre.definitions.lexc.result.prolog \
xre.months.lexc.flag.result.prolog \
xre.months.lexc.result.prolog \
xre.nested-definitions.lexc.flag.result.prolog \
xre.nested-definitions.lexc.result.prolog \
xre.numeric-star.lexc.flag.result.prolog \
xre.numeric-star.lexc.result.prolog \
xre.quotations.lexc.flag.result.prolog \
xre.quotations.lexc.result.prolog \
xre.sharp.lexc.flag.result.prolog \
xre.sharp.lexc.result.prolog \
xre.star-plus-optional.lexc.flag.result.prolog \
xre.star-plus-optional.lexc.result.prolog \
no-newline-before-sublexicon.lexc.flag.result.prolog \
no-newline-before-sublexicon.lexc.result.prolog \
warn.sublexicon-mentioned-but-not-defined.lexc.flag.result.prolog \
warn.sublexicon-mentioned-but-not-defined.lexc.result.prolog \
warn.one-sided-flags.lexc.flag.result.prolog \
warn.one-sided-flags.lexc.result.prolog

# files needed for test programs
EXTRA_DIST=empty-file $(FST_TXTS) $(FST_STRINGS) $(FST_PAIRS) $(FST_PAIRSTRINGS) $(FST_SPACESTRINGS) $(SUBSTITUTE_TXTS) $(XRE_TXTS) $(XFST_TXTS) $(TESTS) $(EXTRA_FILES) $(LEXC_TXTS) $(PMATCH_TXTS) $(PMATCHSCRIPTS) script.xfst lexc2fst-stress.sh lookup-stress.sh proc-stress.sh valgrind.sh \
calculate-functionality.sh shuffle-functionality.sh $(RESULT_FILES) $(TOKENIZE_FILES)


valgrind: $(CHECK_DATA)
	$(srcdir)/valgrind.sh $(srcdir)

clean-local:
	-rm -f $(check_DATA)
