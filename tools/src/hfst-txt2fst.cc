//! @file hfst-txt2fst.cc
//!
//! @brief Transducer text compiling command line tool
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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _MSC_VER
#  include "hfst-getopt.h"
#else
#  include <getopt.h>
#endif

#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"
#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "hfst-tool-metadata.h"

using hfst::HfstTransducer;
using hfst::HfstOutputStream;
using hfst::implementations::HfstBasicTransducer;

#include "inc/globals-common.h"
#include "inc/globals-unary.h"
// add tools-specific variables here
static hfst::ImplementationType output_format = hfst::UNSPECIFIED_TYPE;
static bool read_prolog_format=false;
// whether numbers are used instead of symbol names
static bool use_numbers=false; // not used
// printname for epsilon
static char *epsilonname=NULL;
static const unsigned int EPSILON_KEY=0;
// check if there are epsilon cycles with a negative weight
bool check_negative_epsilon_cycles = false;

bool disjunct_multiple_transducers = false;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "Convert AT&T or prolog format into a binary transducer\n"
           "\n", program_name);

    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "Text and format options:\n"
            "  -f, --format=FMT    Write result using FMT as backend format\n"
            "  -e, --epsilon=EPS   Interpret string EPS as epsilon in att format\n"
            "  -p, --prolog        Read prolog format instead of att\n");
    fprintf(message_out, "Other options:\n"
            "  -C, --check-negative-epsilon-cycles  Issue a warning if there are epsilon cycles\n"
            "                                       with a negative weight in the transducer\n"
            "  -j, --disjunct                       Disjunct transducers\n");
    fprintf(message_out, "\n");
    fprintf(message_out,
        "If OUTFILE or INFILE is missing or -, standard streams will be used.\n"
        "If FMT is not given, OpenFst's tropical format will be used.\n"
        "The possible values for FMT are { foma, openfst-tropical, openfst-log,\n"
        "sfst, optimized-lookup-weighted, optimized-lookup-unweighted }.\n"
        "If EPS is not given, @0@ will be used.\n\n"
        "Space in transition symbols must be escaped as '@_SPACE_@' when using\n"
        "att format."
        "\n"
        );
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
            {"epsilon", required_argument, 0, 'e'},
            {"number", no_argument, 0, 'n'},
            {"format", required_argument, 0, 'f'},
            {"prolog", no_argument, 0, 'p'},
            {"disjunct", no_argument, 0, 'j'},
            {"check-negative-epsilon-cycles", no_argument, 0, 'C'},
            {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "e:nf:pjC",
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
        case 'e':
            epsilonname = hfst_strdup(optarg);
            break;
        case 'j':
            disjunct_multiple_transducers = true;
            break;
        case 'n':
            use_numbers = true;
            break;
        case 'p':
            read_prolog_format = true;
            break;
        case 'f':
            output_format = hfst_parse_format_name(optarg);
            break;
        case 'C':
            check_negative_epsilon_cycles = true;
            break;
#include "inc/getopt-cases-error.h"
          }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    if (epsilonname == NULL)
      {
        epsilonname = hfst_strdup("@0@");
        verbose_printf("Using default epsilon representation %s\n",
                       epsilonname);
      }
    if (output_format == hfst::UNSPECIFIED_TYPE)
      {
        output_format = hfst::TROPICAL_OPENFST_TYPE;
        verbose_printf("Using default output format OpenFst "
                "with tropical weight class\n");
      }

    if (output_format == hfst::XFSM_TYPE && read_prolog_format && check_negative_epsilon_cycles)
      {
        error(EXIT_FAILURE, 0, "Error: checking negative epsilon cycles not supported when reading in prolog format\n"
              "and outputting in xfsm format.\n");
        return EXIT_FAILURE;
      }

    return EXIT_CONTINUE;
}

