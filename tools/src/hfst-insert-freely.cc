//! @file hfst-insert-freely.cc
//!
//! @brief Transducer whoever ever reads these lol derp tool
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
#include "hfst-tool-metadata.h"
#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"
#include "HfstDataTypes.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using hfst::StringPair;
using std::unique_ptr;
using std::make_unique;

// add tools-specific variables here
static char* label = 0;
static bool harmonise_flags = false;
static StringPair* symbol_pair = 0;


// FMT: Copied from hfst-substitute.cc ... should probably go in a library function

/**
 * @brief parse string pair from arc label.
 *
 * @return new stringpair, or null if not a pair.
 */
static
StringPair*
label_to_stringpair(const char* label)
  {
    const char* colon = strchr(label, ':');
    const char* endstr = strchr(label, '\0');
    while (colon != NULL)
      {
        if (colon == label)
          {
            colon = strchr(colon + 1, ':');
          }
        else if (colon == (endstr - 1))
          {
            colon = 0;
          }
        else if (*(colon - 1) == '\\')
          {
            if (colon > (label + 1))
              {
                if (*(colon - 2) == '\\')
                  {
                    break;
                  }
                else
                  {
                    colon = strchr(colon + 1, ':');
                  }
              }
          }
        else
          {
            break;
          }
      }
    char* first = 0;
    char* second = 0;
    if ((label < colon) && (colon < endstr))
      {
        first = hfst_strndup(label, colon-label);
        second = hfst_strndup(colon + 1, endstr - colon - 1);
      }
    else
      {
        return NULL;
      }

    if (strcmp(first, "@0@") == 0)
      {
        free(first);
        first = hfst_strdup(hfst::internal_epsilon.c_str());
      }
    if (strcmp(second, "@0@") == 0)
      {
        free(second);
        second = hfst_strdup(hfst::internal_epsilon.c_str());
      }

    StringPair* rv = new StringPair(first, second);
    free(first);
    free(second);
    return rv;
  }


void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    // Usage line
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "Freely insert a symbol (pair)\n"
        "\n", program_name);

    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "Option:\n"
            "  -a, --symbol-pair=SYM   symbol pair SYM\n"
            "  -H, --harmonise   harmonise \n");
    fprintf(message_out, "\n");
    print_common_unary_program_parameter_instructions(message_out);
    fprintf(message_out, "SYM must be either a single alphabetic"
            "symbol or two symbols separated by a colon, :\n");
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
          {"symbol-pair", required_argument, 0, 'a'},
          {"harmonise", required_argument, 0, 'H'},
          {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "a:H",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
          case 'a':
          { 
            // This will probably break for unicode
            label = hfst_strdup(optarg);
            if (strcmp(label, "@0@") == 0)
              {
                free(label);
                label = hfst_strdup(hfst::internal_epsilon.c_str());
              }
            symbol_pair = label_to_stringpair(label);
            if (strlen(label) == 0)
              {
                error(EXIT_FAILURE, 0, "argument of source label option is "
                      "empty;\n"
                      "if you REALLY want to replace epsilons with something, "
                      "use @0@ or %s", hfst::internal_epsilon.c_str());
              }
            break;
          }
          case 'H':
          {
            harmonise_flags = true;
            break;
          }
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
//            If harmonize is true, then identity and unknown symbols in the
//    transducer will be exapanded byt the symbols in symbol
//    pair. Otherwise they aren't.
// 
//    HFSTDLL HfstTransducer &insert_freely(const StringPair &symbol_pair, bool harmonize=true);

// StringPair("a", "a")

          trans.insert_freely(*symbol_pair, harmonise_flags);
          hfst_set_name(trans, trans, "insert-freely");
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
    unique_ptr<HfstInputStream> instream;
    try {
      instream.reset((inputfile != stdin) ?
        new HfstInputStream(inputfilename) : new HfstInputStream());
    } catch(const HfstException e)  {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              inputfilename);
        return EXIT_FAILURE;
    }
    auto outstream = (outfile != stdout) ?
        make_unique<HfstOutputStream>(outfilename, instream->get_type()) :
        make_unique<HfstOutputStream>(instream->get_type());

    if ( is_input_stream_in_ol_format(*instream, "hfst-insert-freely"))
      {
	return EXIT_FAILURE;
      }
    
    retval = process_stream(*instream, *outstream);
    free(inputfilename);
    free(outfilename);
    return retval;
}

