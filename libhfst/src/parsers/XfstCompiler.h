// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

//! @file XfstCompiler.h
//!
//! @brief A class that encapsulates compilation of Xerox fst language scripts
//!        expressions into HFST automata.
//!
//!        Xerox fst language is described in Finite state morphology (2004) by
//!        Beesley and Karttunen.
//!
//!        This class is merely a wrapper around lex and yacc functions handling
//!        the parsing.

#ifndef GUARD_XfstCompiler_h
#define GUARD_XfstCompiler_h

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <map>
#include <stack>

#include "HfstTransducer.h"
#include "XreCompiler.h"
#include "LexcCompiler.h"

namespace hfst {
//! @brief hfst::xfst namespace contains all functions needed to parse XFST scritpts
namespace xfst {

  // Used internally in function 'apply_unary_operator'.
  enum UnaryOperation
  { DETERMINIZE_NET, EPSILON_REMOVE_NET, INVERT_NET,
    LOWER_SIDE_NET, UPPER_SIDE_NET, OPTIONAL_NET, ONE_PLUS_NET,
    ZERO_PLUS_NET, REVERSE_NET, MINIMIZE_NET, PRUNE_NET_ };

  // Used internally in function 'apply_binaryoperator(_iteratively)'.
  enum BinaryOperation
  { IGNORE_NET, INTERSECT_NET, COMPOSE_NET, CONCATENATE_NET, MINUS_NET,
    UNION_NET, SHUFFLE_NET, CROSSPRODUCT_NET };

  // Used internally in function 'apply'.
  enum ApplyDirection { APPLY_UP_DIRECTION, APPLY_DOWN_DIRECTION };

  // Used internally
  enum Level { LOWER_LEVEL, UPPER_LEVEL, BOTH_LEVELS };

  enum TestOperation { TEST_SUBLANGUAGE_, TEST_OVERLAP_ };

  typedef std::map<std::string,std::string> StringMap;

//! @brief Xfst compiler contains all the methods and variables a session of
//! XFST script parser needs.
class XfstCompiler
{
  public:
  //! @brief Construct compiler for unknown format transducers.
  XfstCompiler();
  //! @brief Create compiler for @a impl format transducers
  XfstCompiler(hfst::ImplementationType impl);
  //! @brief Destructor.
  ~XfstCompiler();

  //! @brief Add properties from file, one property per line
  //! @todo properties cannot be stored in HFST automata
  XfstCompiler& add_props(FILE* infile);
  //! @brief Add properties from text, one property per line
  //! @todo properties cannot be stored in HFST automata
  XfstCompiler& add_props(const char* indata);

  //! @brief Perform lookdowns on top of the stack, one per line
  //! @todo lookdown is missing from HFST
  XfstCompiler& apply_up(FILE* infile);
  //! @brief Perform lookdowns on top of the stack, one per line
  //! @todo lookdown is missing from HFST
  XfstCompiler& apply_up(const char* indata);
  //! @brief Perform lookups on top of the stack, one per line
  //! @todo lookup is missing from HFST
  XfstCompiler& apply_down(FILE* infile);
  //! @brief Perform lookups on top of the stack, one per line
  //! @todo lookup is missing from HFST
  XfstCompiler& apply_down(const char* indata);
  //! @brief Perform lookmeds on top of the stack, one per line
  //! @todo lookmed is missing from HFST
  XfstCompiler& apply_med(FILE* infile);
  //! @brief Perform lookmeds on top of the stack, one per line
  //! @todo lookmed is missing from HFST
  XfstCompiler& apply_med(const char* indata);

  XfstCompiler& lookup_optimize();

  XfstCompiler& remove_optimization();

