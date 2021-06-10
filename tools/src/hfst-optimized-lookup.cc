/*

  Copyright 2009 University of Helsinki

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

*/

/*
  NOTE:
  THIS SINGLE-FILE VERSION WAS PUT TOGETHER FROM A MULTI-FILE VERSION
  SO THE CURRENT STRUCTURE IS NOT SO GREAT. TODO: FIX THIS.
  TODO: USE THE EXISTING HFST-TOOLS FRAMEWORK BETTER.
 */

#include "hfst-optimized-lookup.h"
#include "inc/globals-common.h"  // some cc files include extern declarations for these variables...

//#ifdef _MSC_VER
//#  include <windows.h>
//#endif

#ifdef _MSC_VER
#  include "hfst-string-conversions.h"
using hfst::hfst_fprintf_console;
#endif

#include <cstdarg>
#include <iostream> // DEBUG

static float beam=-1;
static bool pipe_input = false;
static bool pipe_output = false;

bool print_usage(void)
{
  std::cout <<
    "\n" <<
    "Usage: " << PACKAGE_NAME << " [OPTIONS] TRANSDUCER\n" <<
    "Run a transducer on standard input (one word per line) and print analyses\n" <<
    "NOTE: hfst-optimized-lookup does lookup from left to right as opposed to xfst\n" <<
    "      and foma lookup which is carried out from right to left. In order to do\n" <<
    "      lookup in a similar way as xfst and foma, invert the transducer first.\n" <<
    "\n" <<
    "  -h, --help                  Print this help message\n" <<
    "  -V, --version               Print version information\n" <<
    "  -v, --verbose               Be verbose\n" <<
    "  -q, --quiet                 Don't be verbose (default)\n" <<
    "  -s, --silent                Same as quiet\n" <<
    "  -e, --echo                  Echo inputs\n" <<
    "                              (useful if redirecting lots of output to a file)\n" <<
    "  -w, --show-weights          Print final analysis weights (if any)\n" <<
    "  -u, --unique                Suppress duplicate analyses\n" <<
    "  -n N, --analyses=N          Output no more than N analyses\n" <<
    "                              (if the transducer is weighted, the N best analyses)\n" <<
    "  -b, --beam=B                Output only analyses whose weight is within B from\n" <<
    "                              the best analysis\n" <<
    "  -t, --time-cutoff=S         Limit search after having used S seconds per input\n"
    "  -x, --xerox                 Xerox output format (default)\n" <<
    "  -f, --fast                  Be as fast as possible.\n" <<
    "                              (with this option enabled -u and -n don't work and\n" <<
    "                              output won't be ordered by weight).\n" <<
    "  -p, --pipe-mode[=STREAM]    Control input and output streams.\n" <<
    "\n" <<
    "N must be a positive integer. B must be a non-negative float.\n" <<
    "S must be a non-negative float. The default, 0.0, indicates no cutoff.\n"
    "Options -n and -b are combined with AND, i.e. they both restrict the output.\n" <<
    "\n" <<
    "STREAM can be { input, output, both }. If not given, defaults to {both}.\n" <<
#ifdef _MSC_VER
    "Input is read interactively via the console, i.e. line by line from the user.\n" <<
    "If you redirect input from a file, use --pipe-mode=input. Output is by default\n" <<
    "printed to the console. If you redirect output to a file, use --pipe-mode=output.\n" <<
#else
    "Input is read interactively line by line from the user. If you redirect input\n" <<
    "from a file, use --pipe-mode=input. --pipe-mode=output is ignored on non-windows\n" <<
    "platforms.\n" <<
#endif
    "\n" <<
    "Report bugs to " << PACKAGE_BUGREPORT << "\n" <<
    "\n";
  return true;
}

bool print_version(void)
{
  std::cout <<
    "\n" <<
    PACKAGE_STRING << std::endl <<
    __DATE__ << " " __TIME__ << std::endl <<
    "copyright (C) 2009 University of Helsinki\n";
  return true;
}

bool print_short_help(void)
{
  print_usage();
  return true;
}

int main(int argc, char **argv)
{

  int c;

  while (true)
    {
      static struct option long_options[] =
        {
          // first the hfst-mandated options
          {"help",         no_argument,       0, 'h'},
          {"version",      no_argument,       0, 'V'},
          {"verbose",      no_argument,       0, 'v'},
          {"quiet",        no_argument,       0, 'q'},
          {"silent",       no_argument,       0, 's'},
          // the hfst-optimized-lookup-specific options
          {"echo-inputs",  no_argument,       0, 'e'},
          {"show-weights", no_argument,       0, 'w'},
          {"beam",         required_argument, 0, 'b'},
          {"time-cutoff",  required_argument, 0, 't'},
          {"unique",       no_argument,       0, 'u'},
          {"xerox",        no_argument,       0, 'x'},
          {"fast",         no_argument,       0, 'f'},
          {"pipe-mode",    optional_argument,       0, 'p'},
          {"analyses",     required_argument, 0, 'n'},
          {0,              0,                 0,  0 }
        };

      int option_index = 0;
      c = getopt_long(argc, argv, "hVvqsewb:t:uxfn:p::", long_options, &option_index);

      if (c == -1) // no more options to look at
        break;

      switch (c)
        {
        case 'h':
          print_usage();
          return EXIT_SUCCESS;
          break;

        case 'V':
          print_version();
          return EXIT_SUCCESS;
          break;

        case 'v':
#ifdef DEBUG
          printDebuggingInformationFlag = true;
          preserveDiacriticRepresentationsFlag = true;
#endif

#ifdef TIMING
          timingFlag = true;
#endif
          verboseFlag = true;
          break;

        case 'q':
        case 's':
#ifdef DEBUG
          printDebuggingInformationFlag = false;
          preserveDiacriticRepresentationsFlag = false;
#endif

#ifdef TIMING
          timingFlag = false;
#endif
          verboseFlag = false;
          displayWeightsFlag = true;
          break;

        case 'e':
          echoInputsFlag = true;
          break;

        case 'w':
          displayWeightsFlag = true;
          break;

        case 'u':
          displayUniqueFlag = true;
          break;

        case 'b':
          beam = atof(optarg);
          if (beam < 0)
            {
              std::cerr << "Invalid argument for --beam\n";
              return EXIT_FAILURE;
            }
          break;
        case 't':
            time_cutoff = atof(optarg);
            if (time_cutoff < 0.0)
            {
                std::cerr << "Invalid argument for --time-cutoff\n";
                return EXIT_FAILURE;
            }
            break;
        case 'n':
          maxAnalyses = atoi(optarg);
          if (maxAnalyses < 1)
            {
              std::cerr << "Invalid or no argument for analyses count\n";
              return EXIT_FAILURE;
            }
          break;

        case 'x':
          outputType = xerox;
          break;

        case 'f':
          beFast = true;
          break;

        case 'p':
          if (optarg == NULL)
            { pipe_input = true; pipe_output = true; }
          else if (strcmp(optarg, "both") == 0 || strcmp(optarg, "BOTH") == 0)
            { pipe_input = true; pipe_output = true; }
          else if (strcmp(optarg, "input") == 0 || strcmp(optarg, "INPUT") == 0 ||
                   strcmp(optarg, "in") == 0 || strcmp(optarg, "IN") == 0)
            { pipe_input = true; }
          else if (strcmp(optarg, "output") == 0 || strcmp(optarg, "OUTPUT") == 0 ||
                   strcmp(optarg, "out") == 0 || strcmp(optarg, "OUT") == 0)
            { pipe_output = true; }
          else
            { std::cerr << "--pipe-mode argument " << std::string(optarg) << " unrecognised\n\n";
              return EXIT_FAILURE; }
          break;

        default:
          std::cerr << "Invalid option\n\n";
          print_short_help();
          return EXIT_FAILURE;
          break;
        }
    }

#ifdef _MSC_VER
  //hfst::print_output_to_console(!pipe_output); // has no effect on windows or mac
#endif

  // no more options, we should now be at the input filename
  if ( (optind + 1) < argc)
    {
      std::cerr << "More than one input file given\n";
      return EXIT_FAILURE;
    }
  else if ( (optind + 1) == argc)
    {
      FILE * f = fopen(argv[(optind)], "rb");
      if (f == NULL)
        {
          std::cerr << "Could not open file " << argv[(optind)] << std::endl;
          return 1;
        }
      return setup(f);
    }
  else
    {
      std::cerr << "No input file given\n";
      return EXIT_FAILURE;
    }
}

