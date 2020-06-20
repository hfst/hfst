//! @file hfst-push-labels.cc
//!
//! @brief Transducer whoever ever reads these lol tool
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
#include <getopt.h>

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


// add tools-specific variables here
static bool push_initial = false;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    // Usage line
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "Push labels of transducer\n"
        "\n", program_name);

    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "Push options:\n"
            "  -p, --push=DIRECTION   push to DIRECTION\n");
    fprintf(message_out, "\n");
    print_common_unary_program_parameter_instructions(message_out);
    fprintf(message_out, "DIRECTION must be one of start, initial, begin "
            "or end, final\n");
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
          {"push", required_argument, 0, 'p'},
          {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "p:",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
        case 'p':
            if ( (strncasecmp(optarg, "start", 1) == 0) ||
                 (strncasecmp(optarg, "initial", 1) == 0) ||
                 (strncasecmp(optarg, "begin", 1) == 0) )
            {
                push_initial = true;
            }
            else if ( (strncasecmp(optarg, "end", 1) == 0) ||
                      (strncasecmp(optarg, "final", 1) == 0))
            {
                push_initial = false;
            }
            else
            {
                error(EXIT_FAILURE, 0,
                      "unknown push direction %s\n"
                      "should be one of start, initial, begin, end or final.\n",
                      optarg);
                return EXIT_FAILURE;
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
          if (push_initial)
            {
              verbose_printf("Pushing towards start %s...\n", inputname);
            }
          else
            {
              verbose_printf("Pushing towards end %s...\n", inputname);
            }
        }
        else
        {
          if (push_initial)
            {
              verbose_printf("Pushing towards start %s... " SIZE_T_SPECIFIER "\n", inputname,
                             transducer_n);
            }
          else
            {
              verbose_printf("Pushing towards end %s... " SIZE_T_SPECIFIER "\n", inputname,
                             transducer_n);
            }
        }
        
        if (push_initial)
          {
            trans.push_labels(hfst::TO_INITIAL_STATE);
            hfst_set_name(trans, trans, "push-labels-i");
            hfst_set_formula(trans, trans, "Id");
          }
        else
          {
            trans.push_labels(hfst::TO_FINAL_STATE);
            hfst_set_name(trans, trans, "push-labels-f");
            hfst_set_formula(trans, trans, "Id");
          }
        outstream << trans;
        free(inputname);
    }
    instream.close();
    outstream.close();
    return EXIT_SUCCESS;
}


int main( int argc, char **argv ) {
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
  _setmode(1, _O_BINARY);
#endif

    hfst_set_program_name(argv[0], "0.1", "HfstPush");
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
    verbose_printf("Reading from %s, writing to %s\n",
        inputfilename, outfilename);
    // here starts the buffer handling part
    HfstInputStream* instream = NULL;
    try {
      instream = (inputfile != stdin) ?
        new HfstInputStream(inputfilename) : new HfstInputStream();
    } catch(const HfstException e)  {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              inputfilename);
        return EXIT_FAILURE;
    }
    HfstOutputStream* outstream = (outfile != stdout) ?
        new HfstOutputStream(outfilename, instream->get_type()) :
        new HfstOutputStream(instream->get_type());

    if ( is_input_stream_in_ol_format(instream, "hfst-push-labels"))
      {
	return EXIT_FAILURE;
      }
    
    retval = process_stream(*instream, *outstream);
    delete instream;
    delete outstream;
    free(inputfilename);
    free(outfilename);
    return retval;
}

