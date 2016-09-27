//! @file hfst-substitute.cc
//!
//! @brief Transducer label modification
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

#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"
#include "implementations/HfstBasicTransducer.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using hfst::implementations::HfstState;
using hfst::implementations::HfstBasicTransducer;
using hfst::implementations::HfstBasicTransition;
using hfst::String;
using hfst::StringPair;
typedef std::map<String, String> HfstSymbolSubstitutions;
typedef std::map<StringPair, StringPair> HfstSymbolPairSubstitutions;

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "hfst-tool-metadata.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

// Added this to accommodate checking for SFST to handle foma better;
// SH 24.8.2011
#if HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_SFST 0
#endif

static char* from_label = 0;
static StringPair* from_pair = 0;
static char* from_file_name = 0;
static FILE* from_file = 0;
static char* to_label = 0;
static StringPair* to_pair = 0;
static char* to_transducer_filename = 0;
static HfstTransducer* to_transducer = 0;
static bool compose = false;
static HfstTransducer* substitution_trans = 0;
static bool delayed = false;
static HfstSymbolSubstitutions* label_substitution_map = 0;
static HfstSymbolPairSubstitutions* pair_substitution_map = 0;
static bool in_order = false;
static bool allow_transducer_conversion = true;

/**
 * @brief parse string pair from arc label.
 *
 * @return new stringpair, or null if not a pair.
 */
static
StringPair*
label_to_stringpair(const char* label)
  {
    const char* colon = strchr(label, ':');
    const char* endstr = strchr(label, '\0');
    while (colon != NULL)
      {
        if (colon == label)
          {
            colon = strchr(colon + 1, ':');
          }
        else if (colon == (endstr - 1))
          {
            colon = 0;
          }
        else if (*(colon - 1) == '\\')
          {
            if (colon > (label + 1))
              {
                if (*(colon - 2) == '\\')
                  {
                    break;
                  }
                else
                  {
                    colon = strchr(colon + 1, ':');
                  }
              }
          }
        else
          {
            break;
          }
      }
    char* first = 0;
    char* second = 0;
    if ((label < colon) && (colon < endstr))
      {
        first = hfst_strndup(label, colon-label);
        second = hfst_strndup(colon + 1, endstr - colon - 1);
      }
    else
      {
        return NULL;
      }

    if (strcmp(first, "@0@") == 0)
      {
        free(first);
        first = hfst_strdup(hfst::internal_epsilon.c_str());
      }
    if (strcmp(second, "@0@") == 0)
      {
        free(second);
        second = hfst_strdup(hfst::internal_epsilon.c_str());
      }

    StringPair* rv = new StringPair(first, second);
    free(first);
    free(second);
    return rv;
  }

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "Relabel transducer arcs\n"
        "\n", program_name);

    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "Relabeling options:\n"
            "  -f, --from-label=FLABEL      replace FLABEL\n"
            "  -t, --to-label=TLABEL        replace with TLABEL\n"
            "  -T, --to-transducer=TFILE    replace with transducer read from TFILE\n"
            "  -F, --from-file=LABELFILE    read replacements from LABELFILE\n"
            "  -R, --in-order               keep the order of the replacements\n"
            "                               (with -F)\n"
            "Input options:\n"
            "  -C, --do-not-convert         require that transducers in TFILE and INFILE\n"
            "                               have the same type\n"
            "Transient optimisation schemes:\n"
            "  -9, --compose                compose substitutions when possible\n"
           );
    fprintf(message_out, "\n");
    print_common_unary_program_parameter_instructions(message_out);
    fprintf(message_out,
            "LABEL must be a symbol name in single arc in transducer,\n"
            "or colon separated pair defining an arc.\n"
            "If TFILE is specified, FLABEL must be a pair.\n"
            "LABELFILE is a 2 column tsv file where col 1 is FLABEL\n"
            "and col 2 gives TLABEL specifications.\n");
    fprintf(message_out,
            "\n"
            "Examples:\n"
            "  %s -i tr.hfst -o tr_relabeled.hfst -f 'a' -t 'A'\n"
            "      relabel all symbols 'a' with 'A'\n"
            "  %s -i tr.hfst -o tr_relabeled.hfst -f 'a:b' -t 'A:B'\n"
            "      relabel all arcs 'a:b' with 'A:B'\n"
            "  %s -i tr.hfst -o tr_relabeled.hfst -f 'a:b' -T repl.hfst\n"
            "      replace all arcs 'a:b' with transducer repl.hfst\n"
            "\n", program_name, program_name, program_name);
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
            {"from-label", required_argument, 0, 'f'},
            {"from-file", required_argument, 0, 'F'},
            {"to-label", required_argument, 0, 't'},
            {"to-transducer", required_argument, 0, 'T'},
            {"in-order", no_argument, 0, 'R'},
            {"compose", no_argument, 0, '9'},
            {"do-not-convert", no_argument, 0, 'C'},
            {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "f:F:t:T:R9C",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }
        FILE* f = 0;
        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
          // add tool-specific cases here
        case 'f':
            from_label = hfst_strdup(optarg);
            if (strcmp(from_label, "@0@") == 0)
              {
                free(from_label);
                from_label = hfst_strdup(hfst::internal_epsilon.c_str());
              }
            from_pair = label_to_stringpair(from_label);
            if (strlen(from_label) == 0)
              {
                error(EXIT_FAILURE, 0, "argument of source label option is "
                      "empty;\n"
                      "if you REALLY want to replace epsilons with something, "
                      "use @0@ or %s", hfst::internal_epsilon.c_str());
              }
            break;
        case 'F':
            from_file_name = hfst_strdup(optarg);
            from_file = hfst_fopen(from_file_name, "r");
            if (from_file == NULL)
            {
                return EXIT_FAILURE;
            }
            break;
        case 't':
            to_label = hfst_strdup(optarg);
            if (strcmp(to_label, "@0@") == 0)
              {
                free(to_label);
                to_label = hfst_strdup(hfst::internal_epsilon.c_str());
              }
            to_pair = label_to_stringpair(to_label);
            if (strlen(to_label) == 0)
              {
                error(EXIT_FAILURE, 0, "argument of target label option is "
                      "empty;\n"
                      "if you want to substitute something with epsilons, "
                      "use @0@ or %s", hfst::internal_epsilon.c_str());
              }
            break;
        case 'T':
            to_transducer_filename = hfst_strdup(optarg);
            f = hfst_fopen(to_transducer_filename, "r");
            if (f == NULL)
            {
                return EXIT_FAILURE;
            }
            fclose(f);
            break;
        case 'R':
          //error(EXIT_FAILURE, 0, "option --in-order is not implemented\n");
          in_order=true;
            break;
        case '9':
            compose = true;
            break;
        case 'C':
            allow_transducer_conversion = false;
            break;
