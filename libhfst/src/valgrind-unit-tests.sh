#!/bin/sh
VALGRIND="libtool --mode=execute valgrind --leak-check=full"
if ! test -d valgrind-logs ; then
    mkdir valgrind-logs/
fi
if test -z "$srcdir" ; then
    srcdir=./
fi
shift
for p in "$@" ; do
    $VALGRIND --log-file="valgrind-logs/${p}.log" $p
done