  //! @brief Define alias for command sequence
  XfstCompiler& define_alias(const char* name, const char* commands);
  //! @brief Define list by range
  //! @todo lists are not supported by HFST
  //! @todo Unicode ranges are not supported
  XfstCompiler& define_list(const char* name, const char* start, const char* end);
  //! @brief Define list by labels
  //! @todo lists are not supportedd by HFST
  XfstCompiler& define_list(const char* name, const char* list);
  //! @brief Define regex macro
  XfstCompiler& define(const char* name, const char* xre);
  //! @brief Define regex macro
  XfstCompiler& define(const char* name);
  //! @brief Define transducer
  void define(const char* name, HfstTransducer * transducer);
  //! @brief Define regex macro function
  //! @todo Regex parser does not support macro functions
  XfstCompiler& define_function(const char* prototype, const char* xre);
  //! @brief Remove definition
  XfstCompiler& undefine(const char* name_list);
  //! @brief Remove list
  //! @todo HFST does not support lists
  XfstCompiler& unlist(const char* name);

  //! @brief Name top network of stack
  //! @todo HFST automata do not remember their names
  //XfstCompiler& name(const char* name);

  //! @brief Load regex macros from file
  //! @todo Definition names cannot be stored in HFST automata binaries
  XfstCompiler& load_definitions(const char* infilename);

  //! @brief Search help directory
  //! @todo helps have not been written or copied
  XfstCompiler& apropos(const char* text);
  //! @brief Print help topics
  //! @todo helps have not been written or copied
  XfstCompiler& describe(const char* text);

  //! @brief Clear stack
  XfstCompiler& clear();
  //! @brief Pop stack
  XfstCompiler& pop();
  //! @brief Push definition on stack
  XfstCompiler& push(const char* name);
  //! @brief Push last definition on stack
  XfstCompiler& push();
  //! @brief Reverse stack
  XfstCompiler& turn();
  //! @brief Move top of stack to bottom
  XfstCompiler& rotate();
  //! @brief Load stack from file
  XfstCompiler& load_stack(const char* infilename);

  //! @brief Print parts of automaton with epsilon loops
  //! @todo unimplemented yet
  XfstCompiler& collect_epsilon_loops();
  //! @brief Remove unnecessary symbols using ?
  //! @todo HFST does not support ?
  XfstCompiler& compact_sigma();

  //! @brief Eliminate flag diacritic
  //! @todo unimplemented yet
  XfstCompiler& eliminate_flag(const char* name);
  //! @brief Eliminate all flag diacritics
  //! @todo unimplemented yet
  XfstCompiler& eliminate_flags();

  //! @brief Print @a text to stdout
  XfstCompiler& echo(const char* text);
  //! @brief Stop parser, print quit message
  XfstCompiler& quit(const char* message);
  //! @brief Execute @c system()
  XfstCompiler& system(const char* command);

  //! @brief Set variable @c name = @c text
  XfstCompiler& set(const char* name, const char* text);
  //! @brief Set variable @c name = @c number
  XfstCompiler& set(const char* name, unsigned int number);
  //! @brief Get variable \a name.
  std::string get(const char* name);
  //! @brief Show named variable
  XfstCompiler& show(const char* name);
  //! @brief Show all variables
  XfstCompiler& show();

  XfstCompiler& twosided_flags();

  XfstCompiler& test_uni(Level level, bool assertion=false);

  //! @brief Test top transducer in stack for equivalence
  //! @todo tests are not implemented
  XfstCompiler& test_eq(bool assertion=false);
  //! @brief Test top transducer in stack for functionality
  //! @todo tests are not implemented
  XfstCompiler& test_funct(bool assertion=false);
  //! @brief Test top transducer in stack for identity
  //! @todo tests are not implemented
  XfstCompiler& test_id(bool assertion=false);
  //! @brief Test top transducer in stack for upper language boundedness
  //! @todo tests are not implemented
  XfstCompiler& test_upper_bounded(bool assertion=false);
  //! @brief Test top transducer in stack for upper language universality
  //! @todo tests are not implemented
  XfstCompiler& test_upper_uni(bool assertion=false);
  //! @brief Test top transducer in stack for lower language boundedness
  //! @todo tests are not implemented
  XfstCompiler& test_lower_bounded(bool assertion=false);
  //! @brief Test top transducer in stack for lower language universality
  //! @todo tests are not implemented
  XfstCompiler& test_lower_uni(bool assertion=false);
  //! @brief Test top transducer in stack for not emptiness
  //! @todo tests are not implemented
  XfstCompiler& test_nonnull(bool assertion=false);
  //! @brief Test top transducer in stack for emptiness
  //! \a invert_test_result defines whether the result is inverted
  //! (so that 'test_nonnull' can be implemented with the same function).
  //! @todo tests are not implemented
  XfstCompiler& test_null(bool invert_test_result=false, bool assertion=false);
  //! @brief Test top transducer in stack for overlapping
  //! @todo tests are not implemented
  XfstCompiler& test_overlap(bool assertion=false);
  //! @brief Test top transducer in stack for sublanguage
  //! @todo tests are not implemented
  XfstCompiler& test_sublanguage(bool assertion=false);
  //! @brief Test top transducer in stack for unambiguity
  //! @todo tests are not implemented
  XfstCompiler& test_unambiguous(bool assertion=false);
  XfstCompiler& test_infinitely_ambiguous(bool assertion=false);

