//! @file hfst-grep.cc
//!
//! @brief Hfst-based grep clone cool.
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
#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::pair;
using std::map;

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <stdint.h>

#include "hfst-commandline.h"
#include "hfst-program-options.h"
#include "HfstTransducer.h"
#include "HfstTokenizer.h"
#include "parsers/XreCompiler.h"

#include "inc/globals-common.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOutputStream;
using hfst::HfstTokenizer;
using hfst::HfstOneLevelPath;
using hfst::HfstOneLevelPaths;
using hfst::HfstTwoLevelPaths;
using hfst::xre::XreCompiler;
using hfst::is_epsilon;

// add tools-specific variables here
char** infilenames = 0;
FILE** infiles = 0;
unsigned int infile_n = 0;
char* inputfilename = 0;
unsigned long linen = 0;
char* regexp = 0;
FILE* expfile = 0;
char* expfilename = 0;
bool dialect_xerox = false;
bool dialect_posix_bre = false;
bool dialect_posix_ere = false;
bool dialect_perl = false;
bool dialect_fixed_strings = false;
bool match_word = false;
bool match_full_line = false;
char linesep = '\n';
bool very_quiet = false;
bool invert_matches = false;
unsigned long max_count = -1;
bool max_infinite = true;
bool print_offset = false;
bool print_linenumbers = false;
bool flush_newlines = false;
bool print_filenames = false;
bool print_only_matches = false;
bool print_only_matching_filenames = false;
bool print_only_unmatching_filenames = false;
bool print_only_count = false;
bool count_matches = false;
bool print_filename_null = false;
bool color_matches = false;
unsigned long before_context = 0;
unsigned long after_context = 0;
unsigned long matches = 0;
HfstTransducer* matcher = 0;
HfstTransducer* optimised_matcher = 0;

hfst::ImplementationType format = hfst::UNSPECIFIED_TYPE;

