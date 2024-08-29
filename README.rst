.. -*- mode: rst -*-

==================================================================
 Helsinki Finite-State Technology (library and application suite)
==================================================================

This package contains a bridging library for multiple FST libraries and toolkits
and set of tools for processing of finite-state automata especially for
linguistic systems. HFST library and tools are licensed under GNU GPL licence
version 3, you may read the full licence in the file named COPYING. The
authors specified in AUTHORS file may be contacted about licensing issues.

For additional information, see the project page `<https://hfst.github.io>`_.

.. image:: https://travis-ci.org/hfst/hfst.svg
   :target: https://travis-ci.org/hfst/hfst

Installation
============

Installation method depends on operating system and the version you want to
install. For stable versions there exists packages for some of the better
operating system and package manager combinations. For bleeding edge newest
versions, development and non-supported operating systems and versions you
will have to perform `installation from the sources`_.

Installation packages for Debian and Ubuntu
-------------------------------------------

Debian packages for HFST are distributed via `Apertium project
<http://apertium.projectjj.com/apt/nightly/pool/main/h/hfst/>`_. This folder
contains debian packages for HFST API library, command line tools and Python bindings.
The debian packages are experimental; the requirements of debian or ubuntu
installations are same as main packages. SFST is excluded from the packages
as it has portability issues with hash_maps and hash_sets.
For installation instructions, see
`downloads <https://hfst.github.io/downloads/>`_.

Binaries for Windows
--------------------

Binaries for Windows are distributed via `Apertium project
<http://apertium.projectjj.com/win32/nightly/>`_. This folder contains
ready-compiled HFST library and command line tools.
For installation instructions, see
`downloads <https://hfst.github.io/downloads/>`_.
Python bindings for Windows are currently available as wheels for 32-bit
Python versions 2.7, 3.4, 3.5 and 3.6 on our `PyPI page
<https://pypi.python.org/pypi/hfst>`_.
Performing
`installation from the sources`_ is also possible on Windows with MinGW
and Cygwin, if Python bindings are not needed.

Binaries for Mac OS X
---------------------

Binaries for OS X are distributed via `Apertium project
<http://apertium.projectjj.com/osx/nightly/>`_. This folder contains
ready-compiled HFST library and command line tools.
For installation instructions, see
`downloads <https://hfst.github.io/downloads/>`_.
Python bindings for OS X are currently available as wheels for
Python versions 2.7, 3.4, 3.5 and 3.6 on our `PyPI page
<https://pypi.python.org/pypi/hfst>`_.
Performing
`installation from the sources`_ is also possible on Mac.


Installation for Gentoo Linux
-----------------------------

HFST software is available in *science* overlay. For portage, use::

  layman -a science

To add the repository. Then::

  emerge -av hfst

to install. Parts of the spell-checking tools are in *Finnish* overlay. If you
use Paludis, just try to ``cave resolve sci-misc/hfst`` to get the current
instructions.

Installation for other systems
------------------------------

For rest of the systems, HFST needs to be installed from the source, the
traditional GNU/linux way. To begin your installation, you need to start by
gathering the dependencies, as adviced in the following chapter.

Dependencies
============

Please note that the dependencies for the library can be set during the compile
time. Settings that determine dependencies relate to features the resulting
library will have; e.g. if you *disable* openfst you cannot use weighted
finite-state automata. If you *enable* foma, you will be able to read foma
format automata as HFST files and use foma's algorithms to process automata.
The command ``./configure --help`` lists all features that can be controlled
with configure switches and whether they are enabled or disabled by default.

Compilation requirements
------------------------

- OpenFST_ version 1.7.9 or older (newer does not work) (default)

  - to disable OpenFST support, configure switch ``--without-openfst`` may
    be used (however, this seriously limits the use of HFST)

- To use the SFST_ backend (default):

  - the SFST library version 1.4.6g is bundled with HFST and included
    by default when building HFST

  - SFST requires readline and ncurses

  - The SFST *frontend* a.k.a. the SFST-PL parser a.k.a. ``hfst-sfstpl2fst``
    does **not** require the SFST library to be installed; the library is only
    used for library-stuff like reading SFST format automata.

  - to disable SFST backend, configure switch ``--without-sfst`` must be used

- foma_ version 0.10.0 or newer (default)

  - to disable foma backend, configure switch ``--without-foma`` may be used

  - the *hfst-xfst* frontend does not require foma binary or foma library

- **ICU** version 50 or newer

- To compile corpus processing tool ``hfst-proc``, you need to use the
``configure`` switch ``--enable-proc`` (or ``--enable-all-tools``)

- To use the Python interface:

  - Compiling the bindings requires swig

  - *NOTE:* the Python API is not under autotools, you must compile it yourself;
    for more information, see file ``python/README``

Note that if you did install dependent libraries, such as libxml or ICU
to your home directory instead of using your system's package manager
(or supported default location):

- If you only have a local version of a library, you can use it with
  appropriate LDFLAGS and CXXFLAGS, eg.
  ``./configure LDFLAGS=-L/path/to/local/lib
  --prefix=/path/to/local/installation`` and
  ``make CXXFLAGS=-I/path/to/local/headers``

