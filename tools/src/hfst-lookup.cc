//! @file hfst-lookup.cc
//!
//! @brief Transducer lookdown command line tool
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
#  include <io.h>
#endif

#include <iostream>
#include <fstream>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

#ifdef _MSC_VER
#  include "hfst-getopt.h"
#else
#  include <getopt.h>
#endif

#ifdef WINDOWS
#include "hfst-string-conversions.h"
using hfst::hfst_fprintf_console;
#endif

#include <limits>
#include <math.h>

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "HfstLookupFlagDiacritics.h"
#include "HfstFlagDiacritics.h"
#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"
#include "implementations/HfstBasicTransducer.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"
#include "HfstStrings2FstTokenizer.h"
#include "HfstSymbolDefs.h"

using hfst::internal_epsilon;
using hfst::internal_identity;
using hfst::is_epsilon;
using hfst::is_identity;

using hfst::HfstTransducer;
using hfst::HFST_OL_TYPE;
using hfst::HFST_OLW_TYPE;
using hfst::implementations::HfstState;
using hfst::implementations::HfstBasicTransducer;
using hfst::implementations::HfstBasicTransition;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using hfst::HfstTokenizer;
using hfst::HfstOneLevelPath;
using hfst::HfstOneLevelPaths;
using hfst::HfstTwoLevelPath;
using hfst::HfstTwoLevelPaths;
using hfst::FdOperation;
using hfst::HfstEpsilonHandler;

using hfst::StringPair;
using hfst::StringPairVector;
using hfst::StringVector;
using hfst::StringSet;

using std::string;
using std::vector;


// add tools-specific variables here
static char* lookup_file_name;
static FILE* lookup_file;
static bool pipe_input = false;
static bool pipe_output = false;
static size_t linen = 0;
static bool lookup_given = false;
static size_t infinite_cutoff=5;
static size_t max_number=-1;
static size_t MAX_NUMBER=5;
static float beam=-1;

#define CASCADE_UNION 1
#define CASCADE_PRIORITY_UNION 2
#define CASCADE_COMPOSITION 3
static int cascade_ = CASCADE_UNION;

// symbols actually seen in (non-ol) transducers
static std::vector<std::set<std::string> > cascade_symbols_seen;
static std::vector<bool> cascade_unknown_or_identity_seen;

enum lookup_input_format
{
  UTF8_TOKEN_INPUT,
  SPACE_SEPARATED_TOKEN_INPUT,
  APERTIUM_INPUT
};

enum lookup_output_format
{
  XEROX_OUTPUT,
  CG_OUTPUT,
  APERTIUM_OUTPUT
};

static lookup_input_format input_format = UTF8_TOKEN_INPUT;
static lookup_output_format output_format = XEROX_OUTPUT;
static double time_cutoff = 0.0;

// XFST variables for apply
static bool show_flags = false;
static bool obey_flags = true;
static bool print_pairs = false;
static bool print_space = false;
static bool quote_special = false;

static char* epsilon_format = strdup("");
static char* space_format = strdup("");

// the formats for lookup cases go like so:
//  BEGIN LOOKUP LOOKUP LOOKUP... END

// for standard case of more than 0 and less than infinite results:
static char* begin_setf = 0; // print before set of lookups
static char* lookupf = 0; // print before each lookup
static char* end_setf = 0; // print for each lookup
// when there are 0 results:
static char* empty_begin_setf = 0; // print before empty set of results
static char* empty_lookupf = 0; // print in place of empty lookup result
static char* empty_end_setf = 0; // print in end of empty set of results
// when there are 0 results and token is unrecognizable by analyser:
static char* unknown_begin_setf = 0; // print before unknown set of results
static char* unknown_lookupf = 0; // print in place of unknown result
static char* unknown_end_setf = 0; // print in end of set of unknown results
// when there are infinite results:
static char* infinite_begin_setf = 0; // print before infinite set of results
static char* infinite_lookupf = 0; // print in place of infinite results
static char* infinite_end_setf = 0; // print in end of infinite results

static bool print_statistics = false;

static bool show_progress_bar = false;

// predefined formats
// Xerox:
// word     word N SG
// word     word V PRES
static const char* XEROX_BEGIN_SETF = "";
static const char* XEROX_LOOKUPF = "%i\t%l\t%w%n";
static const char* XEROX_END_SETF = "%n";
// notaword notaword+?
static const char* XEROX_EMPTY_BEGIN_SETF = "";
static const char* XEROX_EMPTY_LOOKUPF = "%i\t%i+?\t%w%n";
static const char* XEROX_EMPTY_END_SETF = "%n";
// ¶    ¶+?
static const char* XEROX_UNKNOWN_BEGIN_SETF = "";
static const char* XEROX_UNKNOWN_LOOKUPF = "%i\t%i+?\t%w%n";
static const char* XEROX_UNKNOWN_END_SETF = "%n";
// 0    0 NUM SG
// 0    [...cyclic...]
static const char* XEROX_INFINITE_BEGIN_SETF = "";
static const char* XEROX_INFINITE_LOOKUPF = "%i\t%l\t%w%n";
static const char* XEROX_INFINITE_END_SETF = "%i\t[...cyclic...]%n%n";
// CG:
// "<word>"
//      "word"  N SG
//      "word"  V PRES
static const char* CG_BEGIN_SETF = "\"<%i>\"%n";
static const char* CG_LOOKUPF = "\t\"%b\"%a\t%w%n";
static const char* CG_END_SETF = "%n";
// "<notaword>"
//      "notaword" ?
static const char* CG_EMPTY_BEGIN_SETF = "\"<%i>\"%n";
static const char* CG_EMPTY_LOOKUPF = "\t\"%i\" ?\tInf%n";
static const char* CG_EMPTY_END_SETF = "%n";
// "<¶>"
//      "¶" ?
static const char* CG_UNKNOWN_BEGIN_SETF = "\"<%i>\"%n";
static const char* CG_UNKNOWN_LOOKUPF = "\t\"%i\"\t ?\tInf%n";
static const char* CG_UNKNOWN_END_SETF = "%n";
// "<0>"
//      "0" NUM SG
//      "0" [...cyclic...]
static const char* CG_INFINITE_BEGIN_SETF = "\"<%i>\"%n";
static const char* CG_INFINITE_LOOKUPF = "\t\"%b\"%a\t%w%n";
static const char* CG_INFINITE_END_SETF = "\t\"%i\"...cyclic...%n%n";
// apertium:
// ^word/word N SG/word V PRES$[apertium superblank markup]
static const char* APERTIUM_BEGIN_SETF = "^%i";
static const char* APERTIUM_LOOKUPF = "/%l";
static const char* APERTIUM_END_SETF = "$%m%n";
// ^notaword/*notaword$[apertium superblank markup]
static const char* APERTIUM_EMPTY_BEGIN_SETF = "^%i";
static const char* APERTIUM_EMPTY_LOOKUPF = "/*%i";
static const char* APERTIUM_EMPTY_END_SETF = "$%m%n";
// ¶[apertium superblank markup]
static const char* APERTIUM_UNKNOWN_BEGIN_SETF = " ";
static const char* APERTIUM_UNKNOWN_LOOKUPF = "%i%m";
static const char* APERTIUM_UNKNOWN_END_SETF = " ";
// ^0/0 NUM SG/...$
static const char* APERTIUM_INFINITE_BEGIN_SETF = "^%i";
static const char* APERTIUM_INFINITE_LOOKUPF = "/%l";
static const char* APERTIUM_INFINITE_END_SETF = "/...$%n";