void TransducerHeader::skip_hfst3_header(FILE * f)
{
    const char* header1 = "HFST";
    unsigned int header_loc = 0; // how much of the header has been found
    int c;
    for(header_loc = 0; header_loc < strlen(header1) + 1; header_loc++)
    {
        c = getc(f);
        if(c != header1[header_loc]) {
            break;
        }
    }
    if(header_loc == strlen(header1) + 1) // we found it
    {
        unsigned short remaining_header_len;
        if (fread(&remaining_header_len,
                  sizeof(remaining_header_len), 1, f) != 1 ||
            getc(f) != '\0') {
            throw HeaderParsingException();
        }
        char * headervalue = new char[remaining_header_len];
        if (fread(headervalue, remaining_header_len, 1, f) != 1)
        {
            throw HeaderParsingException();
        }
        if (headervalue[remaining_header_len - 1] != '\0') {
            throw HeaderParsingException();
        }
        std::string header_tail(headervalue, remaining_header_len);
        size_t type_field = header_tail.find("type");
        if (type_field != std::string::npos) {
            if (header_tail.find("HFST_OL") != type_field + 5 &&
                header_tail.find("HFST_OLW") != type_field + 5) {
                delete[] headervalue;
                throw HeaderParsingException();
            }
        }
    } else // nope. put back what we've taken
    {
        ungetc(c, f); // first the non-matching character
            for(int i = header_loc - 1; i>=0; i--) {
// then the characters that did match (if any)
                ungetc(header1[i], f);
            }
    }
}

void TransducerAlphabet::get_next_symbol(FILE * f, SymbolNumber k)
{
  int byte;
  char * sym = line;
  while ( (byte = fgetc(f)) != 0 )
    {
      if (byte == EOF)
        {
          std::cerr << "Could not parse transducer; wrong or corrupt file?" << std::endl;
          exit(1);
        }
      *sym = byte;
      ++sym;
    }
  *sym = 0;
  if (strlen(line) >= 5 && line[0] == '@' && line[strlen(line) - 1] == '@' && line[2] == '.')
    { // a special symbol needs to be parsed
      std::string feat;
      std::string val;
      FlagDiacriticOperator op = P; // g++ worries about this falling through uninitialized
      switch (line[1]) {
      case 'P': op = P; break;
      case 'N': op = N; break;
      case 'R': op = R; break;
      case 'D': op = D; break;
      case 'C': op = C; break;
      case 'U': op = U; break;
      }
      char * c = line;
      // as long as we're working with utf-8, this should be ok
      for (c +=3; *c != '.' && *c != '@'; c++) { feat.append(c,1); }
      if (*c == '.')
        {
          for (++c; *c != '@'; c++) { val.append(c,1); }
        }
      if (feature_bucket.count(feat) == 0)
        {
          feature_bucket[feat] = feat_num;
          ++feat_num;
        }
      if (value_bucket.count(val) == 0)
        {
          value_bucket[val] = val_num;
          ++val_num;
        }
      operations.push_back(FlagDiacriticOperation(op, feature_bucket[feat], value_bucket[val]));
      kt->operator[](k) = strdup("");

#if OL_FULL_DEBUG
      std::cout << "symbol number " << k << " (flag) is \"" << line << "\"" << std::endl;
      kt->operator[](k) = strdup(line);
#endif

      return;
    }
  operations.push_back(FlagDiacriticOperation()); // dummy flag

#if OL_FULL_DEBUG
  std::cout << "symbol number " << k << " is \"" << line << "\"" << std::endl;
#endif

  kt->operator[](k) = strdup(line);
}

void LetterTrie::add_string(const char * p, SymbolNumber symbol_key)
{
  if (*(p+1) == 0)
    {
      symbols[(unsigned char)(*p)] = symbol_key;
      return;
    }
  if (letters[(unsigned char)(*p)] == NULL)
    {
      letters[(unsigned char)(*p)] = new LetterTrie();
    }
  letters[(unsigned char)(*p)]->add_string(p+1,symbol_key);
}

bool LetterTrie::has_key_starting_with(const char c) const
{
    return letters[(unsigned char) c] != NULL;
}

SymbolNumber LetterTrie::find_key(char ** p)
{
  const char * old_p = *p;
  ++(*p);
  if (letters[(unsigned char)(*old_p)] == NULL)
    {
      return symbols[(unsigned char)(*old_p)];
    }
  SymbolNumber s = letters[(unsigned char)(*old_p)]->find_key(p);
  if (s == NO_SYMBOL_NUMBER)
    {
      --(*p);
      return symbols[(unsigned char)(*old_p)];
    }
  return s;
}

