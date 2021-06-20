//! @file hfst-guess.cc
//!
//! @brief Tool for compiling a guesser (and
//! generator) for guessing analyses/paradigms of unknown words.
//!
//! @author Miikka Silfverberg (HFST Team)

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
#include <set>
#include <map>
#include <memory>
#include <sstream>

using std::set;
using std::map;
using std::unique_ptr;
using std::make_unique;

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

#include "generate_model_forms.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using hfst::HFST_OLW_TYPE;
using hfst::TROPICAL_OPENFST_TYPE;

// add tools-specific variables here
static bool generate_model_forms=false;
static std::string model_form_filename="";
static size_t max_number_of_guesses = 5;
static size_t max_number_of_forms = 2;
static float generate_threshold = 50;

size_t get_size_t(const std::string &str)
{
  std::istringstream in(str);
  
  size_t i;
  in >> i;

  if (in.fail())
    { throw "fail"; }

  return i;
}

float get_float(const std::string &str)
{
  std::istringstream in(str);
  
  float i;
  in >> i;

  if (in.fail())
    { return -1; }

  return i;
}

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
        "Use a guesser (and generator) to guess analyses or inflectional\n"
    "paradigms of unknown words.\n"
        "\n", program_name);

    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "Guesser options:\n"
        "  -f, --model-form-filename       Inflectional information for\n"
    "                                  generated model forms is read\n"
    "                                  from this file.\n"
        "  -n, --max-number-of-guesses     Maximal number of analysis\n"
        "                                  per word form (5 by default).\n"
    "  -m  --max-number-of-forms       Maximal number of generated model\n"
    "                                  forms per guess (2 by default).\n"
    "  -g  --generate-threshold        Generate only forms whose weight\n"
    "                                  is better than the weight of the\n"
    "                                  of the best form plus this threshold.\n"
    "                                  (50 by default)."
        );
    fprintf(message_out, "\n");
    fprintf(message_out,
        "The guesser and generator should be constructed using the tool\n"
        "hfst-guessify, which can compile a guesser and generator from a\n"
        "morphological analyzer. hfst-guessify packages the guesser and\n"
        "generator in the same fst-file.\n");
    fprintf(message_out, "\n");
    fprintf(message_out,
        "If option -f is used, but a generator has not been compiled\n"
        "with the guesser, a generator will be compiled, which will\n"
        "increase load time.\n"
        );
    fprintf(message_out, "\n");
    fprintf(message_out, "\n");
    fprintf(message_out,
       "If OUTFILE or INFILE is missing or -, standard streams will be used.\n"
    );
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
        {"generate-threshold", required_argument, 0, 'g'},
        {"model-form-filename", required_argument, 0, 'f'},
        {"max-number-of-guesses", required_argument, 0, 'n'},
        {"max-number-of-forms", required_argument, 0, 'm'},
            {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "f:m:n:g:",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
          // add tool-specific cases here
    case 'f':
      generate_model_forms = true;
      model_form_filename = optarg;
      break;

    case 'g':
      generate_threshold = get_float(optarg);
      if (generate_threshold < 0)
        {
          error(EXIT_FAILURE, 0, "Invalid generate threshold %s. "
                "Give a positive float.", optarg);
        }

      break;

    case 'n':
      try
        {
          max_number_of_guesses = get_size_t(optarg);
        }
      catch (const char * msg)
        {
          (void)msg;
          error(EXIT_FAILURE, 0, "Invalid maximal number of guesses %s. "
                "Give a positive int.", optarg);
        }
      
      break;

    case 'm':
      try
        {
          max_number_of_forms = get_size_t(optarg);
        }
      catch (const char * msg)
        {
          (void)msg;
          error(EXIT_FAILURE, 0, "Invalid maximal number of generated "
                "forms %s. Give a positive int.", optarg);
        }
      
      break;

#include "inc/getopt-cases-error.h"
        }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    return EXIT_CONTINUE;
}

int main( int argc, char **argv )
{
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
  _setmode(1, _O_BINARY);
#endif
  hfst_set_program_name(argv[0], "0.3", "HfstGuess");
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
    unique_ptr<HfstInputStream> instream;

    try
      {
        instream.reset(inputfile != stdin ?
            new HfstInputStream(inputfilename) :
            new HfstInputStream());
      }
    catch(const HfstException e)
      {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              inputfilename);
        return EXIT_FAILURE;
      }

    std::ostream * out;

    try
      {
    out = (outfile != stdout ?
           new std::ofstream(outfilename) :
           &std::cout);
      }
    catch(...)
      {
        error(EXIT_FAILURE, 0, "%s cannot be opened for writing.",
              outfilename);
        return EXIT_FAILURE;
      }
    
    
    HfstTransducer * guesser;
    try
      {
    guesser = new HfstTransducer(*instream);
      }
    catch (const HfstException e)
      {
    error(EXIT_FAILURE, 0, "Error when reading guesser from file %s",
              inputfilename);
        return EXIT_FAILURE;
      }
      
    if (guesser->get_properties().count("reverse input") != 1)
      {
    error(EXIT_FAILURE, 0, "The transducer in %s is not a guesser.",
              inputfilename);
        return EXIT_FAILURE;
      }

    HfstTransducer * generator = NULL;
    
    if (generate_model_forms)
      {
    if (not instream->is_good())
      {
        verbose_printf("No generator found in %s. Compiling generator "
               "from guesser.\n",inputfilename);
        
        generator = new HfstTransducer(*guesser);
        generator->convert(TROPICAL_OPENFST_TYPE);
        generator->invert();
        generator->convert(HFST_OLW_TYPE);
      }
    else
      { generator = new HfstTransducer(*instream); }
      }

    HfstTokenizer tokenizer = get_alphabet_string_tokenizer(*guesser);
    
    StringVectorVector model_forms;
    
    if (generate_model_forms)
      {
    verbose_printf("Reading inflectional information for model forms\n"
               "from %s.\n",model_form_filename.c_str());
    
    try
      {
        model_forms = read_model_forms(model_form_filename,tokenizer);
      }
    catch (const InvalidModelLine &e)
      {
        std::cerr << "Invalid model form line in model form file:"
              << std::endl
              << e.line
              << std::endl;
        return EXIT_FAILURE;
      }
      }
    while (std::cin.peek() != EOF)
      {
    std::string line;
    std::getline(std::cin,line);
    
    StringVectorVector guesses = get_guesses(line,
                         *guesser,
                         max_number_of_guesses,
                         tokenizer);
    
    if (generate_model_forms)
      {
        if (generator == 0) // make scan-build happy, this should not happen
          throw "Error: generator has a NULL value.";
        StringVectorVector paradigms = get_paradigms(line,
                                                     guesses,
                                                     *generator,
                                                     model_forms,
                                                     max_number_of_forms,
                                                     generate_threshold);
        
        for (StringVectorVector::const_iterator it = paradigms.begin();
         it != paradigms.end();
         ++it)
          {
        (*out) << *it << std::endl;
          }
      }
    else
      {
        for (StringVectorVector::iterator it = guesses.begin();
         it != guesses.end();
         ++it)
          {
        std::reverse(it->begin(), it->end());

        (*out) << line << "\t" << *it << std::endl;
          }
      }
    (*out) << std::endl;
      }

    if (outfile != stdout)
      { assert(out != &std::cout); delete out; }

    free(inputfilename);
    free(outfilename);

    delete guesser;
    delete generator;

    return EXIT_SUCCESS;
}

