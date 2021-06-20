//! @file hfst-fst2strings.cc
//!
//! @brief Transducer path printing command line tool
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
#include <memory>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"
#include "HfstSymbolDefs.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::FdOperation;

//using hfst::WeightedPaths;
//using hfst::WeightedPath;
using hfst::StringPairVector;
using hfst::HfstTwoLevelPath;
using hfst::HfstTwoLevelPaths;

using std::unique_ptr;
using std::make_unique;

using hfst::HFST_OL_TYPE;
using hfst::HFST_OLW_TYPE;

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

// the maximum number of strings printed for each transducer
static int max_strings = 0;
static int cycles = -1;
static int nbest_strings=-1;
static int max_random_strings=-1;
static float max_weight=-1;
static float beam=-1;
static bool display_weights=false;
static bool eval_fd=false;
static bool filter_fd=true;
static bool quote_special=false;
static bool print_spaces=false;
static unsigned int max_input_length = 0;
static unsigned int max_output_length = 0;
static std::string input_prefix;
static std::string output_prefix;
static std::string input_exclude;
static std::string output_exclude;

static bool print_in_pairstring_format=false;
static char * epsilon_format=0;

static bool print_separator_after_each_transducer=false;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
        "Display the strings recognized by a transducer\n"
        "\n", program_name);
    print_common_program_options(message_out);
    fprintf(message_out, "Fst2strings options:\n"
"  -n, --max-strings=NSTR     print at most NSTR strings\n"
"  -N, --nbest=NBEST          print at most NBEST best strings\n"
"  -r, --random=NRAND         print at most NRAND random strings\n"
"  -c, --cycles=NCYC          follow cycles at most NCYC times\n"
"  -w, --print-weights        display the weight for each string\n"
"  -S, --print-separator      print separator \"--\" after each transducer\n"
"  -e, --epsilon-format=EPS   print epsilon as EPS\n"
"  -X, --xfst=VARIABLE        toggle xfst compatibility option VARIABLE\n");
    fprintf(message_out, "Path filters:\n"
"  -b, --beam=B               reject output string with weight more than B away from\n"
"                             the weight of the best output string\n"
"  -l, --max-in-length=MIL    reject input string longer than MIL\n"
"  -L, --max-out-length=MOL   reject output string longer than MOL\n"
"  -p, --in-prefix=OPREFIX    input string must begin with IPREFIX\n"
"  -P, --out-prefix=OPREFIX   output string must begin with OPREFIX\n"
"  -u, --in-exclude=IXSTR     input string must not contain IXSTR\n"
"  -U, --out-exclude=OXST     output string must not contain OXSTR\n");

    fprintf(message_out, "\n");

    print_common_unary_program_parameter_instructions(message_out);
    fprintf(message_out, "If all NSTR, NBEST and NCYC are omitted, "
            "all possible paths are printed:\n"
            "NSTR, NBEST and NCYC default to infinity.\n"
            "NBEST overrides NSTR and NCYC\n"
        "NRAND overrides NBEST, NSTR and NCYC\n"
            "B must be a non-negative float\n"
            "If EPS is not given, default is empty string.\n"
            "Numeric options are parsed with strtod(3).\n"
        "Xfst variables supported are { obey-flags, print-flags,\n"
        "print-pairs, print-space, quote-special }.\n");
    fprintf(message_out,
        "\n"
        "Examples:\n"
        "  %s lexical.hfst    generates all forms of lexical.hfst\n"
        "  %s -P \"cat<n>\" -c 0 lexical.hfst\n"
        "                     generates paradigm for cat<n> without following cycles\n"
        "\n", program_name, program_name);

    fprintf(message_out,
        "Known bugs:\n"
        "  Does not work correctly for hfst optimized lookup format.\n"
        "\n"
        );

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
            {"beam", required_argument, 0, 'b'},
            {"cycles", required_argument, 0, 'c'},
            {"epsilon-format", required_argument, 0, 'e'},
            {"in-exclude", required_argument, 0, 'u'},
            {"in-prefix", required_argument, 0, 'p'},
            {"max-in-length", required_argument, 0, 'l'},
            {"max-out-length", required_argument, 0, 'L'},
            {"max-strings", required_argument, 0, 'n'},
            {"nbest", required_argument, 0, 'N'},
            {"random", required_argument, 0, 'r'},
            {"print-separator", no_argument, 0, 'S'},
            {"out-exclude", required_argument, 0, 'U'},
            {"out-prefix", required_argument, 0, 'P'},
            {"print-weights", no_argument, 0, 'w'},
            {"xfst", required_argument, 0, 'X'},
            {0,0,0,0}
          };
        int option_index = 0;
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT
                             "Swb:c:e:u:p:l:L:n:r:N:U:P:X:",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }
        //char *level = NULL;
        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
        case 'n':
            max_strings = hfst_strtoul(optarg, 10);
            break;
        case 'N':
            nbest_strings = hfst_strtoul(optarg, 10);
            break;
        case 'r':
            max_random_strings = hfst_strtoul(optarg, 10);
            break;
        case 'b':
          beam = atof(optarg);
          if (beam < 0)
            {
              std::cerr << "Invalid argument for --beam\n";
              return EXIT_FAILURE;
            }
          break;
        case 'c':
            cycles = hfst_strtoul(optarg, 10);
            break;
        case 'w':
            display_weights = true;
            break;
        case 'X':
            if (strcmp(optarg, "obey-flags") == 0)
              {
                eval_fd = true;
              }
            else if (strcmp(optarg, "print-flags") == 0)
              {
                filter_fd = false;
              }
            else if (strcmp(optarg, "quote-special") == 0)
              {
                quote_special = true;
              }
            else if (strcmp(optarg, "print-pairs") == 0)
              {
                print_in_pairstring_format = true;
              }
            else if (strcmp(optarg, "print-space") == 0)
              {
                print_spaces = true;
              }
            else
              {
                error(0, EXIT_FAILURE, "Unrecognised xfst option. "
                     "available options are obey-flags, print-flags\n");
              }
          break;
        case 'l':
            max_input_length = hfst_strtoul(optarg, 10);
            break;
        case 'L':
            max_output_length = hfst_strtoul(optarg, 10);
            break;
        case 'p':
            input_prefix = optarg;
            break;
        case 'P':
            output_prefix = optarg;
            break;
        case 'u':
            input_exclude = optarg;
            break;
        case 'U':
          output_exclude = optarg;
          break;
        case 'S':
          print_separator_after_each_transducer = true;
          break;
        case 'e':
          epsilon_format = hfst_strdup(optarg);
          break;