  XfstCompiler& substitute_named(const char* variable, const char* label);
  //! @brief Substitute all labels @a list by @a target.
  XfstCompiler& substitute_label(const char* list, const char* target);
  //! @brief Substitute all symbols in @a list by @a target.
  XfstCompiler& substitute_symbol(const char* list, const char* target);

  //! @brief Print aliases
  XfstCompiler& print_aliases(std::ostream * oss);
  //! @brief Print arc count for @a level
  XfstCompiler& print_arc_count(const char* level, std::ostream * oss);
  //! @brief Print arc count
  XfstCompiler& print_arc_count(std::ostream * oss);
  //! @brief Print definition
  XfstCompiler& print_defined(std::ostream * oss);
  //! @brief Print directory contents
  XfstCompiler& print_dir(const char* glob, std::ostream * oss);
  //! @brief Print file info
  XfstCompiler& print_file_info(std::ostream * oss);
  //! @brief Print flag diacritics
  XfstCompiler& print_flags(std::ostream * oss);

  XfstCompiler& print_labels(std::ostream * oss, HfstTransducer* tr);

  //! @brief Print labels in network @a name
  XfstCompiler& print_labels(const char* name, std::ostream * oss);
  //! @brief Print labels
  XfstCompiler& print_labels(std::ostream * oss);
  //! @brief Print label mappings
  XfstCompiler& print_labelmaps(std::ostream * oss);
  //! @brief Print label count
  XfstCompiler& print_label_count(std::ostream * oss);
  //! @brief Print list named @a name
  XfstCompiler& print_list(const char* name, std::ostream * oss);
  //! @brief Print all lists
  XfstCompiler& print_list(std::ostream * oss);

  XfstCompiler& shortest_string
    (const hfst::HfstTransducer* transducer, hfst::HfstTwoLevelPaths& paths);

