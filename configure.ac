## Process this file with autoconf to produce configure script

## Copyright (C) 2010 University of Helsinki

## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Information on package
HFST_NAME=hfst
HFST_MAJOR=3
HFST_MINOR=11
HFST_EXTENSION=0
HFST_VERSION=$HFST_MAJOR.$HFST_MINOR.$HFST_EXTENSION

### When the VERSION is INCREMENTED, REMEMBER to increment the LONGVERSION too.

# for package-config pc file
LIBHFST_NAME=hfst
LIBHFST_MAJOR=3
LIBHFST_MINOR=11
LIBHFST_EXTENSION=0
LIBHFST_VERSION=$LIBHFST_MAJOR.$LIBHFST_MINOR.$LIBHFST_EXTENSION

AC_INIT([hfst], [3.11.0], [hfst-bugs@helsinki.fi], [hfst])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall std-options foreign check-news])

# For automake >= 1.12
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([libhfst/src/HfstTransducer.h])
AC_CONFIG_HEADERS([config.h libhfst/src/hfst.hpp])

AC_SUBST([LIBHFST_MAJOR],     [3])
AC_SUBST([LIBHFST_MINOR],     [11])
AC_SUBST([LIBHFST_EXTENSION], [0])
AC_SUBST([LIBHFST_VERSION],   [3.11.0])
AC_SUBST([LIBHFST_NAME],      [hfst])

# long version = version vector cast in base 10000, for automatic comparisons
# e.g.: 3.3.2 = 0003 0000 0000 + 0003 0000 + 0002

# Experiment to calculate the HFST_LONGVERSION via the other version strings:
# HFST_LONGVERSION_STRING=$LIBHFST_MAJOR * 10000 * 10000 + \
# $LIBHFST_MINOR * 10000 + $LIBHFST_EXTENSION + "L"
# NB! It turned out to be not portable, and can't be used!

AC_DEFINE([HFST_LONGVERSION], [300110000L],
          [Define to hfst version vector as long in base 10000])
AC_DEFINE([HFST_REVISION], ["$Revision$"],
          [Automatically substitute to configure.ac revision])
AC_DEFINE_UNQUOTED([HFST_STRING], ["$PACKAGE_STRING"],
                   [Define to libhfst pretty name for programs to print])

# allow backends to be removed
AC_ARG_WITH([sfst],
            [AS_HELP_STRING([--with-sfst],
                            [process unweighted fsts with SFST @<:@default=yes@:>@])],
            [],
            [with_sfst=yes])
AS_IF([test "x$with_sfst" != xno], [AC_DEFINE([HAVE_SFST], [1],
                                              [Define to compile SFST support in HFST])])
AM_CONDITIONAL([WANT_SFST], [test x$with_sfst != xno])
AC_ARG_WITH([openfst],
            [AS_HELP_STRING([--with-openfst],
                            [process weighted fsts with OpenFst @<:@default=yes@:>@])],
            [],
            [with_openfst=yes])
AS_IF([test "x$with_openfst" != xno], [AC_DEFINE([HAVE_OPENFST], [1],
                                              [Define to compile OpenFst support in HFST])])
AM_CONDITIONAL([WANT_OPENFST], [test x$with_openfst != xno])
AC_ARG_WITH([openfst-log],
            [AS_HELP_STRING([--with-openfst-log],
                            [enable logarithmic weights @<:@default=yes@:>@])],
            [],
            [with_openfst_log=yes])
AS_IF([test "x$with_openfst_log" != xno], [AC_DEFINE([HAVE_OPENFST_LOG], [1],
                                              [Define to compile OpenFst with log weight support in HFST])])
AM_CONDITIONAL([WANT_OPENFST_LOG], [test x$with_openfst_log != xno])
AC_ARG_WITH([foma],
            [AS_HELP_STRING([--with-foma],
                            [process unweighted fsts with foma @<:@default=yes@:>@])],
            [],
            [with_foma=yes])
AS_IF([test "x$with_foma" != xno], [AC_DEFINE([HAVE_FOMA], [1],
                                              [Define to compile foma support in HFST])])
AM_CONDITIONAL([WANT_FOMA], [test x$with_foma != xno])

AC_ARG_WITH([xfsm],
            [AS_HELP_STRING([--with-xfsm],
                            [process unweighted fsts with xfsm @<:@default=no@:>@])],
            [],
            [with_xfsm=no])
AS_IF([test "x$with_xfsm" != xno], [AC_DEFINE([HAVE_XFSM], [1],
                                              [Define to compile xfsm support in HFST])])
AM_CONDITIONAL([WANT_XFSM], [test x$with_xfsm != xno])

AS_IF([test "x$with_xfsm" != "xno"],
[AC_CHECK_LIB([xfsm], [main], [], AC_MSG_ERROR([xfsm back-end requested but libxfsm not found]))])

AS_IF([test "x$with_xfsm" != "xno"],
[AC_CHECK_HEADER([xfsm/xfsm_api.h], [xfsmheader_exists=yes], AC_MSG_ERROR([xfsm back-end requested but libxfsm header not found]))])

### Add here your library ###
#AC_ARG_WITH([my_transducer_library],
#            [AS_HELP_STRING([--with-my-transducer-library],
#                            [process unweighted fsts with my transducer library @<:@default=no@:>@])],
#            [],
#            [with_my_transducer_library=no])
#AS_IF([test "x$with_my_transducer_library" != xno], [AC_DEFINE([HAVE_MY_TRANSDUCER_LIBRARY], [1],
#                                              [Define to compile my transducer library support in HFST])])
#AM_CONDITIONAL([WANT_MY_TRANSDUCER_LIBRARY], [test x$with_my_transducer_library != xno])

AC_ARG_WITH([hfstol],
            [AS_HELP_STRING([--with-hfstol],
                            [run fsts with hfst optimized lookup library @<:@default=bundled@:>@])],
            [],
            [with_hfstol=bundled])
AS_IF([test "x$with_hfstol" != xno], [AC_DEFINE([HAVE_HFSTOL], [1],
                                              [Define to compile optimized lookup support in HFST])])
AM_CONDITIONAL([WANT_HFSTOL], [test x$with_hfstol != xno])

