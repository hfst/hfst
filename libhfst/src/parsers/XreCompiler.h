// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

//! @file XreCompiler.h
//!
//! @brief A class that encapsulates compilation of Xerox compatible regular
//!        expressions into HFST automata.
//!
//!        Xerox compatible regular expressions are a dialect of regular
//!        expressions commonly used for two-level finite state morphologies.
//!        The details can be found in Finite state morphology (2004) by
//!        Beesley and Karttunen.
//!
//!        This class is merely a wrapper around lex and yacc functions handling
//!        the parsing.

#ifndef GUARD_XreCompiler_h
#define GUARD_XreCompiler_h

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <cstdio>
#include "../HfstDataTypes.h"

namespace hfst {
//! @brief hfst::xre namespace is used for all functions related to Xerox
//! Regular Expresisions (XRE) parsing.
namespace xre {

  // needed for merge operation
struct XreConstructorArguments
{
  std::map<std::string,hfst::HfstTransducer*> definitions;
  std::map<std::string, std::string> function_definitions;
  std::map<std::string, unsigned int > function_arguments;
  std::map<std::string, std::set<std::string> > list_definitions;
  hfst::ImplementationType format;

  XreConstructorArguments
  (std::map<std::string,hfst::HfstTransducer*> definitions_,
   std::map<std::string, std::string> function_definitions_,
   std::map<std::string, unsigned int > function_arguments_,
   std::map<std::string, std::set<std::string> > list_definitions_,
   hfst::ImplementationType format_)
  {
    definitions = definitions_;
    function_definitions = function_definitions_;
    function_arguments = function_arguments_;
    list_definitions = list_definitions_;
    format = format_;
  }
};

//! @brief A compiler holding information needed to compile XREs.
class XreCompiler
{
  public:
  //! @brief Construct compiler for unknown format transducers.
  XreCompiler();
  //! @brief Create compiler for @a impl format transducers
  XreCompiler(hfst::ImplementationType impl);
  // ...
  XreCompiler(const struct XreConstructorArguments & args);

  //! @brief Add a definition macro.
  //!        Compilers will replace arcs labeled @a name, with the transducer
  //!        defined by @a xre in later phases of compilation.
  //! @return Whether parsing \a xre was succesful.
  bool define(const std::string& name, const std::string& xre);

  void define_list(const std::string& name, const std::set<std::string>& symbol_list);

  //! @brief Add a function macro.
  //!        Compilers will replace call to function \a name with the transducer
  //!        defined by \a xre when the function is called.
  //! @param name       The name of the function. It must end with a left parenthesis '('.
  //! @param arguments  The number of arguments that the function takes.
  //! @param xre        The regex defining the function. Function arguments must be named
  //!                   as '"@name(N@"' where name is \a name (without the left parenthesis)
  //!                   and N the order of the argument.
  //! For example a definition
  //!   define_function("Concat(", 2, " [\"@Concat(1@\" \"@Concat(2@\"] ");
  //! defines a function that calculates the concatenation of its first and second arguments.
  //! A call
  //!   compile.("[ Concat(foo, bar) ];");
  //! then returns a transducer [ foo bar ].
  bool define_function(const std::string& name,
                       unsigned int arguments,
                       const std::string& xre);

  bool is_definition(const std::string& name);
  bool is_function_definition(const std::string& name);

  //! @brief Add a definition macro.
  //!        Compilers will replace arcs labeled @a name, with the transducer
  //!        \a transducer in later phases of compilation.
  void define(const std::string& name, const HfstTransducer & transducer);

  //! @brief Remove a definition macro.
  void undefine(const std::string& name);

  void add_defined_multichar_symbol(const std::string & symbol);
  void remove_defined_multichar_symbols();

  //! @brief Compile a transducer defined by @a xre.
  //!        May return a pointer to @e empty transducer on non-fatal error.
  //!        A null pointer is returned on fatal error, if abort is not called.
  HfstTransducer* compile(const std::string& xre);

  //! @brief Compile a transducer defined by @a xre and set the value of @a
  //!        as the number of characters read from @a xre. The characters after
  //!        the regular expression that was successfully parsed are ignored.
  //!        May return a pointer to @e empty transducer on non-fatal error.
  //!        A null pointer is returned on fatal error, if abort is not called.
  HfstTransducer* compile_first(const std::string& xre, unsigned int & chars_read);

  //! @brief Whether the last regex compiled contained only comments.
  //!
  //! In that case, the last call to compile ot compile_first has returned NULL,
  //! which also signals an error during regex compilation.

  bool contained_only_comments();

  bool get_positions_of_symbol_in_xre
    (const std::string & symbol, const std::string & xre, std::set<unsigned int> & positions);

  //! @brief Whether transducer names defined with function 'define' are expanded.
  //!        Default is false.
  void set_expand_definitions(bool expand);

  //! @brief Whether binary operators harmonize their argument transducers.
  //!        Default is true,
  void set_harmonization(bool harmonize);

  //! @brief Whether composition operator harmonizes the flags of its argument transducers.
  //!        Default is false.
  void set_flag_harmonization(bool harmonize_flags);

  void set_verbosity(bool verbose);
  bool get_verbosity();
  void set_error_stream(std::ostream * os);
  std::ostream * get_error_stream();

  XreCompiler& setOutputToConsole(bool value);
  bool getOutputToConsole();

  // TODO get rid of global variables so these functions can be non-static
  static std::ostream * get_stream(std::ostream * oss);
  static void flush(std::ostream * oss);

  private:
  std::map<std::string,hfst::HfstTransducer*> definitions_;
  std::map<std::string, std::string> function_definitions_;
  std::map<std::string, unsigned int > function_arguments_;
  std::map<std::string, std::set<std::string> > list_definitions_;
  hfst::ImplementationType format_;
  bool verbose_;
#ifdef WINDOWS
  bool output_to_console_;
  // global std::ostringstream * winoss_;
  // global std::ostream * redirected_stream_;
#endif

}
;
}}
// vim:set ft=cpp.doxygen:
#endif


