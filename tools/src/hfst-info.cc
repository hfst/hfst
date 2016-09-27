//! @file hfst-format.cc
//!
//! @brief Format checking command line tool
//!
//! @author HFST Team


//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, version 3 of the License.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef WINDOWS
#include <io.h>
#endif


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>

#include "hfst.hpp"

#include <set>
#include <string>

using std::set;
using std::string;

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "hfst-tool-metadata.h"
#include "inc/globals-common.h"


static long min_version = -1L;
static long exact_version = -1L;
static long max_version = -1L;
static set<string> required_features;

static
long
parse_version_string(const char* s)
  {
    char* endptr;
    long major = strtoul(s, &endptr, 10L);
    if (*endptr == '\0')
      {
        return (major * 10000 * 10000);
      }
    else if (*endptr != '.')
      {
        error(EXIT_FAILURE, 0, "cannot parse version string from %s", endptr);
      }
    s = endptr + 1;
    long minor = strtoul(s, &endptr, 10L);
    if (*endptr == '\0')
      {
        return (major * 10000 * 10000) + (minor * 10000);
      }
    else if (*endptr != '.')
      {
        error(EXIT_FAILURE, 0, "cannot parse version string from %s", endptr);
      }
    s = endptr + 1;
    long patch = strtoul(s, &endptr, 10L);
    if (*endptr == '\0')
      {
        return (major * 10000 * 10000) + (minor * 10000) + patch;
      }
    else
      {
        error(EXIT_FAILURE, 0, "cannot parse version string from %s", endptr);
      }
    return -1L;
  }

void
print_usage()
{
        // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "show or test HFST versions and features\n"
            "\n", program_name);

    print_common_program_options(message_out);
    fprintf(message_out, "Test features:\n"
            "  -a, --atleast-version=MVER   require at least MVER version "
            "of HFST\n"
            "  -e, --exact-version=EVER     require exactly EVER version "
            "of HFST\n"
            "  -m, --max-version=UVER       require at most UVER version "
            "of HFST\n"
            "  -f, --requirefeature=FEAT    require named FEAT support "
            "from HFST\n");
    fprintf(message_out, "\n");
    fprintf(message_out, "MVER, EVER or UVER version vectors must be composed "
            "of one to three full stop separated runs of digits.\n"
            "FEAT should be name of feature supported by HFST, such as "
            "SFST, foma or openfst\n"
            "\n");
    print_report_bugs();
    fprintf(message_out, "\n");
    print_more_info();
}


int
parse_options(int argc, char** argv)
{
    extend_options_getenv(&argc, &argv);
    // use of this function requires options are settable on global scope
    while (true)
    {
        static const struct option long_options[] =
        {
          HFST_GETOPT_COMMON_LONG,
          {"atleast-version", required_argument, 0, 'a'},
          {"exact-version", required_argument, 0, 'e'},
          {"max-version", required_argument, 0, 'm'},
          {"require-feature", required_argument, 0, 'f'},
          {0,0,0,0}
        };
        int option_index = 0;
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             "a:e:f:m:",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }
        switch (c)
        {
        case 'a':
          min_version = parse_version_string(optarg);
          break;
        case 'e':
          exact_version = parse_version_string(optarg);
          break;
        case 'm':
          max_version = parse_version_string(optarg);
          break;
        case 'f':
          required_features.insert(optarg);
          break;
        case 'h':
          print_usage();
          return EXIT_SUCCESS;
        case 'V':
          print_version();
          return EXIT_SUCCESS;
        }
    }
    if ((min_version == -1L) && (max_version == -1L) && (exact_version == -1L)
        && (required_features.size() == 0) && (verbose == false))
      {
        verbose = true;
        verbose_printf("No tests selected; printing known data\n");
      }
    return EXIT_CONTINUE;
}


int main (int argc, char * argv[])
{
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
#endif

  hfst_set_program_name(argv[0], "0.1", "HfstInfo");
  parse_options(argc, argv);
  if (min_version != -1L)
    {
      verbose_printf("Requiring current version %ld to be greater than %ld\n",
                     HFST_LONGVERSION, min_version);
      if (HFST_LONGVERSION < min_version)
        {
          error(EXIT_FAILURE, 0, "Version requirements not met");
        }
    }
  if (exact_version != -1L)
    {
      verbose_printf("Requiring current version %ld to be exactly %ld\n",
                     HFST_LONGVERSION, exact_version);
      if (HFST_LONGVERSION != exact_version)
        {
          error(EXIT_FAILURE, 0, "Version requirements not met");
        }
    }
  if (max_version != -1L)
    {
      verbose_printf("Requiring current version %ld to be greater than %ld\n",
                     HFST_LONGVERSION, max_version);
      if (HFST_LONGVERSION < max_version)
        {
          error(EXIT_FAILURE, 0, "Version requirements not met");
        }
    }
  for (set<string>::const_iterator f = required_features.begin();
       f != required_features.end();
       ++f)
    {
      if ((*f == "sfst") || (*f == "SFST") || (*f == "HAVE_SFST"))
        {
          verbose_printf("Requiring SFST support from library");
#ifndef HAVE_SFST
          error(EXIT_FAILURE, 0, "Required SFST support not present");
#endif
        }
      else if ((*f == "foma") || (*f == "FOMA") || (*f == "HAVE_FOMA"))
        {
          verbose_printf("Requiring foma support from library");
#ifndef HAVE_FOMA
          error(EXIT_FAILURE, 0, "Required foma support not present");
#endif
        }
      else if ((*f == "xfsm") || (*f == "XFSM") || (*f == "HAVE_XFSM"))
        {
          verbose_printf("Requiring xfsm support from library");
#ifndef HAVE_XFSM
          error(EXIT_FAILURE, 0, "Required xfsm support not present");
#endif
        }
      else if ((*f == "openfst") || (*f == "OPENFST") || (*f == "HAVE_OPENFST"))
        {
          verbose_printf("Requiring OpenFst support from library");
#ifndef HAVE_OPENFST
          error(EXIT_FAILURE, 0, "Required OpenFst support not present");
#endif
        }
      else if ((*f == "glib") || (*f == "USE_GLIB_UNICODE"))
        {
          verbose_printf("Requiring Unicode parsed by Glib");
#ifndef USE_GLIB_UNICODE
          error(EXIT_FAILURE, 0,
                "Required GLIB-based Unicode handling not presesnt");
#endif
        }
      else
        {
          error(EXIT_FAILURE, 0, "Required %s support is unrecognised "
                "and therefore assumed to be missing", f->c_str());
        }
    }
  verbose_printf("HFST info version: %s\n"
                 "HFST packaging: %s\n"
                 "HFST version: %s\n"
                 "HFST long version: %ld\n",
                 //          "HFST configuration revision: %s\n",
                 hfst_tool_version,
                 PACKAGE_STRING,
                 PACKAGE_VERSION,
                 HFST_LONGVERSION);
  //                 HFST_REVISION);
#if HAVE_OPENFST
  verbose_printf("OpenFst supported\n");
#endif
#if HAVE_SFST
  verbose_printf("SFST supported\n");
#endif
#if HAVE_FOMA
  verbose_printf("foma supported\n");
#endif
#if HAVE_XFSM
  verbose_printf("xfsm supported\n");
#endif
#if USE_GLIB_UNICODE
  verbose_printf("Unicode support: glib\n");
#elif USE_ICU_UNICODE
  verbose_printf("Unicode support: ICU\n");
#else
  verbose_printf("Unicode support: no (hfst)\n");
#endif

  return EXIT_SUCCESS;
}
