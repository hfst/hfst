#!/bin/sh
TOOLDIR=../../tools/src
TOOL=
FORMAT_TOOL=
COMPARE_TOOL=
TXT2FST=

if [ "$1" = '--python' ]; then
    TOOL="python3 ./hfst-lexc.py"
    FORMAT_TOOL="python3 ./hfst-format.py"
    COMPARE_TOOL="python3 ./hfst-compare.py"
    TXT2FST="python3 ./hfst-txt2fst.py"
else
    TOOL=$TOOLDIR/hfst-lexc
    FORMAT_TOOL=$TOOLDIR/hfst-format
    COMPARE_TOOL=$TOOLDIR/hfst-compare
    TXT2FST=$TOOLDIR/hfst-txt2fst
    for tool in $TOOL $FORMAT_TOOL $COMPARE_TOOL $TXT2FST;
    do
	if ! test -x $tool ; then
	    echo "missing hfst-lexc, assuming configured off, skipping"
	    exit 77
	fi
    done
fi

LEXCTESTS="basic.cat-dog-bird.lexc basic.colons.lexc basic.comments.lexc
          basic.empty-sides.lexc basic.escapes.lexc
          basic.infostrings.lexc basic.initial-lexicon-empty.lexc
          basic.multichar-symbols.lexc
          basic.no-Root.lexc
          basic.multi-entry-lines.lexc basic.no-newline-at-end.lexc
          basic.punctuation.lexc basic.root-loop.lexc
          basic.spurious-lexicon.lexc basic.string-pairs.lexc
          basic.two-lexicons.lexc basic.UTF-8.lexc basic.zeros-epsilons.lexc
          basic.lowercase-lexicon-end.lexc
          basic.multichar-escaped-zero.lexc
          basic.almost-reserved-words.lexc
          basic.regexps.lexc
          hfst.weights.lexc
          xre.more-than-twice.lexc
          xre.less-than-twice.lexc
          xre.automatic-multichar-symbols.lexc xre.basic.lexc
          xre.definitions.lexc xre.months.lexc xre.nested-definitions.lexc
          xre.numeric-star.lexc xre.sharp.lexc xre.quotations.lexc
          xre.star-plus-optional.lexc
          no-newline-before-sublexicon.lexc xre.any-variations.lexc"

          # basic.lowercase-lexicon-end.lexc fails with -F with python
          # xre.any-variations.lexc # - hfst works file, foma's eliminate_flags removes valid paths (hfst-compare -e)
          # basic.end.lexc -hfst doesn't parse till end
          # xre.any-variations.lexc -foma ?:? problem
          # basic.multichar-symbol-with-0.lexc  - hfst works fine, foma wrong
          # basic.multichar-flag-with-zero.lexc - foma wrong




LEXCXFAIL="xfail.bogus.lexc xfail.ISO-8859-1.lexc xfail.lexicon-semicolon.lexc xfail.sublexicon-defined-more-than-once.lexc"

LEXCWARN="warn.sublexicon-mentioned-but-not-defined.lexc warn.one-sided-flags.lexc"

if test "$srcdir" = ""; then
    srcdir="./"
fi

for i in .sfst .ofst .foma ; do
    FFLAG=
    FNAME=
    case $i in
        .sfst)
            FNAME="sfst";
            FFLAG="-f sfst";;
        .ofst)
            FNAME="openfst-tropical";
            FFLAG="-f openfst-tropical";;
        .foma)
            FNAME="foma";
            FFLAG="-f foma";;
        *)
            FNAME=;
            FFLAG=;;
    esac

    #echo "---- $FNAME --------"

    if ! ($FORMAT_TOOL --test-format $FNAME ) ; then
        continue;
    fi

    if test -f cat$i ; then
        if ! $TOOL $FFLAG $srcdir/cat.lexc -o test 2> /dev/null; then
            echo lexc $FFLAG cat.lexc failed with $?
            exit 1
        fi
        if ! $COMPARE_TOOL -e -s cat$i test ; then
        echo "results differ: " "cat"$i" test"
            exit 1
        fi
        rm test
    fi
    for f in $LEXCTESTS $LEXCWARN ; do

        #check non-flag result
        if ! $TOOL $FFLAG $srcdir/$f -o test 2> /dev/null; then
            echo lexc $FFLAG $f failed with $?
            exit 1
        fi

        RESULT="$f.result.prolog"

     # create foma result
     #   RESULT_GZ="$RESULT.gz"
     #       echo "read lexc $srcdir/$f \n
     #       save stack test.foma.gz \n
     #       quit \n" > tmp-foma-script
     #       foma -q < tmp-foma-script
     #   gunzip test.foma.gz
     #   mv test.foma $RESULT
     #   rm tmp-foma-script

        $TXT2FST --prolog $FFLAG -i $RESULT -o $RESULT.tmp

         #echo "comparing file: $f"
         if ! $COMPARE_TOOL -e -s $RESULT.tmp test ; then
             echo "results differ: $f"
             exit 1
         fi
        rm $RESULT.tmp
        rm test


        # check flag results
        RESULT="$f.flag.result.prolog"


        if ! $TOOL -F $FFLAG $srcdir/$f -o test 2> /dev/null; then
            echo hfst-lexc -F $FFLAG $f failed with $?
            exit 1
        fi

        $TXT2FST --prolog $FFLAG -i $RESULT -o $RESULT.tmp

         #echo "comparing flag file: $f"
         if ! $COMPARE_TOOL -e -s $RESULT.tmp test ; then
             echo "flag results differ: $f: "$RESULT".tmp != test"
             exit 1
         fi
        rm $RESULT.tmp
        rm test



    done

    for f in $LEXCWARN ; do
        if $TOOL -Wall -Werror $FFLAG $srcdir/$f -o test 2> /dev/null; then
            echo lexc $FFLAG $f passed although --Werror was used
            exit 1
        fi
    done

    if ! $TOOL $FFLAG $srcdir/basic.multi-file-1.lexc \
        $srcdir/basic.multi-file-2.lexc \
        $srcdir/basic.multi-file-3.lexc -o test 2> /dev/null; then
        echo hfst-lexc $FFLAG basic.multi-file-{1,2,3}.lexc failed with $?
        exit 1
    fi
    if ! $COMPARE_TOOL -e -s walk_or_dog$i test ; then
        exit 1
    fi
done

#exit 77
