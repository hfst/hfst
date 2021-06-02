#!/bin/sh

export srcdir=`pwd`

for tool in calculate compare compose compose-intersect concatenate conjunct determinize \
	    disjunct format fst2strings fst2txt head invert minimize pmatch2fst pmatch \
	    project prune-alphabet push-weights regexp2fst remove-epsilons \
	    repeat reverse shuffle split strings2fst substitute subtract symbol-harmonization \
	    tail txt2fst;
do
    printf "%-25s" $tool"..."
    ./$tool-functionality.sh --python
    retval=$?
    if [ $retval -eq 0 ]; then
	printf '%s\n' 'pass'
    elif [ $retval -eq 77 ]; then
	printf '%s\n' 'skip'
    else
	printf '%s\n' 'fail'
    fi
done

# one problem with python: lexc-compiler: basic.lowercase-lexicon-end.lexc

# flookup: is this needed?
# lookup: HFST 4.0: rewrite lookup and path extraction...
# optimized-lookup: --""--
# proc
# reweight
# summarize
# tokenize-backtrack
# tokenize-flushing
# tokenize

# fst2fst-functionality doesn't exist
# lexc-compiler-flags not carried out even with c++