AS_IF([test "x$with_sfst" == "xno" -a "x$with_openfst" == "xno" -a "x$with_foma" == "xno" -a "x$with_hfstol" == "xno"], [AC_MSG_ERROR([must build with at least one fst backend])])

# relay unicode support to which backend library
AC_ARG_WITH([unicode_handler],
            [AS_HELP_STRING([--with-unicode-handler=backend],
                            [select library to use for unicode handling @<:@default=hfst@:>@])],
            [with_unicode_handler=$withval],
            [with_unicode_handler=hfst])
# make readline optional since mac os x has sucky implementation of readline
AC_ARG_WITH([readline],
            [AS_HELP_STRING([--with-readline],
                            [use readline for interactive programs @<:@default=no@:>@])],
            [with_readline=$withval],
            [with_readline=no])
AM_CONDITIONAL([WANT_READLINE], [test x$with_readline != xno])

AC_ARG_ENABLE([all_tools],
			  [AS_HELP_STRING([--enable-all-tools],
							  [build all tools @<:@default=no@:>@])],
			  [enable_all_tools=$enableval],
			  [enable_all_tools=no])

AC_ARG_ENABLE([no_tools],
			  [AS_HELP_STRING([--enable-no-tools],
							  [build no tools @<:@default=no@:>@])],
			  [enable_no_tools=$enableval],
			  [enable_no_tools=no])
AM_CONDITIONAL([ENABLE_NO_TOOLS], [test x$enable_no_tools != xno])

# allow programs to be built separately (sort alphabetically)
AC_ARG_ENABLE([affix_guessify],
			  [AS_HELP_STRING([--enable-affix-guessify],
							  [build affix-guessify tool @<:@default=yes@:>@])],
			  [enable_affix_guessify=$enableval],
			  [if test x$enable_no_tools != xno; then enable_affix_guessify=no; else enable_affix_guessify=yes; fi])
AM_CONDITIONAL([WANT_AFFIX_GUESSIFY], [test x$enable_affix_guessify != xno])
AC_ARG_ENABLE([calculate],
			  [AS_HELP_STRING([--enable-calculate],
							  [build calculate tool @<:@default=no@:>@])],
			  [enable_calculate=$enableval],
			  [enable_calculate=$enable_all_tools])
AM_CONDITIONAL([WANT_CALCULATE], [test x$enable_calculate != xno])
AC_ARG_ENABLE([compare],
			  [AS_HELP_STRING([--enable-compare],
							  [build compare tool @<:@default=yes@:>@])],
			  [enable_compare=$enableval],
			  [if test x$enable_no_tools != xno; then enable_compare=no; else enable_compare=yes; fi])
AM_CONDITIONAL([WANT_COMPARE], [test x$enable_compare != xno])
AC_ARG_ENABLE([compose],
			  [AS_HELP_STRING([--enable-compose],
							  [build compose tool @<:@default=yes@:>@])],
			  [enable_compose=$enableval],
			  [if test x$enable_no_tools != xno; then enable_compose=no; else enable_compose=yes; fi])
AM_CONDITIONAL([WANT_COMPOSE], [test x$enable_compose != xno])
AC_ARG_ENABLE([compose_intersect],
			  [AS_HELP_STRING([--enable-compose-intersect],
							  [build compose-intersect tool @<:@default=yes@:>@])],
			  [enable_compose_intersect=$enableval],
			  [if test x$enable_no_tools != xno; then enable_compose_intersect=no; else enable_compose_intersect=yes; fi])
AM_CONDITIONAL([WANT_COMPOSE_INTERSECT], [test x$enable_compose_intersect != xno])
AC_ARG_ENABLE([concatenate],
			  [AS_HELP_STRING([--enable-concatenate],
							  [build concatenate tool @<:@default=yes@:>@])],
			  [enable_concatenate=$enableval],
			  [if test x$enable_no_tools != xno; then enable_concatenate=no; else enable_concatenate=yes; fi])
AM_CONDITIONAL([WANT_CONCATENATE], [test x$enable_concatenate != xno])
AC_ARG_ENABLE([conjunct],
			  [AS_HELP_STRING([--enable-conjunct],
							  [build conjunct tool @<:@default=yes@:>@])],
			  [enable_conjunct=$enableval],
			  [if test x$enable_no_tools != xno; then enable_conjunct=no; else enable_conjunct=yes; fi])
AM_CONDITIONAL([WANT_CONJUNCT], [test x$enable_conjunct != xno])
AC_ARG_ENABLE([multiply],
			  [AS_HELP_STRING([--enable-multiply],
							  [build multiply tool @<:@default=yes@:>@])],
			  [enable_multiply=$enableval],
			  [if test x$enable_no_tools != xno; then enable_multiply=no; else enable_multiply=yes; fi])
AM_CONDITIONAL([WANT_MULTIPLY], [test x$enable_multiply != xno])
AC_ARG_ENABLE([determinize],
			  [AS_HELP_STRING([--enable-determinize],
							  [build determinize tool @<:@default=yes@:>@])],
			  [enable_determinize=$enableval],
			  [if test x$enable_no_tools != xno; then enable_determinize=no; else enable_determinize=yes; fi])
AM_CONDITIONAL([WANT_DETERMINIZE], [test x$enable_determinize != xno])
AC_ARG_ENABLE([disjunct],
			  [AS_HELP_STRING([--enable-disjunct],
							  [build disjunct tool @<:@default=yes@:>@])],
			  [enable_disjunct=$enableval],
			  [if test x$enable_no_tools != xno; then enable_disjunct=no; else enable_disjunct=yes; fi])
AM_CONDITIONAL([WANT_DISJUNCT], [test x$enable_disjunct != xno])
AC_ARG_ENABLE([expand_equivalences],
			  [AS_HELP_STRING([--enable-edit-metadata],
							  [build acx tool @<:@default=no@:>@])],
			  [enable_expand_equivalences=$enableval],
			  [enable_expand_equivalences=$enable_all_tools])