#include "inc/getopt-cases-error.h"
        }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    return EXIT_CONTINUE;
}

/* Replace all strings \a str1 in \a symbol with \a str2. */
static std::string replace_all(std::string symbol,
                   const std::string &str1,
                   const std::string &str2)
{
  size_t pos = symbol.find(str1);
  while (pos != std::string::npos) // while there are str1:s to replace
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
  // print epsilon as defined by the user or use the default
  if (hfst::is_epsilon(s))
    return std::string(strdup(epsilon_format));

  if (not quote_special)
    return std::string(s);

  // escape spaces and colons as they have a special meaning
  return
    replace_all
    (replace_all
     ( replace_all
       (std::string(s), " ", "@_SPACE_@"),
       ":", "@_COLON_@"),
     "\t", "@_TAB_@");
}

//Print results as they come
class Callback : public hfst::ExtractStringsCb
{
public:
  int count;
  int max_num;
  std::ostream* out_;
  Callback(int max, std::ostream* out): count(0), max_num(max), out_(out) {}
  RetVal operator()(HfstTwoLevelPath &path, bool final)
  {
    std::string istring;
    std::string ostring;
    for (StringPairVector::const_iterator it = path.second.begin();
         it != path.second.end(); it++) {
      istring.append(it->first);
      ostring.append(it->second);
    }
    float weight = path.first;

    if ((max_input_length > 0) &&
        (istring.length() > max_input_length))
      {
        // continue searching, break off this path
        return RetVal(true, false);
      }
    if ((max_output_length > 0) &&
        (ostring.length() > max_output_length))
      {
      return RetVal(true, false);
      // continue searching, break off this path
      }
    if (input_prefix.length() > 0)
      {
        if (istring.length() < input_prefix.length())
          {
            return RetVal(true, true);
          }
      if (istring.compare(0, input_prefix.length(), input_prefix) != 0)
        {
          return RetVal(true, false);
          // continue searching, break off this path
        }
      }
    if(output_prefix.length() > 0)
      {
        if (ostring.length() < output_prefix.length())
          {
            return RetVal(true, true);
          }
        if (ostring.compare(0, output_prefix.length(), output_prefix) != 0)
          {
            return RetVal(true, false);
            // continue searching, break off this path
          }
      }
    if (input_exclude.length() > 0 &&
        (istring.find(input_exclude) != std::string::npos))
      {
        return RetVal(true, false);
        // continue searching, break off this path
      }
    if (output_exclude.length() > 0 &&
        (ostring.find(output_exclude) != std::string::npos))
      {
        return RetVal(true, false);
        // continue searching, break off this path
      }
    if (max_weight >= 0 && weight > (max_weight + beam))
      {
        return RetVal(true, false);
        // continue searching, break off this path
      }
    // the path passed the checks. Print it if it is final
    if (final)
      {
        if (print_in_pairstring_format)
          {
        bool first_pair=true;
            for (StringPairVector::const_iterator it = path.second.begin();
                 it != path.second.end(); it++)
              {
        if ((not filter_fd) ||
            (not FdOperation::is_diacritic(it->first))) {
          if (print_spaces && not first_pair)
            {
              *out_ << " ";
            }

          *out_ << get_print_format(it->first);
          first_pair=false;
        }

        if (it->first.compare(it->second) != 0)
          {
            if ((not filter_fd) ||
            (not FdOperation::is_diacritic(it->second)))
              *out_ << ":"
                << get_print_format(it->second);
          }
              }
            if (display_weights)
              {
                *out_ << "\t" << path.first;
              }
            *out_ << "\n";
          }
        else
          {
        bool is_automaton=true;

        bool first_symbol=true;
            for (StringPairVector::const_iterator it = path.second.begin();
                 it != path.second.end(); ++it)
              {
        if ((not filter_fd) ||
            (not FdOperation::is_diacritic(it->first)))
          {
            if (print_spaces && not first_symbol)
              {
            *out_ << " ";
              }
            if (it->first.compare(it->second) != 0)
              is_automaton=false;

            *out_ << get_print_format(it->first);
          }
        first_symbol=false;
          }
        if (print_spaces)
          {
        *out_ << " ";
          }

        if (not is_automaton) {
          *out_ << ":";
          for (StringPairVector::const_iterator it = path.second.begin();
           it != path.second.end(); ++it)
        {
          if ((not filter_fd) ||
              (not FdOperation::is_diacritic(it->second)))
            {
              if (print_spaces)
            {
              *out_ << " ";
            }
              *out_ << get_print_format(it->second);
            }
        }
        }

            if (display_weights)
              {
                *out_ << "\t" << path.first;
              }
            *out_ << std::endl;
           }
        count++;
      }
    // continue until we've printed max_num strings
    return RetVal((max_num < 1) || (count < max_num), true);
  }
};

