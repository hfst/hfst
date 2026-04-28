#!/usr/bin/python3

"""
setup for HFST-swig
"""

import os
from setuptools import setup, Extension

# handle hfst-specific commands
import sys
local_hfst=False
if '--local-hfst' in sys.argv:
        index = sys.argv.index('--local-hfst')
        sys.argv.pop(index)
        local_hfst=True

from sys import platform

libhfst_src_path = '../libhfst/src/'
absolute_libhfst_src_path = os.path.abspath(libhfst_src_path)

extra_link_arguments = []
if platform == "darwin":
        extra_link_arguments.extend(['-mmacosx-version-min=10.7'])

# If you wish to link to the local HFST library:
if local_hfst:
        extra_link_arguments.extend(["-Wl,-rpath=" + absolute_libhfst_src_path + "/.libs"])

extra_compile_arguments = ['-std=c++17']
if platform == "darwin":
        extra_compile_arguments.extend(["-stdlib=libc++", "-mmacosx-version-min=10.7"])

# If you wish to link hfst c++ library statically, use:
# library_dirs = []
# libraries = []
# extra_objects = absolute_libhfst_src_path + "/.libs/libhfst.a"

libhfst_module = Extension('_libhfst',
                           language = "c++",
                           sources = ["libhfst.i"],
                           swig_opts = ["-c++",
                                        "-I" + absolute_libhfst_src_path, "-Wall"],
                           include_dirs = [absolute_libhfst_src_path],
                           library_dirs = [absolute_libhfst_src_path + "/.libs"],
                           libraries = ["hfst"],
                           extra_compile_args = extra_compile_arguments,
                           extra_link_args = extra_link_arguments
                           )

setup(name = 'libhfst_swig',
      version = '3.17.0_beta',
      author = 'HFST team',
      author_email = 'hfst-bugs@helsinki.fi',
      url = 'http://hfst.github.io/',
      description = 'SWIG-bound hfst interface',
      ext_modules = [libhfst_module],
      py_modules = ["libhfst"],
      packages = ["hfst", "hfst.exceptions", "hfst.sfst_rules", "hfst.xerox_rules"],
      data_files = []
      )