#include "inc/getopt-cases-error.h"
        }
    }
    
    if ((from_label == 0) && (from_file_name == 0))
    {
        error(EXIT_FAILURE, 0,
              "Must state name of labels to rewrite with -f or -F");
        return EXIT_FAILURE;
    }
    if ((to_label == 0) && (to_transducer_filename == 0) &&
            (from_file_name == 0))
    {
        error(EXIT_FAILURE, 0,
              "Must give target labels with -t, -T or -F");
        return EXIT_FAILURE;
    }
#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    return EXIT_CONTINUE;
}

static
HfstBasicTransducer&
do_substitute(HfstBasicTransducer& trans, size_t transducer_n)
{
  if (from_pair && to_pair)
    {
      if (transducer_n < 2)
        {
          verbose_printf("Substituting pair %s:%s with pair %s:%s...\n",
                         from_pair->first.c_str(),
                         from_pair->second.c_str(),
                         to_pair->first.c_str(),
                         to_pair->second.c_str());
        }
      else
        {
          verbose_printf("Substituting pair %s:%s with pair %s:%s...\n",
                         from_pair->first.c_str(),
                         from_pair->second.c_str(),
                         to_pair->first.c_str(),
                         to_pair->second.c_str());
        }
      trans.substitute(*from_pair, *to_pair);
    }
  else if (from_label && to_label)
    {
      if (transducer_n < 2)
        {
          verbose_printf("Substituting label %s with label %s...\n", from_label,
                         to_label);
        }
      else
        {
          verbose_printf("Substituting label %s with label %s... " SIZE_T_SPECIFIER "\n",
                         from_label, to_label, transducer_n);
        }
      trans.substitute(from_label, to_label);
    }
  else if (from_pair && to_transducer)
    {
      char* to_name = strdup(to_transducer->get_name().c_str());
      if (strlen(to_name) <= 0)
        {
          to_name = strdup(to_transducer_filename);
        }
      if (transducer_n < 2)
        {
          verbose_printf("Substituting pair %s:%s with transducer %s...\n",
                         from_pair->first.c_str(),
                         from_pair->second.c_str(),
                         to_name);
        }
      else
        {
          verbose_printf("Substituting pair %s:%s with transducer %s... " SIZE_T_SPECIFIER "\n",
                         from_pair->first.c_str(),
                         from_pair->second.c_str(), to_name,
                         transducer_n);
        }
      trans.substitute(*from_pair, *to_transducer);
    }

  else if (from_label && to_transducer)
    {
      char* to_name = strdup(to_transducer->get_name().c_str());
      if (strlen(to_name) <= 0)
        {
          to_name = strdup(to_transducer_filename);
        }
      if (transducer_n < 2)
        {
          verbose_printf("Substituting id. label %s with transducer %s...\n",
                         from_label, to_name);
        }
      else
        {
          verbose_printf("Substituting id. label %s with transducer %s... " SIZE_T_SPECIFIER "\n",
                         from_label, to_name,
                         transducer_n);
        }
      hfst::StringPair from_arc(from_label, from_label);
      trans.substitute(from_arc, *to_transducer);
    }
  return trans;
}

