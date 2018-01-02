#!/bin/sh

mkdir $1
mkdir $1/hfst
mkdir $1/hfst/exceptions
mkdir $1/hfst/sfst_rules
mkdir $1/hfst/xerox_rules

cp README_python_mac.txt $1/README
cp _libhfst.so $1/
cp libhfst.py $1/
cp hfst/__init__.py $1/hfst/__init__.py
cp hfst/exceptions/__init__.py $1/hfst/exceptions/__init__.py
cp hfst/sfst_rules/__init__.py $1/hfst/sfst_rules/__init__.py
cp hfst/xerox_rules/__init__.py $1/hfst/xerox_rules/__init__.py

zip -r $1".zip" $1