// statistic counting
static unsigned long inputs = 0;
static unsigned long no_analyses = 0;
static unsigned long analysed = 0;
static unsigned long analyses = 0;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "perform transducer lookup (apply)\n"
           "NOTE: hfst-lookup does lookup from left to right as opposed to xfst and foma\n"
           "      lookup which is carried out from right to left. In order to do lookup\n"
           "      in a similar way as xfst and foma, use 'hfst-flookup' instead.\n"
        "\n", program_name);

    print_common_program_options(message_out);
    fprintf(message_out,
        "Input/Output options:\n"
        "  -i, --input=INFILE       Read input transducer from INFILE\n"
        "  -o, --output=OUTFILE     Write output to OUTFILE\n"
        "  -p, --pipe-mode[=STREAM] Control input and output streams\n");

    fprintf(message_out, "Lookup options:\n"
            "  -I, --input-strings=SFILE        Read lookup strings from SFILE\n"
            "  -O, --output-format=OFORMAT      Use OFORMAT printing results sets\n"
            "  -e, --epsilon-format=EPS         Print epsilon as EPS\n"
            "  -F, --input-format=IFORMAT       Use IFORMAT parsing input\n"
            "  -x, --statistics                 Print statistics\n"
            "  -X, --xfst=VARIABLE              Toggle xfst VARIABLE\n"
            "  -c, --cycles=INT                 How many times to follow input epsilon cycles\n"
            "                                   (only for non-lookup-optimized transducers)\n"
            "  -n, --max-number=INT             Maximum number of results printed for each input\n"
            "                                   (only for lookup-optimized transducers)\n"
            "  -b, --beam=B                     Output only analyses whose weight is within B from\n"
            "                                   the best analysis\n"
            "  -t, --time-cutoff=S              Limit search after having used S seconds per input\n"
            "                                   (only for lookup-optimized transducers)\n"
            "  -C, --cascade=CASCADE            How multiple transducers in input are handled\n"
            "  -P, --progress                   Show neat progress bar if possible\n");
    fprintf(message_out, "\n");
    print_common_unary_program_parameter_instructions(message_out);
    fprintf(message_out,
            "OFORMAT is one of {xerox,cg,apertium}, xerox being default\n"
            "IFORMAT is one of {text,spaced,apertium}, default being text,\n"
            "unless OFORMAT is apertium\n"
            "VARIABLEs relevant to lookup are {print-pairs,print-space,\n"
            "quote-special,show-flags,obey-flags}\n"
            "Input epsilon cycles are followed by default INT=5 times.\n"
            "Epsilon is printed by default as an empty string.\n"
            "B must be a non-negative float.\n"
            "S must be a non-negative float. The default, 0.0, indicates no cutoff.\n"
            "If the input contains several transducers, a set containing\n"
            "results from all transducers is printed for each input string.\n");
    fprintf(message_out, "\n");

    fprintf(message_out, "CASCADE must be one of { union, priority-union, composition }.\n"
            "If not specified, defaults to {union}.\n");
    fprintf(message_out, "\n");

    fprintf(message_out, "STREAM can be { input, output, both }. If not given, defaults to {both}.\n"
#ifdef _MSC_VER
          "If input file is not specified with -I, input is read interactively via the\n"
          "console, i.e. line by line from the user. If you redirect input from a file,\n"
          "use --pipe-mode=input. Output is by default printed to the console. If you\n"
          "redirect output to a file, use --pipe-mode=output.\n");
#else
          "If input file is not specified with -I, input is read interactively line by\n"
          "line from the user. If you redirect input from a file, use --pipe-mode=input.\n"
          "--pipe-mode=output is ignored on non-windows platforms.\n");