int
process_stream(HfstOutputStream& outstream)
{
  size_t transducer_n = 0;
  unsigned int linecount = 0;

  //outstream.open();
  while (!feof(inputfile))
    {
      transducer_n++;
      if (transducer_n < 2)
        {
          verbose_printf("Reading transducer table...\n");
        }
      else
        {
          verbose_printf("Reading transducer table " SIZE_T_SPECIFIER "...\n", transducer_n);
        }
      if (read_prolog_format)
        {

          if (output_format == hfst::XFSM_TYPE)
            {
              try {
                HfstTransducer * t = HfstTransducer::prolog_file_to_xfsm_transducer(inputfilename);
                outstream << *t;
                delete(t);
                outstream.flush();
                break;
              }
              catch (const HfstException & e) {
                error(EXIT_FAILURE, 0, "Error when trying to convert text file (prolog) into xfsm format\n"
                      "Note that the file may contain only one transducer in text format\n");
                return EXIT_FAILURE;
              }
            }

          try {
            HfstBasicTransducer fsm =
              HfstBasicTransducer::read_in_prolog_format(inputfile, linecount);

            if (check_negative_epsilon_cycles)
              {
                verbose_printf("Checking if the transducer has epsilon cycles with a negative weight...\n");
                if (fsm.has_negative_epsilon_cycles())
                  {
                    if (!silent)
                      {
                        warning(0, 0, "Transducer has epsilon cycles with a negative weight.\n");
                      }
                  }
                else
                  {
                    verbose_printf("No epsilon cycles with a negative weight detected...\n");
                  }
              }

            HfstTransducer t(fsm, output_format);
            hfst_set_name(t, inputfilename, "text");
            hfst_set_formula(t, inputfilename, "T");
            outstream << t;
          }
          catch (NotValidPrologFormatException e) {
            error(EXIT_FAILURE, 0, "Error in processing transducer text file (prolog) on line %u\n", linecount);
            return EXIT_FAILURE;
          }
        }
      else if(disjunct_multiple_transducers) 
        {
            std::vector<HfstTransducer> transducers;
            try {

              while (!feof(inputfile)) 
                {
                  HfstTransducer t(inputfile, output_format,  std::string(epsilonname));
                  transducers.push_back(t);
                }

            } catch (NotValidAttFormatException e) {
                printf("Error reading transducer: not valid AT&T format.\n"); 
            }
            HfstTransducer joined(output_format); 
            std::vector<HfstTransducer>::iterator it;
            for(it = transducers.begin(); it != transducers.end(); it++)
              {
                joined.disjunct(*it);
              }
            // joined.remove_epsilons(); // remove epsilons from the unioned transducers
            outstream << joined;
        }
      else
        {
          try {
            HfstTransducer t(inputfile,
                             output_format,
                             std::string(epsilonname),
                             linecount);
            hfst_set_name(t, inputfilename, "text");
            hfst_set_formula(t, inputfilename, "T");
            if (check_negative_epsilon_cycles)
              {
                verbose_printf("Checking if the transducer has epsilon cycles with a negative weight...\n");
                hfst::implementations::HfstBasicTransducer fsm(t);
                if (fsm.has_negative_epsilon_cycles())
                  {
                    if (!silent)
                      {
                        warning(0, 0, "Transducer has epsilon cycles with a negative weight.\n");
                      }
                  }
                else
                  {
                    verbose_printf("No epsilon cycles with a negative weight detected...\n");
                  }
              }
            outstream << t;
          }
          catch (NotValidAttFormatException e) {
            error(EXIT_FAILURE, 0, "Error in processing transducer text file (att) on line %u\n", linecount);
            return EXIT_FAILURE;
          }
        }
    }
  outstream.close();
  return EXIT_SUCCESS;
}


int main( int argc, char **argv )
{
#ifdef WINDOWS
  _setmode(1, _O_BINARY);
#endif

  hfst_set_program_name(argv[0], "0.1", "HfstTxt2Fst");
    int retval = parse_options(argc, argv);

    if (retval != EXIT_CONTINUE)
    {
        return retval;
    }
    // close buffers, we use streams
    if (outfile != stdout)
    {
        fclose(outfile);
    }
    verbose_printf("Reading from %s, writing to %s\n",
        inputfilename, outfilename);
    switch (output_format)
      {
      case hfst::SFST_TYPE:
        verbose_printf("Using SFST as output handler\n");
        break;
      case hfst::TROPICAL_OPENFST_TYPE:
        verbose_printf("Using OpenFst's tropical weights as output\n");
        break;
      case hfst::LOG_OPENFST_TYPE:
        verbose_printf("Using OpenFst's log weight output\n");
        break;
      case hfst::FOMA_TYPE:
        verbose_printf("Using foma as output handler\n");
        break;
      case hfst::XFSM_TYPE:
        verbose_printf("Using xfsm as output handler\n");
        break;
      case hfst::HFST_OL_TYPE:
        verbose_printf("Using optimized lookup output\n");
        break;
      case hfst::HFST_OLW_TYPE:
        verbose_printf("Using optimized lookup weighted output\n");
        break;
      default:
        error(EXIT_FAILURE, 0, "Unknown format cannot be used as output\n");
        return EXIT_FAILURE;
      }

    if (output_format == hfst::XFSM_TYPE)
      {
        if (strcmp(outfilename, "<stdout>") == 0) {
          error(EXIT_FAILURE, 0, "Writing to standard output not supported for xfsm transducers,\n"
                "use 'hfst-txt2fst [--output|-o] OUTFILE' instead");
          return EXIT_FAILURE;
        }
        if (!read_prolog_format) {
          error(EXIT_FAILURE, 0, "Writing in att format not supported for xfsm transducers,\n"
                "use '--prolog' instead");
          return EXIT_FAILURE;
        }
        if (strcmp(inputfilename, "<stdin>") == 0) {
          error(EXIT_FAILURE, 0, "Reading prolog format from standard input not supported for xfsm transducers,\n"
                "use 'hfst-txt2fst [--input|-i] INFILE' instead");
          return EXIT_FAILURE;
        }
        else {}
      }


    // here starts the buffer handling part
    HfstOutputStream* outstream = (outfile != stdout) ?
                new HfstOutputStream(outfilename, output_format) :
                new HfstOutputStream(output_format);
    process_stream(*outstream);
    if (inputfile != stdin)
      {
        fclose(inputfile);
      }
    free(inputfilename);
    free(outfilename);
    return EXIT_SUCCESS;
}

