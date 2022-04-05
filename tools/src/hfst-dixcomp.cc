//! @file expand-equivalences.cc
//!
//! @brief Transducer label modification for equivalence classes
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

#include "DixCompiler.h"

#include "hfst.hpp"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;

#include "hfst-commandline.h"
#include "hfst-tool-metadata.h"
#include "hfst-program-options.h"
#include "inc/globals-common.h"

char* direction;
char* inputfilename = 0;
char* alt = 0;
char* var = 0;
char* varleft = 0;
char* varright = 0;
bool keepboundaries = false;
static hfst::ImplementationType format = hfst::UNSPECIFIED_TYPE;

void
print_usage()
{
  // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
  fprintf(message_out, "Usage: %s [OPTIONS...] INFILE\n"
         "Compile lttoolbox XML dictionaries\n"
      "\n", program_name);

  print_common_program_options(stdout);
  fprintf(message_out, "Eqv. class extension options:\n"
          "  -f, --format        convert single symbol ISYM to allow OSYM\n"
          "  -t, --to=OSYM       convert to OSYM\n"
          "  -a, --acx=ACXFILE   read extensions in acx format "
          "from ACXFILE\n"
          "  -T, --tsv=TSVFILE   read extensions in tsv format "
          "from TSVFILE\n"
          "  -l, --level=LEVEL   perform extensions on LEVEL of fsa\n"
         );
  fprintf(message_out, "\n");
  fprintf(message_out,
         "Examples:\n"
         "  %s -o rox.hfst -a romanian.acx ro.hfst  extend romanian char"
         "equivalences\n"
         "\n", program_name);
  print_report_bugs();
  print_more_info();
}

int
parse_options(int argc, char** argv) {
  while (true) {
    static const struct option long_options[] =
    {
        HFST_GETOPT_COMMON_LONG,
        HFST_GETOPT_UNARY_LONG,
        {"format",    required_argument, 0, 'f'},
        {"alt",       required_argument, 0, 'a'},
        {"var",       required_argument, 0, 'v'},
        {"var-left",  required_argument, 0, 'l'},
        {"var-right", required_argument, 0, 'r'},
        {"keep-boundaries", no_argument, 0, 'm'},
        {0,0,0,0}
    };
    int option_index = 0;
    int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                        HFST_GETOPT_UNARY_SHORT "f:a:v:l:r:m",
                        long_options, &option_index);
    if (-1 == c) {
      break;
    }
    switch (c) {
#include "inc/getopt-cases-common.h"
      case 'f':
        format = hfst_parse_format_name(optarg);
        break;
      case 'a':
        alt = hfst_strdup(optarg);
        break;
      //case 'v': // TODO: conflicts with verbose flag
      //  var = hfst_strdup(optarg);
      //  break;
      case 'l':
        varleft = hfst_strdup(optarg);
        break;
      case 'r':
        varright = hfst_strdup(optarg);
        break;
      case 'm':
        keepboundaries = true;
        break;
#include "inc/getopt-cases-error.h"
    }
  }
  if (optind != argc - 2) {
    error(EXIT_FAILURE, 0, "direction and XML file are required");
  }
  direction = argv[optind];
  inputfilename = argv[optind+1];
#include "inc/check-params-common.h"
  return EXIT_CONTINUE;
}

static
void
check_options(int, char**) {}


int main( int argc, char **argv )
{
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
  _setmode(1, _O_BINARY);
#endif
  hfst_set_program_name(argv[0], "0.1", "HfstDixComp");
  int retval = parse_options(argc, argv);
  if (retval != EXIT_CONTINUE)
  {
      return retval;
  }
  check_options(argc, argv);

  // close buffers, we use streams
  if (outfile != stdout)
  {
      fclose(outfile);
  }
  verbose_printf("Reading from %s, writing to %s\n",
      inputfilename, outfilename);

  hfst::DixCompiler comp(direction);

  if (alt != 0) comp.setalt(alt);
  if (var != 0) comp.setvar(var);
  if (varleft != 0) comp.setvarleft(varleft);
  if (varright != 0) comp.setvarright(varright);
  comp.setkeepboundaries(keepboundaries);

  comp.parse(inputfilename);

  if (format == hfst::UNSPECIFIED_TYPE) {
    format = hfst::TROPICAL_OPENFST_TYPE;
  }
  
  // here starts the buffer handling part
  HfstOutputStream* outstream = (outfile != stdout) ?
      new HfstOutputStream(outfilename, format) :
      new HfstOutputStream(format);

  (*outstream) << *(comp.getTransducer(format));
  delete outstream;
  //free(inputfilename);
  free(outfilename);
  return EXIT_SUCCESS;
}