  //! @brief Print shortest string of network
  XfstCompiler& print_shortest_string(std::ostream * oss);
  //! @brief Print length of shortest string
  XfstCompiler& print_shortest_string_size(std::ostream * oss);
  //! @brief Print longest string in network
  XfstCompiler& print_longest_string(std::ostream * oss);
  //! @brief Print length of longest string
  XfstCompiler& print_longest_string_size(std::ostream * oss);
  //! @brief Print strings of lower language
  XfstCompiler& print_lower_words(const char * name, unsigned int number,
                                  std::ostream * oss);
  //! @brief Print random strings of lower language
  XfstCompiler& print_random_lower(const char * name, unsigned int number, std::ostream * oss);
  //! @brief Print astrings of upper language
  XfstCompiler& print_upper_words(const char * name, unsigned int number,
                                  std::ostream * oss);
  //! @brief Print random strings of upper language
  XfstCompiler& print_random_upper(const char * name, unsigned int number, std::ostream * oss);
  //! @brief Print pair strings of language
  XfstCompiler& print_words(const char * name, unsigned int number,
                            std::ostream * oss);
  //! @brief Print random pair strings of language
  XfstCompiler& print_random_words(const char * name, unsigned int number, std::ostream * oss);
  //! @brief Print name of top network
  XfstCompiler& print_name(std::ostream * oss);
  //! @brief View top network
  XfstCompiler& view_net();
  //! @brief Print network
  XfstCompiler& print_net(std::ostream * oss);
  //! @brief Print network named @a name
  XfstCompiler& print_net(const char* name, std::ostream * oss);
  //! @brief Print properties of top network
  XfstCompiler& print_properties(std::ostream * oss);
  //! @brief Print properties of network named @a name
  XfstCompiler& print_properties(const char* name, std::ostream * oss);
  //! @brief Print all symbols of network named @a name
  XfstCompiler& print_sigma(const char* name, std::ostream * oss);
  //! @brief Print all symbols of network
  XfstCompiler& print_sigma(std::ostream * oss, bool prompt=true);
  //! @brief Print nnumber of symbols in network
  XfstCompiler& print_sigma_count(std::ostream * oss);
  //! @brief Print number of paths with all symbols on @a level
  XfstCompiler& print_sigma_word_count(const char* level, std::ostream * oss);
  //! @brief Print number of paths with all symbols
  XfstCompiler& print_sigma_word_count(std::ostream * oss);
  //! @brief Print size of network named @a name
  XfstCompiler& print_size(const char* name, std::ostream * oss);
  //! @brief Print size of top network
  XfstCompiler& print_size(std::ostream * oss);
  //! @brief Print all networks in stack
  XfstCompiler& print_stack(std::ostream * oss);
  //! @brief Save @a name network in dot form in @a outfile
  XfstCompiler& write_dot(const char* name, std::ostream * oss);
  //! @brief Save top networks dot form in @a outfile
  XfstCompiler& write_dot(std::ostream * oss);
  //! @brief Save top networks prolog form in @a outfile
  XfstCompiler& write_prolog(std::ostream * oss);
  //! @brief Save top networks spaced paths form in @a outfile
  XfstCompiler& write_spaced(std::ostream * oss);
  //! @brief Save top networks paths form in @a outfile
  XfstCompiler& write_text(std::ostream * oss);
  //! @brief Save function @a name in @a outfile
  //! @todo HFST does not support function macros in automata
  XfstCompiler& write_function(const char* name, const char* outfilename);
  //! @brief Save definition @a name in @a outfile
  //! @todo HFST does not support saving name of definition in file
  XfstCompiler& write_definition(const char* name, const char* outfilename);
  //! @brief Save all definitions in @a outfile
  //! @todo HFST does not support saving name of definition in file
  XfstCompiler& write_definitions(const char* outfilename);
  //! @brief Save all transducers in stack to @a outfile
  XfstCompiler& write_stack(const char* outfilename);

  //! @brief Read properties from @a infile, one per line
  //! @todo HFST automata do not support properties
  XfstCompiler& read_props(FILE* infile);
  //! @brief Read properties from @a indata, one per line
  //! @todo HFST automata do not support properties
  XfstCompiler& read_props(const char* indata);
  //! @brief Compile file data as one regex and save on stack.
  XfstCompiler& read_regex(FILE* infile);
  //! @brief Compile regex of @a indata and save on stack.
  //! Actually, the function assumes that the function compile_regex has
  //! been called earlier when extracting the portion of input that
  //! constitutes the regex \a indata.
  XfstCompiler& read_regex(const char* indata);
  //! @brief Read prolog form transducer from @a infile
  XfstCompiler& read_prolog(FILE* infile);
  //! @brief Read prolog form transducer from @a indata
  XfstCompiler& read_prolog(const char* indata);
  //! @brief Read spaced form transducer from @a infile
  XfstCompiler& read_spaced_from_file(const char * filename);
  //! @brief Read spaced form transducer from @a indata
  XfstCompiler& read_spaced(const char* indata);
  //! @brief Read text form transducer from @a infile
  XfstCompiler& read_text_from_file(const char * filename);
  //! @brief Read text form transducer from @a indata
  XfstCompiler& read_text(const char* indata);
  //! @brief Read lexicons from @a infile
  XfstCompiler& read_lexc_from_file(const char * filename);
  //! @brief Read lexicons from @a indata
  XfstCompiler& read_lexc(const char* indata);
  //! @brief Read a transducer in att format from file @a filename
  XfstCompiler& read_att_from_file(const char * filename);
  //! @brief Write top transducer in att format to @a outfile
  XfstCompiler& write_att(std::ostream * oss);