If you are building a development version you *loaded from the version control
system*, you must have new brand of GNU development tools installed:

- autoconf >=2.62

- automake >=1.11.1

- libtool >=2.2

- >=gettext-0.17

- GNU tool-chain is also needed with distributed packages if the user wishes to
modify Makefile.am or configure.ac files.

- Mac OS X users are advised to use MacPorts; Mac OS X 10.6 with
XCode 2.3.2 at least is not sufficient

- A package loaded from hfst web site does *not* have these requirements

The source codes *loaded from the version controls system* will also require
parse generator system:

- GNU flex 2.5.35 or compatible and

- GNU bison 1.31 (2.4 suggested) or compatible

- flex 2.5.4-2.5.33 will choke on perfectly valid rules used in hfst

- bison older than 1.31 do not support name-prefix needed for having
  multiple parsers in one library

- A package loaded from hfst web site does *not* have these requirements

- source code loaded from version control system requires them only to
  bootstrap; if you use systems with archaic versions of flex or bison and
  cannot install updates, you might be able to get the needed files from
  somewhere

When running HFST software or using HFST libraries from HFST-enabled software:

- If the executable is dynamically linked (almost always), the operating system
  *must* be able to find hfst libraries

- If you install the libraries in non-standard paths, you need to ensure
  that operating system is aware of this; In linux this may happen by setting
  ``LD_LIBRARY_PATH``, on Mac ``DYLD_LIBRARY_PATH``

- the *hfst-xfst* frontend needs GNU compatible getopt, or basic getopts
  *without* GNU-incompatible getopt installed

- the *hfst-sfstpl2fst* frontend does **not** require SFST libraries or
  binaries, you do **not** need to enable SFST libraries (via the switch
  --with-sfst) in order to compile SFST-PL scripts to HFST automata.

Installation from the sources
-----------------------------

INSTALL describes the GNU build system in detail, but for most users the usual::

    ./configure
    make
    (as root) make install

should result in a local installation and::

    (as root) make uninstall

in its uninstallation.

If you aren't going to be linking to the library after
building it and don't need to be able to send debugging information, you can
save a considerable amount of space and memory by doing::

    make install-strip

instead of make install. This strips all the symbols from the binaries,
reducing sizes by a factor of 5-10.