AM_CONDITIONAL([WANT_EXPAND_EQUIVALENCES], [test x$enable_expand_equivalences != xno])
AC_ARG_ENABLE([edit_metadata],
			  [AS_HELP_STRING([--enable-edit-metadata],
							  [build metadata tool @<:@default=yes@:>@])],
			  [enable_edit_metadata=$enableval],
			  [if test x$enable_no_tools != xno; then enable_edit_metadata=no; else enable_edit_metadata=yes; fi])
AM_CONDITIONAL([WANT_EDIT_METADATA], [test x$enable_edit_metadata != xno])
AC_ARG_ENABLE([format],
			  [AS_HELP_STRING([--enable-format],
							  [build format tool @<:@default=yes@:>@])],
			  [enable_format=$enableval],
			  [if test x$enable_no_tools != xno; then enable_format=no; else enable_format=yes; fi])
AM_CONDITIONAL([WANT_FORMAT], [test x$enable_format != xno])
AC_ARG_ENABLE([fst2fst],
			  [AS_HELP_STRING([--enable-fst2fst],
							  [build fst2fst tool @<:@default=yes@:>@])],
			  [enable_fst2fst=$enableval],
			  [if test x$enable_no_tools != xno; then enable_fst2fst=no; else enable_fst2fst=yes; fi])
AM_CONDITIONAL([WANT_FST2FST], [test x$enable_fst2fst != xno])
AC_ARG_ENABLE([fst2strings],
			  [AS_HELP_STRING([--enable-fst2strings],
							  [build fst2strings tool @<:@default=yes@:>@])],
			  [enable_fst2strings=$enableval],
			  [if test x$enable_no_tools != xno; then enable_fst2strings=no; else enable_fst2strings=yes; fi])
AM_CONDITIONAL([WANT_FST2STRINGS], [test x$enable_fst2strings != xno])
AC_ARG_ENABLE([fst2txt],
			  [AS_HELP_STRING([--enable-fst2txt],
							  [build fst2txt tool @<:@default=yes@:>@])],
			  [enable_fst2txt=$enableval],
			  [if test x$enable_no_tools != xno; then enable_fst2txt=no; else enable_fst2txt=yes; fi])
AM_CONDITIONAL([WANT_FST2TXT], [test x$enable_fst2txt != xno])
AC_ARG_ENABLE([grep],
			  [AS_HELP_STRING([--enable-grep],
							  [build grep tool @<:@default=yes@:>@])],
			  [enable_grep=$enableval],
			  [if test x$enable_no_tools != xno; then enable_grep=no; else enable_grep=yes; fi])
AM_CONDITIONAL([WANT_GREP], [test x$enable_grep != xno])
AC_ARG_ENABLE([guess],
			  [AS_HELP_STRING([--enable-guess],
							  [build guess tool @<:@default=yes@:>@])],
			  [enable_guess=$enableval],
			  [if test x$enable_no_tools != xno; then enable_guess=no; else enable_guess=yes; fi])
AM_CONDITIONAL([WANT_GUESS], [test x$enable_guess != xno])
AC_ARG_ENABLE([guessify],
			  [AS_HELP_STRING([--enable-guessify],
							  [build guessify tool @<:@default=yes@:>@])],
			  [enable_guessify=$enableval],
			  [if test x$enable_no_tools != xno; then enable_guessify=no; else enable_guessify=yes; fi])
AM_CONDITIONAL([WANT_GUESSIFY], [test x$enable_guessify != xno])
AC_ARG_ENABLE([head],
			  [AS_HELP_STRING([--enable-head],
							  [build head tool @<:@default=yes@:>@])],
			  [enable_head=$enableval],
			  [if test x$enable_no_tools != xno; then enable_head=no; else enable_head=yes; fi])
AM_CONDITIONAL([WANT_HEAD], [test x$enable_head != xno])
AC_ARG_ENABLE([info],
			  [AS_HELP_STRING([--enable-info],
							  [build info tool @<:@default=yes@:>@])],
			  [enable_info=$enableval],
			  [if test x$enable_no_tools != xno; then enable_info=no; else enable_info=yes; fi])
AM_CONDITIONAL([WANT_INFO], [test x$enable_info != xno])
AC_ARG_ENABLE([invert],
			  [AS_HELP_STRING([--enable-invert],
							  [build invert tool @<:@default=yes@:>@])],
			  [enable_invert=$enableval],
			  [if test x$enable_no_tools != xno; then enable_invert=no; else enable_invert=yes; fi])
AM_CONDITIONAL([WANT_INVERT], [test x$enable_invert != xno])
AC_ARG_ENABLE([lexc-wrapper],
              [AS_HELP_STRING([--enable-lexc-wrapper],
                              [build lexc-wrapper cli and readline @<:@default=no@:>@])],
              [enable_lexc_wrapper=$enableval],
              [enable_lexc_wrapper=$enable_all_tools])
AM_CONDITIONAL([WANT_LEXC_WRAPPER], [test "x$enable_lexc_wrapper" != xno])
AC_ARG_ENABLE([twolc],
              [AS_HELP_STRING([--enable-twolc],
                              [build twolc @<:@default=yes@:>@])],
              [enable_twolc=$enableval],
              [if test x$enable_no_tools != xno; then enable_twolc=no; else enable_twolc=yes; fi])
AM_CONDITIONAL([WANT_TWOLC], [test "x$enable_twolc" != xno])
AC_ARG_ENABLE([tagger],
              [AS_HELP_STRING([--enable-tagger],
                              [build tagger utilities @<:@default=no@:>@])],
              [enable_tagger=$enableval],
              [enable_tagger=$enable_all_tools])
AM_CONDITIONAL([WANT_TAGGER], [test "x$enable_tagger" != xno])
AC_ARG_ENABLE([lexc],
              [AS_HELP_STRING([--enable-lexc],
                              [build lexc cli @<:@default=yes@:>@])],
              [enable_lexc=$enableval],
              [if test x$enable_no_tools != xno; then enable_lexc=no; else enable_lexc=yes; fi])
AM_CONDITIONAL([WANT_LEXC], [test "x$enable_lexc" != xno])
AC_ARG_ENABLE([lookup],
			  [AS_HELP_STRING([--enable-lookup],
							  [build lookup tool @<:@default=yes@:>@])],
			  [enable_lookup=$enableval],
			  [if test x$enable_no_tools != xno; then enable_lookup=no; else enable_lookup=yes; fi])
