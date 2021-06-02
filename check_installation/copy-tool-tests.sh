#!/bin/sh

#
# Copy HFST command line tool tests under ../test/tools to ./tool_tests.
# The copied tests are modifed so that they use the installed versions
# of command line tools instead of the ones in ../tools/src.
#

TESTDIR=./tool_tests/

# Copy hfst3/test/tools/
cd ../test/tools
#files_to_copy=`svn list`
files_to_copy=`git ls-files`
cd ../../check_installation

if [ -d "$TESTDIR" ]; then
    rm -fr $TESTDIR;
fi
mkdir $TESTDIR

for file in $files_to_copy;
do
    cp ../test/tools/$file $TESTDIR
done

cd $TESTDIR

# Modify the tests so that they use the installed version of tools
# and refer to right files.
for file in *.sh symbol-harmonization-functionality.sh;
do
    if [ "$file" != "copy-files.sh" ]; then
	sed -i 's/$\TOOLDIR\//$1/g' $file
	sed -i 's/\$srcdir\//.\//g' $file
	sed -i 's/hfst-proc\/hfst-apertium-proc/hfst-apertium-proc/g' $file
	sed -i 's/test -x \(.*\);/which \1 2>1 > \/dev\/null;/g' $file
	sed -i 's/\$SCRIPTDIR\//$1/g' $file
	sed -i 's/-loc / /g' $file
    fi
done
# this file uses 'tooldir' instead of 'TOOLDIR'
# sed -i 's/$\tooldir\//$1/g' pmatch-tester.sh

# These tests are rewritten in directory check_installation
rm empty-input.sh
cp ../test-empty-input.sh .
rm hfst-check-version.sh
cp ../test-version.sh .
rm hfst-check-help.sh
cp ../test-help.sh .

# These tests are excluded from test/tools/Makefile.am
rm incompatible-formats.sh
rm latin-1-strings.sh
rm lexc-compiler-functionality.sh
rm lexc-compiler-flags-functionality.sh
rm test.sh
rm xfst-att-to-hfst-att.sh
rm xfst-prolog-to-hfst-att.sh
rm check-flag-results.sh

# These tests are not included in basic check
cp ../test-hfst-xfst.sh .
cp ../test-hfst-twolc.sh .

# TODO: add pmatch tests
rm pmatch-tests.sh
rm pmatch-tester.sh
rm pmatch-functionality.sh

# python bindings are tested separately
rm test-python.sh

# The tagger tools are experimental so they are not tested

cd ..
