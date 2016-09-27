//! @file hfst-reweight.cc
//!
//! @brief Transducer reweighting tool
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
#include <cmath>
#include <cfloat>
#include <getopt.h>

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "hfst-tool-metadata.h"
#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"
#include "implementations/HfstBasicTransducer.h"

#include "inc/globals-common.h"
#include "inc/globals-unary.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using hfst::implementations::HfstState;
using hfst::implementations::HfstBasicTransducer;
using hfst::implementations::HfstBasicTransition;


// add tools-specific variables here
static
float
id(float w)
{
  return w;
}
float addition = 0;
float multiplier = 1;
char* funcname = 0;
float (*func)(float) = id;
float upper_bound = FLT_MAX;
float lower_bound = 0;
char* input_symbol = 0;
char* output_symbol = 0;
char* symbol = 0;
bool ends_only = false;
bool arcs_only = false;
char* tsv_file_name = 0;
FILE* tsv_file = 0;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    // Usage line
    fprintf(message_out, "Usage: %s [OPTIONS...] [INFILE]\n"
           "Reweight transducer weights simply\n"
        "\n", program_name);

    print_common_program_options(message_out);
    print_common_unary_program_options(message_out);
    fprintf(message_out, "Reweighting options:\n"
            "  -a, --addition=AVAL        add AVAL to matching weights\n"
            "  -b, --multiplier=BVAL      multiply matching weights by BVAL\n"
            "  -F, --function=FNAME       operate matching weights by FNAME\n"
            "  -l, --lower-bound=LVAL     match weights greater than LVAL\n"
            "  -u, --upper-bound=UVAL     match weights less than UVAL\n"
            "  -I, --input-symbol=ISYM    match arcs with input symbol ISYM\n"
            "  -O, --output-symbol=OSYM   match arcs with output symbol OSYM\n"
            "  -S, --symbol=SYM           match arcs with input or output symbol SYM or both\n"
            "  -e, --end-states-only      match end states only, no arcs\n"
            "  -A, --arcs-only            match arcs only, no end states\n"
            "  -T, --tsv-file=TFILE       read reweighting rules from TFILE\n"
            "\n");
    fprintf(message_out, "\n");
    print_common_unary_program_parameter_instructions(message_out);
    fprintf(message_out, "If AVAL, BVAL or FNAME are omitted, they default to neutral "
            "elements of addition, multiplication or identity function.\n"
            "If LVAL or UVAL are omitted, they default to minimum and maximum "
            "values of the weight structure.\n"
            "If ISYM, OSYM or SYM are omitted, they default to a value that "
            "matches all arcs.\nOnly one ISYM, OSYM and SYM can be given.\n\n"
            "Float values are parsed with strtod(3) and integers strtoul(3).\n"
            "The functions allowed for FNAME are <cmath> float functions with "
            "parameter count of 1 and a matching return value:\n"
            "abs, acos, asin, ... sqrt, tan, tanh\n\n"
            "The precedence of operands follows the formula "
            "BVAL * FNAME(w) + AVAL.\n"
            "The formula is applied iff:\n"
            "((LVAL <= w) && (w <= UVAL)),\n"
            "where w is weight of arc, and \n"
            "(ISYM == i) && (OSYM == o) && ((SYM == i) || (SYM == o)) ^^ \n"
            "(end state && -e).\n\n"
            "TFILE should contain lines with tab-separated pairs of SYM and "
            "AVAL or BVAL. AVAL values must be preceded by a + character, "
            "BVAL should be given as plain digits. "
            "Comment lines starting with # and empty lines are ignored.\n\n"
            "Weights are by default modified for all arcs and end states,\n"
            "unless option --end-states-only or --arcs-only is used.\n");
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
            {"addition", required_argument, 0, 'a'},
            {"multiplier", required_argument, 0, 'b'},
            {"function", required_argument, 0, 'F'},
            {"lower-bound", required_argument, 0, 'l'},
            {"upper-bound", required_argument, 0, 'u'},
            {"input-symbol", required_argument, 0, 'I'},
            {"output-symbol", required_argument, 0, 'O'},
            {"symbol", required_argument, 0, 'S'},
            {"end-states-only", no_argument, 0, 'e'},
            {"arcs-only", no_argument, 0, 'A'},
            {"tsv", required_argument, 0, 'T'},
            {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT "a:b:F:l:u:I:O:S:eT:A",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
#include "inc/getopt-cases-unary.h"
        case 'a':
          addition = hfst_strtoweight(optarg);
          break;
        case 'b':
          multiplier = hfst_strtoweight(optarg);
          break;
        case 'F':
          funcname = hfst_strdup(optarg);
          if (strcmp(optarg, "cos") == 0)
            {
              func = &cosf;
            }
          else if (strcmp(optarg, "sin") == 0)
            {
              func = &sinf;
            }
          else if (strcmp(optarg, "tan") == 0)
            {
              func = &tanf;
            }
          else if (strcmp(optarg, "acos") == 0)
            {
              func = &acosf;
            }
          else if (strcmp(optarg, "asin") == 0)
            {
              func = &asinf;
            }
          else if (strcmp(optarg, "atan") == 0)
            {
              func = &atanf;
            }
          else if (strcmp(optarg, "cosh") == 0)
            {
              func = &coshf;
            }
          else if (strcmp(optarg, "sinh") == 0)
            {
              func = &sinhf;
            }
          else if (strcmp(optarg, "tanh") == 0)
            {
              func = &tanhf;
            }
          else if (strcmp(optarg, "exp") == 0)
            {
              func = &expf;
            }
          else if (strcmp(optarg, "log") == 0)
            {
              func = &logf;
            }
          else if (strcmp(optarg, "log10") == 0)
            {
              func = &log10f;
            }
          else if (strcmp(optarg, "sqrt") == 0)
            {
              func = &sqrtf;
            }
          else if (strcmp(optarg, "floor") == 0)
            {
              func = &floorf;
            }
          else if (strcmp(optarg, "ceil") == 0)
            {
              func = &ceilf;
            }
          else
            {
              error(EXIT_FAILURE, 0, "Cannot parse %s as function name",
                    optarg);
              return EXIT_FAILURE;
            }
          break;
        case 'l':
          lower_bound = hfst_strtoweight(optarg);
          break;
        case 'u':
          upper_bound = hfst_strtoweight(optarg);
          break;
        case 'I':
          input_symbol = hfst_strdup(optarg);
          break;
        case 'O':
          output_symbol = hfst_strdup(optarg);
          break;
        case 'S':
          symbol = hfst_strdup(optarg);
          break;
        case 'e':
          ends_only = true;
          break;
        case 'A':
          arcs_only = true;
          break;
        case 'T':
          tsv_file_name = hfst_strdup(optarg);
          break;
#include "inc/getopt-cases-error.h"
        }
    }

    if (arcs_only && ends_only)
      {
        error(EXIT_FAILURE, 0, "Options '--arcs-only' and '--end-states-only' cannot be used at the same time");
        return EXIT_FAILURE;
      }