static
HfstTransducer&
do_substitute(HfstTransducer& trans, size_t transducer_n)
{
  if (from_pair && to_pair)
    {
      if (transducer_n < 2)
        {
          verbose_printf("Substituting pair %s:%s with pair %s:%s...\n",
                         from_pair->first.c_str(),
                         from_pair->second.c_str(),
                         to_pair->first.c_str(),
                         to_pair->second.c_str());
        }
      else
        {
          verbose_printf("Substituting pair %s:%s with pair %s:%s...\n",
                         from_pair->first.c_str(),
                         from_pair->second.c_str(),
                         to_pair->first.c_str(),
                         to_pair->second.c_str());
        }
      trans.substitute(*from_pair, *to_pair);
    }
  else if (from_label && to_label)
    {
      if (compose)
        {
          if (transducer_n < 2)
            {
              verbose_printf("Delaying substitution of label %s with "
                             "label %s...\n", from_label,
                             to_label);
            }
          else
            {
              verbose_printf("Delaying substitution of label %s with "
                             "label %s... " SIZE_T_SPECIFIER "\n",
                             from_label, to_label, transducer_n);
            
            }
          HfstTransducer substitution(from_label, to_label, trans.get_type());
          substitution_trans->disjunct(substitution);
          delayed = true;
        }
      else
        {
          if (transducer_n < 2)
            {
              verbose_printf("Substituting label %s with "
                             "label %s...\n", from_label,
                             to_label);
            }
          else
            {
              verbose_printf("Substituting label %s with "
                             "label %s... " SIZE_T_SPECIFIER "\n",
                             from_label, to_label, transducer_n);
            
            }
          trans.substitute(from_label, to_label);
        }
    }
  else if (from_pair && to_transducer)
    {
      char* to_name = strdup(to_transducer->get_name().c_str());
      if (strlen(to_name) <= 0)
        {
          to_name = strdup(to_transducer_filename);
        }
      if (transducer_n < 2)
        {
          verbose_printf("Substituting pair %s:%s with transducer %s...\n",
                         from_pair->first.c_str(),
                         from_pair->second.c_str(),
                         to_name);
        }
      else
        {
          verbose_printf("Substituting pair %s:%s with transducer %s... " SIZE_T_SPECIFIER "\n",
                         from_pair->first.c_str(),
                         from_pair->second.c_str(), to_name,
                         transducer_n);
        }
      trans.substitute(*from_pair, *to_transducer);
    }

  else if (from_label && to_transducer)
    {
      char* to_name = strdup(to_transducer->get_name().c_str());
      if (strlen(to_name) <= 0)
        {
          to_name = strdup(to_transducer_filename);
        }
      if (transducer_n < 2)
        {
          verbose_printf("Substituting id. label %s with transducer %s...\n",
                         from_label, to_name);
        }
      else
        {
          verbose_printf("Substituting id. label %s with transducer %s... " SIZE_T_SPECIFIER "\n",
                         from_label, to_name,
                         transducer_n);
        }
      hfst::StringPair from_arc(from_label, from_label);
      trans.substitute(from_arc, *to_transducer);
    }
  return trans;
}