#endif
    fprintf(message_out, "\n");

    fprintf(message_out,
            "Todo:\n"
            "  Support --xfst=obey-flags for optimized lookup format.\n"
            "  Support --cycles for optimized lookup format.\n");

    fprintf(message_out,
            "\n"
            "Known bugs:\n"
            "  'quote-special' quotes spaces that come from 'print-space'\n");

    fprintf(message_out, "\n");
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
        HFST_GETOPT_UNARY_LONG,
          // add tool-specific options here
            {"input-strings", required_argument, 0, 'I'},
            {"output-format", required_argument, 0, 'O'},
            {"input-format", required_argument, 0, 'F'},
            {"statistics", no_argument, 0, 'x'},
            {"cycles", required_argument, 0, 'c'},
            {"max-number", required_argument, 0, 'n'},
            {"xfst", required_argument, 0, 'X'},
            {"epsilon-format", required_argument, 0, 'e'},
            {"epsilon-format2", required_argument, 0, 'E'},
            {"beam", required_argument, 0, 'b'},
            {"time-cutoff", required_argument, 0, 't'},
            {"pipe-mode", optional_argument, 0, 'p'},
            {"progress", no_argument, 0, 'P'},
            {"cascade", required_argument, 0, 'C'},
            {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "I:O:F:xc:n:X:e:E:b:t:p::PC:",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
          // add tool-specific cases here
        case 'I':
            lookup_file_name = hfst_strdup(optarg);
            lookup_file = hfst_fopen(lookup_file_name, "r");
            lookup_given = true;
            break;
        case 'O':
            if (strcmp(optarg, "xerox") == 0)
            {
              output_format = XEROX_OUTPUT;
            }
            else if (strcmp(optarg, "cg") == 0)
            {
              output_format = CG_OUTPUT;
            }
            else if (strcmp(optarg, "apertium") == 0)
            {
              output_format = APERTIUM_OUTPUT;
              input_format = APERTIUM_INPUT;
            }
            else
              {
                error(EXIT_FAILURE, 0,
                      "Unknown output format %s; valid values are: "
                      "xerox, cg, apertium\n", optarg);
                return EXIT_FAILURE;
              }
            break;
        case 'F':
            if (strcmp(optarg, "text") == 0)
              {
                input_format = UTF8_TOKEN_INPUT;
              }
            else if (strcmp(optarg, "spaced") == 0)
              {
                input_format = SPACE_SEPARATED_TOKEN_INPUT;
              }
            else if (strcmp(optarg, "apertium") == 0)
              {
                input_format = APERTIUM_INPUT;
              }
            else
              {
                error(EXIT_FAILURE, 0,
                      "Unknown input format %s; valid values are:"
                       "utf8, spaced, apertium\n", optarg);
                return EXIT_FAILURE;
              }
            break;
        case 'e':
        case 'E':
            epsilon_format = hfst_strdup(optarg);
            break;
        case 'b':
          beam = atof(optarg);
          if (beam < 0)
            {
              std::cerr << "Invalid argument for --beam\n";
              return EXIT_FAILURE;
            }
          break;
        case 't':
            time_cutoff = atof(optarg);
            if (time_cutoff < 0.0)
            {
                std::cerr << "Invalid argument for --time-cutoff\n";
                return EXIT_FAILURE;
            }
            break;
        case 'x':
            print_statistics = true;
            break;
        case 'X':
            if (strcmp(optarg, "print-pairs") == 0)
              {
                print_pairs = true;
                /* error(EXIT_FAILURE, 0, "Unimplemented pair printing"); */
              }
            else if (strcmp(optarg, "print-space") == 0)
              {
                print_space = true;
                space_format = strdup(" ");
              }
            else if (strcmp(optarg, "show-flags") == 0)
              {
                show_flags = true;
              }
            else if (strcmp(optarg, "quote-special") == 0)
              {
                quote_special = true;
              }
            else if (strcmp(optarg, "obey-flags") == 0)
              {
                obey_flags = false;
              }
            else
              {
                error(EXIT_FAILURE, 0, "Xfst variable %s unrecognised",
                      optarg);
              }
            break;
        case 'c':
            infinite_cutoff = (size_t)atoi(hfst_strdup(optarg));
            break;
        case 'n':
            max_number = (size_t)atoi(hfst_strdup(optarg));
            break;
        case 'p':
          if (optarg == NULL)
            { pipe_input = true; pipe_output = true; }
          else if (strcmp(optarg, "both") == 0 || strcmp(optarg, "BOTH") == 0)
            { pipe_input = true; pipe_output = true; }
          else if (strcmp(optarg, "input") == 0 || strcmp(optarg, "INPUT") == 0 ||
                   strcmp(optarg, "in") == 0 || strcmp(optarg, "IN") == 0)
            { pipe_input = true; }
          else if (strcmp(optarg, "output") == 0 || strcmp(optarg, "OUTPUT") == 0 ||
                   strcmp(optarg, "out") == 0 || strcmp(optarg, "OUT") == 0)
            { pipe_output = true; }
          else
            { error(EXIT_FAILURE, 0, "--pipe-mode argument %s unrecognised", optarg); }
          break;

        case 'P':
            show_progress_bar = true;
            break;

        case 'C':
            if (strcmp(optarg, "union") == 0)
              { cascade_ = CASCADE_UNION; }
            else if (strcmp(optarg, "priority-union") == 0)
              { cascade_ = CASCADE_PRIORITY_UNION; }
            else if (strcmp(optarg, "composition") == 0)
              { cascade_ = CASCADE_COMPOSITION; }
            else
              { error(EXIT_FAILURE, 0, "--cascade argument %s unrecognised, possible values are\n"
                      "{ union, priority-union, composition }", optarg); }
            break;
#include "inc/getopt-cases-error.h"
        }
    }

    switch (output_format)
      {
      case XEROX_OUTPUT:
        begin_setf = hfst_strdup(XEROX_BEGIN_SETF);
        lookupf = hfst_strdup(XEROX_LOOKUPF);
        end_setf = hfst_strdup(XEROX_END_SETF);
        empty_begin_setf = hfst_strdup(XEROX_EMPTY_BEGIN_SETF);
        empty_lookupf = hfst_strdup(XEROX_EMPTY_LOOKUPF);
        empty_end_setf = hfst_strdup(XEROX_EMPTY_END_SETF);
        unknown_begin_setf = hfst_strdup(XEROX_UNKNOWN_BEGIN_SETF);
        unknown_lookupf = hfst_strdup(XEROX_UNKNOWN_LOOKUPF);
        unknown_end_setf = hfst_strdup(XEROX_UNKNOWN_END_SETF);
        infinite_begin_setf = hfst_strdup(XEROX_INFINITE_BEGIN_SETF);
        infinite_lookupf = hfst_strdup(XEROX_INFINITE_LOOKUPF);
        infinite_end_setf = hfst_strdup(XEROX_INFINITE_END_SETF);
      break;
    case CG_OUTPUT:
        begin_setf = hfst_strdup(CG_BEGIN_SETF);
        lookupf = hfst_strdup(CG_LOOKUPF);
        end_setf = hfst_strdup(CG_END_SETF);
        empty_begin_setf = hfst_strdup(CG_EMPTY_BEGIN_SETF);
        empty_lookupf = hfst_strdup(CG_EMPTY_LOOKUPF);
        empty_end_setf = hfst_strdup(CG_EMPTY_END_SETF);
        unknown_begin_setf = hfst_strdup(CG_UNKNOWN_BEGIN_SETF);
        unknown_lookupf = hfst_strdup(CG_UNKNOWN_LOOKUPF);
        unknown_end_setf = hfst_strdup(CG_UNKNOWN_END_SETF);
        infinite_begin_setf = hfst_strdup(CG_INFINITE_BEGIN_SETF);
        infinite_lookupf = hfst_strdup(CG_INFINITE_LOOKUPF);
        infinite_end_setf = hfst_strdup(CG_INFINITE_END_SETF);
        break;
    case APERTIUM_OUTPUT:
        begin_setf = hfst_strdup(APERTIUM_BEGIN_SETF);
        lookupf = hfst_strdup(APERTIUM_LOOKUPF);
        end_setf = hfst_strdup(APERTIUM_END_SETF);
        empty_begin_setf = hfst_strdup(APERTIUM_EMPTY_BEGIN_SETF);
        empty_lookupf = hfst_strdup(APERTIUM_EMPTY_LOOKUPF);
        empty_end_setf = hfst_strdup(APERTIUM_EMPTY_END_SETF);
        unknown_begin_setf = hfst_strdup(APERTIUM_UNKNOWN_BEGIN_SETF);
        unknown_lookupf = hfst_strdup(APERTIUM_UNKNOWN_LOOKUPF);
        unknown_end_setf = hfst_strdup(APERTIUM_UNKNOWN_END_SETF);
        infinite_begin_setf = hfst_strdup(APERTIUM_INFINITE_BEGIN_SETF);
        infinite_lookupf = hfst_strdup(APERTIUM_INFINITE_LOOKUPF);
        infinite_end_setf = hfst_strdup(APERTIUM_INFINITE_END_SETF);
        break;
    default:
      fprintf(stderr, "Unknown output format\n");
      return EXIT_FAILURE;
      break;
    }

    if (!lookup_given)
      {
        lookup_file = stdin;
        lookup_file_name = strdup("<stdin>");
      }
#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    return EXIT_CONTINUE;
}

static void print_prompt()
{
  if (!silent && !pipe_input && !lookup_given)
    {
      fprintf(stderr, "> ");
    }
}

static std::string get_print_format(const std::string &s) ;

