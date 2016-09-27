//! @file hfst-regexp2fst.cc
//!
//! @brief regular expression compiling command line tool
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


#include <iostream>
#include <fstream>

#include <vector>
#include <map>
#include <string>

using std::string;
using std::vector;
using std::pair;

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <math.h>
#include <errno.h>

#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"
#include "parsers/XreCompiler.h"
#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "hfst-tool-metadata.h"
#include "hfst-file-to-mem.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

using hfst::HfstOutputStream;
using hfst::HfstTokenizer;
using hfst::HfstTransducer;
using hfst::xre::XreCompiler;

static char *epsilonname=NULL;
static bool disjunct_expressions=false;
static bool line_separated=true;

static bool encode_weights=false;

//static unsigned int sum_of_weights=0;
//static bool sum_weights=false;
//static bool normalize_weights=false;
//static bool logarithmic_weights=false;

static hfst::ImplementationType output_format = hfst::UNSPECIFIED_TYPE;

static bool harmonize=true;
static bool harmonize_flags=false;
static bool minimize_result=true;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
    "Compile (weighted) regular expressions into transducer(s)"
        "\n", program_name);
        print_common_program_options(message_out);
        print_common_unary_program_options(message_out);
        fprintf(message_out,
"String and format options:\n"
"  -f, --format=FMT          Write result in FMT format\n"
"  -j, --disjunct            Disjunct all regexps instead of transforming\n"
"                            each regexp into a separate transducer\n"
                //"      --sum (todo)          Sum weights of duplicate strings instead of \n"
                //"                            taking minimum\n"
                //"      --norm (todo)         Divide each weight by sum of all weights\n"
                //"      --log (todo)          Take negative logarithm of each weight\n"
"  -l, --line                Input is line separated (default)\n"
"  -S, --semicolon           Input is semicolon separated\n"
"  -e, --epsilon=EPS         Map EPS as zero, i.e. epsilon.\n"
"  -x, --xerox-composition=VALUE Whether flag diacritics are treated as ordinary\n"
"                                symbols in composition (default is false).\n"
"  -X, --xfst=VARIABLE       Toggle xfst compatibility option VARIABLE.\n"
"Harmonization and optimization options:\n"
"  -H, --do-not-harmonize    Do not expand '?' symbols.\n"
"  -F, --harmonize-flags     Harmonize flag diacritics.\n"
"  -E, --encode-weights      Encode weights when minimizing (default is false).\n"
"  -M, --do-not-minimize     Determinize result instead of minimizing it.\n"
                );
        fprintf(message_out, "\n");

        fprintf(message_out,
            "If OUTFILE or INFILE is missing or -, standard streams will be used.\n"
            "FMT must be one of the following: "
            "{foma, sfst, openfst-tropical, openfst-log}.\n"
            "If EPS is not defined, the default representation of 0 is used\n"
            "VALUEs recognized are {true,ON,yes} and {false,OFF,no}.\n"
            "Xfst variables are {flag-is-epsilon (default OFF)}.\n"
            "\n"
            );

        fprintf(message_out, "Examples:\n"
"  echo \" {cat}:{dog} \" | %s       create transducer {cat}:{dog}\n"
"  echo \" {cat}:{dog}::3 \" | %s    same but with weight 3\n"
"  echo \" c:d a:o::3 t:g \" | %s    same but with weight 3\n"
"                                               in the middle\n"
//"  echo \" {cat}:{dog} ; 3 \" | %s   legacy way of defining weights\n"
"  echo \" cat ; dog ; \"3\" \" | %s -S  create transducers\n"
"                                               \"cat\" and \"dog\" and \"3\"\n"
                "\n", program_name, program_name, program_name, program_name, program_name);
        print_report_bugs();
        fprintf(message_out, "\n");
        print_more_info();
        fprintf(message_out, "\n");
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
          {"disjunct", no_argument, 0, 'j'},
          {"epsilon", required_argument, 0, 'e'},
        //  {"sum", no_argument, 0, '1'},
        //  {"norm", no_argument, 0, '2'},
        //  {"log", no_argument, 0, '3'},
          {"line", no_argument, 0, 'l'},
          {"semicolon", no_argument, 0, 'S'},
          {"format", required_argument, 0, 'f'},
          {"do-not-harmonize", no_argument, 0, 'H'},
          {"harmonize-flags", no_argument, 0, 'F'},
          {"encode-weights", no_argument, 0, 'E'},
          {"xerox-composition", required_argument, 0, 'x'},
          {"xfst", required_argument, 0, 'X'},
          {"do-not-minimize", no_argument, 0, 'M'},
          {0,0,0,0}
        };
        int option_index = 0;
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "je:lSf:HFEx:X:M"/*"123"*/,
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
        case 'e':
            epsilonname = hfst_strdup(optarg);
            break;
            /*        case '1':
            sum_weights = true;
            break;
        case '2':
            normalize_weights = true;
            break;
        case '3':
            logarithmic_weights = true;
            break;*/
        case 'j':
            disjunct_expressions = true;
            break;
        case 'S':
            line_separated = false;
            break;
        case 'l':
            line_separated = true;
            break;
        case 'f':
            output_format = hfst_parse_format_name(optarg);
            break;
        case 'H':
            harmonize=false;
            break;
        case 'F':
          harmonize_flags=true;
          break;
        case 'E':
          encode_weights=true;
          break;
        case 'M':
          minimize_result=false;
          break;
        case 'x':
          {
            const char * argument = hfst_strdup(optarg);
            if (strcmp(argument, "yes") == 0 ||
                strcmp(argument, "true") == 0 ||
                strcmp(argument, "ON") == 0)
              {
                hfst::set_xerox_composition(true);
              }
            else if (strcmp(argument, "no") == 0 ||
                     strcmp(argument, "false") == 0 ||
                     strcmp(argument, "OFF") == 0)
              {
                hfst::set_xerox_composition(false);
              }
            else
              {
                fprintf(stderr, "Error: unknown option to --xerox-composition: '%s'\n", optarg);
                return EXIT_FAILURE;
              }
          }
          break;
        case 'X':
          {
            const char * argument = hfst_strdup(optarg);
            if (strcmp(argument, "flag-is-epsilon") == 0)
              {
                hfst::set_flag_is_epsilon_in_composition(true);
              }
            else
              {
                fprintf(stderr, "Error: unknown option to --xfst: '%s'\n", optarg);
                return EXIT_FAILURE;
              }
          }
          break;
