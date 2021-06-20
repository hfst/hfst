//! @file hfst-binary-tool.cc
//!
//! @brief Transducer GENERIC BINARY TOOL TEMPLATE tool
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


#include <iostream>
#include <fstream>
#include <memory>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>

#include "HfstTransducer.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using hfst::ImplementationType;
using std::unique_ptr;
using std::make_unique;

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "inc/globals-common.h"
#include "inc/globals-binary.h"

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE1 [INFILE2]]\n"
             "Do things with two transducers\n"
        "\n", program_name );
        print_common_program_options(message_out);
        print_common_binary_program_options(message_out);
        fprintf(message_out, "\n");
        print_common_binary_program_parameter_instructions(message_out);
        fprintf(message_out, "\n");
        fprintf(message_out,
            "\n"
            "Examples:\n"
            "  %s -o catdog.hfst cat.hfst dog.hfst  does things\n"
            "\n",
            program_name );
        print_report_bugs();
        print_more_info();
}

int
parse_options(int argc, char** argv)
{
    // use of this function requires options are settable on global scope
    while (true)
    {
        static const struct option long_options[] =
        {
          HFST_GETOPT_COMMON_LONG,
          HFST_GETOPT_BINARY_LONG,
          {0,0,0,0}
        };
        int option_index = 0;
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_BINARY_SHORT,
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }
        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-binary.h"
#include "inc/getopt-cases-error.h"
        }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-binary.h"
    return EXIT_CONTINUE;
}

int
binaryoperate_streams(HfstInputStream& firststream, HfstInputStream& secondstream,
                    HfstOutputStream& outstream)
{
    firststream.open();
    secondstream.open();
    outstream.open();
    // should be is_good?
    bool bothInputs = firststream.is_good() && secondstream.is_good();
    if (firststream.get_type() != secondstream.get_type())
      {
        warning(0, 0, "Tranducer type mismatch in %s and %s; "
              "using former type as output\n",
              firstfilename, secondfilename);
      }
    size_t transducer_n = 0;
    while (bothInputs) {
        transducer_n++;
        if (transducer_n == 1)
        {
            verbose_printf("Doing things with %s and %s...\n", firstfilename,
                        secondfilename);
        }
        else
        {
            verbose_printf("Doing things with %s and %s... " SIZE_T_SPECIFIER "\n",
                           firstfilename, secondfilename, transducer_n);
        }
        HfstTransducer first(firststream);
        HfstTransducer second(secondstream);
        outstream << first.concatenate(second);
        bothInputs = firststream.is_good() && secondstream.is_good();
    }

    if (firststream.is_good())
    {
      warning(0, 0, "Warning: %s contains more transducers than %s; "
                     "residue skipped\n", firstfilename, secondfilename);
    }
    else if (secondstream.is_good())
    {
      warning(0, 0, "Warning: %s contains fewer transducers than %s; "
                     "residue skipped\n", firstfilename, secondfilename);
    }
    firststream.close();
    secondstream.close();
    outstream.close();
    return EXIT_SUCCESS;
}


int main( int argc, char **argv ) {
    hfst_set_program_name(argv[0], "0.1", "HfstGenericBinaryTool");
    int retval = parse_options(argc, argv);
    if (retval != EXIT_CONTINUE)
    {
        return retval;
    }
    // close buffers, we use streams
    if (firstfile != stdin)
    {
        fclose(firstfile);
    }
    if (secondfile != stdin)
    {
        fclose(secondfile);
    }
    if (outfile != stdout)
    {
        fclose(outfile);
    }
    verbose_printf("Reading from %s and %s, writing to %s\n",
        firstfilename, secondfilename, outfilename);
    // here starts the buffer handling part
    unique_ptr<HfstInputStream> firststream;
    unique_ptr<HfstInputStream> secondstream;
    try {
        firststream.reset(firstfile != stdin ?
            new HfstInputStream(firstfilename) : new HfstInputStream());
    } catch(const HfstException e) {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              firstfilename);
    }
    try {
        secondstream.reset((secondfile != stdin) ?
            new HfstInputStream(secondfilename) : new HfstInputStream());
    } catch(const HfstException e) {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              secondfilename);
    }
    auto outstream = (outfile != stdout) ?
        make_unique<HfstOutputStream>(outfilename, firststream->get_type()) :
        make_unique<HfstOutputStream>(firststream->get_type());

    retval = concatenate_streams(*firststream, *secondstream, *outstream);
    free(firstfilename);
    free(secondfilename);
    free(outfilename);
    return retval;
}
