//! @file hfst-reweight-tagger.cc
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
#include <sstream>

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

typedef std::vector<float> FloatVector;

using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using hfst::HfstTransducer;
using hfst::HFST_OLW_TYPE;
using hfst::TROPICAL_OPENFST_TYPE;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
            "Reweight a tagger according to a configuration file\n"
            "hfst_tagger_config.\n"
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

float coeff;

float reweight(float w)
{ return coeff*w; }

void reweight_fst(HfstInputStream &in,
                  HfstOutputStream &out,
                  float coefficient)
{
  HfstTransducer fst(in);
  coeff = coefficient;
  fst.transform_weights(&reweight);
  out << fst;
}

int process_input_data(const std::string &tagger_file_name,
                       const std::string &output_file_name,
                       const FloatVector &coefficients)
{
  // Read sequence model fsts and reweight.
  verbose_printf("Reading models and rewighting.");

  HfstInputStream seq_in(tagger_file_name + ".seq");

  if (not seq_in.is_good())
    { error(EXIT_FAILURE, 0, "Sequence model file cannot be opened."); }

  HfstOutputStream seq_out(output_file_name + ".seq",TROPICAL_OPENFST_TYPE);
  
  for (FloatVector::const_iterator it = coefficients.begin();
       it != coefficients.end();
       ++it)
    {
      if (not seq_in.is_good())
        { error(EXIT_FAILURE, 0, "Config file has too many patterns."); }

      reweight_fst(seq_in, seq_out, *it);
    }

  if (seq_in.is_good())
    { error(EXIT_FAILURE, 0, "Config file has too few patterns."); }

  // Rename lexicon.
  HfstInputStream lex_in(tagger_file_name + ".lex");

  if (not lex_in.is_good())
    { error(EXIT_FAILURE, 0, "Lexical model file cannot be opened."); }

  HfstTransducer lex(lex_in);

  HfstOutputStream lex_out(output_file_name + ".lex",
                           HFST_OLW_TYPE);
  lex_out << lex;

  return EXIT_SUCCESS;
}

float get_coefficient(const std::string &line)
{
  size_t last_tab_pos = line.find_last_of('\t');
  
  if (last_tab_pos == std::string::npos)
    { return -1; }

  std::istringstream in(line.substr(last_tab_pos));

  float coefficient;

  in >> coefficient;

  if (in.fail())
    { return -1; }

  return coefficient;
}

FloatVector parse_coefficients(void)
{
  std::ifstream coeff_in("hfst_tagger_config");

  if (coeff_in.peek() == EOF)
    { error(EXIT_FAILURE, 0, "Couldn't open configuration file."); }

  FloatVector coefficients;

  while (coeff_in.peek() != EOF)
    {
      std::string line;

      std::getline(coeff_in,line);
      
      if (line.empty())
        { continue; }

      float coefficient = get_coefficient(line);

      if (coefficient == -1)
        { error(EXIT_FAILURE, 0, "Invalid configuration file."); }

      coefficients.push_back(coefficient);
    }

  return coefficients;
}

int main(int argc, char * argv[])
{
  hfst_set_program_name(argv[0], "0.1", "HfstReweightTagger");
  int retval = parse_options(argc, argv);
  
  if (retval != EXIT_CONTINUE)
    {
      return retval;
    }

  std::string tagger_file_name = inputfilename;
  std::string output_file_name = outfilename;

  if (tagger_file_name == "<stdin>")
    { error(EXIT_FAILURE, 0, "Tagger cannot be given in STDIN."); }

  if (output_file_name == "<stdout>")
    { error(EXIT_FAILURE, 0, "Output file cannot be STDOUT."); }

  FloatVector coefficients = parse_coefficients();

  process_input_data(tagger_file_name,output_file_name,coefficients);

  return EXIT_SUCCESS;
}