AM_CONDITIONAL([WANT_LOOKUP], [test x$enable_lookup != xno])
AC_ARG_ENABLE([flookup],
			  [AS_HELP_STRING([--enable-flookup],
							  [build flookup tool @<:@default=yes@:>@])],
			  [enable_flookup=$enableval],
			  [if test x$enable_no_tools != xno; then enable_flookup=no; else enable_flookup=yes; fi])
AM_CONDITIONAL([WANT_FLOOKUP], [test x$enable_flookup != xno])
AC_ARG_ENABLE([pair-test],
			  [AS_HELP_STRING([--enable-pair-test],
							  [build pair-test tool @<:@default=yes@:>@])],
			  [enable_pair_test=$enableval],
			  [if test x$enable_no_tools != xno; then enable_pair_test=no; else enable_pair_test=yes; fi])
AM_CONDITIONAL([WANT_PAIR_TEST], [test x$enable_pair_test != xno])
AC_ARG_ENABLE([minimize],
			  [AS_HELP_STRING([--enable-minimize],
							  [build minimize tool @<:@default=yes@:>@])],
			  [enable_minimize=$enableval],
			  [if test x$enable_no_tools != xno; then enable_minimize=no; else enable_minimize=yes; fi])
AM_CONDITIONAL([WANT_MINIMIZE], [test x$enable_minimize != xno])
AC_ARG_ENABLE([name],
			  [AS_HELP_STRING([--enable-name],
							  [build name tool @<:@default=yes@:>@])],
			  [enable_name=$enableval],
			  [if test x$enable_no_tools != xno; then enable_name=no; else enable_name=yes; fi])
AM_CONDITIONAL([WANT_NAME], [test x$enable_name != xno])
AC_ARG_ENABLE([optimized_lookup],
			  [AS_HELP_STRING([--enable-optimized-lookup],
							  [build optimized-lookup tool @<:@default=yes@:>@])],
			  [enable_optimized_lookup=$enableval],
			  [if test x$enable_no_tools != xno; then enable_optimized_lookup=no; else enable_optimized_lookup=yes; fi])
AM_CONDITIONAL([WANT_OPTIMIZED_LOOKUP], [test x$enable_optimized_lookup != xno])
AC_ARG_ENABLE([pmatch],
			  [AS_HELP_STRING([--enable-pmatch],
							  [build pmatch tool @<:@default=yes@:>@])],
			  [enable_pmatch=$enableval],
			  [if test x$enable_no_tools != xno; then enable_pmatch=no; else enable_pmatch=yes; fi])
AM_CONDITIONAL([WANT_PMATCH], [test x$enable_pmatch != xno])
AC_ARG_ENABLE([tokenize],
			  [AS_HELP_STRING([--enable-tokenize],
							  [build tokenizing tool @<:@default=yes@:>@])],
			  [enable_tokenize=$enableval],
			  [if test x$enable_no_tools != xno; then enable_tokenize=no; else enable_tokenize=yes; fi])
AM_CONDITIONAL([WANT_TOKENIZE], [test x$enable_tokenize != xno])
AC_ARG_ENABLE([project],
			  [AS_HELP_STRING([--enable-project],
							  [build project tool @<:@default=yes@:>@])],
			  [enable_project=$enableval],
			  [if test x$enable_no_tools != xno; then enable_project=no; else enable_project=yes; fi])
AM_CONDITIONAL([WANT_PROJECT], [test x$enable_project != xno])
AC_ARG_ENABLE([prune_alphabet],
			  [AS_HELP_STRING([--enable-prune-alphabet],
							  [build prune-alphabet tool @<:@default=yes@:>@])],
			  [enable_prune_alphabet=$enableval],
			  [if test x$enable_no_tools != xno; then enable_prune_alphabet=no; else enable_prune_alphabet=yes; fi])
AM_CONDITIONAL([WANT_PRUNE_ALPHABET], [test x$enable_prune_alphabet != xno])
AC_ARG_ENABLE([proc],
			  [AS_HELP_STRING([--enable-proc],
							  [build text processing tool @<:@default=no@:>@])],
			  [enable_proc=$enableval],
			  [enable_proc=$enable_all_tools])
AM_CONDITIONAL([WANT_PROC], [test x$enable_proc != xno])
AC_ARG_ENABLE([push_weights],
			  [AS_HELP_STRING([--enable-push-weights],
							  [build push-weights tool @<:@default=yes@:>@])],
			  [enable_push_weights=$enableval],
			  [if test x$enable_no_tools != xno; then enable_push_weights=no; else enable_push_weights=yes; fi])
AM_CONDITIONAL([WANT_PUSH_WEIGHTS], [test x$enable_push_weights != xno])
AC_ARG_ENABLE([regexp2fst],
			  [AS_HELP_STRING([--enable-regexp2fst],
							  [build regexp2fst tool @<:@default=yes@:>@])],
			  [enable_regexp2fst=$enableval],
			  [if test x$enable_no_tools != xno; then enable_regexp2fst=no; else enable_regexp2fst=yes; fi])
AM_CONDITIONAL([WANT_REGEXP2FST], [test x$enable_regexp2fst != xno])
AC_ARG_ENABLE([xfst],
			  [AS_HELP_STRING([--enable-xfst],
							  [build xfst tool @<:@default=no@:>@])],
			  [enable_xfst=$enableval],
			  [enable_xfst=$enable_all_tools])
AM_CONDITIONAL([WANT_XFST], [test x$enable_xfst != xno])
AC_ARG_ENABLE([pmatch2fst],
			  [AS_HELP_STRING([--enable-pmatch2fst],
							  [build pmatch2fst tool @<:@default=yes@:>@])],
			  [enable_pmatch2fst=$enableval],
			  [if test x$enable_no_tools != xno; then enable_pmatch2fst=no; else enable_pmatch2fst=yes; fi])
