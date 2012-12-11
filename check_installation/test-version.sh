#!/bin/sh
TOOLSDIR=`which hfst-repeat | sed 's/hfst-repeat//'`

EXT=
if (uname | egrep "MINGW|mingw" 2>1 > /dev/null); then
    EXT=".exe";
fi

for f in $TOOLSDIR/hfst-* ; do
    if [ "$f" != "$TOOLDIR/hfst-lexc""$EXT" -a \
         "$f" != "$TOOLDIR/hfst-lexc2fst""$EXT" -a \
         "$f" != "$TOOLDIR/hfst-xfst2fst""$EXT" -a \
	 "$f" != "$TOOLDIR/hfst-lexc-compiler""$EXT" -a \
	 "$f" != "$TOOLDIR/hfst-pmatch""$EXT" -a \
	 "$f" != "$TOOLDIR/hfst-preprocess-for-optimized-lookup-format""$EXT" -a \
	 "$f" != "$TOOLDIR/hfst-duplicate""$EXT" -a \
	 "$f" != "$TOOLDIR/hfst-strip-header""$EXT" -a \
	 "$f" != "$TOOLDIR/hfst-info""$EXT" ] ; then
        if [ -x "$f" -a ! -d "$f" ] ; then
            if ! "$f" --version > version.out ; then
                rm version.out
                echo $f has broken version
                exit 1
            fi
            if ! grep -q 'hfst-[^ ]\+ [0-9.]\+' version.out > /dev/null \
            ; then
                rm version.out
                echo $f has malformed version
                exit 1
            fi
        fi
    fi
done
rm version.out