void Encoder::read_input_symbols(KeyTable * kt)
{
  for (SymbolNumber k = 0; k < number_of_input_symbols; ++k)
    {
#if DEBUG
      assert(kt->find(k) != kt->end());
#endif
      const char * p = kt->operator[](k);
      if ((strlen(p) == 1) && (unsigned char)(*p) <= 127
          // we have a single char ascii symbol
          && !letters.has_key_starting_with(*p)) {
          // make sure there isn't a longer symbol we would be shadowing
          ascii_symbols[(unsigned char)(*p)] = k;
      }
      // If there's an ascii tokenized symbol shadowing this, remove it
      if (strlen(p) > 1 &&
          (unsigned char)(*p) <= 127 &&
          ascii_symbols[(unsigned char)(*p)] != NO_SYMBOL_NUMBER) {
          ascii_symbols[(unsigned char)(*p)] = NO_SYMBOL_NUMBER;
      }
      letters.add_string(p,k);
    }
}

SymbolNumber Encoder::find_key(char ** p)
{
  if (ascii_symbols[(unsigned char)(**p)] == NO_SYMBOL_NUMBER)
    {
      return letters.find_key(p);
    }
  SymbolNumber s = ascii_symbols[(unsigned char)(**p)];
  ++(*p);
  return s;
}

template <class genericTransducer>
void runTransducer (genericTransducer T)
{
  SymbolNumber * input_string = (SymbolNumber*)(malloc(2000));
  for (int i = 0; i < 1000; ++i)
    {
      input_string[i] = NO_SYMBOL_NUMBER;
    }

  char * str = (char*)(malloc(MAX_IO_STRING*sizeof(char)));
  *str = 0;
  char * old_str = str;

  while(true)
    {
#ifdef WINDOWS
      if (!pipe_input)
        {
          free(str);
          std::string linestr("");
          if (! hfst::get_line_from_console(linestr, MAX_IO_STRING*sizeof(char)))
            break;
          str = strdup(linestr.c_str());
          old_str = str;
        }
      else
#endif
        {
          if (! std::cin.getline(str,MAX_IO_STRING))
            break;
        }

      if (echoInputsFlag)
        {
#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(stdout, "%s\n", str); // fix: add \r?
          else
#endif
            std::cout << str << std::endl;
        }
      int i = 0;
      SymbolNumber k = NO_SYMBOL_NUMBER;
      bool failed = false;
      for ( char ** Str = &str; **Str != 0; )
        {
          k = T.find_next_key(Str);
#if OL_FULL_DEBUG
          std::cout << "INPUT STRING ENTRY " << i << " IS " << k << std::endl;
#endif
          if (k == NO_SYMBOL_NUMBER)
            {
              if (echoInputsFlag)
                {
                  std::cout << std::endl;
                }
              failed = true;
              break;
            }
          input_string[i] = k;
          ++i;
        }
      str = old_str;
      if (failed)
        { // tokenization failed
          if (outputType == xerox)
            {
#ifdef WINDOWS
          if (!pipe_output)
              hfst_fprintf_console(stdout, "%s\t%s\t+?\n\n", str, str);
          else
#endif
              std::cout << str << "\t" << str << "\t+?" << std::endl << std::endl;

#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(stdout, "\n\n");
          else
#endif
              std::cout << std::endl << std::endl;
            }
          continue;
        }

      input_string[i] = NO_SYMBOL_NUMBER;

      if (time_cutoff > 0.0) {
          start_clock = clock();
          call_counter = 0;
          limit_reached = false;
      }

      T.analyze(input_string);
      T.printAnalyses(std::string(str));
    }
}

int setup(FILE * f)
{
  try {
  TransducerHeader header(f);
  TransducerAlphabet alphabet(f, header.symbol_count());
  
  if (header.probe_flag(Has_unweighted_input_epsilon_cycles) ||
      header.probe_flag(Has_input_epsilon_cycles))
    {
      std::cerr << "!! Warning: transducer has epsilon cycles                  !!\n"
                << "!! This is currently not handled - if they are encountered !!\n"
                << "!! program *will* segfault.                                !!\n";
    }

  if (alphabet.get_state_size() == 0)
    {      // if the state size is zero, there are no flag diacritics to handle
      if (header.probe_flag(Weighted) == false)
        {
          if (displayUniqueFlag)
            { // no flags, no weights, unique analyses only
              TransducerUniq C(f, header, alphabet);
              runTransducer(C);
            } else if (!displayUniqueFlag)
            { // no flags, no weights, all analyses
            Transducer C(f, header, alphabet);
            runTransducer(C);
            }
        }
      else if (header.probe_flag(Weighted) == true)
        {
          if (displayUniqueFlag)
            { // no flags, weights, unique analyses only
              TransducerWUniq C(f, header, alphabet);
              runTransducer(C);
            } else if (!displayUniqueFlag)
            { // no flags, weights, all analyses
              TransducerW C(f, header, alphabet);
              runTransducer(C);
            }
        }
    } else // handle flag diacritics
    {
      if (header.probe_flag(Weighted) == false)
        {
          if (displayUniqueFlag)
            { // flags, no weights, unique analyses only
              TransducerFdUniq C(f, header, alphabet);
              runTransducer(C);
            } else
            { // flags, no weights, all analyses
              TransducerFd C(f, header, alphabet);
              runTransducer(C);
            }
        }
      else if (header.probe_flag(Weighted) == true)
        {
          if (displayUniqueFlag)
            { // flags, weights, unique analyses only
              TransducerWFdUniq C(f, header, alphabet);
              runTransducer(C);
            } else
            { // flags, no weights, all analyses
              TransducerWFd C(f, header, alphabet);
              runTransducer(C);
            }
        }
    }
  }
  catch (const HeaderParsingException & e)
    {
      std::cerr << "Invalid transducer header.\n";
      std::cerr << "The transducer must be in optimized lookup format.\n";
      return EXIT_FAILURE;
    }

  return 0;
}

/**
 * BEGIN old transducer.cc
 */