void
print_usage()
{
    // c.f. http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
    // Usage line
    fprintf(message_out, "Usage: %s [OPTIONS...] PATTERN [FILE...]\n"
           "Search for PATTERN in each FILE or standard input.\n"
           "Pattern is, by default, a Xerox regular expression (XRE).\n"
           "Example: hfst-grep 'h e l l o %%  w o r l d' menu.h menu.c\n"
        "\n", program_name);

    // options, grouped
    print_common_program_options(message_out);
    fprintf(message_out,
            "  -9, --format=TYPE       compile expressions to TYPE "
            "automata\n");
    fprintf(message_out, "\n");
    fprintf(message_out, "Regexp selection and interpretation:\n"
            "  -E, --extended-regexp     PATTERN is an extended regular "
            "expression (ERE)\n"
            "  -F, --fixed-strings       PATTERN is a set of newline-separated "
            "fixed strings\n"
            "  -G, --basic-regexp        PATTERN is a basic regular "
            "expression (BRE)\n"
            "  -P, --perl-regexp         PATTERN is a Perl regular expression\n"
            "  -X, --xerox-regexp        PATTERN is a Xerox regulare "
            "expression\n"
            "  -e, --regexp=PATTERN      use PATTERN for matching\n"
            "  -f, --file=FILE           obtain PATTERN from FILE\n"
            "  -I, --ignore-case         ignore case distinctions\n"
            "  -w, --word-regexp         force PATTERN to match only whole "
            "words\n"
            "  -x, --line-regexp         force PATTERN to match only whole "
            "lines\n"
            "  -z, --null-data           a data line ends in 0 byte, not "
            "newline\n");
    fprintf(message_out, "Miscellaneous options:\n"
            "      --no-messages         suppress error messages\n"
            "      --invert-match        select non-matching lines\n"
            "\n");
    fprintf(message_out, "Output control:\n"
            "  -m, --max-count=NUM       stop after NUM matches\\n"
            "  -b, --byte-offset         print the byte offset with output "
            "lines\n"
            "  -n, --line-number         print line number with output lines\n"
            "      --line-buffered       flush output on every line\n"
            "  -H, --with-filename       print the filename for each match\n"
            "  -h, --no-filename         suppress the prefixing filename on "
            "output\n"
            "      --label=LABEL         print LABEL as filename for standard "
            "input\n"
            "  -o, --only-matching       show only the part of a line matching "
            "PATTERN\n"
            "      --binary-files=TYPE   assume that binary files are TYPE;\n"
            "                            TYPE is `binary', `text', or "
            "`without-match'\n"
            "  -a, --text                equivalent to --binary-files=text\n"
            "  -d, --directories=ACTION  how to handle directories;\n"
            "                            ACTION is `read', `recurse', or "
            "`skip'\n"
            "  -D, --devices=ACTION      how to handle devices, FIFOs and "
            "sockets;\n"
            "                            ACTION is `read' or `skip'\n"
            "  -R, -r, --recursive       equivalent to --directories=recurse\n"
            "      --include=FILE_PATTERN  search only files that match "
            "FILE_PATTERN\n"
            "      --exclude=FILE_PATTERN  skip files and directories matching "
            "FILE_PATTERN\n"
            "      --exclude-from=FILE   skip files matching any file pattern "
            "from FILE\n"
            "      --exclude-dir=PATTERN  directories that match PATTERN will "
            "be skipped\n"
            "  -L, --files-without-match  print only names of FILEs containing "
            " no match\n"
            "  -l, --files-with-matches  print only names of FILEs containing "
            "matches\n"
            "  -c, --count               print only a count of matching lines "
            "per FILE\n"
            "  -T, --initial-tab         make tabs line up (if needed)\n"
            "  -Z, --null                print 0 byte after FILE name\n"
            "\n");
    fprintf(message_out, "Context control:\n"
            "  -B, --before-context=NUM  print NUM lines of leading context\n"
            "  -A, --after-context=NUM   print NUM lines of trailing context\n"
            "  -C, --context=NUM         print NUM lines of output context\n"
            "      --color[=WHEN],\n"
            "      --colour[=WHEN]       use markers to highlight the matching "
            "strings;\n"
            "                            WHEN is `always', `never', or `auto'\n"
            "  -U, --binary              do not strip CR characters at EOL "
            "(MSDOS)\n"
            "  -u, --unix-byte-offsets   report offsets as if CRs were not "
            "there (MSDOS)\n"
            "\n");


    // parameter details
    fprintf(message_out, "\n");
    // bug report address
    print_report_bugs();
    // external docs
    print_more_info();
}


