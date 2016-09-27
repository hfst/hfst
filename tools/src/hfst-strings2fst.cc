//! @file hfst-strings2fst.cc
//!
//! @brief string compiling command line tool
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

using std::vector;
using std::pair;

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _MSC_VER
#  include "hfst-getopt.h"
#else
#  include <getopt.h>
#endif

#include <math.h>
#include <errno.h>

#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"
#include "implementations/HfstBasicTransducer.h"
#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "hfst-tool-metadata.h"
#include "HfstStrings2FstTokenizer.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

using hfst::HfstOutputStream;
using hfst::HfstTokenizer;
using hfst::HfstTransducer;
using hfst::implementations::HfstBasicTransducer;
//using hfst::HfstInternalTransducer;
//using hfst::implementations::HfstTrie;
using hfst::StringPairVector;
using hfst::StringPair;
using hfst::StringSet;
using hfst::StringVector;

static char *epsilonname=NULL; // FIX: use this
static bool has_spaces=false;
static bool disjunct_strings=false;
static bool pairstrings=false;
static char *multichar_symbol_filename=NULL;
static StringVector multichar_symbols;

static float sum_of_weights=0;
static bool normalize_weights=false;
static bool logarithmic_weights_e=false;
static bool logarithmic_weights_10=false;

static hfst::ImplementationType output_format = hfst::UNSPECIFIED_TYPE;

float divide_by_sum_of_weights(float weight) {
  if (sum_of_weights == 0)
    return 0;
  return weight/sum_of_weights;
}
float take_negative_logarithm_e(float weight) {
  float result;
  if (weight == 0)
    result = 0; // shoud be INFINITY, but doesn't work in transitions
  else
    {
      errno = 0;
      result = -log(weight);
      if (errno != 0)
        {
          error(EXIT_FAILURE, errno, "unable to take negative logarithm");
        }
    }
  return result;
}
float take_negative_logarithm_10(float weight) {
  float result;
  if (weight == 0)
    result = 0; // shoud be INFINITY, but doesn't work in transitions
  else
    {
      errno = 0;
      result = -log10(weight);
      if (errno != 0)
        {
          error(EXIT_FAILURE, errno, "unable to take negative logarithm");
        }
    }
  return result;
}

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
        "Compile string pairs and pair-strings into transducer(s)\n"
        "\n", program_name);
        print_common_program_options(message_out);
        fprintf(message_out,
    "Input/Output options:\n"
    "  -i, --input=INFILE     Read input strings from INFILE\n"
    "  -o, --output=OUTFILE   Write output transducer to OUTFILE\n");
        fprintf(message_out, "String and format options:\n"
        "  -f, --format=FMT          Write result in FMT format\n"
        "  -j, --disjunct-strings    Disjunct all strings instead of transforming\n"
        "                            each string into a separate transducer\n"
      /*"      --sum                 Sum weights of duplicate strings\n"
        "                            instead of taking minimum\n"*/
        "      --norm                Divide each weight by sum of all weights\n"
        "                            (with option -j)\n"
        "      --log                 Take negative natural logarithm of each weight\n"
        "      --log10               Take negative 10-based logarithm of each weight\n"
        "  -p, --pairstrings         Input is in pairstring format\n"
        "  -S, --has-spaces          Input has spaces between symbols/symbol pairs\n"
        "  -e, --epsilon=EPS         Interpret string EPS as epsilon.\n"
        "  -m, --multichar-symbols=FILE   Strings that must be tokenized as one symbol.\n"
        );
        fprintf(message_out, "\n");

        fprintf(message_out,
        "If OUTFILE or INFILE is missing or -, standard streams will be used.\n"
        "FMT can be { foma, openfst-tropical, openfst-log, sfst, \n"
    "optimized-lookup-weighted, optimized-lookup-unweighted }.\n"
        "If EPS is not defined, the default representation of @0@ is used.\n"
        "Option --norm precedes option --log.\n"
        "The FILE of option -m lists all multichar-symbols, each symbol\n"
        "on its own line.\n"
        "Backslash '\\' may be used to escape ':', tab and itself. For any\n"
        "other symbol x '\\x' means x literally, i.e. is the same as 'x'.\n"
        "The weight of a string can be given after the string separated\n"
        "by a tabulator. The weight cannot be zero.\n"
        "\n"
            );

        fprintf(message_out, "Examples:\n"
            "  echo \"cat:dog\" | %s            create cat:dog fst\n"
            "  echo \"c:da:ot:g\" | %s -p       same as pairstring\n"
            "  echo \"c:d a:o t:g\" | %s -p -S  same as pairstring with spaces\n"
            "  echo \"c a t:d o g\" | %s -S     same with spaces\n"
                "\n", program_name, program_name, program_name, program_name);
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
          {"disjunct-strings", no_argument, 0, 'j'},
          {"epsilon", required_argument, 0, 'e'},
          {"norm", no_argument, 0, '2'},
          {"log", no_argument, 0, '3'},
          {"log10", no_argument, 0, '4'},
          {"pairstrings", no_argument, 0, 'p'},
          {"has-spaces", no_argument, 0, 'S'},
          {"multichar-symbols", required_argument, 0, 'm'},
          {"format", required_argument, 0, 'f'},
          {0,0,0,0}
        };
        int option_index = 0;
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "je:234pSm:f:",
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
        case '2':
            normalize_weights = true;
            break;
        case '3':
            logarithmic_weights_e = true;
            break;
        case '4':
            logarithmic_weights_10 = true;
            break;
        case 'j':
            disjunct_strings = true;
            break;
        case 'S':
            has_spaces = true;
            break;
        case 'p':
            pairstrings = true;
            break;
    case 'm':
        multichar_symbol_filename = hfst_strdup(optarg);
        break;
        case 'f':
            output_format = hfst_parse_format_name(optarg);
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
    if (epsilonname == 0)
      {
        epsilonname = hfst_strdup("@0@");
      }
    return EXIT_CONTINUE;
}


