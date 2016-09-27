//! @file hfst-affix-guessify.cc
//!
//! @brief Transducer guesser maker
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

#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>

#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"
#include "implementations/HfstBasicTransducer.h"

using std::map;
using std::string;
using std::set;

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using hfst::implementations::HfstTransitionGraph;
using hfst::implementations::HfstBasicTransducer;
using hfst::implementations::HfstBasicTransition;
using hfst::implementations::HfstState;
using hfst::StringSet;

#include "hfst-commandline.h"
#include "hfst-program-options.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

// add tools-specific variables here
enum guess_direction {
    GUESS_PREFIX,
    GUESS_SUFFIX
};

static guess_direction direction = GUESS_SUFFIX;
static float weight = 1.0f;
static hfst::ImplementationType format= hfst::TROPICAL_OPENFST_TYPE;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "Create weighted affix guesser from automaton\n"
        "\n", program_name);

    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    // fprintf(message_out, (tool-specific options and short descriptions)
    fprintf(message_out, "Guesser parameters:\n"
            "  -D, --direction=DIR   set direction of guessing\n"
            "  -w, --weight=WEIGHT   set weight difference of affix lengths\n"
            "\n");
    fprintf(message_out, "\n");
    print_common_unary_program_parameter_instructions(message_out);
    fprintf(message_out, "DIR is either suffix or prefix, "
            "or suffix if omitted.\n"
            "WEIGHT is a weight of each arc not in the known suffix or "
            "prefix being guessed, as parsed with strtod(3), "
            "or 1.0 if omitted.\n");
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
            {"weight", required_argument, 0, 'w'},
            {"direction", required_argument, 0, 'D'},
            {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT
                             "w:D:",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
        case 'w':
          weight = hfst_strtoweight(optarg);
          break;
        case 'D':
          if (strncmp(optarg, "prefix", strlen("prefix")) == 0)
            {
              direction = GUESS_PREFIX;
            }
          else if (strncmp(optarg, "suffix", strlen("suffix")) == 0)
            {
              direction = GUESS_SUFFIX;
            }
          else
            {
              error(EXIT_FAILURE, 0, "Unable to parse guessing direction "
                    "from %s;\n"
                    "please use one of `prefix' or `suffix'",
                    optarg);
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
  size_t transducer_n = 0;
  while (instream.is_good())
    {
      HfstTransducer *trans = new HfstTransducer(instream);
      char* inputname = strdup(trans->get_name().c_str());
      if (strlen(inputname) < 1)
        {
          inputname = strdup(inputfilename);
        }
      if (transducer_n < 2)
        {
          verbose_printf("Guessifying %s...\n", inputname);
        }
      else
        {
          verbose_printf("Guessifying %s... " SIZE_T_SPECIFIER "\n", inputname, transducer_n);
        }
      HfstBasicTransducer* repl = 0;
      HfstTransducer* t = 0;
      HfstState guess_state = 0;
      HfstBasicTransducer* mutt = 0;
      set<HfstState> replayed;
      HfstBasicTransducer::const_iterator i;
      StringSet alpha = trans->get_alphabet();
      switch (direction)
        {
        case GUESS_SUFFIX:
            {
          verbose_printf("Creating guesser prefix...\n");
          mutt = new HfstBasicTransducer(*trans);
          repl = new HfstBasicTransducer();
          guess_state = repl->add_state(0);
          HfstBasicTransition guess_arc(guess_state,
                                        hfst::internal_identity,
                                        hfst::internal_identity,
                                        weight);
          repl->add_transition(guess_state, guess_arc);
          for (StringSet::const_iterator x = alpha.begin();
               x != alpha.end();
               ++x)
            {
              HfstBasicTransition x_arc(guess_state,
                                        *x,
                                        *x,
                                        weight);
              repl->add_transition(guess_state, x_arc);
            }
          verbose_printf("Rebuilding suffix...\n");
          i = mutt->begin();
          for (HfstState s = 0; s <= mutt->get_max_state(); s++)
            {
              HfstState d = repl->add_state(s + 1);
              if (mutt->is_final_state(s))
                {
                  repl->set_final_weight(d,
                                        mutt->get_final_weight(s));
                }
              HfstBasicTransition guess_arc(d,
                                            hfst::internal_identity,
                                            hfst::internal_identity,
                                            weight);
              repl->add_transition(guess_state, guess_arc);
              for (StringSet::const_iterator x = alpha.begin();
                   x != alpha.end();
                   ++x)
                {
                  HfstBasicTransition x_arc(d,
                                            *x,
                                            *x,
                                            weight);
                  repl->add_transition(guess_state, x_arc);
                }
              for (HfstBasicTransducer::HfstTransitions::const_iterator arc =
                   i->begin();
                   arc != i->end();
                   ++arc)
                {
                  HfstBasicTransition newarc(arc->get_target_state() + 1,
                                             arc->get_input_symbol(),
                                             arc->get_output_symbol(),
                                             arc->get_weight());
                  repl->add_transition(d, newarc);
                }
              ++i;
            } // for each state
          verbose_printf("converting and saving...\n");
          t = new HfstTransducer(*repl, format);
          outstream << *t;
          break;
            }
        case GUESS_PREFIX:
            {
          verbose_printf("Creating guesser suffix...\n");
          repl = new HfstBasicTransducer(*trans);
          guess_state = repl->add_state();
          repl->set_final_weight(guess_state, 0);
          HfstBasicTransition guess_arc(guess_state,
                                        hfst::internal_identity,
                                        hfst::internal_identity,
                                        weight);
          repl->add_transition(guess_state, guess_arc);
          verbose_printf("Linking prefix...\n");
          HfstState max_state = repl->get_max_state();
          for (HfstState s = 0; s <= max_state; s++)
            {
              HfstBasicTransition newarc(guess_state,
                                         hfst::internal_identity,
                                         hfst::internal_identity,
                                         weight);
              repl->add_transition(s, newarc);
            }
          verbose_printf("Converting and saving...\n");
          t = new HfstTransducer(*repl, format);
          outstream << *t;
            }
        } // which direction
    } // good instream
  return EXIT_SUCCESS;
}


int main( int argc, char **argv ) {
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
  _setmode(1, _O_BINARY);
#endif
    hfst_set_program_name(argv[0], "0.1", "HfstAffixGuessify");
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
    HfstInputStream* instream = NULL;
    try {
        instream = (inputfile != stdin) ?
        new HfstInputStream(inputfilename) : new HfstInputStream();
    } // NotTransducerStreamException
    catch (const HfstException e) {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              inputfilename);
        return EXIT_FAILURE;
    }
    HfstOutputStream* outstream = (outfile != stdout) ?
                new HfstOutputStream(outfilename, instream->get_type()) :
                new HfstOutputStream(instream->get_type());

    if ( is_input_stream_in_ol_format(instream, "hfst-affix-guessify"))
      {
        return EXIT_FAILURE;
      }

    retval = process_stream(*instream, *outstream);
    if (outfile != stdout)
    {
        fclose(outfile);
    }
    free(inputfilename);
    free(outfilename);
    return EXIT_SUCCESS;
}

