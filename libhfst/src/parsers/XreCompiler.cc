// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

//! @file XreCompiler.cc
//!
//! @brief Functions for building transducers from Xerox regexps

#include "XreCompiler.h"
#include "xre_utils.h"
#include "HfstTransducer.h"

#ifdef WINDOWS
#include "hfst-string-conversions.h"
#endif

#ifndef UNIT_TEST

namespace hfst { namespace xre {

    unsigned int cr=0; // number of chars read from xre input
    unsigned int lr=1; // number of lines read from xre input
    std::set<unsigned int> positions;
    char * position_symbol = NULL;
    std::ostream * error_(&std::cerr);
#ifdef WINDOWS
    std::ostringstream winoss_;
    std::ostream * redirected_stream_ = NULL;
    bool output_to_console_(false);
#endif
    bool verbose_(false);
    std::set<std::string> * defined_multichar_symbols_(NULL);

XreCompiler::XreCompiler() :
    definitions_(),
    function_definitions_(),
    function_arguments_(),
    list_definitions_(),
    format_(hfst::TROPICAL_OPENFST_TYPE),
    verbose_(false)
#ifdef WINDOWS
    , output_to_console_(false)
#endif
{}

XreCompiler::XreCompiler(hfst::ImplementationType impl) :
    definitions_(),
    function_definitions_(),
    function_arguments_(),
    list_definitions_(),
    format_(impl),
    verbose_(false)
#ifdef WINDOWS
    , output_to_console_(false)
#endif
{}

    XreCompiler::XreCompiler(const struct XreConstructorArguments & args) :
    definitions_(args.definitions),
    function_definitions_(args.function_definitions),
    function_arguments_(args.function_arguments),
    list_definitions_(args.list_definitions),
    format_(args.format),
    verbose_(false)
#ifdef WINDOWS
    , output_to_console_(false)
#endif
{}


    void XreCompiler::set_verbosity(bool verbose)
    {
      this->verbose_ = verbose;
      hfst::xre::verbose_ = verbose; // TODO
    }

    bool XreCompiler::get_verbosity()
    {
      return this->verbose_;
    }

    // TODO: get rid of global variables
    void XreCompiler::set_error_stream(std::ostream * os)
    {
      hfst::xre::error_ = os;
    }

    std::ostream * XreCompiler::get_error_stream()
    {
      return hfst::xre::error_;
    }

  XreCompiler&
  XreCompiler::setOutputToConsole(bool value)
  {
#ifdef WINDOWS
    output_to_console_ = value;
    hfst::xre::output_to_console_ = value;
#else
    (void)value;
#endif
    //hfst::print_output_to_console(output_to_console_);
    return *this;
  }

  bool
  XreCompiler::getOutputToConsole()
  {
#ifdef WINDOWs
    return output_to_console_;
#else
    return false;
#endif
  }

    std::ostream * XreCompiler::get_stream(std::ostream * oss)
    {
#ifdef WINDOWS
      if (hfst::xre::output_to_console_ && (oss == &std::cerr || oss == &std::cout))
        {
          hfst::xre::redirected_stream_ = oss;
          return &hfst::xre::winoss_;
        }
#endif
      return oss;
    }

    void XreCompiler::flush(std::ostream * oss)
    {
#ifdef WINDOWS
      if (hfst::xre::output_to_console_ && (oss == &hfst::xre::winoss_))
        {
          if (hfst::xre::redirected_stream_ == &std::cerr)
            hfst_fprintf_console(stderr, hfst::xre::winoss_.str().c_str());
          else if (hfst::xre::redirected_stream_ == &std::cout)
            hfst_fprintf_console(stdout, hfst::xre::winoss_.str().c_str());
          else
            ;
          hfst::xre::redirected_stream_ = NULL;
          hfst::xre::winoss_.str("");
        }
#endif
    }


bool
XreCompiler::define(const std::string& name, const std::string& xre)
{
  HfstTransducer* compiled = compile(xre);
  if (compiled == NULL)
    {
      //fprintf(stderr, "error in XreCompiler::define: xre '%s' could not be parsed, leaving %s undefined\n",
      //        xre.c_str(), name.c_str());
      //*errorstream_ << "error in XreCompiler::define: xre '" << xre << "' could not be parsed, leaving " << name << "undefined" << std::endl;
      if (this->verbose_)
        {
          std::ostream * err = get_stream(get_error_stream());
          *err << "error: could not parse '" << xre << "', leaving '" << name << "' undefined" << std::endl;
          flush(err);
        }
      return false;
    }
  definitions_[name] = compiled;
  return true;
}

void
XreCompiler::define_list(const std::string& name, const std::set<std::string>& symbol_list)
{
  list_definitions_[name] = symbol_list;
}

void
XreCompiler::define(const std::string& name, const HfstTransducer & transducer)
{
  definitions_[name] = new HfstTransducer(transducer);
}

bool
XreCompiler::define_function(const std::string& name,
                             unsigned int arguments,
                             const std::string& xre)
{
  //fprintf(stderr, "XreCompiler: defining function %s(@%i) = [%s]\n", name.c_str(), arguments, xre.c_str()); // DEBUG
  function_arguments_[name] = arguments;
  function_definitions_[name] = xre;
  return true;
}

bool
XreCompiler::is_definition(const std::string & name)
{
  if (definitions_.find(name) != definitions_.end())
    return true;
  return false;
}

bool
XreCompiler::is_function_definition(const std::string & name)
{
  if (function_definitions_.find(name) != function_definitions_.end())
    return true;
  return false;
}

void
XreCompiler::undefine(const std::string& name)
{
if (definitions_.find(name) != definitions_.end())
  {
    definitions_.erase(name);
  }
}

void
XreCompiler::remove_defined_multichar_symbols()
{
  if (defined_multichar_symbols_ != NULL)
    {
      delete defined_multichar_symbols_;
      defined_multichar_symbols_ = NULL;
    }
}

void
XreCompiler::add_defined_multichar_symbol(const std::string & symbol)
{
  if (defined_multichar_symbols_ == NULL)
    {
      defined_multichar_symbols_ = new std::set<std::string>();
    }
  defined_multichar_symbols_->insert(symbol);
}

extern bool expand_definitions;
extern bool harmonize_;
extern bool harmonize_flags_;
    //extern bool verbose_;
    //extern FILE * warning_stream;

void XreCompiler::set_expand_definitions(bool expand)
{
  expand_definitions=expand;
}

void XreCompiler::set_harmonization(bool harmonize)
{
  harmonize_=harmonize;
}

void XreCompiler::set_flag_harmonization(bool harmonize_flags)
{
  harmonize_flags_=harmonize_flags;
}

bool
XreCompiler::contained_only_comments()
{
  return contains_only_comments;
}

HfstTransducer*
XreCompiler::compile(const std::string& xre)
{
  // debug
  //std::cerr << "XreCompiler: " << this << " : compile(\"" << xre << "\")" << std::endl;
  unsigned int cr_before = cr;
  cr = 0;
  try
    {
      HfstTransducer * retval = hfst::xre::compile(xre, definitions_, function_definitions_, function_arguments_, list_definitions_, format_);
      cr = cr_before;
      return retval;
    }
  catch (const char * msg)
    {
      if (strcmp(msg, "Allocation of memory failed in Mem::add_buffer!") == 0) // sfst backend
        HFST_THROW_MESSAGE(HfstException, "Allocation of memory failed in SFST backend.");
      else
        HFST_THROW_MESSAGE(HfstException, msg);
    }
}

HfstTransducer*
XreCompiler::compile_first(const std::string& xre, unsigned int & chars_read)
{
  // debug
  //std::cerr << "XreCompiler: " << this << " : compile_first(\"" << xre << "\"";
  unsigned int cr_before = cr;
  cr = 0;
  try
    {
      HfstTransducer * retval = hfst::xre::compile_first(xre, definitions_, function_definitions_, function_arguments_, list_definitions_, format_, chars_read);
      //std::cerr << ", " << chars_read << ")" << std::endl;
      cr = cr_before;
      return retval;
    }
  catch (const char * msg)
    {
      if (strcmp(msg, "Allocation of memory failed in Mem::add_buffer!") == 0) // sfst backend
        HFST_THROW_MESSAGE(HfstException, "Allocation of memory failed in SFST backend.");
      else
        HFST_THROW_MESSAGE(HfstException, msg);
    }
}

bool XreCompiler::get_positions_of_symbol_in_xre
(const std::string & symbol, const std::string & xre, std::set<unsigned int> & positions_)
{
  position_symbol = strdup(symbol.c_str());
  positions.clear();
  unsigned int cr_before = cr;
  cr = 0;
  HfstTransducer * compiled =
    hfst::xre::compile(xre, definitions_, function_definitions_, function_arguments_, list_definitions_, format_);
  free(position_symbol);
  position_symbol = NULL;
  if (compiled == NULL)
    {
      /*fprintf(stderr, "error in XreCompiler::get_positions_of_symbol_in_xre: xre '%s' "
              "could not be parsed, positions of symbol %s not found\n",
              xre.c_str(), symbol.c_str());*/
      return false;
    }
  positions_ = positions;
  cr = cr_before;
  return true;
}

}}