bool TransducerFd::PushState(FlagDiacriticOperation op)
{ // try to alter the flag diacritic state stack
  switch (op.Operation()) {
  case P: // positive set
    statestack.push_back(statestack.back());
    statestack.back()[op.Feature()] = op.Value();
    return true;
  case N: // negative set (literally, in this implementation)
    statestack.push_back(statestack.back());
    statestack.back()[op.Feature()] = -1*op.Value();
    return true;
  case R: // require
    if (op.Value() == 0) // empty require
      {
        if (statestack.back()[op.Feature()] == 0)
          {
            return false;
          }
        else
          {
            statestack.push_back(statestack.back());
            return true;
          }
      }
    if (statestack.back()[op.Feature()] == op.Value())
      {
        statestack.push_back(statestack.back());
        return true;
      }
    return false;
  case D: // disallow
        if (op.Value() == 0) // empty disallow
      {
        if (statestack.back()[op.Feature()] != 0)
          {
            return false;
          }
        else
          {
            statestack.push_back(statestack.back());
            return true;
          }
      }
    if (statestack.back()[op.Feature()] == op.Value()) // nonempty disallow
      {
        return false;
      }
    statestack.push_back(statestack.back());
    return true;
  case C: // clear
    statestack.push_back(statestack.back());
    statestack.back()[op.Feature()] = 0;
    return true;
  case U: // unification
    if (statestack.back()[op.Feature()] == 0 || // if the feature is unset or
        statestack.back()[op.Feature()] == op.Value() || // the feature is at this value already or
        (statestack.back()[op.Feature()] < 0 &&
         (statestack.back()[op.Feature()] * -1 != op.Value())) // the feature is negatively set to something else
        )
      {
        statestack.push_back(statestack.back());
        statestack.back()[op.Feature()] = op.Value();
        return true;
      }
    return false;
  }
  throw; // for the compiler's peace of mind
}

bool TransitionIndex::matches(SymbolNumber s)
{

  if (input_symbol == NO_SYMBOL_NUMBER)
    {
      return false;
    }
  if (s == NO_SYMBOL_NUMBER)
    {
      return true;
    }
  return input_symbol == s;
}

bool Transition::matches(SymbolNumber s)
{

  if (input_symbol == NO_SYMBOL_NUMBER)
    {
      return false;
    }
  if (s == NO_SYMBOL_NUMBER)
    {
      return true;
    }
  return input_symbol == s;
}


void IndexTableReader::get_index_vector(void)
{
  for (size_t i = 0;
       i < number_of_table_entries;
       ++i)
    {
      size_t j = i * TransitionIndex::SIZE;
      SymbolNumber * input = (SymbolNumber*)(TableIndices + j);
      TransitionTableIndex * index =
        (TransitionTableIndex*)(TableIndices + j + sizeof(SymbolNumber));
      indices.push_back(new TransitionIndex(*input,*index));
    }
}

void TransitionTableReader::Set(TransitionTableIndex pos)
{
  if (pos >= TRANSITION_TARGET_TABLE_START)
    {
      position = pos - TRANSITION_TARGET_TABLE_START;
    }
  else
    {
      position = pos;
    }
}

void TransitionTableReader::get_transition_vector(void)
{
  for (size_t i = 0; i < number_of_table_entries; ++i)
    {
      size_t j = i * Transition::SIZE;
      SymbolNumber * input = (SymbolNumber*)(TableTransitions + j);
      SymbolNumber * output =
        (SymbolNumber*)(TableTransitions + j + sizeof(SymbolNumber));
      TransitionTableIndex * target =
       (TransitionTableIndex*)(TableTransitions + j + 2 * sizeof(SymbolNumber));
      transitions.push_back(new Transition(*input,
                                           *output,
                                           *target));

    }
}

bool TransitionTableReader::Matches(SymbolNumber s)
{
  Transition * t = transitions[position];
  return t->matches(s);
}

bool TransitionTableReader::get_finality(TransitionTableIndex i)
{
  if (i >= TRANSITION_TARGET_TABLE_START)
    {
      return transitions[i - TRANSITION_TARGET_TABLE_START]->final();
    }
  else
    {
      return transitions[i]->final();
    }
}

void Transducer::set_symbol_table(void)
{
  for(KeyTable::iterator it = keys->begin();
      it != keys->end();
      ++it)
    {
      const char * key_name =
        it->second;

      symbol_table.push_back(key_name);
    }
}

void Transducer::try_epsilon_transitions(SymbolNumber * input_symbol,
                                         SymbolNumber * output_symbol,
                                         SymbolNumber * original_output_string,
                                         TransitionTableIndex i)
{
#if OL_FULL_DEBUG
  std::cout << "try_epsilon_transitions " << i << std::endl;
#endif
  while (transitions[i]->get_input() == 0)
    {
      *output_symbol = transitions[i]->get_output();
      get_analyses(input_symbol,
                   output_symbol+1,
                   original_output_string,
                   transitions[i]->target());
      ++i;
    }
}

void TransducerFd::try_epsilon_transitions(SymbolNumber * input_symbol,
                                         SymbolNumber * output_symbol,
                                         SymbolNumber * original_output_string,
                                         TransitionTableIndex i)
{
#if OL_FULL_DEBUG
  std::cout << "try_epsilon_transitions " << i << std::endl;
#endif

  while (true)
    {
    if (transitions[i]->get_input() == 0) // epsilon
        {
          *output_symbol = transitions[i]->get_output();
          get_analyses(input_symbol,
                       output_symbol+1,
                       original_output_string,
                       transitions[i]->target());
          ++i;
        } else if (transitions[i]->get_input() != NO_SYMBOL_NUMBER &&
                   operations[transitions[i]->get_input()].isFlag())
        {
          if (PushState(operations[transitions[i]->get_input()]))
            {
#if OL_FULL_DEBUG
              std::cout << "flag diacritic " <<
                symbol_table[transitions[i]->get_input()] << " allowed\n";
#endif
              // flag diacritic allowed
              *output_symbol = transitions[i]->get_output();
              get_analyses(input_symbol,
                           output_symbol+1,
                           original_output_string,
                           transitions[i]->target());
              statestack.pop_back();
            }
          else
            {
#if OL_FULL_DEBUG
              std::cout << "flag diacritic " <<
                symbol_table[transitions[i]->get_input()] << " disallowed\n";
#endif
            }
          ++i;
        } else
        {
          return;
        }
    }
}

void Transducer::try_epsilon_indices(SymbolNumber * input_symbol,
                                     SymbolNumber * output_symbol,
                                     SymbolNumber * original_output_string,
                                     TransitionTableIndex i)
{
#if OL_FULL_DEBUG
  std::cout << "try_epsilon_indices " << i << std::endl;
#endif
  if (indices[i]->get_input() == 0)
    {
      try_epsilon_transitions(input_symbol,
                              output_symbol,
                              original_output_string,
                              indices[i]->target() -
                              TRANSITION_TARGET_TABLE_START);
    }
}

void Transducer::find_transitions(SymbolNumber input,
                                    SymbolNumber * input_symbol,
                                    SymbolNumber * output_symbol,
                                    SymbolNumber * original_output_string,
                                    TransitionTableIndex i)
{
#if OL_FULL_DEBUG
  std::cout << "find_transitions " << i << "\t" << transitions[i]->get_input() << std::endl;
#endif

  while (transitions[i]->get_input() != NO_SYMBOL_NUMBER)
    {
      if (transitions[i]->get_input() == input)
        {

          *output_symbol = transitions[i]->get_output();
          get_analyses(input_symbol,
                       output_symbol+1,
                       original_output_string,
                       transitions[i]->target());
        }
      else
        {
          return;
        }
      ++i;
    }
}

