//! @file hfst-split.cc
//!
//! @brief Transducer archive exploding tool
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

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;

using std::unique_ptr;
using std::make_unique;

// add tools-specific variables here
char *prefix;
char *extension;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "Extract transducers from archive with systematic file names\n"
        "\n", program_name);
    print_common_program_options(message_out);
    fprintf(message_out, "Input/Output options:\n"
            "  -i, --input=INFILE    Read input transducer from INFILE\n"
            "  -p, --prefix=PRE      Use the prefix PRE in "
            "naming output files\n"
            "  -e, --extension=EXT   Use the extension EXT in "
            "naming output files\n");
    fprintf(message_out, "\n");
    fprintf(message_out,
        "If INFILE is omitted or -, stdin is used.\n"
            "If PRE is omitted, no prefix is used.\n"
        "If EXT is omitted, .hfst is used.\n"
        "The extracted files are named \"PRE\" + N + \"EXT\",\n"
        "where N is the number of the transducer in the archive.\n\n"
        "An example:\n"
        "   cat transducer_a transducer_b | hfst-split -p \"rule\" -e \".tr\"\n\n"
        "This command creates files \"rule1.tr\" (equivalent to transducer_a)\n"
        "and \"rule2.tr\" (equivalent to transducer_b). \n");
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
    extension = hfst_strdup(".hfst");
    prefix = hfst_strdup("");
    while (true)
    {
        static const struct option long_options[] =
        {
          HFST_GETOPT_COMMON_LONG,
          {"input", required_argument, 0, 'i'},
          {"prefix", required_argument, 0, 'p'},
          {"extension", required_argument, 0, 'e'},
          // add tool-specific options here
            {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT "i:p:e:",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
        case 'i':
          inputfilename = hfst_strdup(optarg);
          inputfile = hfst_fopen(inputfilename, "r");
          if (inputfile == stdin)
            {
              free(inputfilename);
              inputfilename = hfst_strdup("<stdin>");
            }
          inputNamed = true;
          break;
        case 'p':
          free(prefix);
          prefix = hfst_strdup(optarg);
          break;
        case 'e':
          free(extension);
          extension = hfst_strdup(optarg);
          break;
#include "inc/getopt-cases-error.h"
        }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    return EXIT_CONTINUE;
}

int
process_stream(HfstInputStream& instream)
{
  //instream.open();
    
    size_t transducer_n=0;
    while(instream.is_good())
    {
        transducer_n++;
        outfilename = static_cast<char*>(hfst_malloc(sizeof(char) *
                             strlen(prefix) + strlen(extension) +
                             strlen("123456789012345678901234567890")));
        sprintf(outfilename, "%s" SIZE_T_SPECIFIER "%s", prefix, transducer_n,
                              extension);
        verbose_printf("Writing " SIZE_T_SPECIFIER " of %s to %s...\n", transducer_n,
                       inputfilename, outfilename);
        auto outstream = make_unique<HfstOutputStream>(outfilename, instream.get_type());
        //outstream->open();
        HfstTransducer trans(instream);
        *outstream << trans;
        outstream->flush();
        outstream->close();
        free(outfilename);
    }
    instream.close();
    return EXIT_SUCCESS;
}


int main( int argc, char **argv ) {
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
#endif

    hfst_set_program_name(argv[0], "0.1", "HfstSplit");
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
    verbose_printf("Reading from %s, writing to %s...%s\n",
        inputfilename, prefix, extension);
    // here starts the buffer handling part
    unique_ptr<HfstInputStream> instream;
    try {
      instream.reset((inputfile != stdin) ?
        new HfstInputStream(inputfilename) : new HfstInputStream());
    } catch(const HfstException e)  {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              inputfilename);
        return EXIT_FAILURE;
    }
    
    retval = process_stream(*instream);
    free(inputfilename);
    return retval;
}