AM_CONDITIONAL([WANT_PMATCH2FST], [test x$enable_pmatch2fst != xno])
AC_ARG_ENABLE([remove_epsilons],
			  [AS_HELP_STRING([--enable-remove-epsilons],
							  [build remove-epsilons tool @<:@default=yes@:>@])],
			  [enable_remove_epsilons=$enableval],
			  [if test x$enable_no_tools != xno; then enable_remove_epsilons=no; else enable_remove_epsilons=yes; fi])
AM_CONDITIONAL([WANT_REMOVE_EPSILONS], [test x$enable_remove_epsilons != xno])
AC_ARG_ENABLE([repeat],
			  [AS_HELP_STRING([--enable-repeat],
							  [build repeat tool @<:@default=yes@:>@])],
			  [enable_repeat=$enableval],
			  [if test x$enable_no_tools != xno; then enable_repeat=no; else enable_repeat=yes; fi])
AM_CONDITIONAL([WANT_REPEAT], [test x$enable_repeat != xno])
AC_ARG_ENABLE([reverse],
			  [AS_HELP_STRING([--enable-reverse],
							  [build reverse tool @<:@default=yes@:>@])],
			  [enable_reverse=$enableval],
			  [if test x$enable_no_tools != xno; then enable_reverse=no; else enable_reverse=yes; fi])
AM_CONDITIONAL([WANT_REVERSE], [test x$enable_reverse != xno])
AC_ARG_ENABLE([reweight],
			  [AS_HELP_STRING([--enable-reweight],
							  [build reweight tool @<:@default=yes@:>@])],
			  [enable_reweight=$enableval],
			  [if test x$enable_no_tools != xno; then enable_reweight=no; else enable_reweight=yes; fi])
AM_CONDITIONAL([WANT_REWEIGHT], [test x$enable_reweight != xno])
AC_ARG_ENABLE([split],
			  [AS_HELP_STRING([--enable-split],
							  [build split tool @<:@default=yes@:>@])],
			  [enable_split=$enableval],
			  [if test x$enable_no_tools != xno; then enable_split=no; else enable_split=yes; fi])
AM_CONDITIONAL([WANT_SPLIT], [test x$enable_split != xno])
AC_ARG_ENABLE([shuffle],
			  [AS_HELP_STRING([--enable-shuffle],
							  [build shuffle tool @<:@default=no@:>@])],
			  [enable_shuffle=$enableval],
			  [enable_shuffle=$enable_all_tools])
AM_CONDITIONAL([WANT_SHUFFLE], [test x$enable_shuffle != xno])
AC_ARG_ENABLE([strings2fst],
			  [AS_HELP_STRING([--enable-strings2fst],
							  [build strings2fst tool @<:@default=yes@:>@])],
			  [enable_strings2fst=$enableval],
			  [if test x$enable_no_tools != xno; then enable_strings2fst=no; else enable_strings2fst=yes; fi])
AM_CONDITIONAL([WANT_STRINGS2FST], [test x$enable_strings2fst != xno])
#AC_ARG_ENABLE([strip_header],
#			  [AS_HELP_STRING([--enable-strip-header],
#							  [build strip-header tool @<:@default=yes@:>@])],
#			  [enable_strip_header=$enableval],
#			  [enable_strip_header=yes])
#AM_CONDITIONAL([WANT_STRIP_HEADER], [test x$enable_strip_header != xno])
AC_ARG_ENABLE([substitute],
			  [AS_HELP_STRING([--enable-substitute],
							  [build substitution tool @<:@default=yes@:>@])],
			  [enable_substitute=$enableval],
			  [if test x$enable_no_tools != xno; then enable_substitute=no; else enable_substitute=yes; fi])
AM_CONDITIONAL([WANT_SUBSTITUTE], [test x$enable_substitute != xno])
AC_ARG_ENABLE([subtract],
			  [AS_HELP_STRING([--enable-subtract],
							  [build subtract tool @<:@default=yes@:>@])],
			  [enable_subtract=$enableval],
			  [if test x$enable_no_tools != xno; then enable_subtract=no; else enable_subtract=yes; fi])
AM_CONDITIONAL([WANT_SUBTRACT], [test x$enable_subtract != xno])
AC_ARG_ENABLE([summarize],
			  [AS_HELP_STRING([--enable-summarize],
							  [build summarize tool @<:@default=yes@:>@])],
			  [enable_summarize=$enableval],
			  [if test x$enable_no_tools != xno; then enable_summarize=no; else enable_summarize=yes; fi])
AM_CONDITIONAL([WANT_SUMMARIZE], [test x$enable_summarize != xno])
AC_ARG_ENABLE([tail],
			  [AS_HELP_STRING([--enable-tail],
							  [build tail tool @<:@default=yes@:>@])],
			  [enable_tail=$enableval],
			  [if test x$enable_no_tools != xno; then enable_tail=no; else enable_tail=yes; fi])
AM_CONDITIONAL([WANT_TAIL], [test x$enable_tail != xno])
AC_ARG_ENABLE([traverse],
			  [AS_HELP_STRING([--enable-traverse],
							  [build traversal tool @<:@default=no@:>@])],
			  [enable_traverse=$enableval],
			  [enable_traverse=$enable_all_tools])
AM_CONDITIONAL([WANT_TRAVERSE], [test x$enable_test != xno])
AC_ARG_ENABLE([test],
			  [AS_HELP_STRING([--enable-test],
							  [build test tool @<:@default=yes@:>@])],
			  [enable_test=$enableval],
			  [if test x$enable_no_tools != xno; then enable_test=no; else enable_test=yes; fi])
AM_CONDITIONAL([WANT_TEST], [test x$enable_test != xno])
AC_ARG_ENABLE([txt2fst],
			  [AS_HELP_STRING([--enable-txt2fst],
							  [build txt2fst tool @<:@default=yes@:>@])],
			  [enable_txt2fst=$enableval],
			  [if test x$enable_no_tools != xno; then enable_txt2fst=no; else enable_txt2fst=yes; fi])
AM_CONDITIONAL([WANT_TXT2FST], [test x$enable_txt2fst != xno])
AC_ARG_ENABLE([foma_wrapper],
			  [AS_HELP_STRING([--enable-foma-wrapper],
							  [build foma wrapper tool @<:@default=no@:>@])],
			  [enable_foma_wrapper=$enableval],
			  [enable_foma_wrapper=$enable_all_tools])
