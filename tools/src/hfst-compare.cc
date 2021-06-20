//! @file hfst-compare.cc
//!
//! @brief Transducer comparison tool
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

#ifdef _MSC_VER
#  include "hfst-getopt.h"
#else
#  include <getopt.h>
#endif

#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"

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

static bool harmonize=true;
static bool eliminate_flags=false;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE1 [INFILE2]]\n"
             "Compare two transducers\n"
        "\n", program_name );
        print_common_program_options(message_out);
        print_common_binary_program_options(message_out);
        fprintf(message_out,
                "Harmonization:\n"
                "  -H, --do-not-harmonize Do not harmonize symbols.\n"
                "  -e, --eliminate-flags  Eliminate flag diacritics.\n");
        fprintf(message_out, "\n");
        print_common_binary_program_parameter_instructions(message_out);
        fprintf(message_out, "\n");
        fprintf(message_out,
            "\n"
            "Examples:\n"
            "  $ %s cat.hfst dog.hfst\n"
            "  cat.hfst[1] != dog.hfst[1]\n"
            "  $ %s cat.hfst cat.hfst\n"
            "  cat.hfst[1] == cat.hfst[1]\n"
            "\n",
                program_name, program_name );
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
          HFST_GETOPT_BINARY_LONG,
          {"do-not-harmonize", no_argument, 0, 'H'},
          {"eliminate-flags", no_argument, 0, 'e'},
          {0,0,0,0}
        };
        int option_index = 0;
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_BINARY_SHORT "He",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }
        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-binary.h"
        case 'H':
          harmonize=false;
          break;
        case 'e':
          eliminate_flags=true;
          break;

#include "inc/getopt-cases-error.h"
        }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-binary.h"
    return EXIT_CONTINUE;
}

int
compare_streams(HfstInputStream& firststream, HfstInputStream& secondstream)
{
    bool continueReading = firststream.is_good() && secondstream.is_good();
    size_t transducer_n_first = 0; // transducers read from first input
    size_t transducer_n_second = 0; // transducers read from second input
    size_t mismatches = 0;

    HfstTransducer * first=0;
    HfstTransducer * second=0;

    while (continueReading) {
        first = new HfstTransducer(firststream);
        transducer_n_first++;
        if (secondstream.is_good()) {
          second = new HfstTransducer(secondstream);
          transducer_n_second++;
        }
        char* firstname = strdup(first->get_name().c_str());
        if (second == NULL) // make scan-build happy, this should not happen
          throw "Error: second stream has a NULL value.";
        char* secondname = strdup(second->get_name().c_str());
        if (strlen(firstname) == 0)
          {
            free(firstname);
            firstname = strdup(firstfilename);
          }
        if (strlen(secondname) == 0)
          {
            free(secondname);
            secondname = strdup(secondfilename);
          }
        if (transducer_n_first == 1)
          {
            verbose_printf("Comparing %s and %s...\n", firstname,
                           secondname);
          }
        else
          {
            verbose_printf("Comparing %s and %s... " SIZE_T_SPECIFIER "\n",
                           firstname, secondname, transducer_n_first);
        }
        try
          {
            if (eliminate_flags)
              {
                verbose_printf("Eliminating flags...\n");
                first->eliminate_flags();
                second->eliminate_flags();
              }
            if (first->compare(*second, harmonize))
              {
                if (transducer_n_first == 1)
                  {
                    if (! silent)
                      fprintf(outfile, "%s == %s\n", firstname, secondname);
                  }
                else
                  {
                    if (! silent)
                      fprintf(outfile, "%s[" SIZE_T_SPECIFIER "] == %s[" SIZE_T_SPECIFIER "]\n",
                              firstname, transducer_n_first,
                              secondname, transducer_n_second);
                  }
              }
            else
              {
                if (transducer_n_first == 1)
                  {
                    if (! silent)
                      fprintf(outfile, "%s != %s\n", firstname, secondname);
                  }
                else
                  {
                    if (! silent)
                      fprintf(outfile, "%s[" SIZE_T_SPECIFIER "] != %s[" SIZE_T_SPECIFIER "]\n",
                              firstname, transducer_n_first,
                              secondname, transducer_n_second);
                  }
                mismatches++;
              }
          }
        catch (TransducerTypeMismatchException ttme)
          {
            // cannot recover yet, but beautify error messages
            error(2, 0, "Cannot compare `%s' and `%s' [" SIZE_T_SPECIFIER "]\n"
                  "the formats %s and %s are not compatible for comparison\n",
                  firstname, secondname, transducer_n_first,
                  hfst_strformat(firststream.get_type()),
                  hfst_strformat(secondstream.get_type()));
          }

        continueReading = firststream.is_good() &&
          (secondstream.is_good() || transducer_n_second == 1);

        delete first;
        first=0;
        // delete the transducer of second stream, unless we continue reading
        // the first stream and there is only one transducer in the second stream
        if ((continueReading && secondstream.is_good()) || ! continueReading)
          {
            delete second;
            second=0;
          }

        free(firstname);
        free(secondname);
    }

    if (firststream.is_good())
    {
        error(EXIT_FAILURE, 0, "second input '%s' contains fewer transducers than first input '%s'; "
              "this is only possible if the second input contains exactly one transducer",
              secondfilename, firstfilename);
    }
    else if (secondstream.is_good())
    {
      error(EXIT_FAILURE, 0, "first input '%s' contains fewer transducers than second input '%s'",
            firstfilename, secondfilename);
    }
    firststream.close();
    secondstream.close();
    fclose(outfile);
    if (mismatches == 0)
      {
        verbose_printf("All " SIZE_T_SPECIFIER " transducers matched\n", transducer_n_first);
        return EXIT_SUCCESS;
      }
    else
      {
        verbose_printf("" SIZE_T_SPECIFIER "/" SIZE_T_SPECIFIER " were not equal\n", mismatches, transducer_n_first);
        return EXIT_FAILURE;
      }
}


int main( int argc, char **argv ) {
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
#endif
    hfst_set_program_name(argv[0], "0.1", "HfstCompare");
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
    verbose_printf("Reading from %s and %s, writing log to %s\n",
        firstfilename, secondfilename, outfilename);
    // here starts the buffer handling part
    unique_ptr<HfstInputStream> firststream;
    unique_ptr<HfstInputStream> secondstream;
    try {
        firststream.reset(firstfile != stdin ?
            new HfstInputStream(firstfilename) : new HfstInputStream());
    } //catch(const HfstException e) {
    catch (const HfstException e) {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              firstfilename);
    }
    try {
        secondstream.reset((secondfile != stdin) ?
            new HfstInputStream(secondfilename) : new HfstInputStream());
    } //catch(const HfstException e) {
    catch (const HfstException e) {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              secondfilename);
    }

    if ( is_input_stream_in_ol_format(*firststream, "hfst-compare") ||
         is_input_stream_in_ol_format(*secondstream, "hfst-compare") )
      {
        return EXIT_FAILURE;
      }

    retval = compare_streams(*firststream, *secondstream);
    if (outfile != stdout)
    {
        fclose(outfile);
    }
    free(firstfilename);
    free(secondfilename);
    free(outfilename);
    return retval;
}
