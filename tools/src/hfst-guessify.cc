//! @file hfst-guessify.cc
//!
//! @brief Tool for compiling a guesser and model form generator from a
//! morphological analyzer.
//!
//! @author Miikka Silfverberg (HFST Team)


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
#include <sstream>

using std::set;
using std::map;
using std::unique_ptr;
using std::make_unique;

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

#include "guessify_fst.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using hfst::HFST_OLW_TYPE;

// add tools-specific variables here
static bool compile_generator = true;
static float default_penalty = 1.0;

float get_float(const std::string &str)
{
  std::istringstream in(str);
  
  float f;
  in >> f;

  if (in.fail())
    { return -1; }

  return f;
}

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
        "Compile a morphological analyzer into a guesser and generator.\n"
        "\n", program_name);

    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "Guesser options:\n"
        "  -p, --default-penalty           Give penalty for skipping one\n"
        "                                  symbol of input (1.0 by default).\n"
        "  -G, --do-not-compile-generator  When compiling the guesser, do\n"
        "                                  not compile a model form\n"
        "                                  generator.\n");
    fprintf(message_out, "\n");
    fprintf(message_out,
       "All analyses in the morphological analyzer should have the form:\n"
       "w o r d f o r m POS " CATEGORY_SYMBOL_PREFIX "CLASS] X Y Z ...\n"
       "where POS is the part-of-speech tag, " CATEGORY_SYMBOL_PREFIX "CLASS]\n"
       "is an inflectional category marker and X, Y and Z are inflectional\n"
       "markers. The form of the inflectional category marker is fixed.\n"
       "CLASS can be any string, which doesn't contain \"]\".\n");
    fprintf(message_out, "\n");
    fprintf(message_out,
       "Using the option -d will reduce the size of the guesser file by\n"
       "approximately half, but may substantially increase the load time of\n"
       "the guesser when generating model forms. If you only need to guess\n"
       "analyses of unknown word forms, -d has no effect on load time.\n");
    fprintf(message_out, "\n");
    fprintf(message_out,
       "If OUTFILE or INFILE is missing or -, standard streams will be used.\n"
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
            {"default-penalty", required_argument, 0, 'p'},
            {"do-not-compile-generator", no_argument, 0, 'G'},
            {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "p:G",
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
        case 'G':
          compile_generator = false;
          break;

        case 'p':
          default_penalty = get_float(optarg);
          
          if (default_penalty < 0)
            {
              error(EXIT_FAILURE, 0, "Invalid default penalty %s. "
                    "Give a positive float.", optarg);
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
process_stream(HfstInputStream& instream, HfstOutputStream &out)
{
  size_t transducer_n = 0;
  while(instream.is_good())
    {
      transducer_n++;
      HfstTransducer analyzer(instream);
      
      verbose_printf("Compiling guesser from the transducer %s.\n",
                     analyzer.get_name().c_str());
      HfstTransducer guesser = guessify_analyzer(analyzer,default_penalty);
      

      if (compile_generator)
        {
          verbose_printf("Compiling generator and storing guesser and "
                         "generator.\n");
        }
      else
        {
          verbose_printf("Storing guesser.\n");
        }

      store_guesser(guesser,out,compile_generator);
    }

  instream.close();

  return EXIT_SUCCESS;
}


int main( int argc, char **argv )
{
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
  _setmode(1, _O_BINARY);
#endif

    hfst_set_program_name(argv[0], "0.3", "HfstGuessify");
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
    unique_ptr<HfstInputStream> instream;

    try
      {
        instream.reset(inputfile != stdin ?
                    new HfstInputStream(inputfilename) :
                    new HfstInputStream());
      }
    catch(const HfstException e)
      {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              inputfilename);
        return EXIT_FAILURE;
      }

    unique_ptr<HfstOutputStream> outstream;

    try
      {
        outstream.reset(outfile != stdout ?
                     new HfstOutputStream(outfilename,HFST_OLW_TYPE) :
                     new HfstOutputStream(HFST_OLW_TYPE));
      }
    catch(const HfstException e)
      {
        error(EXIT_FAILURE, 0, "%s cannot be opened for writing.",
              outfilename);
        return EXIT_FAILURE;
      }
    
    retval = process_stream(*instream, *outstream);
    
    free(inputfilename);
    free(outfilename);

    return retval;
}