AM_CONDITIONAL([WANT_FOMA_WRAPPER], [test x$enable_foma_wrapper != xno])
AC_ARG_ENABLE([xfst],
			  [AS_HELP_STRING([--enable-xfst],
							  [build xfst tool @<:@default=no@:>@])],
			  [enable_xfst=$enableval],
			  [enable_xfst=$enable_all_tools])
AM_CONDITIONAL([WANT_XFST], [test x$enable_xfst != xno])

# Check if zlib is available
AC_CHECK_LIB([z], [main], [zlib_exists=yes], [zlib_exists=no])
# Check if foma and lexc wrappers are wanted and possible to generate
AM_CONDITIONAL([GENERATE_FOMA_WRAPPER], [test "x$enable_foma_wrapper" != "xno" -a "x$with_foma" != "xno" -a "x$zlib_exists" != "xno"])
AM_CONDITIONAL([GENERATE_LEXC_WRAPPER], [test "x$enable_lexc_wrapper" != "xno" -a "x$with_foma" != "xno" -a "x$zlib_exists" != "xno"])
# The same as variables..
AS_IF([test "x$enable_foma_wrapper" != "xno" -a "x$with_foma" != "xno" -a "x$zlib_exists" != "xno"], [generate_foma_wrapper=yes], [generate_foma_wrapper=no])
AS_IF([test "x$enable_lexc_wrapper" != "xno" -a "x$with_foma" != "xno" -a "x$zlib_exists" != "xno"], [generate_lexc_wrapper=yes], [generate_lexc_wrapper=no])

# If foma wrapper is not wanted or possible to generate, set these conditionals/variables to 'no'
AM_COND_IF([GENERATE_FOMA_WRAPPER], [], [AM_CONDITIONAL([WANT_FOMA_WRAPPER], [false])])
AM_COND_IF([GENERATE_FOMA_WRAPPER], [], [enable_foma_wrapper=no])
AM_COND_IF([GENERATE_LEXC_WRAPPER], [], [AM_CONDITIONAL([WANT_LEXC_WRAPPER], [false])])
AM_COND_IF([GENERATE_LEXC_WRAPPER], [], [enable_lexc_wrapper=no])

# Disable stress tests by default
AC_ARG_ENABLE([stress-tests],
              [AS_HELP_STRING([--enable-stress-tests],
                              [include stress tests in check @<:@default=no@:>@])],
                              [enable_stress_tests=$enableval],
                              [enable_stress_tests=no])
AM_CONDITIONAL([WANT_STRESSTESTS], [test x$enable_stress_tests != xno])

# Disable compiling on windows by default
AC_ARG_ENABLE([mingw],
		[AS_HELP_STRING([--enable-mingw],
				[compile on MinGW @<:@default=no@:>@])],
				[mingw_mingw=$enableval],
				[enable_mingw=no])
AM_CONDITIONAL([WANT_MINGW], [test x$enable_mingw != xno])

# Disable fsmbook tests by default
AC_ARG_ENABLE([fsmbook-tests],
		[AS_HELP_STRING([--enable-fsmbook-tests],
				[perform fsmbook tests when make check is run @<:@default=no@:>@])],
				[enable_fsmbook_tests=$enableval],
				[enable_fsmbook_tests=no])
AM_CONDITIONAL([WANT_FSMBOOK_TESTS], [test x$enable_fsmbook_tests != xno])


AS_IF([test "x$enable_fsmbook_tests" != "xno" -a "x$enable_xfst" == "xno"],
      [AC_MSG_FAILURE([fsmbook tests are not supported if hfst-xfst is disabled; enable hfst-xfst (with --enable-xfst) when using --enable-fsmbook-tests])])

# Checks for programs
AC_PROG_CC
AC_PROG_CXX
LT_INIT
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_YACC

#Do not use *.h extension for parser header files, use newer *.hh
yacc_use_parser_h_extension=false

#Verify automake version 1.11 headers for yy files are .h, >= 1.12 uses .hh
automake_version=`automake --version | head -n 1 | cut '-d ' -f 4`
AC_DEFINE_UNQUOTED([AUTOMAKE_VERSION], [$automake_version], [Defines automake version])

if test "$automake_version" \< "1.12" ; then
      #Use *.h extension for parser header file
      yacc_use_parser_h_extension=true
      echo "automake version < 1.12, using .h extension for yacc/bison generated header files"
      AC_DEFINE([YACC_USE_PARSER_H_EXTENSION], [1], [Use *.h extension for parser header file])
else
      echo "automake version >= 1.12, using .hh extension for yacc/bison generated header files"
fi

AM_CONDITIONAL([YACC_USE_PARSER_H_EXTENSION], [test x$yacc_use_parser_h_extension = xtrue])

AC_PROG_LEX

AC_PATH_PROG([GETOPT], [getopts], [false])

AM_PATH_PYTHON([2.4],[],[false])
# if flex 3.X ever comes out, this breaks :)
LEX_VERSION=`$LEX --version`
AS_IF([test -z "`echo "$LEX_VERSION" | grep 'flex 2\.[[5-9]]\.[[3-9][4-9]]'`"],
      [AC_MSG_WARN([Building hfst successfully requires flex newer than 2.5.33 on some platforms. Unless you are building with pre-flex-generated sources, building will probably fail.])])

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PATH_PROG([DOXYGEN], [doxygen], [no])
AM_CONDITIONAL([CAN_DOXYGEN], [test x$DOXYGEN != xno])

# Checks for libraries

AC_LANG_PUSH([C++])
AS_IF([test "x$with_openfst" != "xno" -a "x$enable_mingw" == "xno"],
      [AC_CHECK_LIB([dl], [main])])

AS_IF([test "x$with_openfst" != "xno"],
      [AC_CHECK_LIB([pthread], [main])]
      [AC_CHECK_LIB([m], [main])])

AS_IF([test "x$ac_cv_lib_sfst_main" == xno -a "x$ac_cv_lib_sfst1_main" == xno],
      [AC_MSG_FAILURE([sfst tests failed (--without-sfst to disable)])])
AC_LANG_POP
AC_LANG_PUSH([C])