int
process_stream(HfstOutputStream& outstream)
{
  size_t transducer_n = 0;
  char* line = 0;
  size_t len = 0;
  HfstTokenizer tok;
  HfstBasicTransducer disjunction;
  size_t line_n = 0;

  hfst::HfstStrings2FstTokenizer
    multichar_symbol_tokenizer(multichar_symbols,std::string(epsilonname));

  while (hfst_getline(&line, &len, inputfile) != -1)
    {
      transducer_n++;
      line_n++;
      verbose_printf("Parsing line %u...\n", line_n);
      char* orig_line = hfst_strdup(line);
      // parse line end and weight
      char* tab = strstr(line, "\t");
      char* string_end = tab;
      double weight = 0.0;
      bool weighted = false;
      if (tab == NULL)
        {
          string_end = line;
          while ((*string_end != '\0') && (*string_end != '\n') &&
                 (*string_end != '\r'))
            {
              string_end++;
            }
        }
      else
        {
      // change '\n' to '\0'
      char *p = tab;
      while (*p != '\0') {
        if ((*p == '\n') || (*p == '\r'))
          *p = '\0';
        p++;
      }

          weight = hfst_strtoweight(tab+1);
          weighted = true;
        }
      *string_end = '\0';

      // Parse the string
      StringPairVector spv;
      try
        {
          if (pairstrings)
            { spv = multichar_symbol_tokenizer.tokenize_pair_string
                (line,has_spaces); }
          else
            { spv = multichar_symbol_tokenizer.tokenize_string_pair
                (line,has_spaces); }
        }
      catch (const hfst::UnescapedColsFound &e)
        {
          if (pairstrings)
            {
              error_at_line
                (EXIT_FAILURE, errno, inputfilename, line_n,
                 "String `%s' contains unescaped ':'-symbols,\n"
                 "which are not pair separators. Use `\\:' for literal `:'.",
                 line);
            }
          else
            {
              error_at_line
                (EXIT_FAILURE, errno, inputfilename, line_n,
                 "String `%s' contains unescaped `:'-symbols,\n"
                 "which are not pair separators. Use `\\:\' for literal `:'.\n"
                 "If you are compiling pair strings, use option -p.",
                 line);
            }
        }
      catch (const IncorrectUtf8CodingException &e)
        {
            error_at_line(EXIT_FAILURE, errno, inputfilename, line_n,
                  "Input string `%s' is not valid utf-8.", line);
        }
      // Handle the weight
      float path_weight=0;

      if (weighted)
        {
          sum_of_weights = sum_of_weights + weight;
      path_weight=weight;
          verbose_printf("Using final weight %f...\n", weight);
        }

      if (!disjunct_strings) // each string into a transducer
        {
          HfstBasicTransducer tr;

      if (logarithmic_weights_e)
        {
          path_weight = take_negative_logarithm_e(weight);
        }
      else if (logarithmic_weights_10)
        {
          path_weight = take_negative_logarithm_10(weight);
        }
      else {}

          tr.disjunct(spv, path_weight);
          HfstTransducer res(tr, output_format);
          hfst_set_name(res, "", "string");
          outstream << res;
          free(orig_line);
        }
      else // disjunct all strings into a single transducer
        {
      // do not take negative logarithm yet
          disjunction.disjunct(spv, path_weight);
        }
    }
  free(line);
  if (disjunct_strings)
    {
      HfstTransducer res(disjunction, output_format);

      if (normalize_weights)
        {
          verbose_printf("Normalising weights...\n");
      res.transform_weights(&divide_by_sum_of_weights);
        }
                              if (logarithmic_weights_e)
                            {
                                  verbose_printf("Taking negative logarithm...\n");
                                  res.transform_weights(&take_negative_logarithm_e);
                                }
                              else if (logarithmic_weights_10)
                            {
                                  verbose_printf("Taking negative logarithm...\n");
                                  res.transform_weights(&take_negative_logarithm_10);
                                }
                              else {}

      hfst_set_name(res, "?", "strings");
      outstream << res;
    }
  return EXIT_SUCCESS;
}


int main( int argc, char **argv )
{
#ifdef WINDOWS
  _setmode(1, _O_BINARY);
#endif

  hfst_set_program_name(argv[0], "0.1", "Strings2Fst");
  int retval = parse_options(argc, argv);
  if (retval != EXIT_CONTINUE)
    {
      return retval;
    }

  if (multichar_symbol_filename != NULL)
    {
      verbose_printf("Reading multichar symbols from %s\n",
                     multichar_symbol_filename);
      std::ifstream multichar_in(multichar_symbol_filename);
      (void)multichar_in.peek();
      if (! multichar_in.good())
        { error(EXIT_FAILURE, errno,"Multichar symbol file can't be read."); }
      char multichar_line[1000];
      while (multichar_in.good())
        {
          multichar_in.getline(multichar_line,1000);
          if (strlen(multichar_line) > 0)
            {
              verbose_printf("Defining multichar symbol %s\n",multichar_line);
              multichar_symbols.push_back(multichar_line);
            }
        }
    }

  // close output buffers, we use output streams
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
  free(inputfilename);
  free(outfilename);
  return EXIT_SUCCESS;
}