void Transducer::find_index(SymbolNumber input,
                            SymbolNumber * input_symbol,
                            SymbolNumber * output_symbol,
                            SymbolNumber * original_output_string,
                            TransitionTableIndex i)
{
#if OL_FULL_DEBUG
  std::cout << "find_index " << i << "\t" << indices[i+input]->get_input() << std::endl;
#endif
  if (indices[i+input]->get_input() == input)
    {
      find_transitions(input,
                       input_symbol,
                       output_symbol,
                       original_output_string,
                       indices[i+input]->target() -
                       TRANSITION_TARGET_TABLE_START);
    }
}

void Transducer::note_analysis(SymbolNumber * whole_output_string)
{
  if (beFast)
    {
      for (SymbolNumber * num = whole_output_string; *num != NO_SYMBOL_NUMBER; ++num)
        {
#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(stdout, "%s", symbol_table[*num]);
          else
#endif
            std::cout << symbol_table[*num];
        }

#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "\n");
      else
#endif
        std::cout << std::endl;
    } else
    {
      std::string str = "";
      for (SymbolNumber * num = whole_output_string; *num != NO_SYMBOL_NUMBER; ++num)
        {
          str.append(symbol_table[*num]);
        }
      display_vector.push_back(str);
    }
}

void TransducerUniq::note_analysis(SymbolNumber * whole_output_string)
{
  std::string str = "";
  for (SymbolNumber * num = whole_output_string; *num != NO_SYMBOL_NUMBER; ++num)
    {
      str.append(symbol_table[*num]);
    }
  display_vector.insert(str);
}

void TransducerFdUniq::note_analysis(SymbolNumber * whole_output_string)
{
  std::string str = "";
  for (SymbolNumber * num = whole_output_string; *num != NO_SYMBOL_NUMBER; ++num)
    {
      str.append(symbol_table[*num]);
    }
  display_vector.insert(str);
}

void Transducer::get_analyses(SymbolNumber * input_symbol,
                              SymbolNumber * output_symbol,
                              SymbolNumber * original_output_string,
                              TransitionTableIndex i)
{
    if (time_cutoff > 0.0) {
        // Check to see if time has been overspent. For speed, only check every
        // million calls and set a flag.
        ++call_counter;
        if (limit_reached ||
            (call_counter % 1000000 == 0 &&
             ((((double) clock() - start_clock) / CLOCKS_PER_SEC) > time_cutoff))) {
            limit_reached = true;
            return;
        }
    }
#if OL_FULL_DEBUG
  std::cout << "get_analyses " << i << std::endl;
#endif
  if (i >= TRANSITION_TARGET_TABLE_START )
    {
      i -= TRANSITION_TARGET_TABLE_START;

      try_epsilon_transitions(input_symbol,
                              output_symbol,
                              original_output_string,
                              i+1);

#if OL_FULL_DEBUG
      std::cout << "Testing input string on transition side, " << *input_symbol << " at pointer" << std::endl;
#endif

      // input-string ended.
      if (*input_symbol == NO_SYMBOL_NUMBER)
        {
          *output_symbol = NO_SYMBOL_NUMBER;
          if (final_transition(i))
            {
              note_analysis(original_output_string);
            }
          return;
        }

      SymbolNumber input = *input_symbol;
      ++input_symbol;

      find_transitions(input,
                       input_symbol,
                       output_symbol,
                       original_output_string,
                       i+1);
    }
  else
    {

      try_epsilon_indices(input_symbol,
                          output_symbol,
                          original_output_string,
                          i+1);

#if OL_FULL_DEBUG
      std::cout << "Testing input string on index side, " << *input_symbol << " at pointer" << std::endl;
#endif

      if (*input_symbol == NO_SYMBOL_NUMBER)
        { // input-string ended.
          *output_symbol = NO_SYMBOL_NUMBER;
          if (final_index(i))
            {
              note_analysis(original_output_string);
            }
          return;
        }

      SymbolNumber input = *input_symbol;
      ++input_symbol;

      find_index(input,
                 input_symbol,
                 output_symbol,
                 original_output_string,
                 i+1);
    }
  *output_symbol = NO_SYMBOL_NUMBER;
}

void Transducer::printAnalyses(std::string prepend)
{
  if (!beFast)
    {
      if (outputType == xerox && display_vector.size() == 0)
        {
#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(stdout, "%s\t%s\t+?\n\n", prepend.c_str(), prepend.c_str());
          else
#endif
          std::cout << prepend << "\t" << prepend << "\t+?" << std::endl << std::endl;

#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(stdout, "\n\n");
          else
#endif
          std::cout << std::endl << std::endl;
          return;
        }
      int i = 0;
      DisplayVector::iterator it = display_vector.begin();
      while ( (it != display_vector.end()) && i < maxAnalyses )
        {
          if (outputType == xerox)
            {
#ifdef WINDOWS
              if (!pipe_output)
                hfst_fprintf_console(stdout, "%s\t", prepend.c_str());
              else
#endif
                std::cout << prepend << "\t";
            }

#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(stdout, "%s\n", it->c_str());
          else
#endif
            std::cout << *it << std::endl;

          ++it;
          ++i;
        }
      display_vector.clear(); // purge the display vector

#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "\n");
      else
#endif
        std::cout << std::endl;

    }
}

void TransducerUniq::printAnalyses(std::string prepend)
{
  if (outputType == xerox && display_vector.size() == 0)
    {

#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "%s\t%s\t+?\n\n", prepend.c_str(), prepend.c_str());
      else
#endif
        std::cout << prepend << "\t" << prepend << "\t+?" << std::endl << std::endl;

#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "\n\n");
      else
#endif
        std::cout << std::endl << std::endl;

      return;
    }
  int i = 0;
  DisplaySet::iterator it = display_vector.begin();
  while ( (it != display_vector.end()) && i < maxAnalyses)
    {
      if (outputType == xerox)
        {
#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "%s\t", prepend.c_str());
      else
#endif
        std::cout << prepend << "\t";
        }

#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "%s\n", it->c_str());
      else
#endif
        std::cout << *it << std::endl;

      ++it;
      ++i;
    }
  display_vector.clear(); // purge the display set
#ifdef WINDOWS
  if (!pipe_output)
    hfst_fprintf_console(stdout, "\n");
  else
#endif
    std::cout << std::endl;
}

