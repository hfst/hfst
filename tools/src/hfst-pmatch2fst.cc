//! @file hfst-pmatch2fst.cc
//!
//! @brief regular expression compiling command line tool
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

#ifndef WINDOWS
#include <unistd.h>
#endif

#include <iostream>
#include <fstream>
#include <memory>

#include <vector>
#include <map>
#include <string>
#include <iomanip>

using std::string;
using std::vector;
using std::pair;
using std::unique_ptr;
using std::make_unique;

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _MSC_VER
#  include "hfst-getopt.h"
#else
#  include <getopt.h>
#endif

#include <math.h>
#include <errno.h>
#include <time.h>

#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"
#include "HfstExceptionDefs.h"
#include "implementations/ConvertTransducerFormat.h"
#include "parsers/PmatchCompiler.h"
#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "hfst-tool-metadata.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

using hfst::HfstOutputStream;
using hfst::HfstTokenizer;
using hfst::HfstTransducer;
using hfst::pmatch::PmatchCompiler;

static char *epsilonname=NULL;
static bool disjunct_expressions=false;
static bool line_separated = false;
static bool flatten = false;
static bool include_cosine_distances = false;
static clock_t timer;

#if HAVE_OPENFST
static hfst::ImplementationType compilation_format = hfst::TROPICAL_OPENFST_TYPE;
#elif HAVE_FOMA
static hfst::ImplementationType compilation_format = hfst::FOMA_TYPE;
#elif HAVE_SFST
static hfst::ImplementationType compilation_format = hfst::SFST_TYPE;
#else
static hfst::ImplementationType compilation_format = hfst::ERROR_TYPE;
#endif

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
            "Compile regular expressions into transducer(s)\n (Experimental version)"
            "\n", program_name);
    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "String and format options:\n"
            "  -e, --epsilon=EPS         Map EPS as zero\n"
            "      --flatten             Compile in all RTNs\n"
            "      --cosine-distances    When compiling Like() operations, include cosine distance info\n");
    fprintf(message_out, "\n");

    fprintf(message_out,
            "If OUTFILE or INFILE is missing or -, standard streams will be used.\n"
            "If EPS is not defined, the default representation of 0 is used\n"
            "Weights are currently not implemented.\n"
            "\n"
        );

    fprintf(message_out, "Examples:\n"
            "  echo \"Define TOP  UppercaseAlpha Alpha* LC({professor}) EndTag(ProfName);\" | %s \n"
            "  create matcher that tags \"professor Chomsky\" as \"professor <ProfName>Chomsky</ProfName>\"\n"
            "\n", program_name);
    print_report_bugs();
    fprintf(message_out, "\n");
    print_more_info();
    fprintf(message_out, "\n");
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
                {"epsilon", required_argument, 0, 'e'},
                {"flatten", no_argument, 0, '1'},
                {"cosine-distances", no_argument, 0, '2'},
                {0,0,0,0}
            };
        int option_index = 0;
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "e:",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
        case 'e':
            epsilonname = hfst_strdup(optarg);
            break;
        case '1':
            flatten = true;
            break;
        case '2':
            include_cosine_distances = true;
            break;
#include "inc/getopt-cases-error.h"
        }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    return EXIT_CONTINUE;
}

#ifndef _GNU_SOURCE
char * get_current_dir_name()
{
    size_t PATH_BUFSIZE = 1024;
    size_t PATH_BUFSIZE_MAX = PATH_BUFSIZE * 8; // sanity
    char * retval = (char *) NULL;
    while(retval == NULL) {
        retval = (char *) realloc(retval, PATH_BUFSIZE);
        if(getcwd(retval, PATH_BUFSIZE) != 0) {
            return retval;
        }
        if(errno == ERANGE && PATH_BUFSIZE < PATH_BUFSIZE_MAX) {
            PATH_BUFSIZE *= 2;
            continue;
        }
        if (errno == EACCES) {
            throw std::runtime_error("Unable to access working directory");
        }
        // something else went wrong, just forget about it and try to go on
        retval[0] = '\0';
        return retval;
    }
}
#endif