  //! @brief do some label pushing
  //! @todo HFST automata cannot push labels
  XfstCompiler& cleanup_net();
  //! @brief Make transducer functional
  //! @todo unimplemented
  XfstCompiler& complete_net();
  //! @brief Compose stack
  XfstCompiler& compose_net();
  //! @brief concatenate stack
  XfstCompiler& concatenate_net();
  //! @brief Crossproduct top of stack
  XfstCompiler& crossproduct_net();
  //! @brief Determinize top of stack
  XfstCompiler& determinize_net();
  //! @brief Remove epsilons from top of stack
  XfstCompiler& epsilon_remove_net();
  //! @brief Ignore top of stack with second automaton
  //! @todo unimplemented
  XfstCompiler& ignore_net();
  //! @brief Intersect stack
  XfstCompiler& intersect_net();
  //! @brief invert top of stack
  XfstCompiler& invert_net();
  //! @brief Make top of stack label network
  //! @todo Find out wtf this is
  XfstCompiler& label_net();
  //! @brief Project input for top of stack
  XfstCompiler& lower_side_net();
  //! @brief Project output for top of stack
  XfstCompiler& upper_side_net();
  //! @brief Minimize top of stack
  XfstCompiler& minimize_net();
  //! @brief Subtract second from top of stack
  XfstCompiler& minus_net();
  //! @brief Name top of stack
  //! @todo HFST automata do not remember their names
  XfstCompiler& name_net(const char* name);
  //! @brief Negate top of stack
  XfstCompiler& negate_net();
  //! @brief Kleene plus top network of stack
  XfstCompiler& one_plus_net();
  //! @brief Kleene star top network of stack
  XfstCompiler& zero_plus_net();
  //! @brief Prune top network of stack
  //! @todo Most of HFST automata are pruned by default?
  XfstCompiler& prune_net();
  //! @brief Reverse top network of the stack
  XfstCompiler& reverse_net();
  //! @brief Shuffle top network with second
  //! @todo unimplemented
  XfstCompiler& shuffle_net();
  //! @brief Sigma top network of stack
  //! @todo Find out wtf this is
  XfstCompiler& sigma_net();
  //! @brief Sort top network of the stack
  //! @todo HFST automata sort or not by default
  XfstCompiler& sort_net();
  //! @brief Substring top network of stack
  //! @todo unimplementedd
  XfstCompiler& substring_net();
  //! @brief Disjunct the stack
  XfstCompiler& union_net();
  //! @brief Interactive network traversal tool
  XfstCompiler& inspect_net();
  //! @brief Repeat 0..1 times
  XfstCompiler& optional_net();

  // internal function
  XfstCompiler& compile_replace_net(Level level);
  //! @brief Compile-replace lower
  XfstCompiler& compile_replace_lower_net();
  //! @brief Compile-replace upper
  XfstCompiler& compile_replace_upper_net();


  /* Compile a regex string starting from \a indata, store the resulting
     transducer to variable XfstCompiler::latest_regex_compiled and
     store the number of characters read from \a indata to \a chars_read.

     This function is used by the xfst lexer to determine where a regex
     starting from \a indata ends. For example, if we have the input

     regex foo:bar ! this is a comment with a semicolon;
     baz;
     print net

     the xfst lexer would read "regex" and then call 'compile_regex' on
     the substring starting right after "regex". 'compile_regex' would
     then read until the semicolon ending the regex, i.e. the one right
     after "baz" and store the resulting transducer to
     'latest_regex_compiled'.

     The xfst lexer could then put back all characters after index
     chars_read and continue lexical analysis there, i.e. from
     "print net".
  */
  XfstCompiler& compile_regex(const char * indata, unsigned int & chars_read);