int
lookup_printf(const char* format, const HfstOneLevelPath* input,
              const HfstOneLevelPath* result, const char* markup,
              FILE * ofile)
{
    size_t space = 0;
    size_t input_len = 0;
    size_t lookup_len = 0;
    if (result != NULL)
      {
        bool first = true;
        for (vector<string>::const_iterator s = result->second.begin();
             s != result->second.end();
             ++s)
          {
            if (!first && print_space)
              {
                lookup_len += strlen(space_format);
              }
            first = false;
            if (is_epsilon(*s))
              {
                lookup_len += strlen(epsilon_format);
              }
            else if (FdOperation::is_diacritic(*s))
              {
                if (show_flags)
                  {
                    lookup_len += s->size();
                  }
              }
            else
              {
                lookup_len += s->size();
              }
          }
      }
    if (input != NULL)
      {
        bool first = true;
        for (vector<string>::const_iterator s = input->second.begin();
             s != input->second.end();
             ++s)
          {
            if (!first && print_space)
              {
                input_len += strlen(space_format);
              }
            first = false;
            if (is_epsilon(*s))
              {
                input_len += strlen(epsilon_format);
              }
            else if (FdOperation::is_diacritic(*s))
              {
                if (show_flags)
                  {
                    input_len += s->size();
                  }
              }
            else
              {
                input_len += s->size();
              }
            first = false;
          }
      }
    for (const char *f = format; *f != '\0'; f++)
      {
        if (*f == '%')
          {
            f++;
            if (*f == 'i')
              {
                space += input_len;
              }
            else if (*f == 'l')
              {
                space += lookup_len;
              }
            else if (*f == 'w')
              {
                space += strlen("0.012345678901234567890");
              }
            else if (*f == '\0')
              {
                break;
              }
            else
              {
                space++;
              }
          }
        space++;
      }
    char* lookupform = 0;
    if (result != NULL)
      {
        lookupform = static_cast<char*>(malloc(sizeof(char) * lookup_len +
                                              sizeof(char) * strlen(space_format) * (lookup_len - 1) +1));
        char* p = lookupform;
        bool first = true;
        for (vector<string>::const_iterator s = result->second.begin();
             s != result->second.end();
             ++s)
          {
            if (!first && print_space)
              {
                p = strcpy(p, space_format);
                p += strlen(space_format);
              }
            if (is_epsilon(*s))
              {
                p = strcpy(p, epsilon_format);
                p += strlen(epsilon_format);
              }
            else if (FdOperation::is_diacritic(*s))
              {
                if (show_flags)
                  {
                    p = strcpy(p, s->c_str());
                    p += s->size();
                  }
              }
            else
              {
                p = strcpy(p, s->c_str());
                p += s->size();
              }
            first = false;
          }
        *p = '\0';
      }
    else
      {
        free(lookupform);
        lookupform = strdup("");
      }
    char* inputform = 0;
    if (input != NULL)
      {
        inputform = static_cast<char*>(malloc(sizeof(char) * input_len +
                                              sizeof(char) * strlen(space_format) * (input_len - 1) +1));
        char* p = inputform;
        bool first = true;
        for (vector<string>::const_iterator s = input->second.begin();
             s != input->second.end();
             ++s)
          {
            if (!first && print_space)
              {
                p = strcpy(p, space_format);
                p += strlen(space_format);
              }
            if (is_epsilon(*s))
              {
                p = strcpy(p, epsilon_format);
                p += strlen(epsilon_format);
              }
            else if (FdOperation::is_diacritic(*s))
              {
                if (show_flags)
                  {
                    p = strcpy(p, s->c_str());
                    p += s->size();
                  }
              }
            else
              {
                p = strcpy(p, s->c_str());
                p += s->size();
              }
            first = false;
          }
        *p = '\0';
      }
    else
      {
        free(inputform);
        inputform = strdup("");
      }
    if (markup != NULL)
      {
        space += 2 * strlen(markup);
      }
    char* res = static_cast<char*>(calloc(sizeof(char), space*32 + 1));
    size_t space_left = space;
    const char* src = format;
    char* dst = res;
    char* i; // %i
    char* l; // %l
    char* b; // %b
    char* a; // %a
    char* m; // %m
    float w = 0.0f;
    if (result != NULL)
      {
        w = result->first; // %w
      }
    else
      {
#ifdef _MSC_VER
        w = std::numeric_limits<float>::infinity();
#else
        w = INFINITY;
#endif
      }
    i = strdup(inputform);
    if (lookupform != NULL)
    {
        l = strdup(lookupform);
        const char* lookup_end;
        for (lookup_end = lookupform; *lookup_end != '\0'; lookup_end++)
            ;
        const char* anal_start = strchr(lookupform, '+');
        if (anal_start == NULL)
        {
            anal_start = strchr(lookupform, ' ');
        }
        if (anal_start == NULL)
        {
            anal_start = strchr(lookupform, '<');
        }
        if (anal_start == NULL)
        {
            anal_start = strchr(lookupform, '[');
        }
        if (anal_start == NULL)
        {
            // give up trying
            anal_start = lookupform;
        }
        b = static_cast<char*>(calloc(sizeof(char),
                    anal_start - lookupform + 1));
        b = static_cast<char*>(memcpy(b, lookupform, anal_start - lookupform));
        a = static_cast<char*>(calloc(sizeof(char),
                    lookup_end - anal_start + 1));
        a = static_cast<char*>(memcpy(a, anal_start, lookup_end - anal_start));
    }
    else
    {
        l = strdup("");
        b = strdup("");
        a = strdup("");
    }
    if (markup != NULL)
      {
        m = strdup(markup);
      }
    else
      {
        m = strdup("");
      }
    bool percent = false;
    while ((*src != '\0') && (space_left > 0))
    {
        if (percent)
        {
            if (*src == 'b')
            {
                int skip = snprintf(dst, space_left, "%s", b);
                dst += skip;
                space_left -= skip;
                src++;
            }
            else if (*src == 'l')
            {
                int skip = snprintf(dst, space_left, "%s", l);
                dst += skip;
                space_left -= skip;
                src++;
            }
            else if (*src == 'i')
            {
                int skip = snprintf(dst, space_left, "%s", i);
                dst += skip;
                space_left -= skip;
                src++;
            }
            else if (*src == 'a')
            {
                int skip = snprintf(dst, space_left, "%s", a);
                dst += skip;
                space_left -= skip;
                src++;
            }
            else if (*src == 'm')
              {
                int skip = snprintf(dst, space_left, "%s", m);
                dst += skip;
                space_left -= skip;
                src++;
              }
            else if (*src == 'n')
            {
                *dst = '\n';
                dst++;
                space_left--;
                src++;
            }
            else if (*src == 'w')
              {
                int skip = 0;
#ifdef _MSC_VER
                if (w == std::numeric_limits<float>::infinity())
#else
                if (false)
#endif
                  {
                    skip = snprintf(dst, space_left, "%s", "inf");
                  }
                else
                  {
                    skip = snprintf(dst, space_left, "%f", w);
                  }
                dst += skip;
                space_left -= skip;
                src++;
              }
            else
            {
                // unknown format, retain % as well
                *dst = '%';
                dst++;
                space_left--;
                *dst = *src;
                dst++;
                space_left--;
                src++;
            }
            percent = false;
        }
        else if (*src == '%')
        {
            percent = true;
            src++;
        }
        else
        {
            *dst = *src;
            dst++;
            space_left--;
            src++;
        }
    }
    *dst = '\0';
    free(a);
    free(l);
    free(b);
    free(i);
    free(m);
    free(inputform);
    free(lookupform);
    int rv;
    if (! quote_special)
      {
#ifdef WINDOWS
        if (!pipe_output)
          rv = hfst_fprintf_console(ofile, "%s", res);
        else
#endif
          rv = fprintf(ofile, "%s", res);
      }
    else
      {
#ifdef WINDOWS
        if (!pipe_output)
          rv = hfst_fprintf_console(ofile, "%s", get_print_format(res).c_str());
        else
#endif
          rv = fprintf(ofile, "%s", get_print_format(res).c_str());
      }
    free(res);
    return rv;
}


vector<string>*
string_to_utf8(char* p)
{
  vector<string>* path = new vector<string>;
    while ((p != 0) && (*p != '\0'))
      {
        unsigned char c = static_cast<unsigned char>(*p);
        unsigned short u8len = 1;
        if (c <= 127)
          {
            u8len = 1;
          }
        else if ( (c & (128 + 64 + 32 + 16)) == (128 + 64 + 32 + 16) )
          {
            u8len = 4;
          }
        else if ( (c & (128 + 64 + 32 )) == (128 + 64 + 32) )
          {
            u8len = 3;
          }
        else if ( (c & (128 + 64 )) == (128 + 64))
          {
            u8len = 2;
          }
        else
          {
            error_at_line(EXIT_FAILURE, 0, inputfilename, linen,
                          "%s not valid UTF-8\n", p);
          }
        char* nextu8 = hfst_strndup(p, u8len);
        path->push_back(nextu8);
        free(nextu8);
        p += u8len;
      }
    return path;
}

/* Add a '\' in front of ':', ' ' and '\'. */
static std::string escape_special_characters(char *s)
{
  std::string retval;
  for (unsigned int i=0; s[i] != '\0'; i++)
    {
      if (s[i] == ':' || s[i] == '\\' || s[i] == ' ')
        retval.append(1, '\\');
      retval.append(1, s[i]);
    }
  return retval;
}