int
parse_options(int argc, char** argv)
{
    // use of this function requires options are settable on global scope
    static const short INVERT_OPT = 19;
    static const short LINEBUFFER_OPT =20;
    static const short LABEL_OPT = 21;
    static const short BINARYFILES_OPT = 22;
    static const short INCLUDE_OPT = 23;
    static const short EXCLUDE_OPT = 24;
    static const short INCLUDEFROM_OPT = 25;
    static const short EXCLUDEFROM_OPT = 26;
    static const short COLOR_OPT = 27;
    while (true)
    {
        static const struct option long_options[] =
        {
          HFST_GETOPT_COMMON_LONG,
          HFST_GETOPT_UNARY_LONG,
          // add tool-specific options here
          {"format", required_argument, 0, '9'},
          {"extended-regexp", no_argument, 0, 'E'},
          {"fixed-strings", no_argument, 0, 'F'},
          {"basic-regexp", no_argument, 0, 'G'},
          {"perl-regexp", no_argument, 0, 'P'},
          {"xerox-regexp", no_argument, 0, 'X'},
          {"regexp", required_argument, 0, 'e'},
          {"file", required_argument, 0, 'f'},
          {"ignore-case", no_argument, 0, 'I'},
          {"word-regexp", no_argument, 0, 'w'},
          {"line-regexp", no_argument, 0, 'x'},
          {"null-data", no_argument, 0, 'z'},
          {"no-messages", no_argument, 0, 'q'},
          {"invert-match", no_argument, 0, INVERT_OPT},
          {"max-count", required_argument, 0, 'm'},
          {"byte-offset", no_argument, 0, 'b'},
          {"line-number", no_argument, 0, 'n'},
          {"line-buffered", no_argument, 0, LINEBUFFER_OPT},
          {"with-filename", no_argument, 0, 'H'},
          {"label", required_argument, 0, LABEL_OPT},
          {"only-matching", no_argument, 0, 'O'},
          {"binary-files", required_argument, 0, BINARYFILES_OPT},
          {"text", no_argument, 0, 'a'},
          {"directories", required_argument, 0, 'd'},
          {"devices", required_argument, 0, 'D'},
          {"recursive", no_argument, 0, 'r'},
          {"include", required_argument, 0, INCLUDE_OPT},
          {"exclude", required_argument, 0, EXCLUDE_OPT},
          {"include-from", required_argument, 0, INCLUDEFROM_OPT},
          {"exclude-from", required_argument, 0, EXCLUDEFROM_OPT},
          {"files-without-match", no_argument, 0, 'L'},
          {"files-with-match", no_argument, 0, 'l'},
          {"count", no_argument, 0, 'c'},
          {"null", no_argument, 0, 'Z'},
          {"before-context", required_argument, 0, 'A'},
          {"after-context", required_argument, 0, 'B'},
          {"context", required_argument, 0, 'C'},
          {"colour", no_argument, 0, COLOR_OPT},
          {"color", no_argument, 0, COLOR_OPT},
          {"binary", no_argument, 0, 'u'},
          {"unix-byte-offset", no_argument, 0, 'U'},
          {0,0,0,0}
        };
        int option_index = 0;
        // add tool-specific options here
        int c = getopt_long(argc, argv, HFST_GETOPT_COMMON_SHORT
                             HFST_GETOPT_UNARY_SHORT
                             "EFGPXe:f:IwxzqmbnOad:D:rLlcZA:B:C:uU9:",
                             long_options, &option_index);
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
#include "inc/getopt-cases-common.h"
        case '9':
          format = hfst_parse_format_name(optarg);
          break;
        case 'E':
          error(EXIT_FAILURE, 0, "POSIX ERE syntax not yet "
                     "supported");
          dialect_posix_ere = true;
          break;
        case 'F':
          dialect_fixed_strings = true;
          break;
        case 'G':
          error(EXIT_FAILURE, 0, "POSIX BRE syntax not yet supported");
          dialect_posix_bre = true;
          break;
        case 'P':
          error(EXIT_FAILURE, 0, "Perl syntax not yet supported");
          dialect_perl = true;
          break;
        case 'X':
          dialect_xerox = true;
          break;
        case 'e':
          regexp = hfst_strdup(optarg);
          break;
        case 'f':
          expfile = hfst_fopen(optarg, "r");
          break;
        case 'I':
          error(EXIT_FAILURE, 0, "Ignore case not supported");
          break;
        case 'w':
          match_word = true;
          break;
        case 'x':
          match_full_line = true;
          break;
        case 'z':
          linesep = 0;
          break;
        case INVERT_OPT:
          invert_matches = true;
          break;
        case 'm':
          max_count = hfst_strtoul(optarg, 10);
          count_matches = true;
          break;
        case 'b':
          print_offset = true;
          break;
        case 'n':
          print_linenumbers = true;
          break;
        case LINEBUFFER_OPT:
          flush_newlines = true;
          break;
        case 'H':
          print_filenames = true;
          break;
        case 'O':
          print_only_matches = true;
          break;
        case BINARYFILES_OPT:
          error(EXIT_FAILURE, 0, "No binary handling implemented");
          break;
        case 'a':
          warning(0, 0, "All files are always handled as text");
          break;
        case 'D':
          error(EXIT_FAILURE, 0, "No directory handling implemented");
          break;
        case 'r':
          error(EXIT_FAILURE, 0, "No directory handling implemented");
          break;
        case INCLUDE_OPT:
        case EXCLUDE_OPT:
        case INCLUDEFROM_OPT:
        case EXCLUDEFROM_OPT:
          error(EXIT_FAILURE, 0, "No directory/globbing implemented");
          break;
        case 'L':
          print_only_unmatching_filenames = true;
          break;
        case 'l':
          print_only_matching_filenames = true;
          break;
        case 'c':
          count_matches = true;
          print_only_count = true;
          break;
        case 'Z':
          print_filename_null = true;
          break;
        case 'A':
          before_context = hfst_strtoul(optarg, 10);
          break;
        case 'B':
          after_context = hfst_strtoul(optarg, 10);
          break;
        case 'C':
          before_context = hfst_strtoul(optarg, 10);
          after_context = hfst_strtoul(optarg, 10);
          break;
        case COLOR_OPT:
          color_matches = true;
          break;
        case 'u':
        case 'U':
          error(EXIT_FAILURE, 0, "MSDOS binary format not supported; use "
                "fromdos or dos2unix");
          break;

#include "inc/getopt-cases-error.h"
        }
    }
    if (!dialect_fixed_strings && !dialect_xerox && !dialect_posix_bre &&
        !dialect_posix_ere && !dialect_perl)
      {
        warning(0, 0, "Dialect not defined, defaulting to Xerox for now!");
        dialect_xerox = true;
      }
    if (format == hfst::UNSPECIFIED_TYPE)
      {
        format = hfst::TROPICAL_OPENFST_TYPE;
      }
    if ((regexp == 0) && (expfile == 0))
      {
        if ((argc - optind) <= 0)
          {
            print_usage();
            print_short_help();
            return EXIT_FAILURE;
          }
        else
          {
            regexp = strdup(argv[optind]);
            optind++;
          }
      }
    if ((argc - optind) == 0)
      {
        infilenames = static_cast<char**>(malloc(sizeof(char*) * 1));
        infiles = static_cast<FILE**>(malloc(sizeof(FILE*) * 1));
        infile_n = 1;
        infilenames[0] = strdup("<stdin>");
        infiles[0] = stdin;
      }
    else
      {
        infilenames = static_cast<char**>(malloc(sizeof(char*) * (argc - optind)));
        infiles = static_cast<FILE**>(malloc(sizeof(FILE*) * (argc - optind)));
        infile_n = (argc - optind);
        for (int i = 0; i < (argc - optind); i++)
          {
            infilenames[i] = strdup(argv[optind+i]);
            infiles[i] = hfst_fopen(infilenames[i], "r");
          }
      }