#include "inc/check-params-common.h"
#include "inc/check-params-unary.h"
    if (funcname == 0)
      {
        funcname = hfst_strdup("id");
      }
    if (upper_bound < lower_bound)
      {
        warning(0, 0, "Lower bound %f exceeds upper bound %f so reweight will"
                " never apply", lower_bound, upper_bound);
      }
    if (tsv_file_name != 0)
      {
        tsv_file = hfst_fopen(tsv_file_name, "r");
      }
    return EXIT_CONTINUE;
}

static
float
reweight(float w, const char* i, const char* o)
{
  if ((w < lower_bound) || (w > upper_bound))
    {
      // not within weight bounds, don't apply
      return w;
    }
  if ((i == 0) && (o == 0))
    {
      if (arcs_only)
        {
          return w;
        }
    }
  else if ((i != 0) && (o != 0))
    {
      if (ends_only)
        {
          return w;
        }
      if ((symbol != 0) && ((strcmp(i, symbol) != 0) &&
                            (strcmp(o, symbol) != 0) ) )
        {
          // symbol doesn't match, don't apply
          return w;
        }
      if ((input_symbol != 0) && (output_symbol != 0) &&
          (strcmp(i, input_symbol) != 0) && (strcmp(o, output_symbol) != 0))
        {
          // input doesn't match, don't apply
          return w;
        }
      else if ((input_symbol != 0) && (strcmp(i, input_symbol) != 0))
        {
          // input doesn't match, don't apply
          return w;
        }
      else if ((output_symbol != 0) && (strcmp(o, output_symbol) != 0))
        {
          // output doesn't match, don't apply
          return w;
        }
    }
  return multiplier * (*func)(w) + addition;

}