HfstOneLevelPath*
line_to_lookup_path(char** s, hfst::HfstStrings2FstTokenizer& tok,
                    char** markup, bool* outside_sigma, bool optimized_lookup)
{
    HfstOneLevelPath* rv = new HfstOneLevelPath;
    rv->first = 0;
    *outside_sigma = false;
    inputs++;
    switch (input_format)
      {
      case SPACE_SEPARATED_TOKEN_INPUT:
        {
          /*
          vector<string> path;
          char* token = strtok(*s, " ");
          while (token)
            {
              path.push_back(token);
              token = strtok(NULL, " ");
            }
          rv->second = path;
          break;
          */
          
          std::string S = escape_special_characters(*s);

          StringPairVector spv
            = tok.tokenize_string_pair(S, true);
          
          for (StringPairVector::const_iterator it = spv.begin();
               it != spv.end(); it++)
            {
              rv->second.push_back(it->first);
            }
          break;
        }
      case UTF8_TOKEN_INPUT:
        {
          if (optimized_lookup)
            {
                rv->second.push_back(std::string(*s));
            }
          else
            {
              std::string S = escape_special_characters(*s);

              StringPairVector spv
                = tok.tokenize_string_pair(S, false);
              
              for (StringPairVector::const_iterator it = spv.begin();
                   it != spv.end(); it++)
                {
                  // todo: check if symbol is known to transducer
                  rv->second.push_back(it->first);
                }
            }
          break;
        }
      case APERTIUM_INPUT:
          {
            char* real_s
              = static_cast<char*>(calloc(sizeof(char),strlen(*s)+1));

            *markup = static_cast<char*>(calloc(sizeof(char), strlen(*s)+1));
            char* m = *markup;
            char* sp = real_s;
            bool inbr = false;
            for (const char* p = *s; *p != '\0'; p++)
              {
                if (inbr)
                  {
                    if (*p == ']')
                      {
                        *m = *p;
                        m++;
                        inbr = false;
                      }
                    else if ((*p == '\\') && (*(p+1) == ']'))
                      {
                        p++;
                        *m = *p;
                        m++;
                      }
                    else
                      {
                        *m = *p;
                        m++;
                      }
                  }
                else if (!inbr)
                  {
                    if (*p == '[')
                      {
                        *m = *p;
                        m++;
                        inbr = true;
                      }
                    else if (*p == ']')
                      {
                        *m = *p;
                        m++;
                        continue;
                      }
                    else if (*p == '\\')
                      {
                        p++;
                        *real_s = *p;
                        real_s++;
                      }
                    else
                      {
                        *real_s = *p;
                        real_s++;
                      }
                  }
              } // for each character in input
            vector<string>* path = string_to_utf8(sp);
            free(*s);
            *s = sp;
            rv->second = *path;
            break;
          }
      default:
        fprintf(stderr, "Unknown input format");
        break;
      } // switch input format
    return rv;
}

void lookup_fd_and_print(HfstBasicTransducer * tr, HfstTransducer * TR, HfstOneLevelPaths& results,
                         const HfstOneLevelPath& s, size_t * limit = NULL, bool print_pairs_at_this_point = false,
                         bool print_fail = false, const HfstOneLevelPath * input_to_print = NULL,
                         bool no_newline = false);

HfstOneLevelPaths*
lookup_simple(const HfstOneLevelPath& s, HfstTransducer& t, bool* infinity, bool print_pairs_at_this_point=false, bool print_fail=false, const HfstOneLevelPath * input_to_print = NULL, bool no_newline=false)
{
  HfstOneLevelPaths* results = new HfstOneLevelPaths;
  if (time_cutoff == 0.0 && t.is_lookup_infinitely_ambiguous(s.second))
    {
      size_t maxnum = (max_number == -1)? MAX_NUMBER : max_number;
      if (!silent) {
        if (max_number == -1)
          warning(0, 0, "Got infinite results, number of results limited to " SIZE_T_SPECIFIER "\n"
                  "(can be controlled with --max-number=N)",
                  maxnum);
        else
          warning(0, 0, "Got infinite results, number of results limited to " SIZE_T_SPECIFIER "",
                  maxnum);
      }
      if (print_pairs)
        lookup_fd_and_print(NULL, &t, *results, s, &maxnum, print_pairs_at_this_point, print_fail, input_to_print, no_newline);
      else
        results = t.lookup_fd(s.second, maxnum, time_cutoff);
      *infinity = true;
    }
  else
    {
      if (print_pairs)
        lookup_fd_and_print(NULL, &t, *results, s, &max_number, print_pairs_at_this_point, print_fail, input_to_print, no_newline);
      else
        results = t.lookup_fd(s.second, max_number, time_cutoff);
    }

  if (results->size() == 0)
    {
       // no results as empty result
      verbose_printf("Got no results\n");
    }
  return results;
}



/* Replace all strings \a str1 in \a symbol with \a str2. */
static std::string replace_all(std::string symbol,
                               const std::string &str1,
                               const std::string &str2)
{
  size_t pos = symbol.find(str1);
  while (pos != string::npos) // while there are str1:s to replace
    {
      symbol.erase(pos, str1.size()); // erase str1
      symbol.insert(pos, str2);       // insert str2 instead
      pos = symbol.find               // find next str1
        (str1, pos+str2.size());
    }
  return symbol;
}


static std::string get_print_format(const std::string &s)
{
  if (is_epsilon(s))
    return std::string(strdup(epsilon_format));

  if (quote_special)
    {
      return
        replace_all
        ( replace_all
          ( replace_all
            (s, "\\", "\\\\"),
            ":", "\\:"),
          " ", "\\ ");
    }

  return std::string(s);
}

static void print_lookup_string(const StringVector &s) {
  for (StringVector::const_iterator it = s.begin();
       it != s.end(); it++) {
#ifdef WINDOWS
    if (!pipe_output)
      hfst_fprintf_console(outfile, "%s", get_print_format(*it).c_str());
    else
#endif
      fprintf(outfile, "%s", get_print_format(*it).c_str());
  }
}

static std::string get_lookup_string(const StringVector &s) {
  std::string retval("");
  for (StringVector::const_iterator it = s.begin();
       it != s.end(); it++) {
    retval += get_print_format(*it);
  }
  return retval;
}



bool is_possible_to_get_result(const HfstOneLevelPath & s,
                               const StringSet & symbols_seen,
                               bool unknown_or_identity_seen)
{
  if (unknown_or_identity_seen)
    return true;
  StringVector sv = s.second;
  for (StringVector::const_iterator it = sv.begin(); it != sv.end(); it++)
    {
      if (symbols_seen.find(*it) == symbols_seen.end())
        return false;
    }
  return true;
}

// which transducer in the cascade we are handling
static unsigned int transducer_number=0;