int
process_stream(HfstOutputStream& outstream)
{
    PmatchCompiler comp(compilation_format);
    comp.set_verbose(verbose);
    comp.set_flatten(flatten);
    comp.set_include_cosine_distances(include_cosine_distances);
    std::string file_contents;
    std::map<std::string, HfstTransducer*> definitions;
    int c;

#ifdef _MSC_VER
    if (inputfile == stdin && !silent)
      {
        warning(0, 0, "Reading from standard input. UTF-8 characters\n"
                "outside ascii range are supported only if input comes from a file.");
      }
#endif

    std::string includedir = "";
#ifndef _MSC_VER
    std::string inputfilename_str(inputfilename);
    if (inputfile != stdin && inputfilename_str.size() > 0) {
        if (inputfilename_str[0] == '/') {
            // absolute path
            includedir = inputfilename_str;
        } else {
            char * pwd = get_current_dir_name();
            std::string tmp(pwd);
            includedir = tmp + "/" + inputfilename_str;
            free(pwd);
        }
        size_t slashpos = includedir.rfind('/');
        if (slashpos == std::string::npos) {
            // mysterious, we'll just use the working dir
            includedir = "";
        } else {
            includedir = includedir.substr(0, slashpos + 1);
        }
    }
#endif
    comp.set_include_path(includedir);

    while ((c = fgetc(inputfile)) != EOF) {
        file_contents.push_back(c);
    }
    if (file_contents.size() > 1) {
      try
        {
          definitions = comp.compile(file_contents);
        }
      catch(HfstException & e)
        {
          std::cerr << e.name << std::endl;
          return EXIT_FAILURE;
        }
    }

    if (verbose) {
        std::cerr << std::setiosflags(std::ios::fixed) << std::setprecision(2);
        timer = clock();
        std::cerr << "Building hfst-ol alphabet... ";
    }
    
    // A dummy transducer with an alphabet with all the symbols
    HfstTransducer harmonizer(compilation_format);
    
    // First we need to collect a unified alphabet from all the transducers.
    hfst::StringSet symbols_seen;
    for (std::map<std::string, HfstTransducer *>::const_iterator it =
             definitions.begin(); it != definitions.end(); ++it) {
        hfst::StringSet string_set = it->second->get_alphabet();
        for (hfst::StringSet::const_iterator sym = string_set.begin();
             sym != string_set.end(); ++sym) {
            if (symbols_seen.count(*sym) == 0) {
                harmonizer.disjunct(HfstTransducer(*sym, compilation_format));
                symbols_seen.insert(*sym);
            }
        }
    }
    if (symbols_seen.size() == 0) {
        // We don't recognise anything, go home early
        std::cerr << program_name << ": Empty ruleset, nothing to write\n";
        return EXIT_FAILURE;
    }
    
    // Then we convert it...
    harmonizer.convert(hfst::HFST_OLW_TYPE);
    // Use these for naughty intermediate steps to make sure
    // everything has the same alphabet
    hfst::HfstBasicTransducer * intermediate_tmp;
    hfst_ol::Transducer * harmonized_tmp;
    hfst::HfstTransducer * output_tmp;
    
    if (verbose) {
        double duration = (clock() - timer) /
            (double) CLOCKS_PER_SEC;
        timer = clock();
        std::cerr << "built in " << duration << " seconds\n";
        std::cerr << "Converting TOP... ";
    }
        
    // When done compiling everything, look for TOP and output it first.
    if (definitions.count("TOP") == 1) {
        std::map<std::string, std::string> properties = definitions["TOP"]->get_properties();
        intermediate_tmp = hfst::implementations::ConversionFunctions::
            hfst_transducer_to_hfst_basic_transducer(*definitions["TOP"]);
        harmonized_tmp = hfst::implementations::ConversionFunctions::
            hfst_basic_transducer_to_hfst_ol(intermediate_tmp,
                                             true, // weighted
                                             "", // no special options
                                             &harmonizer); // harmonize with this
        output_tmp = hfst::implementations::ConversionFunctions::
            hfst_ol_to_hfst_transducer(harmonized_tmp);
        output_tmp->set_name("TOP");
        for(std::map<std::string, std::string>::iterator it = properties.begin();
            it != properties.end(); ++it) {
            output_tmp->set_property(it->first, it->second);
        }
        outstream << *output_tmp;
        delete definitions["TOP"];
        definitions.erase("TOP");
        delete intermediate_tmp;
        delete output_tmp;

        if (verbose) {
            double duration = (clock() - timer) /
                (double) CLOCKS_PER_SEC;
            timer = clock();
            std::cerr << "converted in " << duration << " seconds\n";
        }
    
        for (std::map<std::string, HfstTransducer *>::iterator it =
                 definitions.begin(); it != definitions.end(); ++it) {
            if (verbose) {
                std::cerr << "Converting " << it->first << "... ";
                timer = clock();
            }
            intermediate_tmp = hfst::implementations::ConversionFunctions::
                hfst_transducer_to_hfst_basic_transducer(*(it->second));
            harmonized_tmp = hfst::implementations::ConversionFunctions::
                hfst_basic_transducer_to_hfst_ol(intermediate_tmp,
                                                 true, // weighted
                                                 "", // no special options
                                                 &harmonizer); // harmonize with this
            output_tmp = hfst::implementations::ConversionFunctions::
                hfst_ol_to_hfst_transducer(harmonized_tmp);
            output_tmp->set_name(it->first);
            outstream << *output_tmp;;
            delete it->second;
            delete intermediate_tmp;
            delete output_tmp;
            if (verbose) {
                double duration = (clock() - timer) /
                    (double) CLOCKS_PER_SEC;
                std::cerr << "converted in " << duration << " seconds\n";
            }
        }
    } else {
        std::cerr << program_name << ": Empty ruleset, nothing to write\n";
        return EXIT_FAILURE;
    }
    outstream.close();
    return EXIT_SUCCESS;
}

extern int pmatchdebug;

int main( int argc, char **argv )
{
#ifdef WINDOWS
  _setmode(1, _O_BINARY);
#endif

//    pmatchdebug = 1;

    hfst_set_program_name(argv[0], "0.1", "Pmatch2Fst");
    int retval = parse_options(argc, argv);
    if (retval != EXIT_CONTINUE)
    {
        return retval;
    }
    // close buffers, we use streams
    if (outfile != stdout)
    {
        fclose(outfile);
    }
    verbose_printf("Reading from %s, writing to %s\n",
                   inputfilename, outfilename);
    // here starts the buffer handling part
    auto outstream = (outfile != stdout) ?
        make_unique<HfstOutputStream>(outfilename, hfst::HFST_OLW_TYPE) :
        make_unique<HfstOutputStream>(hfst::HFST_OLW_TYPE);
    process_stream(*outstream);
    free(inputfilename);
    free(outfilename);
    return EXIT_SUCCESS;
}