#include "inc/check-params-common.h"
    return EXIT_CONTINUE;
}

vector<string>*
string_to_utf8(char* p)
{
  vector<string>* path = new vector<string>;
    while ((p != 0) && (*p != '\0'))
      {
        unsigned char c = static_cast<unsigned char>(*p);
        unsigned short u8len = 1;
        if (c <= 127)
          {
            u8len = 1;
          }
        else if ( (c & (128 + 64 + 32 + 16)) == (128 + 64 + 32 + 16) )
          {
            u8len = 4;
          }
        else if ( (c & (128 + 64 + 32 )) == (128 + 64 + 32) )
          {
            u8len = 3;
          }
        else if ( (c & (128 + 64 )) == (128 + 64))
          {
            u8len = 2;
          }
        else
          {
            error_at_line(EXIT_FAILURE, 0, inputfilename, linen,
                          "%s not valid UTF-8\n", p);
          }
        char* nextu8 = hfst_strndup(p, u8len);
        path->push_back(nextu8);
        p += u8len;
        free(nextu8);
      }
    return path;
}



int
read_matcher(HfstInputStream& instream)
{
    
    size_t transducer_n=0;
    matcher = new HfstTransducer(instream.get_type());
    while (instream.is_good())
    {
        transducer_n++;
        HfstTransducer trans(instream);
        char* inputname = strdup(trans.get_name().c_str());
        if (strlen(inputname) <= 0)
          {
            inputname = strdup(inputfilename);
          }
        if (transducer_n==1)
        {
          verbose_printf("Reading matcher %s...\n", inputname);
        }
        else
        {
          verbose_printf("Reading matcher %s..." SIZE_T_SPECIFIER "\n", inputname,
                         transducer_n);
        }
        if (transducer_n > 1)
          {
            verbose_printf("and disjuncting...\n");
          }
        matcher->disjunct(trans.input_project());
    }
    verbose_printf("minimising matchers...\n");
    matcher->minimize();
    instream.close();
    return EXIT_SUCCESS;
}