AM_COND_IF([GENERATE_FOMA_WRAPPER], [AC_CHECK_LIB([z], [main], [], [AC_MSG_FAILURE([foma test failed because zlib.h was not found (--without-foma to disable)])])])
AM_COND_IF([GENERATE_LEXC_WRAPPER], [AC_CHECK_LIB([z], [main], [], [AC_MSG_FAILURE([foma test failed because zlib.h was not found (--without-foma to disable)])])])

AM_CONDITIONAL(HAVE_NCURSES,[false])
AM_CONDITIONAL(HAVE_CURSES,[false])
AM_CONDITIONAL(HAVE_TERMCAP,[false])

AC_CHECK_HEADER(ncurses.h,[AC_CHECK_HEADER(term.h,[AC_CHECK_LIB(ncurses,tgetent,[AM_CONDITIONAL(HAVE_NCURSES,[true])])])])
if test x$ac_cv_lib_ncurses_tgetent != xyes ; then
   AC_CHECK_HEADER(curses.h,[AC_CHECK_HEADER(term.h,[AC_CHECK_LIB(curses,tgetent,[AM_CONDITIONAL(HAVE_CURSES,[true])])])])
   if test x$ac_cv_lib_curses_tgetent != xyes ; then
      AC_CHECK_HEADER(termcap.h,[AC_CHECK_LIB(termcap,tgetent,[AM_CONDITIONAL(HAVE_TERMCAP,[true])])])
   fi
fi

AC_LANG_POP

AS_IF([test "x$with_unicode_handler" = "xglib"],
      [AC_CHECK_LIB([glib-2.0], [main])
if test x$ac_cv_lib_glib_2_0_main != xyes ; then
   AC_MSG_FAILURE([--with-unicode-handler=glib requested but glib-2.0 not found])
fi
       AC_DEFINE([USE_GLIB_UNICODE], 1, [if using glib for unicode string handling])])
AS_IF([test "x$with_unicode_handler" = "xicu"],
      [AC_MSG_FAILURE([ICU not yet implemented (--with-unicode-handler=hfst to disable)])])
AS_IF([test "x$with_readline" = "xyes"],
      [AC_SEARCH_LIBS([readline], [readline editline],
                      [AC_DEFINE([HAVE_READLINE], 1, [if readline can be used])],
                      [AC_MSG_FAILURE([readline test failed (--without-readline to disable)])])])

### Add here your library ###
#AS_IF([test "x$with_my_transducer_library" != "xno"],
#      [AC_CHECK_LIB([my_transducer_library], [main], [],
#                [AC_MSG_FAILURE([my transducer library test failed (--without-my-transducer-library to disable)])])])

# Checks for header files
AC_CHECK_HEADERS([limits.h stdlib.h string.h error.h glob.h locale.h langinfo.h])

AC_LANG_PUSH([C++])

# Always use c++11 (flag 'c++0x' is understood also by older compilers)
# Checks for highest supported C++ standard
# But, clang++ does not accept invalid OpenFST code, so limit to C++11 for now
AX_CHECK_COMPILE_FLAG([-std=c++11], [CXXFLAGS="$CXXFLAGS -std=c++11"], [
 AS_IF([echo "$CXXFLAGS" | grep '\-std' | grep -E '((gnu\+\+)|(c\+\+))((11)|(0x)|(14)|(1y)|(17)|(1z))' > /dev/null 2> /dev/null], [], [CXXFLAGS="$CXXFLAGS -std=c++0x"])
])

AC_CHECK_HEADERS([unordered_map], [], [],
[#ifdef HAVE_UNORDERED_MAP
#include <unordered_map>
#endif
])
AC_CHECK_HEADERS([unordered_set], [], [],
[#ifdef HAVE_UNORDERED_SET
#include <unordered_set>
#endif
])

AS_IF([test "x$ac_cv_header_unordered_map" = "xno"],
            [AC_MSG_ERROR(["<unordered_map> not found, it is needed in c++ standard 11 (or higher)"])])
AS_IF([test "x$ac_cv_header_unordered_set" = "xno"],
            [AC_MSG_ERROR(["<unordered_set> not found, it is needed in c++ standard 11 (or higher)"])])

AC_CHECK_HEADERS([ext/slist])

AS_IF([test "x$with_sfst" != "xno"],
      [AC_CHECK_HEADERS([backward/hash_set ext/hash_set hash_set])]
      [AC_CHECK_HEADERS([backward/hash_map ext/hash_map hash_map])])

AC_LANG_POP
AC_LANG_PUSH([C])

AC_LANG_POP
AC_LANG_PUSH([C])
AS_IF([test "x$with_readline" = "xyes"],
      [AC_CHECK_HEADERS([readline/readline.h readline/history.h])])
AS_IF([test "x$with_unicode_handler" = "xglib"],
      [AC_PATH_PROG([PKG_CONFIG], [pkg-config])
       AS_IF([test "x$PKG_CONFIG" != "xno"],
             [AC_SUBST([GLIB_CPPFLAGS], [`$PKG_CONFIG glib-2.0 --cflags`])
              AC_SUBST([GLIB_LIBS], [`$PKG_CONFIG glib-2.0 --libs`])
             ],
             [AC_CHECK_HEADERS([glib.h], [],
                        [AC_MSG_ERROR([glib headers not found; set CPPFLAGS to -I glib install directiories or use other unicode handler instead])])])])
# remove if not needed
AS_IF([test "x$with_unicode_handler" != "xglib"],
      [AC_SUBST([GLIB_CFLAGS])])

### Add here your library ###
#AS_IF([test "x$with_my_transducer_library" != "xno"],
#      [AC_CHECK_HEADERS([my_transducer_library/MyTransducerLibrary.h])])

# Checks for structures
AC_CHECK_DECLS([program_name, program_invocation_name, program_invocation_short_name])
AC_CHECK_DECLS([rl_completion_suppress_append, rl_completion_matches])
AC_CHECK_FUNCS([floor strchr strdup strerror strncasecmp strcspn strtol strtoul error error_at_line strndup getline getdelim getopt_long strtod xstrdup set_program_name setprogname setlocale nl_langinfo])

