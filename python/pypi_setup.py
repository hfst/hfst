#!/usr/bin/python

"""
setup for HFST-swig
"""

import os
#from distutils.core import setup, Extension
from setuptools import setup, Extension

from sys import platform

# libhfst_src_path = '../libhfst/src/'
# absolute_libhfst_src_path = os.path.abspath(libhfst_src_path)

# TODO:
# On Windows: copy C:\pythonXY\libs\pythonXY.lib (setup already does copying?)
# On Windows: cl /LD /Fe_libhfst.pyd (setup handles this?)

# When creating pypi packages
swig_include_dir = "libhfst/src/"
# else
# swig_include_dir = absolute_libhfst_src_path

ext_swig_opts = ["-c++", "-I" + swig_include_dir, "-Wall"]
# list append fails on windows for some reason...
if platform == "win32":
    ext_swig_opts = ["-c++", "-I" + swig_include_dir, "-Wall", 
                     "-IC:\\Program Files (x86)\\Microsoft SDKs\\Windows\\v7.0A\\Include"]

# When creating pypi packages
ext_extra_link_args = []
if platform == "linux" or platform == "linux2":
    ext_extra_link_args = ['-lreadline']
# else
# ext_extra_link_args = []

# When creating pypi packages
ext_include_dirs = [".", "libhfst/src/", "back-ends/foma", "back-ends",
                    "parsers", "libhfst/src/parsers"]
if platform == "win32":
    ext_include_dirs.append("C:/Python33/include/") # TODO: use version of python that is in use
    ext_include_dirs.append("back-ends/openfstwin/src/include")
else:
    ext_include_dirs.append("back-ends/openfst/src/include")
# else
# ext_include_dirs = [absolute_libhfst_src_path]

# When creating pypi packages
ext_define_macros = [ ('HAVE_FOMA', None), ('HAVE_OPENFST', None),
                      ('HAVE_OPENFST_LOG', None) ]
if platform == "linux" or platform == "linux2" or platform == "darwin":
    ext_define_macros.append(('HAVE_READLINE', None))
    ext_define_macros.append(('HAVE_GETLINE', None))
if platform == "win32":
    for macro in ["HFSTEXPORT", "OPENFSTEXPORT", "_MSC_VER", "WINDOWS", "WIN32"]:
        ext_define_macros.append((macro, None))
# else
# ext_define_macros = []

# When creating pypi packages
ext_extra_compile_args = []
if platform == "linux" or platform == "linux2" or platform == "darwin":
    ext_extra_compile_args = ["-std=c++0x"]
if platform == "win32":
    ext_extra_compile_args = ["/EHsc"]
# else
# ext_extra_compile_args = []

# When creating pypi packages
ext_library_dirs = []
ext_libraries = []
# else
# ext_library_dirs = [absolute_libhfst_src_path + "/.libs"]
# ext_libraries = ["hfst"]

cpp = ".cc"
if platform == "win32":
    cpp = ".cpp"

openfstdir = "openfst"
if platform == "win32":
    openfstdir = "openfstwin"