void
read_matcher(const char* expression)
  {
    matcher = new HfstTransducer(format);
    if (dialect_xerox)
      {
        XreCompiler comp(format);
        verbose_printf("parsing %s as Xerox style regular expression...\n",
                       expression);
        HfstTransducer* trans = comp.compile(expression);
        matcher->disjunct(trans->input_project());
      }
    else if (dialect_fixed_strings)
      {
        verbose_printf("parsing %s as fixed string of UTF-8 symbols...\n",
                       expression);
        HfstTokenizer t;
        HfstTransducer trans(expression, expression, t, format);
        matcher->disjunct(trans);
      }
    else
      {
        error(EXIT_FAILURE, 0, "dialect unsupported");
      }
    verbose_printf("minimizing...\n");
    matcher->minimize();
    if (verbose)
      {
        verbose_printf("Resulting FSM:\n");
        std::cerr << *matcher;
      }
  }

void
extend_matcher_with_options()
  {
    if (color_matches)
      {
        verbose_printf("Adding color codes to match boundaries...\n");
        HfstTransducer colorStart("@_EPSILON_SYMBOL_@", "[31m", format);
        HfstTransducer colorEnd("@_EPSILON_SYMBOL_@", "[00m", format);
        HfstTransducer coloured =
            colorStart.concatenate(*matcher).concatenate(colorEnd);
        matcher = new HfstTransducer(coloured, format);
      }
    else
      {
        // bracket matches for now
        verbose_printf("Adding brackets to match boundaries...\n");
        HfstTransducer colorStart("@_EPSILON_SYMBOL_@", "{{{", format);
        HfstTransducer colorEnd("@_EPSILON_SYMBOL_@", "}}}", format);
        HfstTransducer coloured =
            colorStart.concatenate(*matcher).concatenate(colorEnd);
        matcher = new HfstTransducer(coloured, format);
      }
    if (match_word)
      {
        verbose_printf("Delimiting matcher to word boundaries "
                      "(currently space)...\n");
        HfstTransducer nonWordCharLeft(" ", format);
        HfstTransducer nonWordCharRight(" ", format);
        HfstTransducer wordBounded =
            nonWordCharLeft.concatenate(*matcher).concatenate(nonWordCharRight);
        matcher = new HfstTransducer(wordBounded, format);
      }
    if (!match_full_line)
      {
        verbose_printf("Extending matcher for repetitions and rest...\n");
        HfstTransducer leftAny("@_IDENTITY_SYMBOL_@", format);
        HfstTransducer rightAny("@_IDENTITY_SYMBOL_@", format);
        leftAny.repeat_star();
        rightAny.repeat_star();
        HfstTransducer oneMatch =
            leftAny.concatenate(*matcher).concatenate(rightAny);
        matcher = new HfstTransducer(oneMatch, format);
        matcher->repeat_plus();
      }
    verbose_printf("Minimising extended matcher...\n");
    matcher->minimize();
    if (verbose)
      {
        verbose_printf("Resulting FSM:\n");
        std::cerr << *matcher;
      }
  }

void
print_match_line(const HfstOneLevelPath& path)
{
  if (print_only_matching_filenames || print_only_unmatching_filenames)
    {
      return;
    }
  if (print_filenames)
    {
      fprintf(outfile, "%s", inputfilename);
      if (print_filename_null)
        {
          uint8_t zero = 0;
          fwrite(&zero, 1, 1, outfile);
        }
      else
        {
          fprintf(outfile, ": ");
        }
    }
  if (print_linenumbers)
    {
      fprintf(outfile, "%lu: ", linen);
    }
  for (const auto & s : path.second)
    {
      fprintf(outfile, "%s", s.c_str());
    }
  fprintf(outfile, "\n");
}