If you would rather install in e.g. your home directory
(or aren't the system administrator), you can tell ./configure::

        ./configure --prefix=$HOME

The HFST library may link to numerous FST handling backends with varying
licences. If you are going to redistribute the HFST library you compiled, make
sure there are no clashes in the licences of the linked libraries.

If you are checking out the development versions from GIT you must first create
and install the necessary autotools files from the host system:

  autoreconf -i

It is common practice to keep `generated files out of version control
<https://www.gnu.org/software/automake/manual/automake.html#CVS>`_.

For further installation instruction refer to file ``INSTALL``, which contains
the standard installation instructions for GNU autoconf based software.

Troubleshooting
===============

In this section we list the errors that pop up commonly on `our support channel
<irc://irc.oftc.net/hfst>`_ or in `our issue tracker
<https://github.com/hfst/hfst/issues>`_,

\::malloc has not been declared
-------------------------------

**During the compilation** errors of form::

  /usr/include/c++/4.3/cstdlib:124: error: '::malloc' has not been declared

or::

  your configure failed to find malloc, check README for further instructions

are mosts often caused by broken library installation. The simplest solution
in Linux-based platforms is ldconfig::

  ldconfig -v

This is actually told and performed by the autotools libtool library
installation, but it's easy to miss. It looks like this::

  Libraries have been installed in:
     /usr/local/lib

  If you ever happen to want to link against installed libraries
  in a given directory, LIBDIR, you must either use libtool, and
  specify the full pathname of the library, or use the '-LLIBDIR'
  flag during linking and do at least one of the following:
     - add LIBDIR to the 'LD_LIBRARY_PATH' environment variable
       during execution
     - add LIBDIR to the 'LD_RUN_PATH' environment variable
       during linking
     - use the '-Wl,-rpath -Wl,LIBDIR' linker flag
     - have your system administrator add LIBDIR to '/etc/ld.so.conf'

  See any operating system documentation about shared libraries for
  more information, such as the ld(1) and ld.so(8) manual pages.

If you installed a library on non-standard path, or installed it to the
default ``/usr/local/lib``, but your variant of Linux doesn't support libraries
there, you may need to set it up and/or ldconfig the directory explicitly::

  export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib
  ldconfig -v -n /usr/local/lib

If all else fails, try installing the library to wherever your blend of Linux
installs all its libraries, such as /usr/lib.

See also:
<http://nerdland.net/unstumping-the-internet/malloc-has-not-been-declared/> for
the gory details.

Error while loading shared libraries: libhfst.so.0: cannot open shared object file: No such file or directory
-------------------------------------------------------------------------------------------------------------

**After installing HFST and running programs**, the installed programs should
on most systems be able to find and use the shared libraries that just got
installed alongside the programs that were installed on the same go, but this
is not always the case. Typically on first installation of the HFST library
or after a major version update of HFST library, the system may not know about
it. To fix this, you must run ``ldconfig`` on GNU systems. To ensure proper
linking, use ``ldconfig -v`` to get a print out of known libraries, the
listing should include libhfst.so indicating the current version.

If this is the first time you install a library on your system by hand, it may
happen on some systems that the library configuration does not include that
directory at all. Notably on ubuntu it seems that /usr/local/lib is not one of
library directories, and that is the default target for manually installed
libraries. Either fix this by doing ``./configure --prefix=/usr`` or check
your distributions manuals on how to set up new library directories. Same
applies for libraries installed to e.g. home directory.

See also the previous error description.

fatal error: htwolcpre1.h: No such file or directory
----------------------------------------------------

**During the compilation phase**, an error message including things like::

 scanner1.ll:22:27: fatal error: htwolcpre1.h: No such file or directory
 compilation terminated.
 Makefile:1029: recipe for target `scanner1.o' failed

indicates that the header files generated by flex/yacc have the extension `.hh`
instead of `.h`. This error has been encountered at least on some versions of
Cygwin. Currently, the best solution is to manually rename each
flex/yacc-generated header file of the form `foo.hh` as `foo.h`. You probably
have to do this iteratively after each error message of this type.

....libs/lt-hfst-strings2fst: Incorrect utf-8 coding
----------------------------------------------------

**During the make check phase**. This indicates that a test that tests for
expected failure fails expectedly, which is also indicated by a green word
*PASS*  or *XFAIL* on the next line. This is expected behaviour and not an
indication of a bug. If there is a bug effecting that or other tests in the
test suite, it will be indicated by a line starting with word *FAIL* or *XPASS*
in red colour.

Same applies for other messages printed during make check phase. The cases where
something actually fails will be clearly stated in the end of the test suite by
a message such as::

  ======================================
  2 of 36 tests failed
  Please report to hfst-bugs@helsinki.fi
  ======================================

These errors can be reported either to the stated mail address or the `HFST bug
tracker on Github
<https://github.com/hfst/hfst/issues>`_.

cat: hvVqf:o:l:u:: No such file or directory
--------------------------------------------
**During use of bash-based scripts**, an error message including things like::


  cat: -l: No such file or directory
  cat: version,quiet,format:,output:,latin1::,utf8::: No such file or directory
  cat: -n: No such file or directory
  cat: hfst-lexc: No such file or directory
  cat: --: No such file or directory

indicate that script is trying to use Mac OS X's getopt as if it was GNU getopt.
However default getopt in Mac OS X does not work at all like GNU getopt.
Easiest solution is to install working getopt, e.g. by using MacPorts::

  sudo port install getopt

The newer versions of bash scripts detect Mac OS X's getopt and fallback to
using getopts. Note that getopts does not support long options and filenames
must be last parameters on commandline with it, so its use is strongly
discouraged.

libc++-abi.dylib: terminate called throwing an exception
--------------------------------------------------------

**During program execution** *(Mac OS X only)*, errors of form::

  terminate called throwing an exception
  Abort trap: 6

Can be caused by, just about any exceptional situation that does not have
specific handler. On Linux it will read::

  terminate called after throwing an instance of 'ImplementationTypeNotAvailableException'

And then you'll know that this specific exception is about backend that was disabled during `configure` phase. Or it might read::

 terminate called after throwing an instance of 'UndefinedSymbolPairsFound'

And you'd know it's something with the alphabet. But OS X won't tell us this. So
it is an unexpected error situation. Usual suspects are still: missing library
in configure, empty file, reading error, writing error...


Exception: HfstException in file: htwolcpre3-parser.yy on line: XXX
-------------------------------------------------------------------

**During program execution**::

  syntax error
  on line 1:

  Aborted.

  This is an hfst interface bug:
  Exception: HfstException in file: htwolcpre3-parser.yy on line: XXX

This can be caused by buffer size limit in hfst-twolc, a rule file larger than 10 megabytes (after pre-processing) will not be parsed properly.

Further information
===================

The `Hfst wiki site
<https://github.com/hfst/hfst/wiki>`_ contains further
details of the HFST system.

Reporting bugs
==============

Bugs can be reported via email to `HFST team bug mail address
<hfst-bugs@helsinki.fi>`_, or preferably to `HFST's bug tracking system
at Github
<https://github.com/hfst/hfst/issues>`_
When reporting, please include at least following:

* version of software used, if command-line tool (hfst-toolname --version)

* version of hfst-library, if possible

* steps to reproduce, attach or all related files if possible

* information about platform used (e.g. uname -a)

.. _libxml2: http://www.xmlsoft.org/
.. _libreadline: http://www.gnu.org/software/readline/
.. _foma: https://github.com/mhulden/foma
.. _openfst: https://www.openfst.org
.. _sfst: https://www.cis.lmu.de/~schmid/tools/SFST/

.. vim: set ft=rst:
