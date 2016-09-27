// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

//! @file LexcCompiler.h
//!
//! @brief Functions for building trie representation of lexc data
//!
//! LexcCompiler contains a finite state compilater for objects that resemble
//! Xerox lexc style lexicons. This compiler is modeled in LexcCompiler class.
//! LexcCompiler tries to be a minimal implementation needed to clone original
//! lexc functionality, it supports only the methods necessary to implement
//! original lexc, e.g. deleting entries from a lexicon during compilation is
//! not implemented.

#ifndef GUARD_LexcCompiler_h
#define GUARD_LexcCompiler_h

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <cstdio>

//#include "HfstTransducer.h"
namespace hfst { class HfstTransducer; }
#include "XreCompiler.h"
#include "../HfstTokenizer.h"
#include "../implementations/HfstBasicTransducer.h"

namespace hfst {
//! @brief Namespace for Xerox LexC related specific functions and classes.
namespace lexc {

//! @brief A compiler holding information contained in lexc style lexicons.
//! A single LexcCompiler can be extended by adding entries to it, but little
//! else can be done with it. It is sufficient to implement clone of lexc.
class LexcCompiler
{
  public:
  //! @brief create a lexc compiler for unspecified transducer format.
  LexcCompiler();

  //! @brief create a lexc compiler with @c impl as transducer format.
  LexcCompiler(hfst::ImplementationType impl);

  //! @brief create a lexc compiler with @c impl as transducer format and @c withFlags
  // as indicator as the trasnducer should be build with or without flags
  LexcCompiler(hfst::ImplementationType impl, bool withFlags, bool alignStrings);

  void reset();

  //! @brief compile lexc description from @c infile into current compiler
  LexcCompiler& parse(FILE* infile);

  //! @brief compile lexc description from file @a filename into current
  //!        compiler.
  LexcCompiler& parse(const char* filename);

  //! @brief set verbosity options.
  //! 0 means quiet, 1 the default and 2 (or bigger) the verbose mode.
  //! When verbose is 2, LexcCompiler will output the messages that Xerox
  //! lexc compiler does.
  LexcCompiler& setVerbosity(unsigned int verbose);

  unsigned int getVerbosity();

  void set_error_stream(std::ostream * os);

  std::ostream * get_error_stream();

  void setOutputToConsole(bool);

  bool getOutputToConsole();

  bool isQuiet();

  std::ostream * get_stream(std::ostream * oss);

  void flush(std::ostream * oss);

  LexcCompiler& setTreatWarningsAsErrors(bool value);

  bool areWarningsTreatedAsErrors();

  LexcCompiler& setAllowMultipleSublexiconDefinitions(bool value);
  
  LexcCompiler& setAlignStrings(bool value);
  
  LexcCompiler& setWithFlags(bool value);

  LexcCompiler& setMinimizeFlags(bool value);

  LexcCompiler& setRenameFlags(bool value);

  //! @brief add @a alphabet to multicharacter symbol set.
  //! These symbolse may be used for regular expression ? for backends that do
  //! not support open alphabets.
  LexcCompiler& addAlphabet(const std::string& alphabet);

  //! @brief add @a lexname to noflaggable lexicon set.
  //! @deprecated experimental, transitional
  LexcCompiler& addNoFlag(const std::string& lexname);

  //! @brief set current processing lexicon name to @a lexicon_name.
  LexcCompiler& setCurrentLexiconName(const std::string& lexicon_name);

  //! @brief add entry defined by a @a entry to current lexicon, pointing to
  //! @a continuation weighing @a weight to current lexicon.
  LexcCompiler& addStringEntry(const std::string& entry,
                               const std::string& continuation,
                               const double weight);

  //! @brief add entry defined by @a upper:@a lower, pointing to
  //! @a continuation weighing @a weight to current lexicon.
  LexcCompiler& addStringPairEntry(const std::string& upper,
                                   const std::string& lower,
                                   const std::string& continuation,
                                   const double weight);

  //! @brief add entry defined by regular expression @a xre, pointing to
  //! @a continuation weighing @a weight to current lexicon.
  LexcCompiler& addXreEntry(const std::string& xre,
                            const std::string& continuation,
                            const double weight);

  //! @brief add macro definition named @a name matching regular expression
  //! @a xre to known xerox regular expressions.
  LexcCompiler& addXreDefinition(const std::string& name,
                                 const std::string& xre);

  //! @brief set start lexicon's name to @a lexicon_name.
  LexcCompiler& setInitialLexiconName(const std::string& lexicon_name);

  //! @brief create final usable version of current lexicons and entries.
  hfst::HfstTransducer* compileLexical();

  //! @brief get trie formed by current string entries
  const std::map<std::string,hfst::HfstTransducer>& getStringTries() const;

  //! @brief get union formed by current regular expression entries
  const std::map<std::string,hfst::HfstTransducer>& getRegexpUnions() const;

  //! @brief check that current morphotax is connected and print anomalies.
  //! Works like xerox lexc, for compatibility.
  const LexcCompiler& printConnectedness(bool & warnings_printed);

  private:
  bool quiet_;
  bool verbose_;
  bool align_strings_;
  bool with_flags_;
  bool minimize_flags_;
  bool rename_flags_;
  bool treat_warnings_as_errors_;
  bool allow_multiple_sublexicon_definitions_;
  std::ostream * error_;
#ifdef WINDOWS
  bool output_to_console_;
  std::ostringstream winoss_;
  std::ostream * redirected_stream_;
#endif

  hfst::ImplementationType format_;
  hfst::HfstTokenizer tokenizer_;
  hfst::xre::XreCompiler xre_;
  std::string initialLexiconName_;
  std::map<std::string,hfst::HfstTransducer*> stringTries_;
  std::map<std::string,HfstBasicTransducer*> stringVectors_;
  HfstBasicTransducer stringsTrie_;



  std::map<std::string,hfst::HfstTransducer*> regexps_;
  std::set<std::string> lexiconNames_;
  std::set<std::string> noFlags_;
  std::set<std::string> continuations_;
  std::string currentLexiconName_;
  size_t totalEntries_;
  size_t currentEntries_;
  bool parseErrors_;
}
;

// ugh, the global
extern LexcCompiler* lexc_;
} }

// vim:set ft=cpp.doxygen:
#endif

