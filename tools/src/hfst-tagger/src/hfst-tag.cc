//! @file hfst-tag.cc
//!
//! @brief Transducer array printing command line tool
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

#include "use_model_src/SentenceTagger.h"

SentenceTagger * tagger = NULL;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
        "Tag a text file given from stdin using an hfst tagger.\n"
        "\n", program_name);

    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "\n");
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
            {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "wDnf:",
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
        break;
#include "inc/getopt-cases-error.h"
        }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    return EXIT_CONTINUE;
}

int process_input_data(std::string tagger_file_prefix)
{
  // Read fst files from stdin.
  verbose_printf("Read tagger.");

  tagger = new SentenceTagger(tagger_file_prefix + ".lex",
                              tagger_file_prefix + ".seq");
      
  return EXIT_SUCCESS;
}

StringVector get_sentence_vector(void)
{
  StringVector sentence_vector;

  // Add initial buffer symbols.
  sentence_vector.push_back("||");
  sentence_vector.push_back("||");

  std::string word;
  
  bool read_non_empty_word = false;

  while (std::cin.peek() != EOF)
    {
      std::getline(std::cin,word);

      // Skip initial empty lines. The first empty line preceeded by a
      // non emtpy line marks the end of the sentence.
      if (word.empty())
        {
          if (read_non_empty_word)
            { break; }
          else
            { continue; }
        }
      
      read_non_empty_word = true;

      sentence_vector.push_back(word);
    }

  // Add final buffer symbols.
  sentence_vector.push_back("||");
  sentence_vector.push_back("||");

  return sentence_vector;
}

void print_analysis(const WeightedStringPairVector &res,std::ostream * out)
{
  const StringPairVector &v = res.second;

  for (StringPairVector::const_iterator it = v.begin();
       it != v.end();
       ++it)
    {
      // Skip buffer symbols, since the user doesn't want to see
      // those.
      if (std::string(it->first) == "||")
        { continue; }

      (out == NULL ? std::cout : *out) << it->first << "\t" << it->second
                                       << std::endl;
    }

  // Print an empty line at the end to signal sentence boundary.
  (out == NULL ? std::cout : *out) << std::endl;
}

int main(int argc, char * argv[])
{

  hfst_set_program_name(argv[0], "0.1", "HfstTag");

  int retval = parse_options(argc, argv);
  if (retval != EXIT_CONTINUE)
    {
      return retval;
    }

  std::string tagger_file_name = inputfilename;
  std::string output_file_name = outfilename;

  if (tagger_file_name == "<stdin>")
    { error(EXIT_FAILURE, 0, "Tagger cannot be given in STDIN."); }

  process_input_data(tagger_file_name);

  std::ostream * out = NULL;

  if (output_file_name != "<stdout>")
    { out = new std::ofstream(output_file_name.c_str()); }

  while (std::cin.peek() != EOF)
    {
      StringVector v = get_sentence_vector();
      WeightedStringPairVector res = (*tagger)[v];
      print_analysis(res, out);
    }

  delete out;

  return EXIT_SUCCESS;
}
