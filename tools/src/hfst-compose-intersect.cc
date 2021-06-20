//! @file hfst-compose.cc
//!
//! @brief Transducer composition tool
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
#include <getopt.h>
#include <set>

#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using hfst::ImplementationType;
using hfst::HfstTransducerVector;
using std::unique_ptr;
using std::make_unique;

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "hfst-tool-metadata.h"

#include "inc/globals-common.h"
#include "inc/globals-binary.h"

//static bool insert_missing_flags=false;

// If invert is true, the intersection of the rules is composed eith
// the lexicon. Otherwise the lexicon is composed with the
// intersection of the rules.
static bool invert=false;
static bool encode_weights=false;
static bool fast_ci=false;
static bool harmonize=false;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE1 [INFILE2]]\n"
             "Compose a lexicon with one or more rule transducers.\n"
        "\n", program_name );
        print_common_program_options(message_out);
        print_common_binary_program_options(message_out);
        fprintf(message_out, "Composition options:\n"
            "  -I, --invert                 Compose the intersection of the\n"
            "                               rules with the lexicon instead\n"
            "                               of composing the lexicon with\n"
            "                               the intersection of the rules.\n"
            "  -f, --fast                   Faster compose instersect using\n"
            "                               more memory.\n"
            "  -e, --encode-weights         Encode weights when minimizing\n"
            "                               (default is false).\n"
            "  -a, --harmonize              Harmonize symbols.\n"
           );
        //print_common_binary_program_parameter_instructions(message_out);
        fprintf(message_out,
"\nIf OUTFILE, or either INFILE1 or INFILE2 is missing or -, standard\n"
"streams will be used. INFILE1, INFILE2, or both, must be specified\n"
"The format of INFILE1 and INFILE2 must be the same; the result will\n"
"have the same format as these.\n"
"INFILE1 (the lexicon) must contain exactly one transducer.\n"
                "INFILE2 (rule file) may contain several transducers.\n");
        fprintf(message_out,
"\n"
"Examples:\n"
"  %s -o analyzer.hfst lexicon.hfst rules.hfst\n"
"compose rules with lexicon\n"
                "\n",
            program_name );
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
          {"invert", no_argument, 0, 'I'},
          {"encode-weights", no_argument, 0, 'e'},
          {"fast", no_argument, 0, 'f'},
          {"harmonize", no_argument, 0, 'a'},
          {0,0,0,0}
        };
        int option_index = 0;
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_BINARY_SHORT "FIeHfa",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }
        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-binary.h"
#include "inc/getopt-cases-error.h"
        case 'I':
          invert = true;
          break;
        case 'e':
          encode_weights = true;
          break;
        case 'f':
          fast_ci = true;
          break;
        case 'a':
          harmonize = true;
          break;
        }
    }

#include "inc/check-params-common.h"
#include "inc/check-params-binary.h"
    return EXIT_CONTINUE;
}

using hfst::implementations::HfstBasicTransducer;
using hfst::implementations::HfstState;
using hfst::HfstTokenizer;

typedef std::set<std::string> StringSet;

bool is_special_symbol(const std::string &symbol)
{ return symbol.size() > 2 and symbol[0] == '@' and *(symbol.rbegin()) == '@';}

std::string check_all_symbols(const HfstTransducer &lexicon,
                              const HfstTransducer &rule)
{
  HfstBasicTransducer rule_b(rule);

  StringSet rule_input_symbols;

  for (HfstState s = 0; s <= rule_b.get_max_state(); ++s)
    {
      for (hfst::implementations::HfstBasicTransitions::const_iterator it =
             rule_b[s].begin();
           it != rule_b[s].end();
           ++it)
        {
          const std::string &input_symbol = it->get_input_symbol();
          rule_input_symbols.insert(input_symbol);
        }
    }

  if (rule_input_symbols.count(hfst::internal_identity) != 0)
    { return ""; }

  HfstBasicTransducer lexicon_b(lexicon);

  for (HfstState s = 0; s <= lexicon_b.get_max_state(); ++s)
    {
      for (hfst::implementations::HfstBasicTransitions::const_iterator it =
             lexicon_b[s].begin();
           it != lexicon_b[s].end();
           ++it)
        {
          const std::string &output_symbol = it->get_output_symbol();

          if (rule_input_symbols.count(output_symbol) == 0)
            { return output_symbol; }
        }
    }

  return "";
}