void
print_match_transducer(const HfstTransducer& path)
{
  HfstTwoLevelPaths p;
  path.extract_paths(p, 1);
  if (print_only_matching_filenames || print_only_unmatching_filenames)
    {
      return;
    }
  if (print_filenames)
    {
      fprintf(outfile, "%s", inputfilename);
      if (print_filename_null)
        {
          uint8_t zero = 0;
          fwrite(&zero, 1, 1, outfile);
        }
      else
        {
          fprintf(outfile, ": ");
        }
    }
  if (print_linenumbers)
    {
      fprintf(outfile, "%lu: ", linen);
    }
  for (const auto & s : p.begin()->second)
    {
      if (!is_epsilon(s.first))
        {
          fprintf(outfile, "%s", s.first.c_str());
        }
    }
  fprintf(outfile, "\n");

}

/**
 * @return true if matches in @a infile
 */
bool
match_lines(FILE* infile, char* infilename)
  {
    char* line = 0;
    size_t len = 0;
    verbose_printf("matching against %s...\n", infilename);
    bool matched = false;
    size_t matches_n = 0;
#ifndef HFST_OPTIMISED_LOOKUP_CAN_IDENTITY
    HfstTokenizer tokeniser;
#endif
    while (hfst_getline(&line, &len, infile) != -1)
      {
        linen++;
        char* p = line;
        while (*p != '\0')
          {
            if (*p == '\n')
              {
                *p = '\0';
                break;
              }
            p++;
          }
        verbose_printf("matching %s...\n", line);
#if HFST_OPTIMISED_LOOKUP_CAN_IDENTITY
        HfstOneLevelPath path;
        vector<string>* letters = string_to_utf8(line);
        path.second = *letters;
        path.first = 0;
        HfstOneLevelPaths* results = optimised_matcher->lookup(*letters);
        if (results->size() == 0)
          {
            verbose_printf("no matches\n");
            if (invert_matches)
              {
                print_match_line(path);
              }
          }
        else if (results->size() >= 1)
          {
            verbose_printf("" SIZE_T_SPECIFIER " match combinations\n", results->size());
            if (!invert_matches)
              {
                // only print one of the matches for now
                HfstOneLevelPath match = *(results->begin());
                print_match_line(match);
              }
            matched = true;
            matches_n++;
          }
#else
        if (*line == '\0')
          {
            continue;
          }
        HfstTransducer lineTrans(line, line, tokeniser, format);
        verbose_printf("composing...\n");
        HfstTransducer results = lineTrans.compose(*matcher).output_project();
        HfstTransducer empty(format);
        if (results.compare(empty))
          {
            verbose_printf("no matches\n");
            if (invert_matches)
              {
                print_match_transducer(lineTrans);
              }
          }
        else
          {
            verbose_printf("matches\n");
            if (!invert_matches)
              {
                print_match_transducer(results);
              }
            matched = true;
            matches_n++;
          }
#endif
        if (flush_newlines)
          {
            fflush(outfile);
          }
        if ((max_count > 0) && (matches_n >= max_count))
          {
            break;
          }
      }
    return invert_matches? !matched: matched;
  }

void
optimise_matcher()
  {
    verbose_printf("Optimising...\n");
    optimised_matcher = new HfstTransducer(*matcher, hfst::HFST_OL_TYPE);
  }

int main( int argc, char **argv ) {
    hfst_setlocale();
    hfst_set_program_name(argv[0], "0.1", "HfstGrep");
    int retval = parse_options(argc, argv);
    if (retval != EXIT_CONTINUE)
    {
        return retval;
    }
    verbose_printf("Writing to %s\n", outfilename);
    read_matcher(regexp);
    extend_matcher_with_options();
#if HFST_OPTIMISED_LOOKUP_CAN_IDENTITY_SYMBOL
    optimise_matcher();
#endif
    for (unsigned int i = 0; i < infile_n; i++)
      {
        inputfilename = infilenames[i];
        linen = 0;
        match_lines(infiles[i], infilenames[i]);
      }

    free(outfilename);
    return retval;
}