void lookup_fd_and_print(HfstBasicTransducer * tr, HfstTransducer * TR, HfstOneLevelPaths& results,
                         const HfstOneLevelPath& s, size_t * limit /* = NULL*/, bool print_pairs_at_this_point/* = false*/,
                         bool print_fail /* = false*/, const HfstOneLevelPath * input_to_print /* = NULL*/,
                         bool no_newline /* = false*/)
{
  /* If we want a StringPairVector representation */
  HfstTwoLevelPaths results_spv;
  StringPairVector path_spv;

  if (tr != NULL)
    {
      if (is_possible_to_get_result(s, cascade_symbols_seen[transducer_number],
                                    cascade_unknown_or_identity_seen[transducer_number]))
        {
          tr->lookup(s.second, results_spv, limit,
                     NULL /*no weight limit, variable 'beam' defines which paths are printed */,
                     obey_flags);
        }
    }
  else
    {
      // TODO: is copying slow?
      std::string lookup_str("");
      for (std::vector<std::string>::const_iterator it = s.second.begin(); it != s.second.end(); it++)
        {
          lookup_str += *it;
        }
      HfstTwoLevelPaths * htlp = TR->lookup_pairs(lookup_str, *limit, time_cutoff);
      results_spv = HfstTwoLevelPaths(*htlp);
      delete htlp;
    }

  if (print_pairs_at_this_point && print_pairs) {
    
    /* No results, print just the lookup string. */
    if (results_spv.size() == 0) {
      if (print_fail)
        {
          std::string input = get_lookup_string(s.second);
#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(outfile, "%s\t%s+?\tinf\n\n", input.c_str(), input.c_str());
          else
#endif
            fprintf(outfile, "%s\t%s+?\tinf\n\n", input.c_str(), input.c_str());
          fflush(outfile);
        }
    }
    else {
      float lowest_weight = -1;
      /* For all result strings, */
      for (HfstTwoLevelPaths::const_iterator it
             = results_spv.begin(); it != results_spv.end(); it++) {

        // Extract output side for testing if it is a valid flag diacritic path
        StringVector sv;
        for (StringPairVector::const_iterator spv_it = it->second.begin();
             spv_it != it->second.end(); spv_it++)
          {
            sv.push_back(spv_it->second);
          }

          if (it == results_spv.begin())
            lowest_weight = it->first;
          if (beam < 0 || it->first <= (lowest_weight + beam))
            {
              /* print the lookup string */
              if (input_to_print != NULL)
                print_lookup_string(input_to_print->second);
              else
                print_lookup_string(s.second);

#ifdef WINDOWS
              if (!pipe_output)
                hfst_fprintf_console(outfile, "\t");
              else
#endif
                fprintf(outfile, "\t");

              /* and the path that yielded the result string */
              bool first_pair=true;
              for (StringPairVector::const_iterator IT = it->second.begin();
                   IT != it->second.end(); IT++) {
                if (show_flags || ! FdOperation::is_diacritic(IT->second))
                  {
#ifdef WINDOWS
                    if (!pipe_output)
                      {
                        if (print_space && ! first_pair) {
                          hfst_fprintf_console(outfile, " ");
                        }
                        hfst_fprintf_console(outfile, "%s:%s",
                          get_print_format(IT->first).c_str(),
                          get_print_format(IT->second).c_str());
                        first_pair=false;
                      }
                    else
#endif
                    {
                      if (print_space && ! first_pair) {
                        fprintf(outfile, " ");
                      }
                      fprintf(outfile, "%s:%s",
                              get_print_format(IT->first).c_str(),
                              get_print_format(IT->second).c_str());
                      first_pair=false;
                    }
                  }
              }
              /* and the weight of that path (add the weight of input). */
#ifdef WINDOWS
              if (!pipe_output)
                hfst_fprintf_console(outfile, "\t%f\n", it->first + s.first);
              else
#endif
                fprintf(outfile, "\t%f\n", it->first + s.first);
            }

      }
      if (!no_newline)
        {
#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(outfile, "\n");
          else
#endif
          fprintf(outfile, "\n");
        }

    }
    fflush(outfile);
  }

  // Convert HfstTwoLevelPaths into HfstOneLevelPaths
    for (HfstTwoLevelPaths::const_iterator it = results_spv.begin();
       it != results_spv.end(); it++)
    {
      StringVector sv;
      for (StringPairVector::const_iterator spv_it = it->second.begin();
           spv_it != it->second.end(); spv_it++)
        {
          sv.push_back(spv_it->second);
        }
      HfstOneLevelPath path(it->first, sv);
      results.insert(path);
    }
}



HfstOneLevelPaths*
lookup_simple(const HfstOneLevelPath& s, HfstBasicTransducer& t, bool* infinity, bool print_pairs_at_this_point=false, bool print_fail=false, const HfstOneLevelPath * input_to_print = NULL, bool no_newline=false)
{
  HfstOneLevelPaths* results = new HfstOneLevelPaths;

  bool possible = is_possible_to_get_result
    (s, cascade_symbols_seen[transducer_number],
     cascade_unknown_or_identity_seen[transducer_number]);

  if (possible && time_cutoff == 0.0 && t.is_lookup_infinitely_ambiguous(s, obey_flags))
    {
      if (!silent && infinite_cutoff > 0) {
    warning(0, 0, "Got infinite results, number of cycles limited to " SIZE_T_SPECIFIER "",
        infinite_cutoff);
      }
      lookup_fd_and_print(&t, NULL, *results, s, &infinite_cutoff, print_pairs_at_this_point, print_fail, input_to_print, no_newline);
      *infinity = true;
    }
  else
    {
      lookup_fd_and_print(&t, NULL, *results, s, NULL, print_pairs_at_this_point, print_fail, input_to_print, no_newline);
    }

  if (results->size() == 0)
    {
       // no results as empty result
      verbose_printf("Got no results\n");
    }

  return results;
}



HfstOneLevelPaths*
lookup_cascading(const HfstOneLevelPath& s, vector<HfstTransducer> & cascade,
                 bool* infinity)
{
  HfstOneLevelPaths* results = new HfstOneLevelPaths;

  // go through all transducers in the cascade
  for (unsigned int i = 0; i < cascade.size(); i++)
    {
      HfstOneLevelPaths* result = NULL;

      if ((cascade_ == CASCADE_COMPOSITION) && (i != 0))
        {
          result = new HfstOneLevelPaths;
          // use previous value of 'results' as input to composition
          for (HfstOneLevelPaths::const_iterator it = results->begin();
               it != results->end(); it++)
            {
              HfstOneLevelPaths * one_result = lookup_simple(*it, cascade[i], infinity, ((i+1) == cascade.size()), false, &s, true);
              //HfstOneLevelPaths * one_result = lookup_simple(*it, cascade[i], infinity);
              for (HfstOneLevelPaths::const_iterator IT = one_result->begin();
                   IT != one_result->end(); IT++)
                {
                  // add the weights
                  result->insert(HfstOneLevelPath(IT->first + it->first, IT->second));
                }
              delete one_result;
            }
          // zero 'results'
          delete results;
          results = new HfstOneLevelPaths();

          // cascading composition done
          if ( ((i+1) == cascade.size()) && print_pairs )
            {
              // no results from cascading composition
              if (result->size() == 0)
                {
                  std::string input;
                  for (StringVector::const_iterator it = s.second.begin(); it != s.second.end(); it++)
                    {
                      input += *it;
                    }
#ifdef WINDOWS
                  if (!pipe_output)
                    hfst_fprintf_console(outfile, "%s\t%s+?\tinf\n\n", input.c_str(), input.c_str());
                  else
#endif
                    fprintf(outfile, "%s\t%s+?\tinf\n\n", input.c_str(), input.c_str());
                }
              // results printed
              else
                {
#ifdef WINDOWS
                  if (!pipe_output)
                    hfst_fprintf_console(outfile, "\n");
                  else
#endif
                    fprintf(outfile, "\n");
                }
              fflush(outfile);
            }
        }
      else
        {
          result = lookup_simple(s, cascade[i], infinity);
        }

      if (infinity)
        {
          verbose_printf("Inf results @ level %u\n", i);
        }
      else
        {
          verbose_printf("" SIZE_T_SPECIFIER " results @ level %u\n", result->size(), i);
        }
      for (HfstOneLevelPaths::const_iterator it = result->begin();
           it != result->end(); it++)
        {
          results->insert(*it);
        }
      delete result;
      
      if ( (cascade_ == CASCADE_PRIORITY_UNION) && (results->size() != 0) )
        {
          verbose_printf("results found @ level %u, skipping rest of transducers (--cascade=priority-union)\n", i);
          break;
        }
    }
  // all transducers gone through

  return results;
}


