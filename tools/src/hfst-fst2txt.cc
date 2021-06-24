//! @file hfst-fst2txt.cc
//!
//! @brief Transducer array printing command line tool
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
#include <set>
#include <map>
#include <memory>

using std::set;
using std::map;

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <errno.h>

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"
#include "HfstPrintDot.h"
#include "HfstPrintPCKimmo.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstBasicTransducer;
using hfst::implementations::HfstState;

// add tools-specific variables here
static bool use_numbers=false;
static bool print_weights=false;
static bool do_not_print_weights=false;

enum fst_text_format {
    ATT_TEXT, // AT&T / OpenFst compatible TSV
    DOT_TEXT, // Graphviz / dotty
    PCKIMMO_TEXT, // PCKIMMO format
    PROLOG_TEXT, // prolog format
};

static fst_text_format format=ATT_TEXT;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
        "Print transducer in AT&T, dot, prolog or pckimmo format\n"
        "\n", program_name);

    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "Text format options:\n"
        "  -w, --print-weights          If weights are printed in all cases\n"
        "  -D, --do-not-print-weights   If weights are not printed in any "
        "case\n"
        "  -f, --format=TFMT            Print output in TFMT format "
            "[default=att]\n");
    fprintf(message_out, "\n");
    fprintf(message_out,
          "If OUTFILE or INFILE is missing or -, "
      "standard streams will be used.\n"
          "Unless explicitly requested with option -w or -D, "
      "weights are printed\n"
          "if and only if the transducer is in weighted format.\n"
          "TFMT is one of {att, dot, prolog, pckimmo}.\n"
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
            {"print-weights",        no_argument,       0, 'w'},
            {"do-not-print-weights", no_argument,       0, 'D'},
            {"use-numbers",          no_argument,       0, 'n'},
            {"format",               required_argument, 0, 'f'},
            {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "wDnf:",
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
        case 'w':
            print_weights = true;
            break;
    case 'D':
        do_not_print_weights = true;
        break;
    case 'n':
        use_numbers = true;
        break;
    case 'f':
        if ((strcmp(optarg, "att") == 0) || (strcmp(optarg, "AT&T") == 0) ||
            (strcmp(optarg, "openfst") == 0) ||
            (strcmp(optarg, "OpenFst") == 0))
          {
            format = ATT_TEXT;
          }
        else if ((strcmp(optarg, "dot") == 0) ||
            (strcmp(optarg, "graphviz") == 0) ||
            (strcmp(optarg, "GraphViz") == 0))
          {
            format = DOT_TEXT;
          }
        else if (strcmp(optarg, "pckimmo") == 0)
          {
            format = PCKIMMO_TEXT;
          }
        else if ((strcmp(optarg, "prolog") == 0) ||
                 (strcmp(optarg, "Prolog") == 0))
          {
            format = PROLOG_TEXT;
          }
        else
          {
            error(EXIT_FAILURE, 0, "Cannot parse %s as text format; Use one of "
                  "att, pckimmo, dot, prolog", optarg);
          }
        break;
#include "inc/getopt-cases-error.h"
        }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    return EXIT_CONTINUE;
}