  //! @brief Sekrit HFST raw command mode!
  XfstCompiler& hfst(const char * data);
  //! @brief Get current stack of compiler
  const std::stack<HfstTransducer*>& get_stack() const;
  //! @brief Parse input from @a infile
  int parse(FILE * infile);
  //! @brief Parse file @a filename
  int parse(const char * filename);
  //! @brief Parse @a line
  int parse_line(char line []);
  //! @brief Parse @a line
  int parse_line(std::string line);
  //! @brief Define whether readline library is used to read input in apply up etc.
  XfstCompiler& setReadline(bool readline);
  //! @brief Define whether input is read from stdin in apply up etc.
  XfstCompiler& setReadInteractiveTextFromStdin(bool value);
  //! @brief Define whether output is printed directly to windows console.
  XfstCompiler& setOutputToConsole(bool value);
  //! @brief Whether readline is used to read input in apply up etc.
  bool getReadline();
  //! @brief Whether stdin is used to read input in apply up etc.
  bool getReadInteractiveTextFromStdin();
  //! @brief Whether output is printed directly to windows console.
  bool getOutputToConsole();
  //! @brief Define wheter prompts and XFST outputs are printed.
  XfstCompiler& setVerbosity(bool verbosity);
  //! @brief Define wheter prompts are printed.
  XfstCompiler& setPromptVerbosity(bool verbosity);
  //! @brief Explicitly print the prompt to stdout.
  const XfstCompiler& prompt();
  //! @brief Get the prompt string.
  char* get_prompt() const;

  //! @brief Allow read and write operations only in current directory, do not allow system calls.
  XfstCompiler& setRestrictedMode(bool value);
  //! @brief Whether restricted mode is on.
  bool getRestrictedMode() const;

  //! @brief Whether it has been requested to quit the program.
  //  Needed in interactive mode where user input is read line by line.
  bool quit_requested() const;
  //! @brief Handle unknown command \a s.
  //  @return Whether the parser should go on, 0 signifying true.
  int unknown_command(const char * s);
  // For xfst parser.
  bool get_fail_flag() const;

  /* Set the stream where error messages and warnings are printed. */
  void set_error_stream(std::ostream & os);
  /* Get the stream where error messages and warnings are printed. */
  std::ostream & get_error_stream();
  /* Set the stream where output is printed. */
  void set_output_stream(std::ostream & os);
  /* Get the stream where output is printed. */
  std::ostream & get_output_stream();

  /* A wrapper around file close function. */
  int xfst_fclose(FILE * f, const char * name);
  /* A wrapper around file open function. */
  FILE * xfst_fopen(const char* path, const char* mode);

  /* The following three functions are wrappers around output and error streams.
     On Unix and Mac, output() and error() just return members output_ and error_,
     and flush(std::ostream *) does nothing. On windows, it is possible that
     output() and error() return an ostringstream which is actually printed when
     flush(std::ostream *) is called.

     This delayed printing is needed because Windows makes a difference between
     standard output and error streams and console output and error streams.
     If output_to_console_ is true and the stream where we are writing is a
     standard stream, we first need to write everything in a ostringstream and
     then call a specific function hfst_fprintf_console that prints the contents
     of the ostringstream to console. */

  /* Get the output stream. */
  std::ostream & output();
  /* Get the error stream. */
  std::ostream & error();
  /* Flush the stream. */
  void flush(std::ostream * oss);

  bool check_filename(const char * filename);

 protected:
  //! @brief Get the prompt that is used when applying up or down
  //! (as specified by \a direction).
  const char* get_apply_prompt(ApplyDirection direction);
  //! @brief Get the print symbol for \a symbol.
  //! @see print_flags
  const char* get_print_symbol(const char* symbol);
  //! @brief Print \a n first paths (or all, if n is negative)
  //! from \a paths to \a outfile.
  bool print_paths(const hfst::HfstTwoLevelPaths &paths,
                   std::ostream * oss = &std::cout, int n=-1);
  //! @brief Print \a n first paths (or all, if n is negative)
  //! from \a paths to \a outfile.
  bool print_paths(const hfst::HfstOneLevelPaths &paths,
                   std::ostream * oss = &std::cout, int n=-1);
  // A method used by function print_longest_string_or_its_size.
  XfstCompiler& print_one_string_or_its_size
    (std::ostream * oss, const HfstTwoLevelPaths & paths, const char * level, bool print_size);
  //! @brief Print the longest string of topmost transducer in the stack
  //! (if print_size is false) or the size of that string (if print_size is true)
  //! to \a outfile.
  XfstCompiler& print_longest_string_or_its_size(std::ostream * oss, bool print_size);
  //! @brief Try to extract a maximum of \a number paths from topmost
  //! transducer in the stack and print them to \a outfile. \a level
  //! defines whether the input or output level is printed or both are printed.
  XfstCompiler& print_words(const char * name, unsigned int number,
                            std::ostream * oss, Level level);
  //! @brief Read strings (with or without spaces between the symbols,
  //! as defined by \a spaces) from \a infile, disjunct them into
  //! a single transducer and push it to the stack.
  XfstCompiler& read_text_or_spaced(const char * filename, bool spaces);

