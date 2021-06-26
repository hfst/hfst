//! @file hfst-edit-metadata.cc
//!
//! @brief Transducer metadata tool
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
#include <string>
#include <map>
#include <memory>

using std::string;
using std::map;

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

using std::map;

// add tools-specific variables here

static map<string, string> properties;
static bool properties_given          = false;
static bool print_all_properties      = true;
static char* print_property           = 0;
static unsigned long truncate_length  = 0;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    // Usage line
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "Name a transducer\n"
        "\n", program_name);
    fprintf(message_out, "Name options:\n"
            "  -a, --add=ANAME=VALUE       add or replace property ANAME"
            "with VALUE\n"
            "  -p, --print[=NAME]          print the current PNAME\n"
            "  -t, --truncate_length=LEN   truncate added properties' lengths to LEN\n");
    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "\n");
    print_common_unary_program_parameter_instructions(message_out);
    fprintf(message_out,
            "If PNAME is omitted, all values are printed\n");
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
          {"add", required_argument, 0, 'a'},
          {"print-name", optional_argument, 0, 'p'},
          {"truncate_length", required_argument, 0, 't'},
          {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "a:p::t:",
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
        case 'a':
            {
          char* equals = strstr(optarg, "=");
          if (equals == NULL)
            {
              error(EXIT_FAILURE, 0, "Equals sign `=' missing from %s",
                    optarg);
            }
          char* property = hfst_strndup(optarg, equals-optarg);
          char* value = hfst_strdup(equals + 1);
          properties[property] = value;
          properties_given=true;
          print_all_properties=false;
          break;
            }
        case 'p':
          if (optarg != NULL)
            {
              print_property = hfst_strdup(optarg);
            }
          else
            {
              print_all_properties = true;
            }
          break;
        case 't':
          truncate_length = hfst_strtoul(optarg, 10);
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

        if ((transducer_n > 1) && (print_all_properties ||
                                   (print_property != NULL))) {
          std::cerr << "--- \n";
        }

        if (transducer_n==1)
        {
          verbose_printf("Metadata %s...\n", inputfilename);
        }
        else
          {
            verbose_printf("Metadata %s..." SIZE_T_SPECIFIER "\n",
                   inputfilename, transducer_n);
          }

        HfstTransducer trans(instream);
        if (!print_all_properties && (print_property == NULL)) {
            for (map<string,string>::const_iterator prop = properties.begin();
                 prop != properties.end();
                 ++prop)
              {
                if (prop->first == "type")
                  {
                    warning(0, 0, "Changing `type' metadata will not change "
                            "type of transducer in file;\n"
                            "having wrong type may cause breakage, use with "
                            "caution");
                  }
                else if (prop->first == "version")
                  {
                    warning(0, 0, "Changing `version' changes parsing "
                            "semantics for header;\n"
                            "use with caution");
                  }
                else if (prop->first == "character-encoding" &&
                    !((prop->second == "utf-8") || (prop->second == "UTF-8")))
                  {
                    error(EXIT_FAILURE, 0, "Cannot set `character-encoding' "
                          "to unsupported value;\n"
                          "consider recoding sources of automaton");
                  }
                if (truncate_length > 0)
                  {
                    trans.set_property(prop->first,
                                       hfst_strndup(prop->second.c_str(),
                                                    truncate_length));
                  }
                else
                  {
                    trans.set_property(prop->first, prop->second);
                  }
              }
          outstream << trans;
        }
        else
          {
            const map<string, string>& props = trans.get_properties();
            if (print_all_properties)
              {
                for (const auto & prop : props)
                  {
                    fprintf(outfile, "%s: %s\n", prop.first.c_str(),
                            prop.second.c_str());
                  }
              }
            else
              {
                fprintf(outfile, "%s\n",
                        props.find(print_property)->second.c_str());
              }
          }
      }
    instream.close();
    outstream.close();
    return EXIT_SUCCESS;
}


int main( int argc, char **argv ) {
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
#endif

    hfst_set_program_name(argv[0], "0.1", "HfstEditMetadata");
    int retval = parse_options(argc, argv);
    if (retval != EXIT_CONTINUE)
    {
        return retval;
    }

#ifdef WINDOWS
    if (!print_all_properties && print_property == NULL)
      {
        _setmode(1, _O_BINARY);
      }
#endif

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
    std::unique_ptr<HfstInputStream> instream;
    try {
      instream.reset((inputfile != stdin) ?
        new HfstInputStream(inputfilename) : new HfstInputStream());
    } catch(const HfstException e)  {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              inputfilename);
        return EXIT_FAILURE;
    }
    auto outstream = (outfile != stdout) ?
        std::make_unique<HfstOutputStream>(outfilename, instream->get_type()) :
        std::make_unique<HfstOutputStream>(instream->get_type());

    retval = process_stream(*instream, *outstream);
    free(inputfilename);
    free(outfilename);
    return retval;
}
