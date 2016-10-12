#!/bin/sh

TOOLDIR=../../tools/src

if [ "$srcdir" = "" ]; then
    srcdir="./";
fi

# Prerequisites
if ! $TOOLDIR/hfst-lexc -q < $srcdir/tokenize-dog-in.lexc > $srcdir/tokenize-dog-gen.hfst; then
    echo lexc dog fail
    exit 1
fi
if ! $TOOLDIR/hfst-invert < $srcdir/tokenize-dog-gen.hfst > $srcdir/tokenize-dog.hfst; then
    echo invert dog fail
    exit 1
fi
if ! $TOOLDIR/hfst-pmatch2fst < $srcdir/tokenize-dog.pmscript > $srcdir/tokenize-dog.pmhfst; then
    echo pmatch2fst tokenize-dog fail
    exit 1
fi

# basic lookup
if ! echo "test dog be dog catdog" | $TOOLDIR/hfst-tokenize $srcdir/tokenize-dog.pmhfst > test.strings ; then
    echo tokenize fail:
    cat test.strings
    exit 1
fi
if ! diff test.strings $srcdir/tokenize-dog-out.strings ; then
    echo diff test.strings $srcdir/tokenize-dog-out.strings
    exit 1
fi

# --cg
if ! echo "test dog be dog catdog" | $TOOLDIR/hfst-tokenize --cg $srcdir/tokenize-dog.pmhfst > test.strings ; then
    echo tokenize --cg fail:
    cat test.strings
    exit 1
fi
if ! diff test.strings $srcdir/tokenize-dog-out-cg.strings ; then
    echo diff test.strings $srcdir/tokenize-dog-out-cg.strings 
    exit 1
fi

# --giella-cg
if ! echo "test dog be dog catdog собака" | $TOOLDIR/hfst-tokenize --giella-cg $srcdir/tokenize-dog.pmhfst > test.strings ; then
    echo tokenize --giella-cg fail:
    cat test.strings
    exit 1
fi
if ! diff test.strings $srcdir/tokenize-dog-out-giella-cg.strings ; then
    echo diff test.strings $srcdir/tokenize-dog-out-giella-cg.strings 
    exit 1
fi

# --xerox
if ! echo "test dog be dog catdog" | $TOOLDIR/hfst-tokenize --xerox $srcdir/tokenize-dog.pmhfst > test.strings ; then
    echo tokenize --xerox fail:
    cat test.strings
    exit 1
fi
if ! diff test.strings $srcdir/tokenize-dog-out-xerox.strings ; then
    echo diff test.strings $srcdir/tokenize-dog-out-xerox.strings 
    exit 1
fi


rm test.strings tokenize-dog.pmhfst tokenize-dog.hfst tokenize-dog-gen.hfst
exit 0