  //! @brief Convert format of \a t read from file \a filename to common
  //! format used by this xfst compiler and print a warning message
  //! about loss of information during conversion, if needed.
  void convert_to_common_format
    (HfstTransducer * t, const char * filename = NULL);

  //! @brief Open HfstInputStream to file \a filename.
  //! Print an error message and return NULL, if not succesful.
  HfstInputStream * open_hfst_input_stream(const char * filename);

  //! @brief Read transducers from file \a infilename and either push
  //! them to the stack (if \a definitions is false) or add them as definitions
  //! (if \a definitions is true).
  //!
  //! Transducers are pushed to the stack in the order they are read from the file.
  //! This is because 'write_stack' writes the transducers starting from the
  //! bottom-most transducer in the stack.
  //!
  //! Definitions are added using the function 'add_loaded_definition'.
  XfstCompiler& load_stack_or_definitions(const char *infilename, bool definitions);

  //! @brief Add a transducer definition with name given by 't.get_name()'
  //! and value \a t.
  //! If a transducer with the same name exists, it is overwritten.
  //! If 't.get_name()' gives an empty string, no definition is added.
  //! In both cases, a warning message is printed.
  XfstCompiler& add_loaded_definition(HfstTransducer * t);

  //! @brief Set fail flag to true if quit-on-fail is ON,
  //! else do nothing.
  void xfst_fail();

  //! @brief Set fail flag to true if quit-on-fail is ON and hfst-xfst
  //! is not used in interactive mode, else do nothing.
  void xfst_lesser_fail();

  //! @brief Print alphabet \a alpha to \a outfile. \a unknown and \a identity
  //! define whether these symbols occur in the transitions of the transducer
  //! whose alphabet we are printing.
  void print_alphabet(const StringSet & alpha, bool unknown, bool identity, std::ostream * oss);

  void print_level
    (const std::vector<unsigned int> & whole_path,
     const std::vector<unsigned int> & shortest_path);

  bool can_level_be_reached(int level, size_t whole_path_length);

  bool can_arc_be_followed(int number, unsigned int number_of_arcs);

  unsigned int print_arcs(const hfst::implementations::HfstBasicTransitions & transitions);

  //! @brief Perform lookup on the top transducer using strings in \a infile.
  //! \a direction specifies whether apply is done on input (up) or output (down)
  //! side. If infile is stdin, interactive mode with prompts is used.
  //! The results are printed to standard output.
  XfstCompiler& apply(FILE* infile, ApplyDirection direction);

  //! @brief Apply \a operation on top transducer in the stack.
  //! If the stack is empty, print a warning.
  XfstCompiler& apply_unary_operation(UnaryOperation operation);
  //! @brief Apply \a operation on two top transducers in the stack.
  //! The top transducers are popped, the operation is applied
  //! (the topmost transducer is the first transducer in the operation),
  //! and the result is pushed to the top of the stack.
  //! If the stack has less than two transducers, print a warning.
  XfstCompiler& apply_binary_operation(BinaryOperation operation);

  //! @brief Apply \a operation on all transducers in the stack.
  //! The top transducer (n1) is popped, the operation is applied iteratively
  //! for all next transducers (n2, n3, n4 ...) in the stack:
  //! [[[n1 OPERATION n2] OPERATION n3] OPERATION n4] ...
  //! popping each of them and the result is pushed to the stack.
  //! If the stack is empty, print a warning.
  XfstCompiler& apply_binary_operation_iteratively(BinaryOperation operation);

