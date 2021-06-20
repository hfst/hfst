//! @file hfst-minimize.cc
//!
//! @brief Transducer minimisation tool
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

#ifdef PROFILE
 #include <time.h>
#endif

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "hfst-tool-metadata.h"
#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;

using std::unique_ptr;
using std::make_unique;

static bool encode_weights=false;

// add tools-specific variables here

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    // Usage line
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "Minimize a transducer\n"
        "\n", program_name);

    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "Command-specific options:\n");
    fprintf(message_out, "  -E, --encode-weights         Encode weights when minimizing\n"
            "                               (default is false).\n\n");
    print_common_unary_program_parameter_instructions(message_out);
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
          {"encode-weights", no_argument, 0, 'E'},
          {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "E",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
#include "inc/getopt-cases-error.h"
        case 'E':
          encode_weights=true;
          break;

        }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    return EXIT_CONTINUE;
}

int
process_stream(HfstInputStream& instream, HfstOutputStream& outstream)
{
  //instream.open();
  //outstream.open();
    
    size_t transducer_n=0;
    while(instream.is_good())
    {
        transducer_n++;
        HfstTransducer trans(instream);
        char* inputname = hfst_get_name(trans, inputfilename);
        if (transducer_n==1)
        {
          verbose_printf("Minimizing %s...\n", inputname);
        }
        else
        {
          verbose_printf("Minimizing %s..." SIZE_T_SPECIFIER "\n", inputname, transducer_n);
        }

#ifdef PROFILE
        bool val = hfst::get_minimize_even_if_already_minimal();
        hfst::set_minimize_even_if_already_minimal(true);
        clock_t t;
        t = clock();
#endif
        trans.minimize();
#ifdef PROFILE
        t = clock() - t;
        hfst::set_minimize_even_if_already_minimal(val);
        std::cerr << "Minimization took " << ((float)t)/CLOCKS_PER_SEC << " seconds" << std::endl;
#endif

        hfst_set_name(trans, trans, "minimize");
        hfst_set_formula(trans, trans, "M");
        outstream << trans;
        free(inputname);
    }
    outstream.flush();
    instream.close();
    outstream.close();
    return EXIT_SUCCESS;
}


int main( int argc, char **argv ) {
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
  _setmode(1, _O_BINARY);
#endif

    hfst_set_program_name(argv[0], "0.1", "HfstMinimize");
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
    if (outfile != stdout)
    {
        fclose(outfile);
    }


    bool enc = hfst::get_encode_weights();
    if (encode_weights)
      {
        hfst::set_encode_weights(true);
      }

    verbose_printf("Reading from %s, writing to %s\n",
        inputfilename, outfilename);
    // here starts the buffer handling part
    unique_ptr<HfstInputStream> instream;
    try {
      instream.reset((inputfile != stdin) ?
        new HfstInputStream(inputfilename) : new HfstInputStream());
    }
    catch(const ImplementationTypeNotAvailableException & e) {
      error(EXIT_FAILURE, 0, "file %s is in %s format which is not available",
            inputfilename, hfst::implementation_type_to_format(instream->get_type()));
      return EXIT_FAILURE;
      }
    catch(const HfstException & e)    {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              inputfilename);
        return EXIT_FAILURE;
    }
    auto outstream = (outfile != stdout) ?
        make_unique<HfstOutputStream>(outfilename, instream->get_type()) :
        make_unique<HfstOutputStream>(instream->get_type());
    
    if ( is_input_stream_in_ol_format(*instream, "hfst-minimize"))
      {
        return EXIT_FAILURE;
      }

    retval = process_stream(*instream, *outstream);

    if (encode_weights)
      {
        hfst::set_encode_weights(enc);
      }

    free(inputfilename);
    free(outfilename);
    return retval;
}

