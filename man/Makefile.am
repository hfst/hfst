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

hfst_tools=hfst-compare.1 hfst-compose.1 \
	hfst-concatenate.1 hfst-determinize.1 \
	hfst-disjunct.1 \
	hfst-fst2fst.1 hfst-fst2strings.1 \
	hfst-fst2txt.1 \
	hfst-head.1 hfst-conjunct.1 \
	hfst-invert.1 hfst-minimize.1 hfst-project.1 \
	hfst-push-weights.1 hfst-regexp2fst.1 \
	hfst-remove-epsilons.1 hfst-repeat.1 \
	hfst-reverse.1 hfst-split.1 hfst-strings2fst.1 \
	hfst-subtract.1 hfst-summarize.1 hfst-tail.1 hfst-txt2fst.1 \
	hfst-calculate.1 hfst-compose-intersect.1 \
	hfst-lookup.1 \
	hfst-substitute.1 hfst-format.1 hfst-name.1 hfst-lexc.1 \
	hfst-affix-guessify.1 hfst-edit-metadata.1 hfst-expand-equivalences.1 \
	hfst-grep.1 hfst-guess.1 hfst-guessify.1 hfst-info.1 hfst-multiply.1 \
	hfst-pair-test.1 hfst-prune-alphabet.1 hfst-reweight.1 hfst-shuffle.1 \
	hfst-traverse.1 hfst-tokenize.1 hfst-pmatch.1 hfst-pmatch2fst.1 \
	hfst-optimized-lookup.1
hfst_xfst=hfst-xfst.1
hfst_apertium_proc=hfst-apertium-proc.1

hfst_tag_and_reweight=hfst-tag.1 hfst-reweight-tagger.1

if WANT_TRAIN_TAGGER
  hfst_train_tagger=hfst-train-tagger.1
  hfst_train_tagger_subprograms=hfst-open-input-file-for-tagger.1 \
	hfst_tagger_compute_data_statistics.py.1 hfst-build-tagger.1 # hand-written
else
  hfst_train_tagger=
  hfst_train_tagger_subprograms=
endif

hfst_twolc=hfst-twolc.1

if WANT_TWOLC_SCRIPT
  hfst_twolc_subprograms= htwolcpre1.1 htwolcpre2.1 htwolcpre3.1 hfst-twolc-script.1 # hand-written
else
  hfst_twolc_subprograms=
endif

aliases= hfst-determinise.1 hfst-expand.1 hfst-intersect.1 hfst-minimise.1 \
	 hfst-minus.1 hfst-sfstpl2fst.1 hfst-summarise.1 hfst-union.1 hfst-proc.1 \
	 hfst-optimised-lookup.1 \
	 hfst-proc2.1 hfst-tokenise.1

dist_man1_MANS=$(hfst_tools) $(hfst_xfst) $(hfst_apertium_proc) \
	$(hfst_tag_and_reweight) $(hfst_train_tagger) $(hfst_train_tagger_subprograms) $(hfst_twolc) \
	$(hfst_twolc_subprograms) $(aliases)

EXTRA_DIST=hfst-train-tagger.1 \
	hfst-open-input-file-for-tagger.1 hfst_tagger_compute_data_statistics.py.1 \
	hfst-build-tagger.1 htwolcpre1.1 htwolcpre2.1 htwolcpre3.1 hfst-twolc-script.1

LN_S=ln -s -f

hfst-determinise.1:
	$(LN_S) hfst-determinize.1 hfst-determinise.1

hfst-expand.1:
	$(LN_S) hfst-fst2strings.1 hfst-expand.1

hfst-intersect.1:
	$(LN_S) hfst-conjunct.1 hfst-intersect.1

hfst-minimise.1:
	$(LN_S) hfst-minimize.1 hfst-minimise.1

hfst-minus.1:
	$(LN_S) hfst-subtract.1 hfst-minus.1

hfst-optimised-lookup.1:
	$(LN_S) hfst-optimized-lookup.1 hfst-optimised-lookup.1

hfst-proc.1:
	$(LN_S) hfst-apertium-proc.1 hfst-proc.1

hfst-sfstpl2fst.1:
	$(LN_S) hfst-calculate.1 hfst-sfstpl2fst.1

hfst-summarise.1:
	$(LN_S) hfst-summarize.1 hfst-summarise.1

hfst-union.1:
	$(LN_S) hfst-disjunct.1 hfst-union.1

hfst-proc2.1:
	$(LN_S) hfst-tokenize.1 hfst-proc2.1

hfst-tokenise.1:
	$(LN_S) hfst-tokenize.1 hfst-tokenise.1


# Generate man pages for all tools and their aliases
man:
	make bootstrap && make aliases

# Generate man pages from help messages
bootstrap:
	for f in $(hfst_tools); do \
		N=`echo $$f | sed 's/.1/ \-\-help/' | sed 's/hfst/..\/tools\/src\/hfst/' | sh | sed -n 2p`; \
		help2man \
		-n="$$N" \
		-s 1 -o $$f -N -S HFST `echo $$f | sed 's/.1//' | sed 's/hfst/..\/tools\/src\/hfst/'`; \
	done;
	for f in $(hfst_xfst); do \
		N=`echo $$f | sed 's/.1/ \-\-help/' | sed 's/hfst/..\/tools\/src\/parsers\/hfst/' | sh | sed -n 2p`; \
		help2man \
		-n="$$N" \
		-s 1 -o $$f -N -S HFST `echo $$f | sed 's/.1//' | sed 's/hfst/..\/tools\/src\/parsers\/hfst/'`; \
	done;
#	for f in $(hfst_apertium_proc); do \
#		N=`echo $$f | sed 's/.1/ \-\-help/' | sed 's/hfst/..\/tools\/src\/hfst-proc\/hfst/' | sh | sed -n 2p`; \
#		help2man \
#		-n="$$N" \
#		-s 1 -o $$f -N -S HFST `echo $$f | sed 's/.1//' | sed 's/hfst/..\/tools\/src\/hfst-proc\/hfst/'`; \
#	done;
	for f in $(hfst_tag_and_reweight); do \
		N=`echo $$f | sed 's/.1/ \-\-help/' | sed 's/hfst/..\/tools\/src\/hfst-tagger\/src\/hfst/' | sh | sed -n 2p`; \
		help2man --no-discard-stderr \
		-n="$$N" \
		-s 1 -o $$f -N -S HFST `echo $$f | sed 's/.1//' | sed 's/hfst/..\/tools\/src\/hfst-tagger\/src\/hfst/'`; \
	done;
	for f in $(hfst_train_tagger); do \
		N=`echo "../tools/src/hfst-tagger/src/hfst-open-input-file-for-tagger --help" | sh | sed -n 2p`; \
		help2man --no-discard-stderr \
		-n="$$N" \
		-s 1 -o $$f -N -S HFST `echo "../tools/src/hfst-tagger/src/hfst-train-tagger"`; \
	done;
	for f in $(hfst_twolc); do \
		N=`echo "../tools/src/hfst-twolc/src/hfst-twolc --help" | sh | sed -n 2p`; \
		help2man --no-discard-stderr \
		-n="$$N" \
		-s 1 -o $$f -N -S HFST `echo "../tools/src/hfst-twolc/src/hfst-twolc"`; \
	done;
# hfst-foma man page is hand-written

# Create symbolic links to aliases
aliases:
	for f in $(aliases) ; do \
		make $$f; \
	done;