std::string check_multi_char_symbols
(const HfstTransducer &lexicon, const HfstTransducer &rule)
{
  HfstBasicTransducer lexicon_b(lexicon);
  HfstBasicTransducer rule_b(rule);

  HfstTokenizer tokenizer;

  StringSet rule_input_symbols;

  for (HfstState s = 0; s <= rule_b.get_max_state(); ++s)
    {
      for (hfst::implementations::HfstBasicTransitions::const_iterator it =
             rule_b[s].begin();
           it != rule_b[s].end();
           ++it)
        {
          const std::string &input_symbol = it->get_input_symbol();
          rule_input_symbols.insert(input_symbol);
        }
    }

  for (HfstState s = 0; s <= lexicon_b.get_max_state(); ++s)
    {
      for (hfst::implementations::HfstBasicTransitions::const_iterator it =
             lexicon_b[s].begin();
           it != lexicon_b[s].end();
           ++it)
        {
          const std::string &output_symbol = it->get_output_symbol();

          if (rule_input_symbols.count(output_symbol) == 0)
            {
              if (is_special_symbol(output_symbol))
                { continue; }

              if (tokenizer.tokenize_one_level(output_symbol).size() > 1)
                { return output_symbol; }
            }
        }
    }

  return "";
}

void harmonize_rules(HfstTransducer & lexicon, std::vector<HfstTransducer> & rules)
{
  for (std::vector<HfstTransducer>::iterator it = rules.begin(); it != rules.end(); it++)
    {
      it->harmonize(lexicon);
    }
  return;
}