int
process_stream(HfstInputStream& instream, FILE* outf)
{
  //instream.open();
    size_t transducer_n = 0;
    while(instream.is_good())
    {
        transducer_n++;
        HfstTransducer* t=NULL;
        try
          {
            t = new HfstTransducer(instream);
          }
        catch (const TransducerTypeMismatchException &e)
          {
            error(EXIT_FAILURE, 0, "input transducers do not have the same type");
          }
        string inputname = t->get_name();
        if (inputname.length() == 0)
          {
            inputname = inputfilename;
          }
        if (transducer_n == 1)
        {
          verbose_printf("Converting %s...\n", inputname.c_str());
        }
        else
        {
          if (instream.get_type() == hfst::XFSM_TYPE) {
            error(EXIT_FAILURE, 0, "Writing more than one transducer in text format to file not supported for xfsm transducers,\n"
                  "use [hfst-head|hfst-tail|hfst-split] to extract individual transducers from input");
            return EXIT_FAILURE;
          }
          verbose_printf("Converting %s..." SIZE_T_SPECIFIER "\n", inputname.c_str(),
                         transducer_n);
        }

        if(transducer_n > 1)
            fprintf(outf, "--\n");

        bool printw; // whether weights are printed
        hfst::ImplementationType type = t->get_type();
        if (print_weights)
          printw=true;
        else if (do_not_print_weights)
          printw=false;
        else if ( (type == hfst::SFST_TYPE || type == hfst::FOMA_TYPE || type == hfst::XFSM_TYPE) )
          printw = false;
        else if ( (type == hfst::TROPICAL_OPENFST_TYPE || type == hfst::LOG_OPENFST_TYPE) )
          printw = true;
        else  // this should not happen
          printw = true;
    switch (format)
      {
      case ATT_TEXT:
        if (use_numbers) // xfsm case checked earlier
          t->write_in_att_format_number(outf,printw);
        else { // xfsm not yet supported
          //if (type == hfst::XFSM_TYPE) // weights are never printed
          //  t->write_xfsm_transducer_in_att_format(outfilename);
          //else
          t->write_in_att_format(outf,printw);
        }
        break;
      case DOT_TEXT: // xfsm case checked earlier
        fprintf(outf, "// This graph generated with hfst-fst2txt\n");
        hfst::print_dot(outf, *t);
        break;
      case PCKIMMO_TEXT: // xfsm case checked earlier
        hfst::print_pckimmo(outf, *t);
        break;
      case PROLOG_TEXT:
        {
          try
            {
              if (type == hfst::XFSM_TYPE) {
                t->write_xfsm_transducer_in_prolog_format(outfilename); // no name or weights printed
              }
              else {
                std::string namestr = t->get_name();
                std::ostringstream ostr;
                ostr << transducer_n;
                std::string alt_namestr = "NO_NAME_" + ostr.str();

                if (namestr.empty()) {
                  namestr = alt_namestr;
                  if (!silent) {
                    fprintf(stderr, "Transducer has no name, giving it a name '%s'...\n", namestr.c_str()); }
                }
                else {
                  namestr = alt_namestr;
                  if (!silent) {
                    fprintf(stderr, "Renaming transducer into '%s'...\n", namestr.c_str()); }
                }

                t->write_in_prolog_format(outf,namestr,printw);
              }
            }
          catch (const HfstException & e)
            {
              error(EXIT_FAILURE, 0, "Error encountered when writing in prolog format: %s", e.name.c_str());
            }
          break;
        }
      default:
        error(EXIT_FAILURE, 0, "Unknown print format");
      }
    delete t;
    }
    instream.close();
    if (outf != stdout)
      {
        fclose(outf);
      }
    return EXIT_SUCCESS;
}


int main( int argc, char **argv )
{
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
#endif

    hfst_set_program_name(argv[0], "0.3", "HfstFst2Txt");
    int retval = parse_options(argc, argv);
    if (retval != EXIT_CONTINUE)
    {
        return retval;
    }
    // close buffers, we use streams
    if (inputfile != stdin)
    {
        fclose(inputfile);
    }
    
    verbose_printf("Reading from %s, writing to %s\n",
        inputfilename, outfilename);
    // here starts the buffer handling part
    std::unique_ptr<HfstInputStream> instream;
    try {
      instream.reset((inputfile != stdin) ?
        new HfstInputStream(inputfilename) : new HfstInputStream());
    } catch(const HfstException e)  {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              inputfilename);
        return EXIT_FAILURE;
    }

    if (instream->get_type() == hfst::XFSM_TYPE)
      {
        if (format == DOT_TEXT) {
          error(EXIT_FAILURE, 0, "Output format 'dot' not supported for xfsm transducers, use 'prolog'");
          return EXIT_FAILURE;
        }
        if (format == PCKIMMO_TEXT) {
          error(EXIT_FAILURE, 0, "Output format 'pckimmo' not supported for xfsm transducers, use 'prolog'");
          return EXIT_FAILURE;
        }
        if (format == ATT_TEXT) {
          error(EXIT_FAILURE, 0, "Output format 'att' not supported for xfsm transducers, use 'prolog'");
          return EXIT_FAILURE;
        }
        if (use_numbers) {
          error(EXIT_FAILURE, 0, "Option '--use-numbers' not supported for xfsm transducers");
          return EXIT_FAILURE;
        }
        if (strcmp(inputfilename, "<stdin>") == 0) {
          error(EXIT_FAILURE, 0, "Reading from standard input not supported for xfsm transducers,\n"
                "use 'hfst-fst2txt [--input|-i] INFILE' instead");
          return EXIT_FAILURE;
        }
        if (strcmp(outfilename, "<stdout>") == 0) {
          error(EXIT_FAILURE, 0, "Writing to standard output not supported for xfsm transducers,\n"
                "use 'hfst-fst2txt [--output|-o] OUTFILE' instead");
          return EXIT_FAILURE;
        }
      }
    
    retval = process_stream(*instream, outfile);

    free(inputfilename);
    free(outfilename);
    return retval;
}

