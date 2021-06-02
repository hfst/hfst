#!/bin/sh
#
# A script for generating static binaries for eight tools:
# hfst-xfst, hfst-lexc, hfst-lookup, hfst-optimized-lookup,
# hfst-pmatch, hfst-pmatch2fst, hfst-twolc and hfst-proc.
#

# recognize whther we are on Linux or Mac
ENVIRONMENT=      # linux/mac
SHOW_LINKS=       # ldd/otool -L
CONFIGURE_ARGS='--enable-xfst --with-readline'
DYLIB_EXTENSION=  # so/dylib
if (uname -a | grep 'Linux' > /dev/null); then
    ENVIRONMENT='Linux';
    SHOW_LINKS='ldd';
#   CONFIGURE_ARGS are ok as such
    DYLIB_EXTENSION='so';
    echo "generating for Linux environment"
elif (uname -a | grep 'Darwin' > /dev/null); then
    ENVIRONMENT='Mac';
    SHOW_LINKS='otool -L'
    CONFIGURE_ARGS=$CONFIGURE_ARGS' --disable-dependency-tracking CFLAGS="-arch i386 -arch x86_64" CXXFLAGS="-arch i386 -arch x86_64" LDFLAGS="-arch i386 -arch x86_64"'
    DYLIB_EXTENSION='dylib'
    echo "generating for Mac environment"
else
    echo "Unknown environment, exiting program"
    exit 1
fi

# compile tool given as first argument and link it statically to libhfst
compile_statically ()
{
    echo "compiling "$1"..."
    # compile tool normally but save the commands in log file
    make LDFLAGS="-static-libstdc++" $1 > LOG
    echo "linking "$1" statically..."
    # tweak the link command to make libhfst linking static
    grep 'libtool: link:' LOG | sed 's/libhfst.'$DYLIB_EXTENSION'/libhfst.a/; s/libtool: link: //' | sh -v
    if ($SHOW_LINKS .libs/$1 | grep 'libhfst'); then
        echo "static linking of libhfst failed foor tool "$1
        exit 1
    fi
}


autoreconf -i &&
./configure $CONFIGURE_ARGS && scripts/generate-cc-files.sh

# these are needed for compiling individual tools
cd back-ends && make LDFLAGS="-static-libstdc++" && cd ../libhfst/src && make LDFLAGS="-static-libstdc++"

# compile all eight tools
cd ../../tools/src/parsers &&
compile_statically hfst-xfst &&
cd ../../../tools/src &&
compile_statically hfst-lexc &&
compile_statically hfst-lookup &&
compile_statically hfst-optimized-lookup &&
compile_statically hfst-pmatch &&
compile_statically hfst-pmatch2fst &&
cd ../../tools/src/hfst-proc &&
compile_statically hfst-apertium-proc &&
cd ../../../tools/src/hfst-twolc/src &&
compile_statically htwolcpre1 &&
compile_statically htwolcpre2 &&
compile_statically htwolcpre3
cd ../../../..