void TransducerFdUniq::printAnalyses(std::string prepend)
{
  if (outputType == xerox && display_vector.size() == 0)
    {
#ifdef WINDOWS
  if (!pipe_output)
    hfst_fprintf_console(stdout, "%s\t%s\t+?\n\n", prepend.c_str(), prepend.c_str());
  else
#endif
      std::cout << prepend << "\t" << prepend << "\t+?" << std::endl << std::endl;

#ifdef WINDOWS
  if (!pipe_output)
    hfst_fprintf_console(stdout, "\n\n");
  else
#endif
      std::cout << std::endl << std::endl;
  return;
    }
  int i = 0;
  DisplaySet::iterator it = display_vector.begin();
  while ( (it != display_vector.end()) && i < maxAnalyses)
    {
      if (outputType == xerox)
        {
#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(stdout, "%s\t", prepend.c_str());
          else
#endif
            std::cout << prepend << "\t";
        }

#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "%s\n", it->c_str());
      else
#endif
        std::cout << *it << std::endl;

      ++it;
      ++i;
    }
  display_vector.clear(); // purge the display set

#ifdef WINDOWS
  if (!pipe_output)
    hfst_fprintf_console(stdout, "\n");
  else
#endif
    std::cout << std::endl;
}

/**
 * BEGIN old transducer-weighted.cc
 */

bool TransitionWIndex::matches(SymbolNumber s)
{

  if (input_symbol == NO_SYMBOL_NUMBER)
    {
      return false;
    }
  if (s == NO_SYMBOL_NUMBER)
    {
      return true;
    }
  return input_symbol == s;
}

bool TransitionW::matches(SymbolNumber s)
{

  if (input_symbol == NO_SYMBOL_NUMBER)
    {
      return false;
    }
  if (s == NO_SYMBOL_NUMBER)
    {
      return true;
    }
  return input_symbol == s;
}

bool TransducerWFd::PushState(FlagDiacriticOperation op)
{
  switch (op.Operation()) {
  case P: // positive set
    statestack.push_back(statestack.back());
    statestack.back()[op.Feature()] = op.Value();
    return true;
  case N: // negative set (literally, in this implementation)
    statestack.push_back(statestack.back());
    statestack.back()[op.Feature()] = -1*op.Value();
    return true;
  case R: // require
    if (op.Value() == 0) // empty require
      {
        if (statestack.back()[op.Feature()] == 0)
          {
            return false;
          }
        statestack.push_back(statestack.back());
        return true;
      }
    if (statestack.back()[op.Feature()] == op.Value())
      {
        statestack.push_back(statestack.back());
        return true;
      }
    return false;
  case D: // disallow
    if (op.Value() == 0) // empty disallow
      {
        if (statestack.back()[op.Feature()] != 0)
          {
            return false;
          }
        else
          {
            statestack.push_back(statestack.back());
            return true;
          }
      }
    if (statestack.back()[op.Feature()] == op.Value()) // nonempty disallow
      {
        return false;
      }
    statestack.push_back(statestack.back());
    return true;
  case C: // clear
    statestack.push_back(statestack.back());
    statestack.back()[op.Feature()] = 0;
    return true;
  case U: // unification
    if (statestack.back()[op.Feature()] == 0 || // if the feature is unset or
        statestack.back()[op.Feature()] == op.Value() || // the feature is at this value already or
        (statestack.back()[op.Feature()] < 0 &&
         (statestack.back()[op.Feature()] * -1 != op.Value())) // the feature is negatively set to something else
        )
      {
        statestack.push_back(statestack.back());
        statestack.back()[op.Feature()] = op.Value();
        return true;
      }
    return false;
  }
  throw; // for the compiler's peace of mind
}

void IndexTableReaderW::get_index_vector(void)
{
  for (size_t i = 0;
       i < number_of_table_entries;
       ++i)
    {
      size_t j = i * TransitionWIndex::SIZE;
      SymbolNumber * input = (SymbolNumber*)(TableIndices + j);
      TransitionTableIndex * index =
        (TransitionTableIndex*)(TableIndices + j + sizeof(SymbolNumber));
      indices.push_back(new TransitionWIndex(*input,*index));
    }
}

void TransitionTableReaderW::Set(TransitionTableIndex pos)
{
  if (pos >= TRANSITION_TARGET_TABLE_START)
    {
      position = pos - TRANSITION_TARGET_TABLE_START;
    }
  else
    {
      position = pos;
    }
}

void TransitionTableReaderW::get_transition_vector(void)
{
  for (size_t i = 0; i < number_of_table_entries; ++i)
    {
      size_t j = i * TransitionW::SIZE;
      SymbolNumber * input = (SymbolNumber*)(TableTransitions + j);
      SymbolNumber * output =
        (SymbolNumber*)(TableTransitions + j + sizeof(SymbolNumber));
      TransitionTableIndex * target =
        (TransitionTableIndex*)(TableTransitions + j + 2 * sizeof(SymbolNumber));
      Weight * weight =
        (Weight*)(TableTransitions + j + 2 * sizeof(SymbolNumber) + sizeof(TransitionTableIndex));
      transitions.push_back(new TransitionW(*input,
                                            *output,
                                            *target,
                                            *weight));

    }
  transitions.push_back(new TransitionW());
  transitions.push_back(new TransitionW());
}

bool TransitionTableReaderW::Matches(SymbolNumber s)
{
  TransitionW * t = transitions[position];
  return t->matches(s);
}

bool TransitionTableReaderW::get_finality(TransitionTableIndex i)
{
  if (i >= TRANSITION_TARGET_TABLE_START)
    {
      return transitions[i - TRANSITION_TARGET_TABLE_START]->final();
    }
  else
    {
      return transitions[i]->final();
    }
}


void TransducerW::set_symbol_table(void)
{
  for(KeyTable::iterator it = keys->begin();
      it != keys->end();
      ++it)
    {
      const char * key_name =
        it->second;

      symbol_table.push_back(key_name);
    }
}

void TransducerW::try_epsilon_transitions(SymbolNumber * input_symbol,
                                          SymbolNumber * output_symbol,
                                          SymbolNumber *
                                          original_output_string,
                                          TransitionTableIndex i)
{
#if OL_FULL_DEBUG
  std::cerr << "try epsilon transitions " << i << " " << current_weight << std::endl;
#endif

  if (transitions.size() <= i)
    {
      return;
    }

  while ((transitions[i] != NULL) && (transitions[i]->get_input() == 0))
    {
      *output_symbol = transitions[i]->get_output();
      current_weight += transitions[i]->get_weight();
      get_analyses(input_symbol,
                   output_symbol+1,
                   original_output_string,
                   transitions[i]->target());
      current_weight -= transitions[i]->get_weight();
      ++i;
    }
  *output_symbol = NO_SYMBOL_NUMBER;
}