static
HfstTransducer&
do_reweight(HfstTransducer& trans)
  {
        HfstBasicTransducer original(trans);
        HfstBasicTransducer replication;
        HfstState state_count = 1;
        std::map<HfstState,HfstState> rebuilt;
        rebuilt[0] = 0;
        HfstState source_state=0;
        for (HfstBasicTransducer::const_iterator state = original.begin();
             state != original.end();
             ++state)
          {
            if (rebuilt.find(source_state) == rebuilt.end())
              {
                replication.add_state(state_count);
                if (original.is_final_state(source_state))
                  {
                    replication.set_final_weight(state_count,
                                                 reweight(original.get_final_weight(source_state), 0, 0));
                  }
                rebuilt[source_state] = state_count;
                state_count++;
              }
            for (HfstBasicTransducer::HfstTransitions::const_iterator arc =
                 state->begin();
                 arc != state->end();
                 ++arc)
              {
                if (rebuilt.find(arc->get_target_state()) == rebuilt.end())
                  {
                    replication.add_state(state_count);
                    if (original.is_final_state(arc->get_target_state()))
                      {
                        replication.set_final_weight(state_count,
                                                     reweight(original.get_final_weight(arc->get_target_state()), 0, 0));
                      }
                    rebuilt[arc->get_target_state()] = state_count;
                    state_count++;
                  }
                HfstBasicTransition nu(rebuilt[arc->get_target_state()],
                                       arc->get_input_symbol(),
                                       arc->get_output_symbol(),
                                       reweight(arc->get_weight(),
                                                arc->get_input_symbol().c_str(),
                                                arc->get_output_symbol().c_str()));
                replication.add_transition(rebuilt[source_state], nu);
              }
            source_state++;
          }
        trans = HfstTransducer(replication, trans.get_type());
        return trans;
  }


int
process_stream(HfstInputStream& instream, HfstOutputStream& outstream)
  {
    size_t transducer_n=0;
    while(instream.is_good())
    {
        transducer_n++;
        HfstTransducer trans(instream);
        char* inputname = hfst_get_name(trans, inputfilename);
        if (transducer_n==1)
        {
          verbose_printf("Reweighting %s...\n", inputname);
        }
        else
        {
          verbose_printf("Reweighting %s..." SIZE_T_SPECIFIER "\n", inputname, transducer_n);
        }
        if (NULL == tsv_file)
          {
            trans = do_reweight(trans);
            hfst_set_name(trans, trans, "reweight");
            hfst_set_formula(trans, trans, "W");
          }
        else
          {
            rewind(tsv_file);
            free(symbol);
            addition = 0;
            multiplier = 1;
            char* line = NULL;
            size_t len = 0;
            size_t linen = 0;
            verbose_printf("Reading reweights from %s\n", tsv_file_name);
            while (hfst_getline(&line, &len, tsv_file) != -1)
              {
                linen++;
                if (*line == '\n')
                  {
                    continue;
                  }
                if (*line == '#')
                  {
                    continue;
                  }
                const char* tab = strstr(line, "\t");
                if (NULL == tab)
                  {
                    if (*line == '#')
                      {
                        continue;
                      }
                    else
                      {
                        error_at_line(EXIT_FAILURE, 0, tsv_file_name, linen,
                                      "at least one tab required per line");
                      }
                  }
                const char* endstr = tab + 1;
                while ((*endstr != '\0') && (*endstr != '\n'))
                  {
                    endstr++;
                  }
                symbol = hfst_strndup(line, tab - line);
                char* weightspec = hfst_strndup(tab + 1, endstr - tab - 1);
                if (*weightspec == '+')
                  {
                    addition = hfst_strtoweight(weightspec + 1);
                  }
                else
                  {
                    multiplier = hfst_strtoweight(weightspec);
                  }
                free(weightspec);
                verbose_printf("Modifying weights %f < w < %f as %f * %s(w) + %f for symbol %s\n",
                   lower_bound, upper_bound, multiplier, funcname, addition, symbol);
                trans = do_reweight(trans);
              } // getline
              free(line);
            hfst_set_name(trans, trans, "reweight");
            hfst_set_formula(trans, trans, "W");
          } // if tsv_file
        outstream << trans.remove_epsilons();
        free(inputname);
      } // foreach transducer
    instream.close();
    outstream.close();
    return EXIT_SUCCESS;
  }



int main( int argc, char **argv ) {
#ifdef WINDOWS
  _setmode(0, _O_BINARY);
  _setmode(1, _O_BINARY);
#endif

    hfst_set_program_name(argv[0], "0.1",
                          "HfstReweight");
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
    verbose_printf("Modifying weights %f < w < %f as %f * %s(w) + %f\n",
                   lower_bound, upper_bound, multiplier, funcname, addition);
    if (symbol)
      {
        verbose_printf("only if arc has symbol %s\n", symbol);
      }
    if (input_symbol)
      {
        verbose_printf("only if input symbol is %s\n", input_symbol);
      }
    if (output_symbol)
      {
        verbose_printf("only if output symbol is %s\n", output_symbol);
      }
    if (ends_only)
      {
        verbose_printf("only on final weights, no arcs\n");
      }
    if (arcs_only)
      {
        verbose_printf("only on arc weights, no end states\n");
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
    HfstOutputStream* outstream = (outfile != stdout) ?
        new HfstOutputStream(outfilename, instream->get_type()) :
        new HfstOutputStream(instream->get_type());
    
    if ( is_input_stream_in_ol_format(instream, "hfst-reweight"))
      {
        return EXIT_FAILURE;
      }

    retval = process_stream(*instream, *outstream);
    delete instream;
    delete outstream;
    free(inputfilename);
    free(outfilename);
    return retval;
}