static
void
perform_delayed(HfstTransducer& trans)
{
  verbose_printf("Finalising substitution transducer...\n");
  HfstTransducer sigmaMinusSubs = HfstTransducer(hfst::internal_identity,
                                            hfst::internal_identity,
                                            trans.get_type());
  HfstTransducer subsIn(*substitution_trans);
  subsIn.input_project();
  sigmaMinusSubs.subtract(subsIn);
  substitution_trans->disjunct(sigmaMinusSubs);
  substitution_trans->repeat_star();
  verbose_printf("Composing delayed substitutions on right...\n");
  trans.compose(*substitution_trans);
  verbose_printf("Minimising...\n");
  trans.minimize();
  substitution_trans->invert();
  verbose_printf("Composing delayed substitutions on left...\n");
  trans = substitution_trans->compose(trans);
  verbose_printf("Minimising...\n");
  trans.minimize();
}

int
process_stream(HfstInputStream& instream)
{
  bool symbol_pair_map_in_use=false;
  bool symbol_map_in_use=false;

  size_t transducer_n = 0;

  hfst::ImplementationType output_type = hfst::UNSPECIFIED_TYPE;

  if (to_transducer_filename != NULL)
    {
      try {
        HfstInputStream tostream(to_transducer_filename);
        to_transducer = new HfstTransducer(tostream);
        tostream.close();
      } catch (NotTransducerStreamException ntse)
        {
          error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
                to_transducer_filename);
          return EXIT_FAILURE;
        }
      hfst::ImplementationType to_transducer_type = to_transducer->get_type();
      hfst::ImplementationType instream_type = instream.get_type();
      if (to_transducer_type != instream_type)
          {
            if (allow_transducer_conversion)
              {
                int ct = conversion_type(instream_type, to_transducer_type);
                std::string warnstr("Transducer type mismatch in " + std::string(inputfilename) + " and " + std::string(to_transducer_filename) + "; ");
                if (ct == 1)
                  { warnstr.append("using former type as output"); output_type = instream_type; }
                else if (ct == 2)
                  { warnstr.append("using latter type as output"); output_type = to_transducer_type; }
                else if (ct == -1)
                  { warnstr.append("using former type as output, loss of information is possible"); output_type = instream_type; }
                else /* should not happen */
                  { throw "Error: hfst-disjunct: conversion_type returned an invalid integer"; }
                warning(0, 0, warnstr.c_str());
                to_transducer->convert(output_type);
              }
            else
              {
                error(EXIT_FAILURE, 0, "Transducer type mismatch in %s and %s; "
                      "formats %s and %s are not compatible for substitution (--do-not-convert was requested)",
                      inputfilename, to_transducer_filename, hfst_strformat(instream_type), hfst_strformat(to_transducer_type));
              }
          }
      else
        {
          output_type = instream.get_type();
        }
    }
  else
    {
      output_type = instream.get_type();
    }

  HfstOutputStream outstream = (outfile != stdout) ?
    HfstOutputStream(outfilename, output_type) : HfstOutputStream(output_type);

  HfstBasicTransducer* fallback = 0;
  bool warnedAlready = false;
  bool fellback = false;
  while (instream.is_good())
    {
        // SH 24.8.2011:
        // for some reason converting between foma and basic transducer
        // for substitution can leak lots and lots of space.
        // For this reason we currently do substitution in sfst and finally
        // convert back to foma.
        //bool got_foma = false;
      transducer_n++;
      HfstTransducer trans(instream);
      /*#if HAVE_SFST
      if (trans.get_type() == hfst::FOMA_TYPE) {
        if (!silent) {
          warning(0, 0, "NB: substitution for foma transducers will be done "
                  "via conversion to\n"
                  "SFST and back (if available)\n");
        }
        got_foma = true;
        trans = trans.convert(hfst::SFST_TYPE);
      }
      #endif*/
      char* inputname = strdup(trans.get_name().c_str());
      if (strlen(inputname) <= 0)
        {
          inputname = strdup(inputfilename);
        }
      if (transducer_n == 1)
        {
          verbose_printf("performing substitutions in %s...\n", inputname);
        }
      else
        {
          verbose_printf("performing substitutions in %s... " SIZE_T_SPECIFIER "\n", inputname,
                         transducer_n);
        }
      // initialize delayed substitutor automaton
      substitution_trans = new HfstTransducer(trans.get_type());
      if (from_file)
        {
          char* line = NULL;
          size_t len = 0;
          size_t line_n = 0;
          verbose_printf("reading substitutions from %s...\n", from_file_name);
          while (hfst_getline(&line, &len, from_file) != -1)
            {
              line_n++;
              if (*line == '\n')
                {
                  continue;
                }
              const char* tab = strstr(line, "\t");
              if (tab == NULL)
                {
                  if (*line == '#')
                    {
                      continue;
                    }
                  else
                    {
                      error_at_line(EXIT_FAILURE, 0, from_file_name, line_n,
                                    "At least one tab required per line");
                    }
                }
              const char* endstr = tab+1;
              while ((*endstr != '\0') && (*endstr != '\n'))
                {
                  endstr++;
                }
              from_label = hfst_strndup(line, tab-line);
              to_label = hfst_strndup(tab+1, endstr-tab-1);
              from_pair = label_to_stringpair(from_label);
              to_pair = label_to_stringpair(to_label);
              if (strlen(from_label) == 0)
                {
                  error_at_line(EXIT_FAILURE, 0, from_file_name, line_n,
                                "First field is empty;\n"
                                "if you REALLY want to replace epsilons with"
                                "something, use @0@ or %s",
                                hfst::internal_epsilon.c_str());
                }
              if (strlen(to_label) == 0)
                {
                  error_at_line(EXIT_FAILURE, 0, from_file_name, line_n,
                                "Second field seems empty;\n"
                                "if you want to substitute something with "
                                "epsilons, use @0@ or %s",
                                hfst::internal_epsilon.c_str());
                }

              if (from_pair && to_pair)
                {
                  if (!in_order) {
                    pair_substitution_map->operator[](*from_pair) = *to_pair;
                    symbol_pair_map_in_use=true;
                  }
                  else {
                    do_substitute(trans, transducer_n);
                  }
                }
              else if (from_label && to_label)
                {
                  if (!in_order) {
                    label_substitution_map->operator[](std::string(from_label)) = std::string(to_label);
                    symbol_map_in_use=true;
                  }
                  else {
                    do_substitute(trans, transducer_n);
                  }
                }
              else {
                try
                  {
                    do_substitute(trans, transducer_n);
                  }
                catch (FunctionNotImplementedException fnse)
                  {
                    if (!warnedAlready)
                      {
                        if (!silent) {
                          warning(0, 0, "substitution is not supported for this transducer type"
                                  " falling back to internal formats and trying...");
                        }
                        fallback = new HfstBasicTransducer(trans);
                        warnedAlready = true;
                      }
                    do_substitute(*fallback, transducer_n);
                    fellback = true;
                  }
                free(from_label);
                free(to_label);
              }
            } // while getline
          free(line);

          // perform label-to-label substitution right away
          if (!in_order && symbol_map_in_use)
            {
              trans.substitute(*label_substitution_map);
              symbol_map_in_use=false;
            }

          // perform symbol pair-to-symbol pair substitution right away
          if (!in_order && symbol_pair_map_in_use)
            {
              trans.substitute(*pair_substitution_map);
              symbol_pair_map_in_use=false;
            }

        }

      // if not from file
      else
        {
          try
            {
              do_substitute(trans, transducer_n);
            }
          catch (FunctionNotImplementedException fnse)
            {
              if (!warnedAlready)
                {
                  if (!silent) {
                    warning(0, 0, "substitution is not supported for this transducer type"
                            " falling back to internal formats and trying...");
                  }
                  fallback = new HfstBasicTransducer(trans);
                }
              do_substitute(*fallback, transducer_n);
              fellback = true;
            }
        }
      if (fellback)
        {
          trans = HfstTransducer(*fallback, trans.get_type());
        }
      else if (delayed)
        {
          perform_delayed(trans);
        }
      if (from_file)
        {
            char* composed_name = static_cast<char*>(malloc(sizeof(char) *
                                         (strlen(from_file_name) +
                                          strlen("substitutions-from-%s"))
                                          + 1));
            if (sprintf(composed_name, "substitute-from-%s",
                        from_file_name) > 0)
              {
                hfst_set_name(trans, trans, composed_name);
                free(composed_name);
              }
            composed_name = static_cast<char*>(malloc(sizeof(char) *
                                         (strlen(from_file_name) +
                                          strlen("♲%s"))
                                          + 1));
            if (sprintf(composed_name, "♲%s",
                        from_file_name) > 0)
              {
                hfst_set_formula(trans, trans, composed_name);
                free(composed_name);
              }
        }
      else if (from_label && to_label)
        {
            char* composed_name = static_cast<char*>(malloc(sizeof(char) *
                                         (strlen(from_label) +
                                          strlen(to_label) +
                                          strlen("substitute-%s-with-%s"))
                                          + 1));
            if (sprintf(composed_name, "substitute-%s-with-%s",
                        from_label, to_label) > 0)
              {
                hfst_set_name(trans, trans, composed_name);
                free(composed_name);
              }
            composed_name = static_cast<char*>(malloc(sizeof(char) *
                                         (strlen(from_label) +
                                          strlen(to_label) +
                                          strlen("%s ♲ %s"))
                                          + 1));
            if (sprintf(composed_name, "%s ♲ %s",
                        from_label, to_label) > 0)
              {
                hfst_set_formula(trans, trans, composed_name);
                free(composed_name);
              }
            
        }
      else if (to_transducer_filename)
        {
          if (from_label == NULL) // make scan-build happy, this should not happen
            throw "Error: from_label has a NULL value.";

            char* composed_name = static_cast<char*>(malloc(sizeof(char) *
                                         (strlen(from_label) +
                                          strlen(to_transducer_filename) +
                                          strlen("substitute-%s-with-net-%s"))
                                         + 1));
            if (sprintf(composed_name, "substitute-%s-with-net-%s",
                        from_label, to_transducer_filename) > 0)
              {
                hfst_set_name(trans, trans, composed_name);
                free(composed_name);
              }
            composed_name = static_cast<char*>(malloc(sizeof(char) *
                                         (strlen(from_label) +
                                          strlen(to_transducer_filename) +
                                          strlen("%s ♲ %s"))
                                         + 1));
            if (sprintf(composed_name, "%s ♲ %s",
                        from_label, to_transducer_filename) > 0)
              {
                hfst_set_formula(trans, trans, composed_name);
                free(composed_name);
              }


        }
      delete fallback;
      // TODO: remove this, this should be taken care in the interface
      //fallback = new HfstBasicTransducer(trans);
      //fallback->prune_alphabet();
      //trans = HfstTransducer(*fallback, trans.get_type());
      //#if HAVE_SFST
      //      if (got_foma) {
      //          trans = trans.convert(hfst::FOMA_TYPE);
      //      }
      //#endif
      outstream << trans;
      //delete fallback;
      free(inputname);
    }
  delete to_transducer;
  return EXIT_SUCCESS;
}


int main( int argc, char **argv )
{
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
  _setmode(1, _O_BINARY);
#endif

  hfst_set_program_name(argv[0], "0.1", "HfstSubstitute");
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

    if (from_file != NULL)
      {
#ifdef DEBUG_SUBSTITUTE
        std::cerr << "from_file != NULL\n" << std::endl;
#endif
        label_substitution_map = new HfstSymbolSubstitutions();
        pair_substitution_map = new HfstSymbolPairSubstitutions();
        // TODO: delete
      }

    // here starts the buffer handling part
    HfstInputStream* instream = NULL;
    try {
      instream = (inputfile != stdin) ?
        new HfstInputStream(inputfilename) : new HfstInputStream();
    } catch(const HfstException e)  {
            error(EXIT_FAILURE, 0, "%s is not a valid transducer file",
          inputfilename);
            return EXIT_FAILURE;
    }

    if ( is_input_stream_in_ol_format(instream, "hfst-substitute"))
      {
        return EXIT_FAILURE;
      }

    process_stream(*instream);
    free(inputfilename);
    free(outfilename);
    return EXIT_SUCCESS;
}

