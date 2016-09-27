// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include <string>
#include <vector>

namespace hfst
{
namespace xfst
{

typedef std::vector<std::string> StringVector;

// find help message for a certain command
#define HELP_MODE_ONE_COMMAND 0
// get all commands and their help messages
#define HELP_MODE_ALL_COMMANDS 1
// get all command-help message pairs, where a certain word is found
#define HELP_MODE_APROPOS 2

// handle case: if text matches, list all command names, and possibly return
#define COMMAND(names, args, description) if (handle_case(names, args, description, text, message, help_mode, true)) { return true; }
// handle case: if text matches, do not list all command names, and continue
#define CONT_COMMAND(names, args, description) handle_case(names, args, description, text, message, help_mode, false)
// handle case: if text matches and ambiguous cases are not ignored, list help messages for all matching commands and possibly return
#define AMBIGUOUS_COMMAND(name, namelist) if (!skip_ambiguous_cases && handle_ambiguous_case(name, namelist, text, message, help_mode)) { return true; }

// Convert \a str to upper case.
std::string to_upper_case(const std::string & str)
{
  std::string retval;
  for (unsigned int i=0; i<str.length(); i++) {
    if (str[i] >= 97 && str[i] <= 122) {
      retval.append(1, str[i] - 32); }
    else {
      retval.append(1, str[i]); } }
  return retval;
}

// Whether \a c is a punctuation character.
bool is_punctuation_char(char c)
{
  std::string chars = " \n\t.,;:?!-/'\"<>()|";
  for (size_t i=0; i < chars.size(); i++)
    {
      if (chars[i] == c) {
        return true; }
    }
  return false;
}

// Whether word \a str_ is found in text \a text_.
// Punctuation characters and upper/lower case are handled in this function.
bool word_found_in_text(const std::string & str_, const std::string & text_)
{
  // Convert both arguments to upper case, so case does not matter.
  std::string str = to_upper_case(str_);
  std::string text = to_upper_case(text_);
  // Find str_ in text_:
  std::size_t pos = text.find(str);
  // (1) If not found, return false
  if (pos == std::string::npos)
    {
      return false;
    }
  // (2) if found, see whether both preceeding and succeeding
  // characters, if they exist, are punctuation characters:
  if (pos == 0 || is_punctuation_char(text[pos-1]))
    {
      if (pos+str.length() == text.length() ||
          is_punctuation_char(text[pos+str.length()]))
        {
          // If they are, return true,
          return true;
        }
    }
  // if not, return false.
  return false;
}


// Convert the list of names separated by commas \a namelist into a vector of names.
StringVector namelist_to_name_vector(const std::string & namelist)
{
  StringVector names;
  size_t pos=0; // latest name start position
  // Go through the namelist char by char:
  for (size_t i=0; i < namelist.length(); i++)
    {
      // If a separating comma is found,
      if (namelist[i] == ',')
        {
          // add the name to vector,
          names.push_back(std::string(namelist, pos, i-pos));
          // and skip empty space after the comma.
          i++;
          while(namelist[i] == ' ') {
            i++;
          }
          pos = i; // next name starts here
          i--;     // i will be incremented in the for loop
        }
    }
  // Add the last name to vector.
  names.push_back(std::string(namelist, pos, std::string::npos));
  return names;
}
    
// Append help message for command known by names in \a namelist, taking arguments
// listed in \a arguments, and described by \a description to \a message.
// \a all_names defines whether all names in \a namelist are included or just the first one.
void append_help_message(const std::string & namelist, const std::string & arguments,
                         const std::string & description, std::string & message, bool all_names = true)
{
  size_t NAME_AND_ARGUMENTS_FIELD_WIDTH = 30;

  StringVector names = namelist_to_name_vector(namelist);

  message.append(names.front());
  message.append(" ");
  message.append(arguments);

  size_t name_and_arguments_length
    = names.front().length() + 1 + arguments.length();

  if (name_and_arguments_length > NAME_AND_ARGUMENTS_FIELD_WIDTH)
    message.append(1, ' ');
  else
    message.append(NAME_AND_ARGUMENTS_FIELD_WIDTH - name_and_arguments_length, ' ');

  message.append(description);
  message.append("\n");

  // If all names are included and there is more than one name,
  // list names starting from the second name separated by commas,
  // inside parenthesis.
  if (all_names && names.size() > 1)
    {
      for (StringVector::const_iterator it = names.begin();
           it != names.end(); it++)
        {
          if (it == names.begin())
            {
              message.append("(");
              continue;
            }
          else if (it != ++(names.begin()))
            {
              message.append(", ");
            }
          message.append(*it);
        }
      message.append(")\n");
    }
}

bool text_matches_some_name(const std::string & text, const std::string & namelist)
{
  StringVector names = namelist_to_name_vector(namelist);
  for (StringVector::const_iterator it = names.begin();
       it != names.end(); it++)
    {
      if (text == *it)
        {
          return true;
        }
    }
  return false;
}