HfstOneLevelPaths*
lookup_cascading(const HfstOneLevelPath& s, vector<HfstBasicTransducer> & cascade,
                 bool* infinity)
{
  HfstOneLevelPaths* results = new HfstOneLevelPaths;

  // go through all transducers in the cascade
  for (unsigned int i = 0; i < cascade.size(); i++)
    {
      transducer_number=i; // needed for lookup_simple

      HfstOneLevelPaths* result = NULL;
      if ((cascade_ == CASCADE_COMPOSITION) && (i != 0))
        {
          result = new HfstOneLevelPaths;
          // use previous value of 'results' as input to composition
          for (HfstOneLevelPaths::const_iterator it = results->begin();
               it != results->end(); it++)
            {
              //std::cerr << "using previous value of results as input to composition..." << std::endl;
              // if last transducer in cascade, print results if --print-pairs is requested
              HfstOneLevelPaths * one_result = lookup_simple(*it, cascade[i], infinity, ((i+1) == cascade.size()), false, &s, true);
              for (HfstOneLevelPaths::const_iterator IT = one_result->begin();
                   IT != one_result->end(); IT++)
                {
                  // add the weights
                  result->insert(HfstOneLevelPath(IT->first + it->first, IT->second));
                }
              delete one_result;
            }
          // zero 'results'
          delete results;
          results = new HfstOneLevelPaths();

          // cascading composition done
          if ( ((i+1) == cascade.size()) && print_pairs)
            {
              // no results from cascading composition
              if (result->size() == 0)
                {
                  std::string input;
                  for (StringVector::const_iterator it = s.second.begin(); it != s.second.end(); it++)
                    {
                      input += *it;
                    }
#ifdef WINDOWS
                  if (!pipe_output)
                    hfst_fprintf_console(outfile, "%s\t%s+?\tinf\n\n", input.c_str(), input.c_str());
                  else
#endif
                    fprintf(outfile, "%s\t%s+?\tinf\n\n", input.c_str(), input.c_str());
                }
              // results printed
              else
                {
#ifdef WINDOWS
                  if (!pipe_output)
                    hfst_fprintf_console(outfile, "\n");
                  else
#endif
                    fprintf(outfile, "\n");
                }
              fflush(outfile);
            }
        }
      else
        {
          result = lookup_simple(s, cascade[i], infinity, (cascade_ != CASCADE_COMPOSITION), false);
        }

      if (infinity)
        {
          verbose_printf("Inf results @ level %u\n", i);
        }
      else
        {
          verbose_printf("" SIZE_T_SPECIFIER " results @ level %u\n", result->size(), i);
        }

      for (HfstOneLevelPaths::const_iterator it = result->begin();
           it != result->end(); it++)
        {
          results->insert(*it);
        }
      delete result;

      if ( (cascade_ == CASCADE_PRIORITY_UNION) && (results->size() != 0) )
        {
          verbose_printf("results found @ level %u, skipping rest of transducers (--cascade=priority-union)\n", i);
          break;
        }
    }
  // all transducers gone through
  return results;
}


// limits kvs with beam
void
print_lookups(const HfstOneLevelPaths& kvs,
              const HfstOneLevelPath& kv, char* markup,
              bool outside_sigma, bool inf, FILE * ofile)
{
  float lowest_weight = -1;

    if (outside_sigma)
      {
        lookup_printf(unknown_begin_setf, &kv, NULL, markup, ofile);
        lookup_printf(unknown_lookupf, &kv, NULL, markup, ofile);
        lookup_printf(unknown_end_setf, &kv, NULL, markup, ofile);
        no_analyses++;
      }
    else if (kvs.size() == 0)
      {
        lookup_printf(empty_begin_setf, &kv, NULL, markup, ofile);
        lookup_printf(empty_lookupf, &kv, NULL, markup, ofile);
        lookup_printf(empty_end_setf, &kv, NULL, markup, ofile);
        no_analyses++;
      }
    else if (inf)
      {
        analysed++;
        lookup_printf(infinite_begin_setf, &kv, NULL, markup, ofile);
        for (HfstOneLevelPaths::const_iterator lkv = kvs.begin();
                lkv != kvs.end();
                ++lkv)
          {
            HfstOneLevelPath lup = *lkv;
            if (lkv == kvs.begin())
              lowest_weight = lup.first;
            if (beam < 0 || lup.first <= (lowest_weight + beam))
              {
                lookup_printf(infinite_lookupf, &kv, &lup, markup, ofile);
                analyses++;
              }
          }
        lookup_printf(infinite_end_setf, &kv, NULL, markup, ofile);
      }
    else
      {
        analysed++;

        lookup_printf(begin_setf, &kv, NULL, markup, ofile);
        for (HfstOneLevelPaths::const_iterator lkv = kvs.begin();
                lkv != kvs.end();
                ++lkv)
          {
            HfstOneLevelPath lup = *lkv;
            if (lkv == kvs.begin())
              lowest_weight = lup.first;
            if (beam < 0 || lup.first <= (lowest_weight + beam))
              {
                lookup_printf(lookupf, &kv, &lup, markup, ofile);
                analyses++;
              }
        }
        lookup_printf(end_setf, &kv, NULL, markup, ofile);
      }
}


HfstOneLevelPaths*
perform_lookups(HfstOneLevelPath& origin, std::vector<HfstTransducer>& cascade,
                bool unknown, bool* infinite)
{
  HfstOneLevelPaths* kvs;
    if (!unknown)
      {
        if (cascade.size() == 1)
          {
            kvs = lookup_simple(origin, cascade[0], infinite, true, true);
          }
        else
         {
           kvs = lookup_cascading(origin, cascade, infinite);
         }
      }
    else
      {
        kvs = new HfstOneLevelPaths;
      }
    return kvs;
}


HfstOneLevelPaths*
perform_lookups(HfstOneLevelPath& origin, std::vector<HfstBasicTransducer>& cascade,
                bool unknown, bool* infinite)
{
  HfstOneLevelPaths* kvs;
    if (!unknown)
      {
        if (cascade.size() == 1)
          {
            kvs = lookup_simple(origin, cascade[0], infinite, true, true);
          }
        else
         {
           kvs = lookup_cascading(origin, cascade, infinite);
         }
      }
    else
      {
        kvs = new HfstOneLevelPaths;
      }
    return kvs;
}

