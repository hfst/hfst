//! @file hfst-traverse.cc
//!
//! @brief Transducer traversal tool
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
#include <map>
#include <string>
#include <vector>
#include <memory>

using std::multimap;
using std::string;
using std::vector;
using std::pair;

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#if HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif
#if HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using hfst::HfstTransducer;
using hfst::implementations::HfstBasicTransducer;
using hfst::implementations::HfstState;

// add tools-specific variables here
bool cave_mode = false;
vector<string> _rl_arcs;

#if HAVE_DECL_RL_COMPLETION_MATCHES
char*
arclabel_generator(const char* text, int state)
{
    static int list_index, len;
    if (!state) {
        list_index = 0;
        len = strlen (text);
    }
    const char* name;
    while (list_index < _rl_arcs.size())
      {
        name = _rl_arcs[list_index].c_str();
        list_index++;
        if (strncmp (name, text, len) == 0)
          {
            return (strdup(name));
          }
    }
    return ((char *)NULL);

}
static char**
arclabel_completion(const char* text, int start, int end)
{
    char **matches;
    matches = (char **)NULL;
    if (start == 0)
      {
        matches = rl_completion_matches ((char*)text, &arclabel_generator);
      }
    return (matches);

}
#endif

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    // Usage line
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "Walk through the transducer arc by arc\n"
        "\n", program_name);

    // options, grouped
    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "\n");
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
            {"cave", no_argument, 0, 'X'},
            {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT,
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
        case 'X':
          cave_mode = true;
          break;
#include "inc/getopt-cases-error.h"
        }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    return EXIT_CONTINUE;
}

int
main_loop(HfstBasicTransducer trans)
{
    fprintf(message_out,
            "Enter labels to seek all paths\n");
    // record current paths with their end states
    multimap<string, HfstState> paths;
    paths.insert(pair<string, HfstState>("", 0));
    HfstBasicTransducer::const_iterator state = trans.begin();
    (void)state;
#if HAVE_READLINE_READLINE_H
#if HAVE_RL_COMPLETION_MATCHES
    rl_attempted_completion_function = arclabel_completion;
#endif
    rl_bind_key('\t', rl_complete);
    rl_completion_append_character = '\0';
#if HAVE_DECL_RL_COMPLETION_SUPPRESS_APPEND
    rl_completion_suppress_append = 1;
#endif
#endif
#if HAVE_READLINE_HISTORY_H
    using_history();
#endif
    while (true)
      {
        // print available paths
        for (multimap<string, HfstState>::const_iterator p = paths.begin();
             p != paths.end();
             ++p)
          {
            _rl_arcs.clear();
            fprintf(message_out, "On path `%s' are continuations:\n",
                    p->first.c_str());
            if (trans[p->second].begin() == trans[p->second].end())
              {
                fprintf(message_out,
                        "<Nothing, you've hit a dead end here>\n");
              }
            for (hfst::implementations::HfstBasicTransitions::const_iterator arc = trans[p->second].begin();
                 arc != trans[p->second].end();
                 ++arc)
              {
                fprintf(message_out, "%s\t%s\n",
                        arc->get_input_symbol().c_str(),
                        arc->get_output_symbol().c_str());
                _rl_arcs.push_back(arc->get_input_symbol());
              }
          }
        char* label = hfst_readline("traverse> ");
        if (NULL == label)
          {
            return EXIT_SUCCESS;
          }
        multimap<string, HfstState> new_paths;
        for (multimap<string, HfstState>::const_iterator p = paths.begin();
             p != paths.end();
             ++p)
          {
            for (hfst::implementations::HfstBasicTransitions::const_iterator arc = trans[p->second].begin();
                 arc != trans[p->second].end();
                 ++arc)
              {
                if (arc->get_input_symbol() == label)
                  {
                    string newpath = p->first + arc->get_input_symbol() +
                        ":" + arc->get_output_symbol() + " ";
                    new_paths.insert(pair<string,HfstState>(newpath,
                                                            arc->get_target_state()));
                  }
              }
          }
        if (new_paths.empty())
          {
            if ((strcmp(label, "quit") == 0) || (strlen(label) == 0))
              {
                fprintf(message_out, "Use EOF (Ctrl-D or similar) to quit\n");
              }
            else if (strcmp(label, "XYZZY") == 0)
              {
                fprintf(message_out, "Nothing happens\n");
              }
            fprintf(message_out, "could not advance with %s\n", label);
          }
        else
          {
            paths = new_paths;
          }
#if HAVE_READLINE_HISTORY_H
        add_history(label);
#endif
      } // while paths not empty
    return EXIT_SUCCESS;
}

int
process_stream(HfstInputStream& instream)
{
  //instream.open();
  //outstream.open();
    
    size_t transducer_n=0;
    while(instream.is_good())
    {
        transducer_n++;
        HfstTransducer trans(instream);
        string trans_name = trans.get_name();
        if (trans_name.empty())
          {
            trans_name = inputfilename;
          }
        HfstBasicTransducer walkable(trans);
        if (cave_mode)
          {
            fprintf(message_out,
                    "WELCOME TO ADVENTURE!! WOULD YOU LIKE INSTRUCTIONS?\n\n");
            char* yesno = hfst_readline("");
            if ((strcmp(yesno, "YES") == 0) || (strcmp(yesno, "yes") == 0))
              {
                fprintf(message_out,
                        "SOMEWHERE NEARBY IS COLOSSAL CAVE "
                        "WHERE OTHERS HAVE FOUND\n"
                        "FORTUNES IN TREASURES AND GOLD, "
                        "THOUGH IT IS RUMORED\n"
                        "THAT SOME WHO ENTER ARE NEVER SEEN AGAIN. "
                        "MAGIC IS SAID\n"
                        "TO WORK IN THE CAVE.  I WILL BE YOUR EYES AND HANDS. "
                        "DIRECT\n"
                        "ME WITH COMMANDS OF 1 ARC LABEL.\n"
                        "(ERRORS, COMPLAINTS, SUGGESTIONS TO HFST-BUGS)\n"
                        "(IF STUCK TYPE HELP FOR SOME HINTS)\n\n");
              }
            fprintf(message_out,
                    "YOU ARE STANDING AT THE END OF A ROAD BEFORE A "
                    "SMALL FINITE\n"
                    "STATE AUTOMATON . AROUND YOU IS A FOREST. A SMALL\n"
                    "STREAM OF ARCS FLOWS OUT OF THE AUTOMATON AND "
                    "DOWN A GULLY:\n\n");
          }
        else
          {
            fprintf(message_out, "Traversing automaton %s\n\n",
                    trans_name.c_str());
          }
        if (walkable.begin() == walkable.end())
          {
            fprintf(message_out, "Nowhere to go\n");
            return EXIT_SUCCESS;
          }
        return main_loop(walkable);
    }
    instream.close();
    return EXIT_SUCCESS;
}


int main( int argc, char **argv ) {
    hfst_set_program_name(argv[0], "0.1", "HfstDeterminize");
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
    std::unique_ptr<HfstInputStream> instream;
    try {
      instream.reset((inputfile != stdin) ?
        new HfstInputStream(inputfilename) : new HfstInputStream());
    } catch(const HfstException e)    {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              inputfilename);
        return EXIT_FAILURE;
    }
    auto outstream = (outfile != stdout) ?
        std::make_unique<HfstOutputStream>(outfilename, instream->get_type()) :
        std::make_unique<HfstOutputStream>(instream->get_type());
    
    retval = process_stream(*instream);
    free(inputfilename);
    free(outfilename);
    return retval;
}