#else // UNIT_TEST
#include <cstdlib>
#include <cassert>

using namespace hfst;
using namespace hfst::xre;

extern int xredebug;

int
main(int, char**)
  {
    std::cout << "Unit tests for " __FILE__ ":";
    std::cout << std::endl << "constructors: " << std::endl;
    std::cout << " (default)..." << std::endl;;
    XreCompiler defaultXre();
#if HAVE_SFST
    std::cout << " (SFST)..." << std::endl;;
    XreCompiler sfstXre = XreCompiler(SFST_TYPE);
#endif
#if HAVE_OPENFST
    std::cout << " (OpenFst)..." << std::endl;;
    XreCompiler ofstXre = XreCompiler(TROPICAL_OPENFST_TYPE);
#endif
#if HAVE_FOMA
    std::cout << " (Foma)..." << std::endl;;
    XreCompiler fomaXre = XreCompiler(FOMA_TYPE);
#endif
    HfstBasicTransducer basicCat;
    basicCat.add_state(1);
    basicCat.add_state(2);
    basicCat.add_state(3);
    basicCat.add_transition(0, HfstBasicTransition(1, "c", "c", 0));
    basicCat.add_transition(1, HfstBasicTransition(2, "a", "a", 0));
    basicCat.add_transition(2, HfstBasicTransition(3, "t", "t", 0));
    basicCat.set_final_weight(3, 0);
    HfstBasicTransducer basicFight;
    basicFight.add_state(1);
    basicFight.add_state(2);
    basicFight.add_state(3);
    basicFight.add_state(4);
    basicFight.add_state(5);
    basicFight.add_state(6);
    basicFight.add_transition(0, HfstBasicTransition(1, "f", "f", 0));
    basicFight.add_transition(1, HfstBasicTransition(2, "i", "o", 0));
    basicFight.add_transition(2, HfstBasicTransition(3, hfst::internal_epsilon,
                                                     "u", 0));
    basicFight.add_transition(3, HfstBasicTransition(4, "g", "g", 0));
    basicFight.add_transition(4, HfstBasicTransition(5, "h", "h", 0));
    basicFight.add_transition(5, HfstBasicTransition(6, "t", "t", 0));
    basicFight.set_final_weight(6, 0);
    HfstBasicTransducer basicCatOrDog;
    basicCatOrDog.add_state(1);
    basicCatOrDog.add_state(2);
    basicCatOrDog.add_state(3);
    basicCatOrDog.add_transition(0, HfstBasicTransition(1, "c", "c", 0));
    basicCatOrDog.add_transition(1, HfstBasicTransition(2, "a", "a", 0));
    basicCatOrDog.add_transition(2, HfstBasicTransition(3, "t", "t", 0));
    basicCatOrDog.add_transition(0, HfstBasicTransition(3, "dog", "dog", 0));
    basicCatOrDog.set_final_weight(3, 0);
    // <http://sourceforge.net/tracker/
    // ?func=detail&atid=1061990&aid=3468932&group_id=224521>
    HfstBasicTransducer basicAaOrBc;
    basicAaOrBc.add_state(1);
    basicAaOrBc.add_state(2);
    basicAaOrBc.add_state(3);
    basicAaOrBc.add_transition(0, HfstBasicTransition(1, "a", "a", 0));
    basicAaOrBc.add_transition(1, HfstBasicTransition(2, "a", "a", 0));
    basicAaOrBc.add_transition(0, HfstBasicTransition(3, "b", "b", 0));
    basicAaOrBc.add_transition(3, HfstBasicTransition(2, "c", "c", 0));
    basicAaOrBc.set_final_weight(2, 0);
    std::cout << std::endl << "compilation: " << std::endl;;
#if HAVE_SFST
    std::cout << " sfst compile(c a t)..." << std::endl;;
    HfstTransducer* sfstCat = sfstXre.compile("c a t");
    assert(sfstCat != 0);
    assert(sfstCat->compare(HfstTransducer(basicCat, SFST_TYPE)));
    delete sfstCat;
    std::cout << " (f i:o 0:u g h t)..." << std::endl;;
    HfstTransducer* sfstFight = sfstXre.compile("f i:o 0:u g h t");
    assert(sfstFight != 0);
    assert(sfstFight->compare(HfstTransducer(basicFight, SFST_TYPE)));
    delete sfstFight;
    std::cout << " (c a t | dog)..." << std::endl;;
    HfstTransducer* sfstCatOrDog = sfstXre.compile("c a t | dog");
    assert(sfstCatOrDog != 0);
    assert(sfstCatOrDog->compare(HfstTransducer(basicCatOrDog, SFST_TYPE)));
    delete sfstCatOrDog;
    std::cout << " (a a | b c)..." << std::endl;;
    HfstTransducer* sfstAaOrBc = sfstXre.compile("a a | b c");
    assert(sfstAaOrBc != 0);
    assert(sfstAaOrBc->compare(HfstTransducer(basicAaOrBc, SFST_TYPE)));
    delete sfstAaOrBc;
#endif
#if HAVE_OPENFST
    std::cout << " ofst compile(c a t)..." << std::endl;;
    HfstTransducer* ofstCat = ofstXre.compile("c a t");
    assert(ofstCat != 0);
    assert(ofstCat->compare(HfstTransducer(basicCat, TROPICAL_OPENFST_TYPE)));
    delete ofstCat;
    std::cout << " (f i:o 0:u g h t)..." << std::endl;;
    HfstTransducer* ofstFight = ofstXre.compile("f i:o 0:u g h t");
    assert(ofstFight != 0);
    assert(ofstFight->compare(HfstTransducer(basicFight,
                                             TROPICAL_OPENFST_TYPE)));
    delete ofstFight;
    std::cout << " (c a t | dog)..." << std::endl;;
    HfstTransducer* ofstCatOrDog = ofstXre.compile("c a t | dog");
    assert(ofstCatOrDog != 0);
    assert(ofstCatOrDog->compare(HfstTransducer(basicCatOrDog,
                                                TROPICAL_OPENFST_TYPE)));
    delete ofstCatOrDog;
    std::cout << " (a a | b c)..." << std::endl;;
    HfstTransducer* ofstAaOrBc = ofstXre.compile("a a | b c");
    assert(ofstAaOrBc != 0);
    assert(ofstAaOrBc->compare(HfstTransducer(basicAaOrBc,
                                              TROPICAL_OPENFST_TYPE)));
    delete ofstAaOrBc;
#endif
#if HAVE_FOMA
    std::cout << " foma compile(c a t)..." << std::endl;;
    HfstTransducer* fomaCat = fomaXre.compile("c a t");
    assert(fomaCat != 0);
    assert(fomaCat->compare(HfstTransducer(basicCat, FOMA_TYPE)));
    delete fomaCat;
    std::cout << " (f i:o 0:u g h t)..." << std::endl;;
    HfstTransducer* fomaFight = fomaXre.compile("f i:o 0:u g h t");
    assert(fomaFight != 0);
    assert(fomaFight->compare(HfstTransducer(basicFight, FOMA_TYPE)));
    delete fomaFight;
    std::cout << " (c a t | dog)..." << std::endl;;
    HfstTransducer* fomaCatOrDog = fomaXre.compile("c a t | dog");
    assert(fomaCatOrDog != 0);
    assert(fomaCatOrDog->compare(HfstTransducer(basicCatOrDog, FOMA_TYPE)));
    delete fomaCatOrDog;
    std::cout << " (a a | b c)..." << std::endl;;
    HfstTransducer* fomaAaOrBc = fomaXre.compile("a a | b c");
    assert(fomaAaOrBc != 0);
    assert(fomaAaOrBc->compare(HfstTransducer(basicAaOrBc, FOMA_TYPE)));
    delete fomaAaOrBc;
#endif
    std::cout << std::endl << "define:" << std::endl;;
#if HAVE_SFST
    std::cout << " sfst define(vowels, a | e | i | o | u | y)..." << std::endl;;
    sfstXre.define("vowels", "a | e | i | o | u | y");
#endif
#if HAVE_OPENFST
    std::cout << " openfst define(vowels, a | e | i | o | u | y)..." << std::endl;;
    ofstXre.define("vowels", "a | e | i | o | u | y");
#endif
#if HAVE_FOMA
    std::cout << " foma define(vowels, a | e | i | o | u | y)..." << std::endl;;
    fomaXre.define("vowels", "a | e | i | o | u | y");
#endif
    std::cout << "ok." << std::endl;
    return EXIT_SUCCESS;
  }
#endif