void TransducerWFd::try_epsilon_transitions(SymbolNumber * input_symbol,
                                            SymbolNumber * output_symbol,
                                            SymbolNumber *
                                            original_output_string,
                                            TransitionTableIndex i)
{
  if (transitions.size() <= i)
    { return; }

  // Endless loop protection
  if (output_symbol > &output_string.back()) {
     return;
  }

  while (true)
    {
    if (transitions[i]->get_input() == 0) // epsilon
        {
          *output_symbol = transitions[i]->get_output();
          current_weight += transitions[i]->get_weight();
          get_analyses(input_symbol,
                       output_symbol+1,
                       original_output_string,
                       transitions[i]->target());
          current_weight -= transitions[i]->get_weight();
          ++i;
        } else if (transitions[i]->get_input() != NO_SYMBOL_NUMBER &&
                   operations[transitions[i]->get_input()].isFlag())
        {
            if (PushState(operations[transitions[i]->get_input()]))
            {
#if OL_FULL_DEBUG
              std::cout << "flag diacritic " <<
                symbol_table[transitions[i]->get_input()] << " allowed\n";
#endif
              // flag diacritic allowed
              *output_symbol = transitions[i]->get_output();
              current_weight += transitions[i]->get_weight();
              get_analyses(input_symbol,
                           output_symbol+1,
                           original_output_string,
                           transitions[i]->target());
              current_weight -= transitions[i]->get_weight();
              statestack.pop_back();
            }
          else
            {
#if OL_FULL_DEBUG
              std::cout << "flag diacritic " <<
                symbol_table[transitions[i]->get_input()] << " disallowed\n";
#endif
            }
          ++i;
        } else
        {
          return;
        }
    }
}

void TransducerW::try_epsilon_indices(SymbolNumber * input_symbol,
                                      SymbolNumber * output_symbol,
                                      SymbolNumber * original_output_string,
                                      TransitionTableIndex i)
{
#if OL_FULL_DEBUG
  std::cerr << "try indices " << i << " " << current_weight << std::endl;
#endif
  if (indices[i]->get_input() == 0)
    {
      try_epsilon_transitions(input_symbol,
                              output_symbol,
                              original_output_string,
                              indices[i]->target() -
                              TRANSITION_TARGET_TABLE_START);
    }
}

void TransducerW::find_transitions(SymbolNumber input,
                                   SymbolNumber * input_symbol,
                                   SymbolNumber * output_symbol,
                                   SymbolNumber * original_output_string,
                                   TransitionTableIndex i)
{
#if OL_FULL_DEBUG
  std::cerr << "find transitions " << i << " " << current_weight << std::endl;
#endif

  if (transitions.size() <= i)
    {
      return;
    }

  // Endless loop protection
  if (output_symbol > &output_string.back()) {
    return;
  }

  while (transitions[i]->get_input() != NO_SYMBOL_NUMBER)
    {

      if (transitions[i]->get_input() == input)
        {
          current_weight += transitions[i]->get_weight();
          *output_symbol = transitions[i]->get_output();
          get_analyses(input_symbol,
                       output_symbol+1,
                       original_output_string,
                       transitions[i]->target());
          current_weight -= transitions[i]->get_weight();
        }
      else
        {
          return;
        }
      ++i;
    }

}

void TransducerW::find_index(SymbolNumber input,
                             SymbolNumber * input_symbol,
                             SymbolNumber * output_symbol,
                             SymbolNumber * original_output_string,
                             TransitionTableIndex i)
{
#if OL_FULL_DEBUG
  std::cerr << "find index " << i << " " << current_weight << std::endl;
#endif
  if (indices.size() <= i)
    {
      return;
    }

  if (indices[i+input]->get_input() == input)
    {

      find_transitions(input,
                       input_symbol,
                       output_symbol,
                       original_output_string,
                       indices[i+input]->target() -
                       TRANSITION_TARGET_TABLE_START);
    }
}

void TransducerW::note_analysis(SymbolNumber * whole_output_string)
{
  std::string str = "";
  for (SymbolNumber * num = whole_output_string;
       num <= &output_string.back() && *num != NO_SYMBOL_NUMBER;
       ++num)
    {
      str.append(symbol_table[*num]);
    }
  display_map.insert(std::pair<Weight, std::string>(current_weight, str));
}

void TransducerWUniq::note_analysis(SymbolNumber * whole_output_string)
{
  std::string str = "";
  for (SymbolNumber * num = whole_output_string;
       *num != NO_SYMBOL_NUMBER;
       ++num)
    {
      str.append(symbol_table[*num]);
    }
  if ((display_map.count(str) == 0) || (display_map[str] > current_weight))
    { // if there isn't an entry yet or we've found a lower weight
      display_map.insert(std::pair<std::string, Weight>(str, current_weight));
    }
}

void TransducerWFdUniq::note_analysis(SymbolNumber * whole_output_string)
{
  std::string str = "";
  for (SymbolNumber * num = whole_output_string;
       *num != NO_SYMBOL_NUMBER;
       ++num)
    {
      str.append(symbol_table[*num]);
    }
  if ((display_map.count(str) == 0) || (display_map[str] > current_weight))
    { // if there isn't an entry yet or we've found a lower weight
      display_map.insert(std::pair<std::string, Weight>(str, current_weight));
    }
}

void TransducerW::printAnalyses(std::string prepend)
{
  if (outputType == xerox && display_map.size() == 0)
    {
#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "%s\t%s\t+?\n\n", prepend.c_str(), prepend.c_str());
      else
#endif
          std::cout << prepend << "\t" << prepend << "\t+?" << std::endl << std::endl;

#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "\n\n");
      else
          std::cout << std::endl << std::endl;
#endif

      return;
    }
  int i = 0;
  float lowest_weight = -1;
  DisplayMultiMap::iterator it = display_map.begin();
  while ( (it != display_map.end()) && (i < maxAnalyses))
    {
      if (it == display_map.begin())
        lowest_weight = it->first;
      // if beam is not set, i.e. has a negative value (-1.0), the only constraint
      // is maxAnalyses
      if (beam < 0 || it->first <= (lowest_weight + beam))
      {
        if (outputType == xerox)
          {
#ifdef WINDOWS
            if (!pipe_output)
              hfst_fprintf_console(stdout, "%s\t", prepend.c_str());
            else
#endif
              std::cout << prepend << "\t";
          }

#ifdef WINDOWS
        if (!pipe_output)
          hfst_fprintf_console(stdout, "%s", (*it).second.c_str());
        else
#endif
          std::cout << (*it).second;

        if (displayWeightsFlag)
          {
#ifdef WINDOWS
            if (!pipe_output)
              hfst_fprintf_console(stdout, "\t%f", (*it).first);
            else
#endif
              std::cout << '\t' << (*it).first;
          }

#ifdef WINDOWS
        if (!pipe_output)
          hfst_fprintf_console(stdout, "\n");
        else
#endif
          std::cout << std::endl;

      }
      ++it;
      ++i;
    }
  display_map.clear();

#ifdef WINDOWS
  if (!pipe_output)
    hfst_fprintf_console(stdout, "\n");
  else
#endif
    std::cout << std::endl;
}

