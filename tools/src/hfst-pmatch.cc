//! @file hfst-pmatch.cc
//!
//! @brief Pmatch utility for continuous matching on input streams
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

#include <vector>
#include <map>
#include <string>

using std::string;
using std::vector;
using std::pair;

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _MSC_VER
#  include "hfst-getopt.h"
#  include "hfst-string-conversions.h"
#else
#  include <getopt.h>
#endif

#ifdef HAVE_READLINE
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

/* Read a string, and point buffer to it. */
void libreadline_getline(char ** buffer)
{
  /* If the buffer has already been allocated,
     return the memory to the free pool. */
  if (*buffer)
    {
        free (*buffer);
        *buffer = (char *)NULL;
    }

  /* Get a line from the user. */
  *buffer = readline("");

  /* If the line has any text in it,
     save it on the history. */
  if (*buffer && **buffer)
    add_history (*buffer);
}

#endif

#include <math.h>
#include <errno.h>

#include "HfstExceptionDefs.h"
#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "hfst-tool-metadata.h"
#include "implementations/optimized-lookup/pmatch.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

static bool blankline_separated = true;

enum var_val{on, off, not_defined};
static var_val count_patterns = not_defined;
static var_val delete_patterns = not_defined;
static var_val extract_patterns = not_defined;
static var_val locate_mode = not_defined;
static var_val print_weights = not_defined;
static var_val mark_patterns = not_defined;
static int max_recursion = -1;
static int max_context = -1;

static double time_cutoff = 0.0;
static hfst_ol::Weight weight_cutoff = hfst_ol::INFINITE_WEIGHT;
static bool profile = false;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] TRANSDUCER\n"
            "perform matching/lookup on text streams\n"
            "\n", program_name);
    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out,
            "Pmatch options:\n"
            "  -n  --newline           Newline as input separator (default is blank line)\n"
            "  -x  --extract-patterns  Only print tagged parts in output\n"
            "  -l  --locate            Only print locations of matches\n"
            "  -w  --print-weights     In locate mode, include weights of the matches\n"
            "  -c  --count-patterns    Print the total number of matches when done\n"
            "      --delete-patterns   Replace matches with opening tags\n"
            "      --no-mark-patterns  Don't tag matched patterns\n"
            "      --max-context       Upper limit to context length allowed\n"
            "      --max-recursion     Upper limit for recursion\n"
            "      --weight-cutoff=W   Upper limit for allowed weight\n"
            "  -t, --time-cutoff=S     Limit search after having used S seconds per input\n"
            "  -p  --profile           Produce profiling data\n");
    fprintf(message_out,
            "Use standard streams for input and output.\n"
            "\n"
        );
    
    print_report_bugs();
    fprintf(message_out, "\n");
    print_more_info();
    fprintf(message_out, "\n");
}

void match_and_print(hfst_ol::PmatchContainer & container,
                std::ostream & outstream,
                std::string & input_text)
{
    if (input_text.size() > 0 && input_text.at(input_text.size() - 1) == '\n') {
        // Remove final newline
        input_text.erase(input_text.size() -1, 1);
    }
    if (!container.is_in_locate_mode()) {
#ifndef _MSC_VER
        outstream << container.match(input_text, time_cutoff, weight_cutoff);
#else
        hfst::hfst_fprintf_console(stdout, "%s", container.match(input_text, time_cutoff, weight_cutoff).c_str());
#endif
        outstream << std::endl;
        if (blankline_separated) {
            outstream << std::endl;
        }
    } else {
        hfst_ol::LocationVectorVector locations = container.locate(input_text, time_cutoff, weight_cutoff);
        bool printed_something = false;
        for(const auto & location : locations) {
            if (location.at(0).output.compare("@_NONMATCHING_@") != 0) {
                printed_something = true;
#ifndef _MSC_VER
                outstream << location.at(0).start << "|" << location.at(0).length << "|"
                          << location.at(0).output << "|" << location.at(0).tag;
                if (print_weights) {
                    outstream << "|" << location.at(0).weight;
                }
                outstream << std::endl;
#else
                if (print_weights){
                    hfst::hfst_fprintf_console(stdout, "%i|%i|%s|%s|%f\n", it->at(0).start, it->at(0).length, it->at(0).output.c_str(), it->at(0).tag.c_str(), it->at(0).weight);
                } else {
                    hfst::hfst_fprintf_console(stdout, "%i|%i|%s|%s\n", it->at(0).start, it->at(0).length, it->at(0).output.c_str(), it->at(0).tag.c_str());
                }
#endif
            }
        }
        if (printed_something) {
            outstream << std::endl;
        }
    }
}