int
process_stream(HfstInputStream& instream, std::ostream& outstream)
{
  //instream.open();

  bool first_transducer=true;
  while(instream.is_good())
  {
    if (!first_transducer && print_separator_after_each_transducer)
      outstream << "--" << std::endl;
    first_transducer=false;

    HfstTransducer t(instream);

    /* Pairstring format is not supported on optimized lookup format. */
    if (print_in_pairstring_format &&
    (instream.get_type() == HFST_OL_TYPE ||
     instream.get_type() == HFST_OLW_TYPE) ) {
      fprintf(stderr,
          "Error: option --print-in-pairstring-format not supported on "
          "       optimized lookup transducers, exiting program\n" );
      exit(1);
    }

    if(input_prefix != "")
      verbose_printf("input_prefix: '%s'\n", input_prefix.c_str());

    if(beam >= 0)
      {
        verbose_printf("Finding the weight of the best path...\n");
      try
        {
          HfstTransducer tc(t);
          tc.n_best(1);
          HfstTwoLevelPaths best_paths;
          tc.extract_paths(best_paths);
          if (best_paths.size() != 1)
            {
              error(EXIT_FAILURE, 0, "n_best(1) produced more than one path");
            }
          max_weight = best_paths.begin()->first;
        }
      catch (const FunctionNotImplementedException & e)
        {
          if (instream.get_type() == hfst::HFST_OL_TYPE ||
              instream.get_type() == hfst::HFST_OLW_TYPE)
            {
              error(EXIT_FAILURE, 0, "option --beam not implemented for optimized lookup format");
            }
          else
            {
              error(EXIT_FAILURE, 0, "option --beam not implemented");
            }
          return EXIT_FAILURE;
        }
      catch(const HfstFatalException & e)
        {
          error(EXIT_FAILURE, 0, "n_best runs out of memory");
          return EXIT_FAILURE;
        }
      }

    if(nbest_strings > 0)
    {
      verbose_printf("Pruning transducer to %i best path(s)...\n",
             nbest_strings);
      try
        {
          t.n_best(nbest_strings);
        }
      catch (const FunctionNotImplementedException & e)
        {
          if (instream.get_type() == hfst::HFST_OL_TYPE ||
              instream.get_type() == hfst::HFST_OLW_TYPE)
            {
              error(EXIT_FAILURE, 0, "option --nbest not implemented for optimized lookup format");
            }
          else
            {
              error(EXIT_FAILURE, 0, "option --nbest not implemented");
            }
          return EXIT_FAILURE;
        }
      catch(const HfstFatalException & e)
        {
          error(EXIT_FAILURE, 0, "n_best runs out of memory");
          return EXIT_FAILURE;
        }
    }
    else
    {
      if(max_random_strings <= 0 && max_strings <= 0 && max_input_length <= 0
     && max_output_length <= 0 &&
     cycles < 0 && t.is_cyclic())
      {
        error(EXIT_FAILURE, 0,
              "Transducer is cyclic. Use one or more of these options: "
              "-n, -N, -r, -l, -L, -c");
        return EXIT_FAILURE;
      }
    }

    if(max_strings > 0)
      verbose_printf("Finding at most %i path(s)...\n", max_strings);
    else if(max_random_strings > 0)
      verbose_printf("Finding at most %i random path(s)...\n",
             max_random_strings);
    else
      verbose_printf("Finding strings...\n");

    /* not random strings */
    if (max_random_strings <= 0)
      {
    Callback cb(max_strings, &outstream);
    if(eval_fd)
      t.extract_paths_fd(cb, cycles, filter_fd);
    else
      t.extract_paths(cb, cycles);
    verbose_printf("Printed %i string(s)\n", cb.count);
      }
    /* random strings */
    else
      {
    HfstTwoLevelPaths results;
    try {
      if (eval_fd) {
        t.extract_random_paths_fd(results, max_random_strings, filter_fd);
      }
      else {
        t.extract_random_paths(results, max_random_strings);
      }
    }
    catch (const FunctionNotImplementedException & e) {
      if (instream.get_type() == hfst::HFST_OL_TYPE ||
          instream.get_type() == hfst::HFST_OLW_TYPE)
        {
          error(EXIT_FAILURE, 0, "option --random not implemented for optimized lookup format");
        }
      else
        {
          error(EXIT_FAILURE, 0, "option --random not implemented");
        }
      return EXIT_FAILURE;
    }

    Callback cb(max_random_strings, &outstream);
    for (HfstTwoLevelPaths::const_iterator it = results.begin();
         it != results.end(); it++)
      {
        HfstTwoLevelPath path = *it;
        cb(path, true /*final*/);
      }
    verbose_printf("Printed %i random string(s)\n", cb.count);
      }

    //if (print_separator_after_each_transducer)
    //  outstream << "--" << std::endl;
  }

  instream.close();
  return EXIT_SUCCESS;
}