    bool get_help_message(const std::string & text, std::string & message, int help_mode, bool skip_ambiguous_cases=false);

bool handle_ambiguous_case(const std::string & name, const std::string & namelist,
                           const std::string & text, std::string & message, int help_mode)
{
  if (help_mode == HELP_MODE_ALL_COMMANDS ||
      help_mode == HELP_MODE_APROPOS)
    {
      return false; // do nothing, and continue because all commands are gone through
    }
  // HELP_MODE_ONE_COMMAND
  if (name != text)
    {
      return false;
    }
  StringVector names = namelist_to_name_vector(namelist);
  for (StringVector::const_iterator it = names.begin();
       it != names.end(); it++)
    {
      if (it != names.begin())
        message.append("##\n");
      (void)get_help_message(*it, message, help_mode, true);
    }
  return true;
}

// If \a text matches (depending on \a help_mode) a command known by names
// listed in \a names, taking arguments \a arguments, described by \a description,
// add help message for the command to \a message. \a all_names defines whether
// all names in \a names are included or just the first one.
// Return whether the search should continue (depends on \a help_mode).
bool handle_case(const std::string & names, const std::string & arguments,
                 const std::string & description, const std::string & text,
                 std::string & message, int help_mode, bool all_names=true)
{
  if (help_mode == HELP_MODE_ALL_COMMANDS)
    {
      append_help_message(names, arguments, description, message, all_names);
      return false; // go through all commands
    }
  else if (help_mode == HELP_MODE_APROPOS)
    {
      if (word_found_in_text(text, names) ||
          word_found_in_text(text, description))
        {
          append_help_message(names, arguments, description, message, all_names);
        }
      return false; // continue search
    }
  else // HELP_MODE_ONE_COMMAND
    {
      if (text_matches_some_name(text, names))
        {
          append_help_message(names, arguments, description, message, all_names);
          return true; // command found
        }
      return false; // continue search
    }
}

// Generate help message(s) for command(s) named \a text and append the help message(s)
// to \a message. \a help_mode defines whether we are generating help messages for \a text,
// all commands (in that case, \a message is ignored) or for commands that contain or
// whose help messages contain the word \a text. \a skip_ambiguous_cases defines whether
// ambiguous cases where \a text matches more than one command are ignored.
// @return Whether the help message could be generated.
    bool get_help_message(const std::string & text, std::string & message, int help_mode,
                          bool skip_ambiguous_cases)
{
  // Apropos with no arguments -> return help message for apropos.
  if (help_mode == HELP_MODE_APROPOS && text == "")
    {
      return get_help_message("apropos", message, HELP_MODE_ONE_COMMAND);
    }

  // To see whether any help message was appended.
  std::string message_at_start(message);

  COMMAND("ambiguous upper, ambiguous", "",
          "returns the input words which have multiple paths in a transducer");
    
  AMBIGUOUS_COMMAND("apply", "apply down, apply up");

  CONT_COMMAND("apply down, down", "<string>",
              "apply <string> down to the top network on stack");
  
  COMMAND("apply down, down", "",
          "enter apply down mode (Ctrl-D exits)");
  
  CONT_COMMAND("apply up, up", "<string>",
              "apply <string> up to the top network on stack");
  
  COMMAND("apply up, up", "",
          "enter apply up mode (Ctrl-D exits)");
  //COMMAND("apply med <string>, apply med", "find approximate matches to string in top network by minimum edit distance");
  COMMAND("apropos", "<string>",
          "search help for <string>");
  //COMMAND("add properties, add", "", "<not implemented>");
  //COMMAND("alias", "", "<not implemented>");
  //COMMAND("cleanup net, cleanup", "", "<not implemented>");
  COMMAND("clear stack, clear", "",
          "clears the stack");
  //COMMAND("collect epsilon-loops, epsilon-loops", "", "<not implemented>");
  //"compile-replace lower, com-rep lower"
  //"compile-replace upper, com-rep upper"
  COMMAND("compact sigma", "", "removes redundant symbols from FSM");
  COMMAND("complete net, complete", "", "completes the FSM");
  COMMAND("compose net, compose", "", "composes networks on stack");
  COMMAND("concatenate net, concatenate", "", "concatenates networks on stack");
  COMMAND("crossproduct net, crossproduct", "", "cross-product of top two FSMs on stack");
  CONT_COMMAND("define", "<name> <r.e.>", "define a network");
  COMMAND("define", "<fname>(<v1,..,vn>) <r.e.>", "define function");
  COMMAND("determinize net, determinize, determinise net, determinise", "", "determinizes top FSM on stack");
  COMMAND("echo", "<string>", "echo a string");
  //COMMAND("edit properties, edit", "", "<not implemented>");
  //"epsilon-remove net, epsilon-remove"
  COMMAND("eliminate flag", "<name>", "eliminate flag <name> diacritics from the top network");
  COMMAND("eliminate flags", "", "eliminate all flag diacritics from the top network");
  COMMAND("export cmatrix", "(filename)", "export the confusion matrix as an AT&T transducer");
  COMMAND("extract ambiguous", "", "extracts the ambiguous paths of a transducer");
  COMMAND("extract unambiguous", "", "extracts the unambiguous paths of a transducer");
  //COMMAND("hfst", "", "<no description>");
  COMMAND("ignore net, ignore", "", "applies ignore to top two FSMs on stack");
  COMMAND("intersect net, intersect, conjunct net, conjunct", "", "intersects FSMs on stack");
  COMMAND("invert net, invert", "", "inverts top FSM");
  COMMAND("inspect, inspect net", "", "interactively traverse top FSM");
  AMBIGUOUS_COMMAND("help", "help, help license, help warranty");
  CONT_COMMAND("help", "", "lists all commands");
  COMMAND("help", "<name>", "prints help message of a command");
  COMMAND("help license, help licence", "", "prints license");
  COMMAND("help warranty", "", "prints warranty information");
  COMMAND("label net", "", "extracts all attested symbol pairs from FSM");
  COMMAND("letter machine", "", "Converts top FSM to a letter machine");
  COMMAND("list <symbol> <symbols>", "", "Associates the given symbol with a set of symbols");
  AMBIGUOUS_COMMAND("load", "load stack, load defined");
  COMMAND("load stack, load", "<filename>", "Loads networks and pushes them on the stack");
  COMMAND("load defined, loadd", "<filename>", "Restores defined networks from file");
  COMMAND("lookup-optimize, lookup-optimise", "", "optimize top network for fast lookup");
  COMMAND("lower-side net, lower-side", "", "takes lower projection of top FSM");
  COMMAND("minimize net, minimize, minimise net, minimise", "", "minimizes top FSM");
  COMMAND("minus net, minus, subtract", "", "subtracts networks on stack");
  COMMAND("name net, name", "<string>", "names top FSM");
  COMMAND("negate net, negate", "", "complements top FSM");
  COMMAND("one-plus net, one-plus", "", "Kleene plus on top FSM");
  COMMAND("pop, pop stack", "", "remove top FSM from stack");
  AMBIGUOUS_COMMAND("print", "print aliases, print arc-tally, print defined, print definition, print directory, "
                    "print dot, print att, print file-info, print flags, print labels, print label-maps, print label-tally, "
                    "print list, print lists, print longest-string, print longest-string-size, print lower-words, "
                    "print name, print net, print properties, print random-lower, print random-upper, print random-words, "
                    "print shortest-string-size, print shortest-string, print sigma, print sigma-tally, print size, "
                    "print stack, print upper-words, print words");
  AMBIGUOUS_COMMAND("write", "write definition, write dot, write att, print properties, write prolog, write text, write spaced-text");
  //COMMAND("print aliases, aliases", "", "<not implemented>");
  //COMMAND("print arc-tally, arc-tally", "", "<not implemented>");
  // print cmatrix
  COMMAND("print defined, pdefined", "", "prints defined symbols and functions");
  COMMAND("write definition, wdef", "<definition> > <filename>", "write defined network <defined> into file <filename>");
  COMMAND("write definitions, wdefs", "> <filename>", "write all defined networks into file <filename>");
#ifndef _WIN32
  COMMAND("print directory, directory", "","print contents of current directory");
#endif
  COMMAND("write dot, wdot, dot", "", "prints top FSM in Graphviz dot format");
  //COMMAND("print file-info, file-info", "", "<not implemented>");
  //COMMAND("print flags, flags", "", "<not implemented>");
  COMMAND("print labels, labels", "", "print symbol pairs of top FSM");
  //COMMAND("print label-maps, label-maps", "> <filename>", "<not implemented>");
  COMMAND("print label-tally, label-tally", "", "print statistics about symbol pairs in top FSM");
  COMMAND("print list <symbol>", "", "Print the set of symbols that is associated to <symbol>");
  COMMAND("print lists", "", "Print all symbols that are associated to a set of symbols");
  COMMAND("print longest-string, longest-string, pls", "", "print longest input and output string of top FSM");
  COMMAND("print longest-string-size, longest-string-size, plz", "", "print length of longest input and output string of top FSM");
  COMMAND("print lower-words, lower-words", "", "print words on the lower-side of top FSM");
  COMMAND("print name, pname", "", "prints the name of the top FSM");
  COMMAND("print net", "", "prints all information about top FSM");
  //COMMAND("print properties, print props, write properties, write props", "", "<not implemented>");
  COMMAND("print random-lower, random-lower", "", "prints random words from lower side");
  COMMAND("print random-upper, random-upper", "", "prints random words from upper side");
  COMMAND("print random-words, random-words", "", "prints random words from top FSM");
  COMMAND("print shortest-string-size, print shortest-string-length, psss, pssl", "", "prints length of shortest string");
  COMMAND("print shortest-string, shortest-string, pss", "", "prints the shortest string of the top FSM");
  COMMAND("print sigma, sigma", "", "prints the alphabet of the top FSM");
  //COMMAND("print sigma-tally, sigma-tally, sitally, print sigma-word-tally", "", "<not implemented>");
  COMMAND("print size, size", "", "prints size information about top FSM");
  COMMAND("print stack, stack", "", "print info about FSMs in stack");
  COMMAND("print upper-words, upper-words", "", "print words on the upper-side of top FSM");
  COMMAND("print words, words", "", "print words recognized by top FSM");
  COMMAND("prune net, prune", "", "makes top network coaccessible");
  COMMAND("push (defined)", "<name>", "adds a defined FSM to top of stack");
  COMMAND("quit, exit, bye, stop, hyvästi, au revoir, näkemiin, viszlát, auf wiedersehen, has", "", "exit hfst-xfst");
  COMMAND("read lexc", "(<filename>)", "read and compile lexc format file");
  COMMAND("read att", "(<filename>)", "read a file in AT&T FSM format and add to top of stack");
  //COMMAND("read properties, rprops", "", "<not implemented>");
  COMMAND("read prolog, rpl", "<filename>", "reads prolog format file");
  COMMAND("read regex, regex", "", "read a regular expression");
  COMMAND("read spaced-text, rs", "(<filename>)", "compile space-separated words/word-pairs separated by newlines into a FST");
  COMMAND("read text, rt", "(<filename>)", "compile a list of words separated by newlines into an automaton");
  //"read cmatrix <filename>", "read a confusion matrix and associate it with the network on top of the stack"
  COMMAND("remove-optimization, remove-optimisation", "", "convert top network (optimized for fast lookup) to ordinary format");
  COMMAND("reverse net, reverse", "", "reverses top FSM");
  COMMAND("rotate stack, rotate", "", "rotates stack");
  COMMAND("save defined, saved", "<filename>", "save all defined networks to binary file");
  COMMAND("save stack, save, ss", "<filename>", "save stack to binary file");
  COMMAND("set", "<variable> <ON|OFF>", "sets a global variable (see show variables)");
  AMBIGUOUS_COMMAND("show", "show variables, show variable");
  COMMAND("show variables", "", "prints all variable/value pairs");
  COMMAND("show variable, show", "<name>", "show variable/value pair <name>");
  COMMAND("shuffle net, shuffle", "", "asynchronous product on top two FSMs on stack");
  COMMAND("sigma net", "", "Extracts the alphabet and creates a FSM that accepts all single symbols in it");
  COMMAND("source", "<file>", "read and compile script file <file>");
  AMBIGUOUS_COMMAND("sort", "sort net, sort in, sort out");
  COMMAND("sort net, sort", "", "sorts arcs topologically on top FSM");
  COMMAND("sort in", "", "sorts input arcs by sigma numbers on top FSM");
  COMMAND("sort out", "", "sorts output arcs by sigma number on top FSM");
  AMBIGUOUS_COMMAND("substitute", "substitute defined <X> for <Y>, substitute symbol <X> for <Y>");
  COMMAND("substitute defined", "<X> for <Y>", "substitutes defined network X at all arcs containing Y");
  //"substitute label"
  COMMAND("substitute symbol", "<X> for <Y>", "substitutes all occurrences of Y in an arc with X");
  //COMMAND("substring", "", "<not implemented>");
  COMMAND("system", "<cmd>", "execute the system command <cmd>");
  AMBIGUOUS_COMMAND("test", "test unambiguous, test equivalent, test functional, test identity, test lower-universal, "
                    "test upper-universal, test non-null, test null, test lower-bounded, test overlap, test sequential, "
                    "test star-free, test sublanguage, test upper-bounded, test infinitely-ambiguous");
  COMMAND("test unambiguous", "", "test if top FST is unambiguous");
  COMMAND("test equivalent, equivalent, te", "", "test if the top two FSMs are equivalent");
  COMMAND("test functional, functional, tf", "", "test if the top FST is functional (single-valued)");
  COMMAND("test identity, identity, ti", "", "test if top FST represents identity relations only");
  COMMAND("test infinitely-ambiguous", "", "test if top FST is infinitely ambiguous");
  COMMAND("test lower-universal. lower-universal, tlu", "", "test if lower side is ..");
  COMMAND("test upper-universal, upper-universal, tuu", "", "test if upper side is ..");
  COMMAND("test non-null, tnn", "", "test if top machine is not the empty language");
  COMMAND("test null, tnu", "", "test if top machine is the empty language");
  COMMAND("test lower-bounded, lower-bounded, tlb", "", "test if top FSM accepts a finite number of input strings");
  COMMAND("test overlap, overlap, to", "", "test if intersection of two top FSMs is not empty");
  COMMAND("test sequential", "", "tests if top machine is sequential");
  COMMAND("test star-free", "", "test if top FSM is star-free");
  COMMAND("test sublanguage, sublanguage, ts", "", "test if top FSM is a sublanguage of the next FSM in stack");
  COMMAND("test upper-bounded, upper-bounded, tub", "", "test if top FSM produces a finite number of output strings");
  COMMAND("turn stack, turn", "", "turns stack upside down");
  COMMAND("twosided flag-diacritics, tfd", "", "changes flags to always be identity pairs");
  COMMAND("undefine", "<name>", "remove <name> from defined networks");
  COMMAND("unlist <symbol>", "", "disassociates the given symbol from the set that was bound to it");
  COMMAND("union net, union, disjunct", "", "union of top two FSMs");
  COMMAND("upper-side net, upper-side", "", "upper projection of top FSM");
  COMMAND("view net", "", "display top network (if supported)");
  //COMMAND("write spaced-text, wspaced-text", "", "<not implemented>");
  //COMMAND("write text, wt", "", "<not implemented>");
  COMMAND("write prolog, wpl", "(> filename)", "writes top network to prolog format file/stdout");
  COMMAND("write att", "(> <filename>)", "writes top network in AT&T format to file/stdout");
  COMMAND("zero-plus net, zero-plus", "", "Kleene star on top fsm");
  AMBIGUOUS_COMMAND("variable", "variable compose-tristate, variable show-flags, variable obey-flags, variable minimal, "
                    "variable print-pairs, variable print-space, variable print-sigma, variable print-weight, variable quit-on-fail, "
                    "variable recursive-define, variable verbose, variable hopcroft-min, variable med-limit, "
                    "variable med-cutoff, variable att-epsilon");
  COMMAND("variable compose-tristate", "", "use the tristate composition algorithm");
  COMMAND("variable show-flags", "", "show flag diacritics in `apply'");
  COMMAND("variable obey-flags", "", "obey flag diacritics in `apply'");
  COMMAND("variable minimal", "", "ON = minimize resulting FSMs, OFF = remove epsilons and determinize");
  COMMAND("variable print-pairs", "", "always print both sides when applying");
  COMMAND("variable print-space", "", "print spaces between symbols");
  COMMAND("variable print-sigma", "", "print the alphabet when printing network");
  COMMAND("variable print-weight", "", "show weights when printing networks or strings");
  COMMAND("variable quit-on-fail", "", "Abort operations when encountering errors");
  COMMAND("variable recursive-define", "", "Allow recursive definitions");
  COMMAND("variable verbose", "", "Verbosity of interface");
  COMMAND("variable hopcroft-min", "", "ON = Hopcroft minimization, OFF = Brzozowski minimization");
  COMMAND("variable med-limit", "", "the limit on number of matches in apply med");
  COMMAND("variable med-cutoff", "", "the cost limit for terminating a search in apply med");
  COMMAND("variable att-epsilon", "", "the EPSILON symbol when reading/writing AT&T files");

  // If any message was appended.
  return (message != message_at_start);
}

}
}