int
compose_streams(HfstInputStream& firststream, HfstInputStream& secondstream)
{
  // there must be at least one transducer in both input streams
  //bool continueReading = firststream.is_good() && secondstream.is_good();

  hfst::ImplementationType type1 = firststream.get_type();
  hfst::ImplementationType type2 = secondstream.get_type();
  hfst::ImplementationType output_type = hfst::UNSPECIFIED_TYPE;
  if (type1 != type2)
    {
      if (allow_transducer_conversion)
        {
          int ct = conversion_type(type1, type2);
          std::string warnstr("Transducer type mismatch in " + std::string(firstfilename) + " and " + std::string(secondfilename) + "; ");
          if (ct == 1)
            { warnstr.append("using former type as output"); output_type = type1; }
          else if (ct == 2)
            { warnstr.append("using latter type as output"); output_type = type2; }
          else if (ct == -1)
            { warnstr.append("using former type as output, loss of information is possible"); output_type = type1; }
          else /* should not happen */
            { throw "Error: hfst-compose-intersect: conversion_type returned an invalid integer"; }
          warning(0, 0, warnstr.c_str());
        }
      else
        {
          error(EXIT_FAILURE, 0, "Transducer type mismatch in %s and %s; "
                "formats %s and %s are not compatible for compose-intersect (--do-not-convert was requested)",
                firstfilename, secondfilename, hfst_strformat(type1), hfst_strformat(type2));
        }
    }
  else
    {
      output_type = type1;
    }

  HfstOutputStream outstream = (outfile != stdout) ?
    HfstOutputStream(outfilename, output_type) : HfstOutputStream(output_type);

    bool bothInputs = firststream.is_good() && secondstream.is_good();
    (void)bothInputs;

    if ( is_input_stream_in_ol_format(firststream, "hfst-compose-intersect") ||
         is_input_stream_in_ol_format(secondstream, "hfst-compose-intersect") )
      {
        return EXIT_FAILURE;
      }

    HfstTransducerVector rules;
    size_t rule_n = 1;

    while (secondstream.is_good()) {
      HfstTransducer rule(secondstream);
      rule.convert(output_type);
      const char* rulename = rule.get_name().c_str();
      if (strlen(rulename) > 0)
        {
          verbose_printf("Reading and minimizing rule %s...\n",
                         rulename);
        }
      else
        {
          verbose_printf("Reading and minimizing rule " SIZE_T_SPECIFIER "...\n",
                         rule_n);
        }
      bool enc = hfst::get_encode_weights();
      if (encode_weights)
        {
          hfst::set_encode_weights(true);
        }
      rule.minimize();
      if (encode_weights)
        {
          hfst::set_encode_weights(enc);
        }

      rules.push_back(rule);
      rule_n++;
    }

    while (firststream.is_good())
      {
        verbose_printf("Reading lexicon...");
        HfstTransducer lexicon(firststream);
        lexicon.convert(output_type);
        char* lexiconname = hfst_get_name(lexicon, firstfilename);
        verbose_printf(" %s read\n", lexiconname);

        verbose_printf("Computing intersecting composition...\n");

        if (rules.size() > 0)
          {
            std::string symbol;
            if ((symbol = check_all_symbols(lexicon,rules[0])) != "")
              {
                warning(0, 0,
                        "\nFound output symbols (e.g. \"%s\") in transducer in\n"
                        "file %s which will be filtered out because they are\n"
                        "not found on the input tapes of transducers in file\n"
                        "%s.",
                        symbol.c_str(), firstfilename, secondfilename);
              }
            else if ((symbol = check_multi_char_symbols(lexicon,rules[0])) != "")
              {
                warning(0, 0,
                        "\nFound output multi-char symbols (\"%s\") in \n"
                        "transducer in file %s which are not found on the\n"
                        "input tapes of transducers in file %s.",
                        symbol.c_str(), firstfilename, secondfilename);
              }
          }

        if (harmonize)
          {
            harmonize_rules(lexicon, rules);
          }

        if (fast_ci)
          {
            // To hopefully speed up stuff: Compose intersect the output
            // of the lexicon with the rules and then compose the original
            // lexicon with the result.

            if (invert)
              {
                HfstTransducer lexicon_input(lexicon);
                lexicon_input.input_project().minimize();
                lexicon_input.compose_intersect(rules,true);

                lexicon_input.compose(lexicon);
                lexicon = lexicon_input;
              }
            else
              {
                HfstTransducer lexicon_output(lexicon);
                lexicon_output.output_project().minimize();
                lexicon_output.compose_intersect(rules,false);
                lexicon.compose(lexicon_output);
              }
          }
        else
          {
            lexicon.compose_intersect(rules,invert);
          }

        char* composed_name = static_cast<char*>(malloc(sizeof(char) *
                                                        (strlen(lexiconname) +
                                                         strlen(secondfilename) +
                                                         strlen("compose(%s, interserct(%s))"))
                                                        + 1));
        if (sprintf(composed_name, "compose(%s, intersect(%s))",
                    lexiconname, secondfilename) > 0)
          {
            lexicon.set_name(composed_name);
          }
        hfst_set_formula(lexicon, lexicon, " ∘ ⋂R");

        verbose_printf("Storing result in %s...\n", outfilename);
        outstream << lexicon;
      }

    firststream.close();
    secondstream.close();
    outstream.close();
    return EXIT_SUCCESS;
}


int main( int argc, char **argv ) {
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
  _setmode(1, _O_BINARY);
#endif
    hfst_set_program_name(argv[0], "0.1", "HfstComposeIntersect");
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
    if (outfile != stdout)
    {
        fclose(outfile);
    }
    verbose_printf("Reading from %s and %s, writing to %s\n",
        firstfilename, secondfilename, outfilename);
    // here starts the buffer handling part
    unique_ptr<HfstInputStream> firststream;
    unique_ptr<HfstInputStream> secondstream;
    try {
        firststream.reset(firstfile != stdin ?
            new HfstInputStream(firstfilename) : new HfstInputStream());
    } catch(const HfstException e) {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              firstfilename);
    }
    try {
        secondstream.reset((secondfile != stdin) ?
            new HfstInputStream(secondfilename) : new HfstInputStream());
    } catch(const HfstException e) {
        error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
              secondfilename);
    }

    retval = compose_streams(*firststream, *secondstream);
    free(firstfilename);
    free(secondfilename);
    free(outfilename);
    return retval;
}