int main( int argc, char **argv ) {
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
#endif

  hfst_set_program_name(argv[0], "0.1", "HfstFst2Strings");
  epsilon_format = hfst_strdup("");
    int retval = parse_options(argc, argv);

    if (max_strings > 0 && max_random_strings > 0 && !silent)
      {
    fprintf(stderr,
        "warning: option --max_strings ignored, --random used\n");
    max_strings = -1;
      }

    if (retval != EXIT_CONTINUE)
    {
        return retval;
    }
    // close buffers, we use streams
    if (inputfile != stdin)
    {
        fclose(inputfile);
    }
    if (outfile != stdout)
    {
        fclose(outfile);
    }
    verbose_printf("Reading from %s, writing to %s\n",
        inputfilename, outfilename);
    // here starts the buffer handling part
    unique_ptr<HfstInputStream> instream;
    try {
      instream.reset((inputfile != stdin) ?
        new HfstInputStream(inputfilename) : new HfstInputStream());
    } catch(const HfstException e) {
        fprintf(stderr, "%s is not a valid transducer file\n", inputfilename);
        return EXIT_FAILURE;
    }

    if (outfile != stdout)
    {
      std::ofstream outstream(outfilename);
      retval = process_stream(*instream, outstream);
    }
    else
      retval = process_stream(*instream, std::cout);

    free(inputfilename);
    free(outfilename);
    free(epsilon_format);
    return retval;
}