  //! @brief Print the result of \a operation when applied to the whole stack.
  XfstCompiler& test_operation(TestOperation operation, bool assertion=false);

  //! @brief The topmost transducer in the stack.
  //! If empty, print a warning message and return NULL.
  HfstTransducer * top();

  //! @brief Get next line from \a file. Return NULL if end of file is reached.
  //! Use \a promptstr as prompt for readline, or print it to stderr if readline is not in use.
  char * xfst_getline(FILE * file, const std::string & promptstr = "");

  //! @brief Remove newline ('\n' and '\r') from the end of \a str.
  char * remove_newline(char * str);

  //! @brief Get current readline history index.
  int current_history_index();

  //! @brief Remove all readline history after \a index.
  void ignore_history_after_index(int index);

  //! @brief Get the precision that is used when printing weights.
  int get_precision();

  private:
  /* */
  const XfstCompiler& error(const char* message) const;
  XfstCompiler& print_transducer_info();
  XfstCompiler& add_prop_line(char* line);

  XfstCompiler& lookup(char* line, const HfstTransducer * t, size_t cutoff);
  XfstCompiler& lookup(char* line, HfstBasicTransducer * t);

  XfstCompiler& apply_up_line(char* line);
  XfstCompiler& apply_down_line(char* line);
  XfstCompiler& apply_med_line(char* line);

  XfstCompiler& print_bool(bool value);

  /* A wrapper around stream objects, see flush(std::ostream *) for more information. */
  std::ostream * get_stream(std::ostream * oss);

  /* Whether readline library is used when reading user input. */
  bool use_readline_;
  /* Whether interactive text is read from standard input. */
  bool read_interactive_text_from_stdin_;
  /* Windows-specific: whether output, error messages and warnings are printed to the console. */
  bool output_to_console_;
  /* The regular expression compiler. */
  hfst::xre::XreCompiler xre_;
  /* The lexc compiler. */
  hfst::lexc::LexcCompiler lexc_;
#if HAVE_TWOLC
  /* The twolc compiler. */
  hfst::twolc::TwolcCompiler twolc_;
#endif
  std::map<std::string,std::string> original_definitions_;
  std::map<std::string,hfst::HfstTransducer*> definitions_;
  std::map<std::string,std::string> original_function_definitions_;
  std::map<std::string,std::string> function_definitions_;
  std::map<std::string,unsigned int> function_arguments_;
  std::stack<hfst::HfstTransducer*> stack_;
  std::map<std::string,hfst::HfstTransducer*> names_;
  std::map<std::string,std::string> aliases_;
  std::map<std::string,std::string> variables_;
  std::map<std::string,std::string> properties_;
  std::map<std::string,std::set<std::string> > lists_;
  hfst::ImplementationType format_;
  bool verbose_;
  bool verbose_prompt_;
  /* The latest regex that has been compiled when 'compile_regex' has been
     called. The xfst lexer often needs to parse regexps in order to determine
     where they end before giving them to the actual parser. By storing the result
     in this variable, there is no need to parse a regexp again on the parse level. */
  hfst::HfstTransducer * latest_regex_compiled;
  // Whether the script has encountered the quit command ('quit', 'exit', etc.).
  // Needed in interactive mode, where user input is read line by line.
  bool quit_requested_;
  // Whether the compiler has encountered an error when compiling input given to
  // 'parse' or 'parse_line' function that should quit the compilation and make
  // the function return a non-zero value. Note that if the variable 'quit-on-fail'
  // is false, fail_flag_ will always be false.
  bool fail_flag_;
  // Where output is printed.
  std::ostream * output_;
  // Where error messages and warnings are printed.
  std::ostream * error_;
#ifdef WINDOWS
  // On Windows: when printing to console standard output, the contents are stored here
  // and actually written when flush(std::ostream *) is called.
  std::ostringstream winoss_stdout_;
  // On Windows: when printing to console standard error, the contents are stored here
  // and actually written when flush(std::ostream *) is called.
  std::ostringstream winoss_stderr_;
#endif
  bool restricted_mode_;
}
;

// ugh, the global
extern XfstCompiler* xfst_;
}}
// vim:set ft=cpp.doxygen:
#endif