int
process_stream(HfstInputStream& inputstream, FILE* outstream)
{
    std::vector<HfstTransducer> cascade;
    std::vector<HfstBasicTransducer> cascade_mut;
    // set to false if non-ol transducer is pushed into the cascade
    bool only_optimized_lookup=true;
    
    size_t transducer_n=0;
    StringVector mc_symbols;
    bool id_or_unk_seen = false;
    while (inputstream.is_good())
    {
        transducer_n++;
        HfstTransducer trans(inputstream);
        hfst::ImplementationType type = trans.get_type();
        std::set<std::string> symbols_seen;

        if (type != HFST_OL_TYPE && type != HFST_OLW_TYPE)
          {
            only_optimized_lookup = false;
          }
        char* inputname = strdup(trans.get_name().c_str());
        if (strlen(inputname) <= 0)
          {
            inputname = strdup(inputfilename);
          }
        if (transducer_n==1)
          {
            verbose_printf("Reading %s...\n", inputname);
          }
        else
          {
            verbose_printf("Reading %s..." SIZE_T_SPECIFIER "\n", inputname,
                           transducer_n);
          }

        // add multicharacter symbols to mc_symbols
        if (type == hfst::SFST_TYPE ||
            type == hfst::TROPICAL_OPENFST_TYPE ||
            type == hfst::LOG_OPENFST_TYPE ||
            type == hfst::FOMA_TYPE)
        {
            HfstBasicTransducer basic(trans);
            for (HfstBasicTransducer::const_iterator it = basic.begin();
                 it != basic.end(); it++)
              {
                for (HfstBasicTransducer::HfstTransitions::const_iterator
                       tr_it = it->begin(); tr_it != it->end(); tr_it++)
                  {
                    std::string mcs = tr_it->get_input_symbol();
                    symbols_seen.insert(mcs);
                    if (mcs == hfst::internal_unknown || mcs == hfst::internal_identity)
                      id_or_unk_seen = true;
                    if (mcs.size() > 1) {
                      mc_symbols.push_back(mcs);
                      verbose_printf("multicharacter symbol: %s\n",
                                     mcs.c_str());
                    }
                  }
              }
            cascade_mut.push_back(basic);
            cascade_symbols_seen.push_back(symbols_seen);
            if (id_or_unk_seen)
              cascade_unknown_or_identity_seen.push_back(true);
            else
              cascade_unknown_or_identity_seen.push_back(false);
        }

        cascade.push_back(trans);
        id_or_unk_seen = false;
      }

    inputstream.close();

    /*
    if ((cascade_ == CASCADE_COMPOSITION || cascade_ == CASCADE_PRIORITY_UNION) &&
        (inputstream.get_type() == HFST_OL_TYPE ||
         inputstream.get_type() == HFST_OLW_TYPE) ) {
      error(EXIT_FAILURE, 0, "option --cascade not supported on "
              "optimized lookup transducers");
              }*/

    /*
    if (print_pairs &&
        (inputstream.get_type() == HFST_OL_TYPE ||
         inputstream.get_type() == HFST_OLW_TYPE) ) {
      error(EXIT_FAILURE, 0, "pair printing not supported on "
              "optimized lookup transducers");
    }
    */

    if ((!obey_flags) &&
        (inputstream.get_type() == HFST_OL_TYPE ||
         inputstream.get_type() == HFST_OLW_TYPE) ) {
      error(EXIT_FAILURE, 0, "not obeying flags not supported on "
              "optimized lookup transducers");
    }

    // if transducer type is other than optimized_lookup,
    // convert to HfstBasicTransducer

    char* line = 0;
    size_t llen = 0;

    hfst::HfstStrings2FstTokenizer input_tokenizer(mc_symbols,
                         std::string(epsilon_format));

    if (!only_optimized_lookup)
      {
        char* format_string = hfst_strformat(cascade[0].get_type());
        if (!silent) {
          warning(0, 0,
                  "It is not possible to perform fast lookups with %s "
                  "format automata.\n"
                  "Using HFST basic transducer format "
                  "and performing slow lookups",
                  format_string);
        }
        free(format_string);
      }
    long filesize = -1;
    if (show_progress_bar)
      {
        fprintf(stderr, "Counting file size...\n");
        fseek(lookup_file, 0L, SEEK_END);
        filesize = ftell(lookup_file);
        fprintf(stderr, "%ld... rewinding\n", filesize);
        rewind(lookup_file);
      }
    print_prompt();
    long filepos = ftell(lookup_file);
    while (true)
      {
#ifdef WINDOWS
        if (lookup_file == stdin && !pipe_input)
          {
            std::string str("");
            size_t bufsize = 1000;
            if (! hfst::get_line_from_console(str, bufsize))
              {
                break;
              }
            line = strdup(str.c_str());
          }
        else
          {
#endif
            if (hfst_getline(&line, &llen, lookup_file) == -1)
              break;
#ifdef WINDOWS
          }
#endif

        char * p = line;
        linen++;

        while (*p != '\0')
          {
            if (*p == '\n' || *p == '\r') // '\r' is possible on Windows
              {
                *p = '\0';
                break;
              }
            p++;
          }
        verbose_printf("Looking up %s...\n", line);
        filepos = ftell(lookup_file);
        if (show_progress_bar)
          {
            if (filesize != -1)
              {
                fprintf(stderr, "%ld / %ld...\r", filepos, filesize);
              }
            else
              {
                fprintf(stderr, "%ld / ?...\r", linen);
              }
          }

        char* markup = 0;
        bool unknown = false;
        bool infinite = false;
        HfstOneLevelPaths* kvs;

        HfstOneLevelPath* kv = line_to_lookup_path(&line, input_tokenizer,
                                                   &markup,
                                                   &unknown, only_optimized_lookup);

        if (verbose)
          {
            verbose_printf("Tokenized to: ");
            for (StringVector::const_iterator s = kv->second.begin();
                 s != kv->second.end();
                 ++s)
              {
                verbose_printf("%s ", s->c_str());
              }
            verbose_printf("\n");
          }
        if (only_optimized_lookup)
          {
            kvs = perform_lookups(*kv, cascade, unknown,
                                  &infinite);
          }
        else
          {
            kvs = perform_lookups(*kv, cascade_mut,
                                  unknown, &infinite);
          }

        if (! print_pairs) {
          // printing was already done in function lookup_fd
          print_lookups(*kvs, *kv, markup, unknown, infinite, outstream);
          fflush(outstream);
        }
        delete kv;
        delete kvs;

        print_prompt();
      } // while lines in input
    if (show_progress_bar)
      {
        fprintf(stderr, "%ld/%ld... Done\n", filepos, filesize);
      }
    free(line);
    if (print_statistics)
      {
        fprintf(outstream, "Strings\tFound\tMissing\tResults\n"
                "%lu\t%lu\t%lu\t%lu\n",
                inputs, analysed, no_analyses, analyses);
        fprintf(outstream, "Coverage\tAmbiguity\n"
                "%f\t%f\n",
                (float)analysed/(float)inputs,
                (float)analyses/(float)inputs);
      }
    return EXIT_SUCCESS;
}


int main( int argc, char **argv ) {
    hfst_setlocale();
    hfst_set_program_name(argv[0], "0.6", "HfstLookup");

    int retval = parse_options(argc, argv);
    if (retval != EXIT_CONTINUE)
    {
        return retval;
    }

#ifdef WINDOWS
    if (lookup_given)
      {
        _setmode(0, _O_BINARY);
      }

    if (inputfile == stdin && pipe_input && show_progress_bar)
      {
        // todo: print warning?
        show_progress_bar = false;
      }

    // has no effect on windows or mac
    //hfst::print_output_to_console(!pipe_output);
#endif

    // close buffers, we use streams
    if (inputfile != stdin)
    {
        fclose(inputfile);
    }
    verbose_printf("Reading from %s, writing to %s\n",
        inputfilename, outfilename);
    verbose_printf("Output formats:\n"
            "  regular:`%s'`%s...'`%s',\n"
            "  unanalysed:`%s'`%s'`%s',\n"
            "  untokenised:`%s'`%s'`%s',\n"
            "  infinite:`%s'`%s'`%s\n"
            "  epsilon: `%s', space: `%s', flags: %d\n",
            begin_setf, lookupf, end_setf,
            empty_begin_setf, empty_lookupf, empty_end_setf,
            unknown_begin_setf, unknown_lookupf, unknown_end_setf,
            infinite_begin_setf, infinite_lookupf, infinite_end_setf,
            epsilon_format, space_format, show_flags);
    // here starts the buffer handling part
    HfstInputStream* instream = NULL;
    try
      {
        instream = (inputfile != stdin) ?
          new HfstInputStream(inputfilename) :
          new HfstInputStream();
      }
    catch(const HfstException e)
      {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              inputfilename);
        return EXIT_FAILURE;
      }
    process_stream(*instream, outfile);
    if (outfile != stdout)
    {
        fclose(outfile);
    }
    delete instream;
    free(inputfilename);
    free(outfilename);
    return EXIT_SUCCESS;
}