int process_input(hfst_ol::PmatchContainer & container,
                  std::ostream & outstream)
{
    std::string input_text;
    char * line = NULL;
    size_t len = 0;
    while (true) {
#ifndef _MSC_VER
#ifdef HAVE_READLINE
        if (isatty(STDIN_FILENO)) {
            libreadline_getline(&line);
            if (!line)
                break;
        } else {
            if (!(hfst_getline(&line, &len, stdin) > 0))
                break;
        }
#else
        if (!(hfst_getline(&line, &len, stdin) > 0))
            break;
#endif
#else
        std::string linestr("");
        size_t bufsize = 1000;
        if (! hfst::get_line_from_console(linestr, bufsize, true /* keep newlines */))
            break;
        line = strdup(linestr.c_str());
#endif
      
        if (!blankline_separated) {
            // newline separated
#ifndef _MSC_VER
#ifdef HAVE_READLINE
            input_text = line;
            match_and_print(container, outstream, input_text);
        } else if (line[0] == '\0' || line[0] == '\n') {
            match_and_print(container, outstream, input_text);
            input_text.clear();
        } else {
            input_text.append(line);
            if (isatty(STDIN_FILENO)) {
                input_text.push_back('\n');
            }
        }
#else
            input_text = line;
            match_and_print(container, outstream, input_text);
        } else if (line[0] == '\n') {
            match_and_print(container, outstream, input_text);
            input_text.clear();
        } else {
            input_text.append(line);
        }
#endif
#else
            input_text = line;
            match_and_print(container, outstream, input_text);
        } else if (line[0] == '\n') {
            match_and_print(container, outstream, input_text);
            input_text.clear();
        } else {
            input_text.append(line);
        }
#endif

        free(line);
        line = NULL;
    }

    if (blankline_separated && !input_text.empty()) {
        match_and_print(container, outstream, input_text);
    }
    if (count_patterns == on) {
        outstream << "\n" << container.get_pattern_count_info() << "\n";
    }
    if (profile) {
        outstream << "\n" << container.get_profiling_info() << "\n";
    }
    return EXIT_SUCCESS;
}