# These are needed when creating pypi packages
libhfst_source_files = ["libhfst/src/parsers/XfstCompiler" + cpp,
                        "libhfst/src/HfstApply" + cpp,
                        "libhfst/src/HfstInputStream" + cpp,
                        "libhfst/src/HfstTransducer" + cpp,
                        "libhfst/src/HfstOutputStream" + cpp,
                        "libhfst/src/HfstRules" + cpp,
                        "libhfst/src/HfstXeroxRules" + cpp,
                        "libhfst/src/HfstDataTypes" + cpp,
                        "libhfst/src/HfstSymbolDefs" + cpp,
                        "libhfst/src/HfstTokenizer" + cpp,
                        "libhfst/src/HfstFlagDiacritics" + cpp,
                        "libhfst/src/HfstExceptionDefs" + cpp,
                        "libhfst/src/HarmonizeUnknownAndIdentitySymbols" + cpp,
                        "libhfst/src/HfstLookupFlagDiacritics" + cpp,
                        "libhfst/src/HfstEpsilonHandler" + cpp,
                        "libhfst/src/HfstStrings2FstTokenizer" + cpp,
                        "libhfst/src/HfstPrintDot" + cpp,
                        "libhfst/src/HfstPrintPCKimmo" + cpp,
                        "libhfst/src/hfst-string-conversions" + cpp,
                        "libhfst/src/string-utils" + cpp,
                        "libhfst/src/implementations/HfstBasicTransducer" + cpp,
                        "libhfst/src/implementations/HfstBasicTransition" + cpp,
                        "libhfst/src/implementations/ConvertTransducerFormat" + cpp,
                        "libhfst/src/implementations/HfstTropicalTransducerTransitionData" + cpp,
                        "libhfst/src/implementations/ConvertTropicalWeightTransducer" + cpp,
                        "libhfst/src/implementations/ConvertLogWeightTransducer" + cpp,
                        "libhfst/src/implementations/ConvertFomaTransducer" + cpp,
                        "libhfst/src/implementations/ConvertOlTransducer" + cpp,
                        "libhfst/src/implementations/TropicalWeightTransducer" + cpp,
                        "libhfst/src/implementations/LogWeightTransducer" + cpp,
                        "libhfst/src/implementations/FomaTransducer" + cpp,
                        "libhfst/src/implementations/HfstOlTransducer" + cpp,
                        "libhfst/src/implementations/compose_intersect/ComposeIntersectRulePair" + cpp,
                        "libhfst/src/implementations/compose_intersect/ComposeIntersectLexicon" + cpp,
                        "libhfst/src/implementations/compose_intersect/ComposeIntersectRule" + cpp,
                        "libhfst/src/implementations/compose_intersect/ComposeIntersectFst" + cpp,
                        "libhfst/src/implementations/compose_intersect/ComposeIntersectUtilities" + cpp,
                        "libhfst/src/implementations/optimized-lookup/transducer" + cpp,
                        "libhfst/src/implementations/optimized-lookup/convert" + cpp,
                        "libhfst/src/implementations/optimized-lookup/ospell" + cpp,
                        "libhfst/src/implementations/optimized-lookup/pmatch" + cpp,
                        "libhfst/src/implementations/optimized-lookup/find_epsilon_loops" + cpp,
                        "libhfst/src/parsers/xre_lex" + cpp,
                        "libhfst/src/parsers/xre_parse" + cpp,
                        "libhfst/src/parsers/pmatch_parse" + cpp,
                        "libhfst/src/parsers/pmatch_lex" + cpp,
                        "libhfst/src/parsers/lexc-parser" + cpp,
                        "libhfst/src/parsers/lexc-lexer" + cpp,
                        "libhfst/src/parsers/xfst-parser" + cpp,
                        "libhfst/src/parsers/xfst-lexer" + cpp,
                        "libhfst/src/parsers/LexcCompiler" + cpp,
                        "libhfst/src/parsers/PmatchCompiler" + cpp,
                        "libhfst/src/parsers/XreCompiler" + cpp,
                        "libhfst/src/parsers/lexc-utils" + cpp,
                        "libhfst/src/parsers/pmatch_utils" + cpp,
                        "libhfst/src/parsers/xre_utils" + cpp,
                        "libhfst/src/parsers/xfst-utils" + cpp,
                        "libhfst/src/parsers/xfst_help_message" + cpp,
                        "back-ends/" + openfstdir + "/src/lib/compat" + cpp,
                        "back-ends/" + openfstdir + "/src/lib/flags" + cpp,
                        "back-ends/" + openfstdir + "/src/lib/fst" + cpp,
                        "back-ends/" + openfstdir + "/src/lib/properties" + cpp,
                        "back-ends/" + openfstdir + "/src/lib/symbol-table" + cpp,
                        "back-ends/" + openfstdir + "/src/lib/symbol-table-ops" + cpp,
                        "back-ends/" + openfstdir + "/src/lib/util" + cpp,
                        "back-ends/foma/int_stack.c",
                        "back-ends/foma/define.c",
                        "back-ends/foma/determinize.c",
                        "back-ends/foma/apply.c",
                        "back-ends/foma/rewrite.c",
                        "back-ends/foma/topsort.c",
                        "back-ends/foma/flags.c",
                        "back-ends/foma/minimize.c",
                        "back-ends/foma/reverse.c",
                        "back-ends/foma/extract.c",
                        "back-ends/foma/sigma.c",
                        "back-ends/foma/structures.c",
                        "back-ends/foma/constructions.c",
                        "back-ends/foma/coaccessible.c",
                        "back-ends/foma/io.c",
                        "back-ends/foma/utf8.c",
                        "back-ends/foma/spelling.c",
                        "back-ends/foma/dynarray.c",
                        "back-ends/foma/mem.c",
                        "back-ends/foma/stringhash.c",
                        "back-ends/foma/trie.c",
                        "back-ends/foma/lex.yy.c",
                        "back-ends/foma/regex.c"]
# todo: see what files are actually needed...
if platform == "linux" or platform == "linux2" or platform == "darwin":
    libhfst_source_files.append("back-ends/foma/lexcread.c")
    libhfst_source_files.append("back-ends/foma/lex.lexc.c")
# else
# libhfst_source_files = []

libhfst_module = Extension('_libhfst',
                           language = "c++",
                           sources = ["libhfst.i"] + libhfst_source_files,
                           swig_opts = ext_swig_opts,
                           include_dirs = ext_include_dirs,
                           library_dirs = ext_library_dirs,
                           libraries = ext_libraries,
                           define_macros = ext_define_macros,
                           extra_link_args = ext_extra_link_args,
                           extra_compile_args = ext_extra_compile_args
                           )

# When making the windows package, replace data_files with
# ["libhfst-NN.dll", "libgcc_s_seh-1.dll"] or
# ["libhfst-NN.dll", "libgcc_s_dw2-1.dll"] or

setup(name = 'hfstpy',
      version = '3.11.0_beta',
      author = 'HFST team',
      author_email = 'hfst-bugs@helsinki.fi',
      url = 'http://hfst.github.io/',
      description = 'SWIG-bound hfst interface',
      license = 'GNU GPL3',
      ext_modules = [libhfst_module],
      py_modules = ["libhfst"],
      packages = ["hfst", "hfst.exceptions", "hfst.rules", "hfst.types"],
      data_files = []
      )
