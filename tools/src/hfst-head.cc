//! @file hfst-head.cc
//!
//! @brief Transducer archive head splitting tool
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
#include <deque>
#include <memory>

using std::deque;

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

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using std::unique_ptr;
using std::make_unique;

// add tools-specific variables here
long head_count = 1;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    // Usage line
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "Get first transducers from an archive\n"
        "\n", program_name);

    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "Archive options:\n"
            "  -n, --n-first=[-]K   print the first K transducers;\n"
            "                       with the leading `-', print all but "
            "last K transducers\n");
    fprintf(message_out, "\n");
    print_common_unary_program_parameter_instructions(message_out);
    fprintf(message_out, "K must be an integer, as parsed by "
            "strtoul base 10, and not 0.\n"
            "If K is omitted default is 1.");
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
          {"n-first", required_argument, 0, 'n'},
          // add tool-specific options here
            {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "n:",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
        case 'n':
          head_count = hfst_strtol(optarg, 10);
          break;
#include "inc/getopt-cases-error.h"
        }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    if (head_count == 0)
      {
        warning(0, 0, "Argument 0 for count is not sensible");
      }
    return EXIT_CONTINUE;
}

int
process_stream(HfstInputStream& instream, HfstOutputStream& outstream)
  {
    size_t transducer_n=0;
    if (head_count > 0)
      {
        while (instream.is_good() && (transducer_n < head_count))
        {
            transducer_n++;
            HfstTransducer trans(instream);
            char* inputname = strdup(trans.get_name().c_str());
            if (strlen(inputname) <= 0)
              {
                inputname = strdup(inputfilename);
              }
            verbose_printf("Forwarding %s..." SIZE_T_SPECIFIER "\n", inputname, transducer_n);
            outstream << trans;
          }
      }
    else if (head_count < 0)
      {
        deque<HfstTransducer> first_but_n;
        verbose_printf("Counting all but last " SIZE_T_SPECIFIER "\n", head_count);
        while (instream.is_good())
          {
            transducer_n++;
            HfstTransducer trans(instream);
            first_but_n.push_back(trans);
          }
        if (-head_count > first_but_n.size())
          {
            warning(0, 0, "Stream in %s has less than " SIZE_T_SPECIFIER " automata; "
                    "Nothing will be written to output",
                    inputfilename, -head_count);
          }
        for (unsigned int i = 0; i < -head_count; i++)
          {
            if (!first_but_n.empty())
              {
                first_but_n.pop_back();
              }
          }
        while (!first_but_n.empty())
          {
            HfstTransducer trans = first_but_n.front();
            char* inputname = strdup(trans.get_name().c_str());
            if (strlen(inputname) <= 0)
              {
                inputname = strdup(inputfilename);
              }
            verbose_printf("Forwarding %s..." SIZE_T_SPECIFIER "\n", inputname, transducer_n);
            outstream << trans;
            first_but_n.pop_front();
          }
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

    hfst_set_program_name(argv[0], "0.2", "HfstHead");
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
    unique_ptr<HfstInputStream> instream;
    try {
      instream.reset((inputfile != stdin) ?
        new HfstInputStream(inputfilename) : new HfstInputStream());
    } catch(const HfstException e)    {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              inputfilename);
        return EXIT_FAILURE;
    }
    auto outstream = (outfile != stdout) ?
        make_unique<HfstOutputStream>(outfilename, instream->get_type()) :
        make_unique<HfstOutputStream>(instream->get_type());
    
    retval = process_stream(*instream, *outstream);
    free(inputfilename);
    free(outfilename);
    return retval;
}