int parse_options(int argc, char** argv)
{
    extend_options_getenv(&argc, &argv);
    // use of this function requires options are settable on global scope
    while (true)
    {
        static const struct option long_options[] =
            {
                HFST_GETOPT_COMMON_LONG,
                HFST_GETOPT_UNARY_LONG,
                {"newline", no_argument, 0, 'n'},
                {"extract-patterns", no_argument, 0, 'x'},
                {"locate", no_argument, 0, 'l'},
                {"print-weights", no_argument, 0, 'w'},
                {"count-patterns", no_argument, 0, 'c'},
                {"delete-patterns", no_argument, 0, 'z'},
                {"no-mark-patterns", no_argument, 0, 'm'},
                {"max-context", required_argument, 0, 'b'},
                {"max-recursion", required_argument, 0, 'r'},
                {"weight-cutoff", required_argument, 0, 'W'},
                {"time-cutoff", required_argument, 0, 't'},
                {"profile", no_argument, 0, 'p'},
                {0,0,0,0}
            };
        int option_index = 0;
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT HFST_GETOPT_UNARY_SHORT "nxlwcdmb:r:W:t:p",
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
            blankline_separated = false;
            break;
        case 'x':
            extract_patterns = on;
            break;
        case 'l':
            locate_mode = on;
            break;
        case 'w':
            print_weights = on;
            break;
        case 'c':
            count_patterns = on;
            break;
        case 'z':
            delete_patterns = on;
            break;
        case 'm':
            mark_patterns = off;
            break;
        case 'b':
            max_context = atoi(optarg);
            if (max_context < 0)
            {
                std::cerr << "Invalid argument for --max-context\n";
                return EXIT_FAILURE;
            }
            break;
        case 'r':
            max_recursion = atoi(optarg);
            if (max_recursion < 0)
            {
                std::cerr << "Invalid argument for --max-recursion\n";
                return EXIT_FAILURE;
            }
            break;
        case 'W':
            weight_cutoff = atof(optarg);
            if (weight_cutoff < 0.0)
            {
                std::cerr << "Invalid argument for --weight-cutoff\n";
                return EXIT_FAILURE;
            }
        case 't':
            time_cutoff = atof(optarg);
            if (time_cutoff < 0.0)
            {
                std::cerr << "Invalid argument for --time-cutoff\n";
                return EXIT_FAILURE;
            }
            break;
        case 'p':
            profile = true;
            break;
#include "inc/getopt-cases-error.h"
        }
        
        
        
    }
        // no more options, we should now be at the input filename
        if ( (optind + 1) < argc)
        {
            std::cerr << "More than one input file given\n";
            return EXIT_FAILURE;
        }
        else if ( (optind + 1) == argc)
        {
            if (inputfilename != NULL) {
                std::cerr << "More than one input file given\n";
                return EXIT_FAILURE;
            } else {
                inputfilename = hfst_strdup(argv[optind]);
                inputfile = hfst_fopen(inputfilename, "r");
                if (inputfile == stdin)
                {
                    free(inputfilename);
                    inputfilename = hfst_strdup("<stdin>");
                }
            }
            return EXIT_CONTINUE;
        } else
        {
            if (inputfilename == NULL) {
                std::cerr << "No input file given\n";
                return EXIT_FAILURE;
            } else {
                return EXIT_CONTINUE;
            }
        }


#include "inc/check-params-common.h"


    
    return EXIT_FAILURE;
}

int main(int argc, char ** argv)
{
    hfst_set_program_name(argv[0], "0.1", "HfstPmatch");
    hfst_setlocale();
    int retval = parse_options(argc, argv);
    if (retval != EXIT_CONTINUE) {
        return retval;
    }
#ifdef HAVE_READLINE
    // Disable tab completion
    rl_bind_key('\t', rl_insert);
#endif

    std::ifstream instream(inputfilename,
                           std::ifstream::binary);
    if (!instream.good()) {
        std::cerr << "Could not open file " << inputfilename << std::endl;
        return EXIT_FAILURE;
    }
    try {
        hfst_ol::PmatchContainer container(instream);
        container.set_verbose(verbose);
        if (extract_patterns != not_defined)
            container.set_extract_patterns(extract_patterns == on);
        if (locate_mode != not_defined)
            container.set_locate_mode(locate_mode == on);
        if (count_patterns != not_defined)
            container.set_count_patterns(count_patterns == on);
        if (delete_patterns != not_defined)
            container.set_delete_patterns(delete_patterns == on);
        if (mark_patterns != not_defined)
            container.set_mark_patterns(mark_patterns == on);
        if (max_context >= 0)
            container.set_max_context(max_context);
        if (max_recursion >= 0)
            container.set_max_recursion(max_recursion);
        container.set_profile(profile);
#ifdef _MSC_VER
        //hfst::print_output_to_console(true);
#endif
    return process_input(container, std::cout);
    } catch(HfstException & e) {
        std::cerr << "The archive in " << inputfilename << " doesn't look right."
            "\nDid you make it with hfst-pmatch2fst or make sure it's in weighted optimized-lookup format?\n";
        return 1;
    }
//     if (outfile != stdout) {
//         std::filebuf fb;
// fb.open(outfilename, std::ios::out);
// std::ostream outstream(&fb);
// return process_input(container, outstream);
// fb.close();
//     } else {

}