#include "inc/getopt-cases-error.h"
        }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    if (output_format == hfst::UNSPECIFIED_TYPE)
      {
        verbose_printf("Output format not specified, "
             "defaulting to openfst tropical\n");
        output_format = hfst::TROPICAL_OPENFST_TYPE;
      }
    return EXIT_CONTINUE;
}

int
process_stream(HfstOutputStream& outstream)
{
  size_t transducer_n = 0;
  char* line = 0;
  size_t len = 0;
  unsigned int line_count = 0;
  XreCompiler comp(output_format);
  comp.set_verbosity(verbose);
  comp.set_error_stream(&std::cerr);
  comp.set_harmonization(harmonize);
  comp.set_flag_harmonization(harmonize_flags);
  hfst::set_minimization(minimize_result);
  HfstTransducer disjunction(output_format);

  char delim = (line_separated)? '\n' : ';';
  char* first_line = 0;

  if (!line_separated)
    {
      char * filebuf_ = hfst_file_to_mem(inputfilename);
      unsigned int chars_read = 0;
      HfstTransducer * compiled;

      while(true)
        {
          transducer_n++;
          verbose_printf("Compiling expression #%i\n", (int)transducer_n);
          try
            {
              compiled = comp.compile_first(filebuf_, chars_read);
            }
          catch (const HfstException & e)
            {
              error(EXIT_FAILURE, 0, "%s: XRE parsing failed "
                    "in expression #%u separated by semicolons:\n%s", inputfilename,
                    (unsigned int)transducer_n, e.what().c_str());
            }
          if (compiled == NULL)
            {
              if (comp.contained_only_comments())
                {
                  if (transducer_n == 1)
                    {
                      error(EXIT_FAILURE, 0, "%s: XRE parsing failed: expression #%u "
                            "contains only whitespace or comments", inputfilename,
                            (unsigned int)transducer_n);
                    }
                  break;
                }
              else
                {
                  error(EXIT_FAILURE, 0, "%s: XRE parsing failed "
                        "in expression #%u separated by semicolons", inputfilename,
                        (unsigned int)transducer_n);
                }
            }
          for (unsigned int i=0; i < chars_read; i++)
            {
              filebuf_++;
            }
          if (compiled != NULL)
            {
              if (disjunct_expressions)
                {
                  disjunction.disjunct(*compiled, harmonize);
                }
              else
                {
                  hfst_set_name(*compiled, "?", "xre");
                  outstream << *compiled;
                }
              delete compiled;
            }
          if (*filebuf_ == '\0')
            {
              break;
            }
        }
    }
  else
    {
      bool input_contains_only_whitespace_or_comments = true;
      while (true)
        {
          if (hfst_getdelim(&line, &len, delim, inputfile) == -1)
            {
              if (input_contains_only_whitespace_or_comments)
                {
                  error(EXIT_FAILURE, 0, "%s: XRE parsing failed:"
                        " input contains only whitespace or comments", inputfilename);
                }
              break;
            }
          if (first_line == 0)
            {
              first_line = strdup(line);
            }
          char* exp = line;
          while ((*exp == '\n') || (*exp == '\r') || (*exp == ' '))
            {
              exp++;
            }
          line_count++;
          if (*exp == '\0')
            {
              verbose_printf("Skipping whitespace expression #%u", line_count);
              continue;
            }
          transducer_n++;
          HfstTransducer* compiled;
          verbose_printf("Compiling expression %u\n", line_count);
          try
            {
              compiled = comp.compile(exp);
            }
          catch (HfstException & e)
            {
              error_at_line(EXIT_FAILURE, 0, inputfilename, line_count,
                            "XRE parsing failed");
            }
          if (compiled == NULL)
            {
              if (!comp.contained_only_comments())
                {
                  error_at_line(EXIT_FAILURE, 0, inputfilename, line_count,
                                "XRE parsing failed");
                }
              continue;
            }
          input_contains_only_whitespace_or_comments = false;

          if (disjunct_expressions)
            {
              disjunction.disjunct(*compiled, harmonize);
            }
          else
            {
              hfst_set_name(*compiled, "?", "xre");
              outstream << *compiled;
            }
          delete compiled;
          
        }
    }


  if (disjunct_expressions)
    {
      if (delim == '\n')
        {
          hfst_set_name(disjunction,
                        "?",
                        "xre");
        }
      else
        {
          hfst_set_name(disjunction, "?", "xre");
        }
      outstream << disjunction;
    }
  free(line);
  free(first_line);
  return EXIT_SUCCESS;
}

//extern int xredebug;

int main( int argc, char **argv )
  {
#ifdef WINDOWS
    _setmode(1, _O_BINARY);
#endif

    hfst_set_program_name(argv[0], "0.2", "Regexp2Fst");
    int retval = parse_options(argc, argv);
    if (retval != EXIT_CONTINUE)
      {
        return retval;
      }
    if (debug)
      {
        //xredebug = 1;
      }

    bool enc = hfst::get_encode_weights();
    if (encode_weights)
      {
        hfst::set_encode_weights(true);
      }

  // close buffers, we use streams
  if (outfile != stdout)
    {
      fclose(outfile);
    }
  verbose_printf("Reading from %s, writing to %s\n",
                 inputfilename, outfilename);
  // here starts the buffer handling part
  HfstOutputStream* outstream = (outfile != stdout) ?
        new HfstOutputStream(outfilename, output_format) :
        new HfstOutputStream(output_format);
  process_stream(*outstream);
  delete outstream;

    if (encode_weights)
      {
        hfst::set_encode_weights(enc);
      }


  free(inputfilename);
  free(outfilename);
  return EXIT_SUCCESS;
}

