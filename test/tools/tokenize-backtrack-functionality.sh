#!/bin/sh

TOOLDIR=../../tools/src

if [ "$srcdir" = "" ]; then
    srcdir="./";
fi

# Prerequisites:
if ! $TOOLDIR/hfst-lexc -q < $srcdir/tokenize-backtrack.lexc > $srcdir/tokenize-backtrack-gen.hfst; then
    echo punct backtrack gen fail
    exit 1
fi
if ! $TOOLDIR/hfst-invert < $srcdir/tokenize-backtrack-gen.hfst > $srcdir/tokenize-backtrack.hfst; then
    echo invert backtrack fail
    exit 1
fi
if ! $TOOLDIR/hfst-pmatch2fst < $srcdir/tokenize-backtrack.pmscript > $srcdir/tokenize-backtrack.pmhfst; then
    echo pmatch2fst backtrack fail
    exit 1
fi

# Only --gtd supports this:
if ! echo "busse skuvla skuvla busse Jan." | $TOOLDIR/hfst-tokenize --gtd $srcdir/tokenize-backtrack.pmhfst > test.strings ; then
    echo tokenize --gtd fail:
    cat test.strings
    exit 1
fi
if ! diff test.strings $srcdir/tokenize-backtrack-out-gtd.strings ; then
    echo diff test.strings $srcdir/tokenize-backtrack-out-gtd.strings
    exit 1
fi

if ! echo "su. su" | $TOOLDIR/hfst-tokenize --gtd $srcdir/tokenize-backtrack.pmhfst > test.strings ; then
    echo tokenize --gtd contiguous fail:
    cat test.strings
    exit 1
fi
if ! diff test.strings $srcdir/tokenize-backtrack-out-gtd-contiguous.strings ; then
    echo diff test.strings $srcdir/tokenize-backtrack-out-gtd-contiguous.strings
    exit 1
fi


rm test.strings tokenize-backtrack.pmhfst tokenize-backtrack.hfst tokenize-backtrack-gen.hfst
exit 0