AM_CONDITIONAL([WINDOWS], [test x$version_type = xwindows])

# Checks for system services

# config files
AC_CONFIG_FILES([Makefile doc/Makefile test/Makefile
                 test/libhfst/Makefile test/tools/Makefile
                 test/tools/fsmbook-tests/Makefile
                 libhfst/Makefile libhfst/src/Makefile
                 libhfst/src/implementations/Makefile
		 libhfst/src/parsers/Makefile
                 libhfst/hfst.pc
		 tools/Makefile tools/src/Makefile tools/src/hfst-proc/Makefile
		 tools/src/hfst-twolc/Makefile
		 tools/src/hfst-twolc/src/Makefile
		 tools/src/hfst-twolc/test/Makefile
		 tools/src/hfst-tagger/Makefile
		 tools/src/hfst-tagger/src/Makefile
		 tools/src/hfst-tagger/test/Makefile
                 tools/src/parsers/Makefile
                 tools/src/parsers/test/Makefile
		 back-ends/Makefile
		 back-ends/dlfcn/Makefile
		 back-ends/openfstwin/Makefile
		 back-ends/openfstwin/src/Makefile
		 back-ends/openfstwin/src/lib/Makefile
		 back-ends/openfst/Makefile
		 back-ends/openfst/src/Makefile
		 back-ends/openfst/src/lib/Makefile
		 back-ends/foma/Makefile
		 back-ends/sfst/Makefile
		 man/Makefile
		 python/Makefile
		 python/test/Makefile
		 scripts/Makefile])
AC_CONFIG_FILES([scripts/hfst-foma-wrapper.sh], [chmod +x scripts/hfst-foma-wrapper.sh])
AC_CONFIG_FILES([scripts/hfst-foma], [chmod +x scripts/hfst-foma])



# output
AC_OUTPUT

## keep the list alphabetically sorted (vim; !sort)
### Add your library to libhfst targets ###
#    *         with my transducer library: $with_my_transducer_library
cat <<EOF
-- Building $PACKAGE_STRING:
    * libhfst with sfst: $with_sfst
    *         with openfst: $with_openfst
    *         with openfst log weights: $with_openfst_log
    *         with foma: $with_foma
    *         with optimized lookup: $with_hfstol
    * tools including affix-guessify: $enable_affix_guessify
    *                 calculate: $enable_calculate (sfstpl2fst)
    *                 compare: $enable_compare
    *                 compose: $enable_compose
    *                 concatenate: $enable_concatenate
    *                 conjunct: $enable_conjunct (intersect)
    *                 determinize: $enable_determinize (determinise)
    *                 disjunct: $enable_disjunct (union)
    *                 foma: $enable_foma_wrapper
    *                 format: $enable_format
    *                 fst2fst: $enable_fst2fst
    *                 fst2strings: $enable_fst2strings (expand)
    *                 fst2txt: $enable_fst2txt
    *                 grep: $enable_grep
    *                 guessify: $enable_guessify
    *                 guess: $enable_guess
    *                 head: $enable_head
    *                 invert: $enable_invert
    *                 lexc: $enable_lexc
    *                 lexc-wrapper: $enable_lexc_wrapper
    *                 lookup: $enable_lookup
    *                 minimize: $enable_minimize (minimise)
    *                 name: $enable_name
    *                 optimized-lookup: $enable_optimized_lookup
    *                 pmatch: $enable_pmatch
    *                 tokenize: $enable_tokenize
    *                 proc: $enable_proc
    *                 project: $enable_project
    *                 prune-alphabet: $enable_prune_alphabet
    *                 push-weights: $enable_push_weights
    *                 regexp2fst: $enable_regexp2fst
    *                 pmatch2fst: $enable_pmatch2fst
    *                 remove-epsilons: $enable_remove_epsilons
    *                 repeat: $enable_repeat
    *                 reverse: $enable_reverse
    *                 shuffle: $enable_shuffle
    *                 split: $enable_split
    *                 strings2fst: $enable_strings2fst
    *                 substitute: $enable_substitute
    *                 subtract: $enable_subtract (minus)
    *                 summarize: $enable_summarize (summarise)
    *                 tail: $enable_tail
    *                 test: $enable_test
    *                 twolc: $enable_twolc
    *                 tagger: $enable_tagger
    *                 txt2fst: $enable_txt2fst
    *                 xfst: $enable_xfst
    * unicode parsed in corpus tools with: $with_unicode_handler
EOF
dnl stick important warnings to bottom
dnl important licensing information
dnl according to GNU GPLv2 strict (not plus) is incompatible with Apache
dnl hope end users never disable ofst
AS_IF([test "x$with_openfst" == "xno"],
      [AC_MSG_WARN([Disabling openfst backend will severely cripple libhfst])])
dnl HFST's case mapping in corpus tools is mainly MES-2 set with other characters
dnl added as we go
AS_IF([test "x$with_unicode_handler" = "xhfst"],
      [AC_MSG_WARN([HFST only supports basic unicode handling with limited case mapping tables etc.; for better support consider using glib or ICU --with-unicode-handler])])
dnl warn about missing "important" tools
AS_IF([test "x$enable_lexc" == "xno"],
      [AC_MSG_WARN([hfst-lexc is not enabled; enable with --enable-lexc])])
AS_IF([test "x$enable_twolc" == "xno"],
      [AC_MSG_WARN([hfst-twolc is not enabled; you will not be able to compile xerox twolc grammars; enable using --enable-twolc])])

AS_IF([test "x$enable_tagger" == "xno"],
      [AC_MSG_WARN([hfst-train-tagger and hfst-tag are not enabled; you will not be able to train and use taggers; enable using --enable-tagger])])
AS_IF([test "x$enable_calculate" == "xno"],
      [AC_MSG_WARN([hfst-calculate is not enabled; you will not be able to compile SFST-PL scripts; enable using --enable-calculate])])
AS_IF([test "x$enable_xfst" == "xno"],
      [AC_MSG_WARN([hfst-xfst is not enabled; you will not be able to compile XFST scripts; enable using --enable-xfst])])
AC_MSG_WARN([Python bindings for HFST are not under autotools; see python/README for instructions about how to build and install them])