void TransducerWUniq::printAnalyses(std::string prepend)
{
  if (outputType == xerox && display_map.size() == 0)
    {
#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "%s\t%s\t+?\n", prepend.c_str(), prepend.c_str());
      else
#endif
        std::cout << prepend << "\t" << prepend << "\t+?" << std::endl;

#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "\n");
      else
#endif
        std::cout << std::endl;

      return;
    }
  int i = 0;
  float lowest_weight = -1 ;
  std::multimap<Weight, std::string> weight_sorted_map;
  DisplayMap::iterator it = display_map.begin();
  while (it != display_map.end())
    {
      if (it == display_map.begin())
        lowest_weight = it->second;
      if (beam < 0 || it->second <= (lowest_weight + beam))
        weight_sorted_map.insert(std::pair<Weight, std::string>((*it).second, (*it).first));
      ++it;
    }
  std::multimap<Weight, std::string>::iterator display_it = weight_sorted_map.begin();
  while ( (display_it != weight_sorted_map.end()) && (i < maxAnalyses))
    {
      if (outputType == xerox)
        {
#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(stdout, "%s\t", prepend.c_str());
          else
#endif
            std::cout << prepend << "\t";
        }

#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(stdout, "%s", (*display_it).second.c_str());
          else
#endif
            std::cout << (*display_it).second;

      if (displayWeightsFlag)
        {
#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(stdout, "\t%f", (*display_it).first);
          else
#endif
            std::cout << '\t' << (*display_it).first;
        }

#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "\n");
      else
#endif
        std::cout << std::endl;

      ++display_it;
      ++i;
    }
  display_map.clear();

#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "\n");
      else
#endif
        std::cout << std::endl;
}

void TransducerWFdUniq::printAnalyses(std::string prepend)
{
  if (outputType == xerox && display_map.size() == 0)
    {
#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "%s\t%s\t+?", prepend, prepend);
      else
#endif
        std::cout << prepend << "\t" << prepend << "\t+?" << std::endl;

#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "\n");
      else
#endif
        std::cout << std::endl;

      return;
    }
  int i = 0;
  float lowest_weight = -1 ;
  std::multimap<Weight, std::string> weight_sorted_map;
  DisplayMap::iterator it;
  for (it = display_map.begin(); it != display_map.end(); it++)
    {
      if (it == display_map.begin())
        lowest_weight = it->second;
      if (beam < 0 || it->second <= (lowest_weight + beam))
        weight_sorted_map.insert(std::pair<Weight, std::string>((*it).second, (*it).first));
    }
  std::multimap<Weight, std::string>::iterator display_it;
  for (display_it = weight_sorted_map.begin();
       display_it != weight_sorted_map.end() && i < maxAnalyses;
       display_it++, i++)
    {
      if (outputType == xerox)
        {
#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(stdout, "%s\t", prepend);
          else
#endif
            std::cout << prepend << "\t";
        }

#ifdef WINDOWS
      if (!pipe_output)
        hfst_fprintf_console(stdout, "%s", (*display_it).second);
      else
#endif
        std::cout << (*display_it).second;

      if (displayWeightsFlag)
        {
#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(stdout, "\t%f", (*display_it).first);
          else
#endif
            std::cout << '\t' << (*display_it).first;
        }

#ifdef WINDOWS
          if (!pipe_output)
            hfst_fprintf_console(stdout, "\n");
          else
#endif
            std::cout << std::endl;
    }
  display_map.clear();

#ifdef WINDOWS
  if (!pipe_output)
    hfst_fprintf_console(stdout, "\n");
  else
#endif
    std::cout << std::endl;
}

void TransducerW::get_analyses(SymbolNumber * input_symbol,
                               SymbolNumber * output_symbol,
                               SymbolNumber * original_output_string,
                               TransitionTableIndex i)
{
#if OL_FULL_DEBUG
  std::cerr << "get analyses " << i << " " << current_weight << std::endl;
#endif
  if (time_cutoff > 0.0) {
      // Check to see if time has been overspent. For speed, only check every
      // million calls and set a flag.
      ++call_counter;
      if (limit_reached ||
          (call_counter % 1000000 == 0 &&
           ((((double) clock() - start_clock) / CLOCKS_PER_SEC) > time_cutoff))) {
          limit_reached = true;
          return;
      }
  }

  // Endless loop protection
  if (output_symbol > &output_string.back()) {
    return;
  }

  if (i >= TRANSITION_TARGET_TABLE_START )
    {
      i -= TRANSITION_TARGET_TABLE_START;

      try_epsilon_transitions(input_symbol,
                              output_symbol,
                              original_output_string,
                              i+1);

      // input-string ended.
      if (*input_symbol == NO_SYMBOL_NUMBER)
        {
          *output_symbol = NO_SYMBOL_NUMBER;
          if (transitions.size() <= i)
            {
              return;
            }
          if (final_transition(i))
            {
              current_weight += get_final_transition_weight(i);
              note_analysis(original_output_string);
              current_weight -= get_final_transition_weight(i);
            }
          return;
        }

      SymbolNumber input = *input_symbol;
      ++input_symbol;


      find_transitions(input,
                       input_symbol,
                       output_symbol,
                       original_output_string,
                       i+1);
    }
  else
    {

      try_epsilon_indices(input_symbol,
                          output_symbol,
                          original_output_string,
                          i+1);
      // input-string ended.
      if (*input_symbol == NO_SYMBOL_NUMBER)
        {
          *output_symbol = NO_SYMBOL_NUMBER;
          if (final_index(i))
            {
              current_weight += get_final_index_weight(i);
              note_analysis(original_output_string);
              current_weight -= get_final_index_weight(i);
            }
          return;
        }

      SymbolNumber input = *input_symbol;
      ++input_symbol;

      find_index(input,
                 input_symbol,
                 output_symbol,
                 original_output_string,
                 i+1);
    }
}

