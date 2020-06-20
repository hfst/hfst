// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

/*  @file HfstTransducer.cc
    \brief Implementations of functions declared in file HfstTransducer.h

    The implementations call backend implementations that are declared in
    files in the directory implementations. */

#include <string>
#include <map>
#include <cassert>

using std::string;
using std::map;

#include "HfstTransducer.h"
#include "HfstFlagDiacritics.h"
#include "HfstExceptionDefs.h"
#include "implementations/compose_intersect/ComposeIntersectLexicon.h"

using hfst::implementations::ConversionFunctions;

#ifndef MAIN_TEST

namespace hfst
{

  // -----------------------------------------------------------------------
  //
  //     Interfaces through which the backend implementations are called
  //
  // -----------------------------------------------------------------------

#if HAVE_SFST
  hfst::implementations::SfstTransducer HfstTransducer::sfst_interface;
#endif
#if HAVE_OPENFST
  hfst::implementations::TropicalWeightTransducer
  HfstTransducer::tropical_ofst_interface;
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
  hfst::implementations::LogWeightTransducer
  HfstTransducer::log_ofst_interface;
#endif
#endif
#if HAVE_FOMA
  hfst::implementations::FomaTransducer HfstTransducer::foma_interface;
#endif
  hfst::implementations::HfstOlTransducer HfstTransducer::hfst_ol_interface;
  /* Add here the interface between HFST and your transducer library. */
  //#if HAVE_MY_TRANSDUCER_LIBRARY
  //hfst::implementations::MyTransducerLibraryTransducer
  // HfstTransducer::my_transducer_library_interface;
  //#endif


  // -----------------------------------------------------------------------
  //
  //                   Testing and optimization functions
  //
  // -----------------------------------------------------------------------

#if HAVE_XFSM
  void initialize_xfsm()
  {
    XfsmTransducer::initialize_xfsm();
  }

  InitializeXfsm::InitializeXfsm()
  {
    initialize_xfsm();
  }

  InitializeXfsm dummy;
#endif

  /* The default minimization algorithm if Hopcroft. */
  MinimizationAlgorithm minimization_algorithm=HOPCROFT;
  /* By default, we do not minimize transducers that are already minimal.
     This variable is for debugging and profiling. */
  bool minimize_even_if_already_minimal=false;
  /* By default, weights are not encoded in minimization. */
  bool encode_weights=false;
  /* Allow minimization of intermediary results, used in some more complex functions.
     A false value indicates that only epsilon removal and determinization is allowed. */
  bool can_minimize=true;
  /* By default, harmonization is optimized. */
  bool harmonize_smaller=true;
  /* By default, unknown symbols are used. */
  bool unknown_symbols_in_use=true;

  /* Xerox-style composition where flag diacritics match unknowns and identities. */
  bool xerox_composition=false;
  /* Xerox option where flag diacritic are treated as epsilons in composition. */
  bool flag_is_epsilon_in_composition=false;

  void set_xerox_composition(bool value) {
    xerox_composition=value;
  }

  bool get_xerox_composition() {
    return xerox_composition;
  }

  void set_minimization(bool value) {
    can_minimize=value;
  }

  bool get_minimization() {
    return can_minimize;
  }

  void set_minimize_even_if_already_minimal(bool value) {
    minimize_even_if_already_minimal=value;
#if HAVE_XFSM
    XfsmTransducer::set_minimize_even_if_already_minimal(value);
#endif
  }

  bool get_minimize_even_if_already_minimal() {
    return minimize_even_if_already_minimal;
  }

  void set_flag_is_epsilon_in_composition(bool value) {
    flag_is_epsilon_in_composition=value;
#if HAVE_XFSM
    XfsmTransducer::set_compose_flag_as_special(value);
#endif
  }

  bool get_flag_is_epsilon_in_composition() {
    return flag_is_epsilon_in_composition;
  }


void set_harmonize_smaller(bool value) {
    harmonize_smaller=value; }

bool get_harmonize_smaller(void) {
    return harmonize_smaller; }

void set_encode_weights(bool value) {
  encode_weights=value; }

  bool get_encode_weights(void) {
    return encode_weights; }

  void set_warning_stream(std::ostream * os)
  {
#if HAVE_OPENFST
    hfst::implementations::TropicalWeightTransducer::set_warning_stream(os);
#else
    (void)os;
#endif
  }

  std::ostream * get_warning_stream()
  {
#if HAVE_OPENFST
    return hfst::implementations::TropicalWeightTransducer::get_warning_stream();
#else
    HFST_THROW_MESSAGE(FunctionNotImplementedException, "get_warning_stream");
#endif
  }

void set_minimization_algorithm(MinimizationAlgorithm a) {
    minimization_algorithm=a;
#if HAVE_SFST
    if (minimization_algorithm == HOPCROFT)
    hfst::implementations::sfst_set_hopcroft(true);
    else
    hfst::implementations::sfst_set_hopcroft(false);
#endif
#if HAVE_OPENFST
    if (minimization_algorithm == HOPCROFT)
    hfst::implementations::openfst_tropical_set_hopcroft(true);
    else
    hfst::implementations::openfst_tropical_set_hopcroft(false);
#if HAVE_OPENFST_LOG
    if (minimization_algorithm == HOPCROFT)
    hfst::implementations::openfst_log_set_hopcroft(true);
    else
    hfst::implementations::openfst_log_set_hopcroft(false);
#endif
#endif
   // in foma, Hopcroft is always used
}

MinimizationAlgorithm get_minimization_algorithm() {
    return minimization_algorithm; }

void set_unknown_symbols_in_use(bool value) {
    unknown_symbols_in_use=value; }

bool get_unknown_symbols_in_use() {
    return unknown_symbols_in_use; }

float HfstTransducer::get_profile_seconds(ImplementationType type)
{
#if HAVE_SFST
    if (type == SFST_TYPE)
    return sfst_interface.get_profile_seconds();
#endif
#if HAVE_OPENFST
    if (type == TROPICAL_OPENFST_TYPE)
    return tropical_ofst_interface.get_profile_seconds();
#endif
    return 0;
}


// -----------------------------------------------------------------------
//
//                   Alphabet and harmonization
//
// -----------------------------------------------------------------------

// used only for SFST_TYPE
StringPairSet HfstTransducer::get_symbol_pairs()
{
#if HAVE_SFST
    if (this->type == SFST_TYPE)
    return sfst_interface.get_symbol_pairs(this->implementation.sfst);
    else
#endif
    HFST_THROW_MESSAGE(FunctionNotImplementedException, "get_symbol_pairs");
}

void HfstTransducer::insert_to_alphabet(const std::string &symbol)
{
    HfstTokenizer::check_utf8_correctness(symbol);

    if (symbol == "")
      HFST_THROW_MESSAGE(EmptyStringException, "insert_to_alphabet");

#if HAVE_HFSTOL
    if (this->type == HFST_OL_TYPE || this->type == HFST_OLW_TYPE) {
        this->implementation.hfst_ol->include_symbol_in_alphabet(symbol);
        return;
    }
#endif
    if (this->type != XFSM_TYPE)
      {
        hfst::implementations::HfstBasicTransducer * net
          = convert_to_basic_transducer();
        net->add_symbol_to_alphabet(symbol);
        convert_to_hfst_transducer(net);
      } else {
#if HAVE_XFSM
        this->xfsm_interface.add_symbol_to_alphabet(this->implementation.xfsm, symbol);
#else
        throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, XFSM_TYPE);
#endif
      }
}

void HfstTransducer::insert_to_alphabet(const std::set<std::string> &symbols)
{
    for (std::set<std::string>::const_iterator it = symbols.begin();
         it != symbols.end(); it++)
      {
        HfstTokenizer::check_utf8_correctness(*it);
        if (*it == "")
          { HFST_THROW_MESSAGE(EmptyStringException, "insert_to_alphabet"); }
      }

    if (this->type != XFSM_TYPE)
      {
        hfst::implementations::HfstBasicTransducer * net
          = convert_to_basic_transducer();
        net->add_symbols_to_alphabet(symbols);
        convert_to_hfst_transducer(net);
      }
    else
      {
#if HAVE_XFSM
        this->xfsm_interface.add_symbols_to_alphabet(this->implementation.xfsm, symbols);
#else
        throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, XFSM_TYPE);
#endif
      }
}



void HfstTransducer::remove_from_alphabet(const std::string &symbol)
{

  HfstTokenizer::check_utf8_correctness(symbol);

  if (symbol == "")
    HFST_THROW_MESSAGE(EmptyStringException, "remove_from_alphabet");

    hfst::implementations::HfstBasicTransducer * net
      = convert_to_basic_transducer();
    net->remove_symbol_from_alphabet(symbol);
    convert_to_hfst_transducer(net);
}


void HfstTransducer::remove_from_alphabet(const std::set<std::string> &symbols)
{
  for (std::set<std::string>::const_iterator it = symbols.begin();
       it != symbols.end(); it++)
    {
      this->remove_from_alphabet(*it);
    }
}
  
/* Implemented for XFSM_TYPE, as conversion between HfstBasicFormat and XFSM_TYPE is slow. */
void HfstTransducer::remove_symbols_from_alphabet(const StringSet & symbols)
{
  if (this->type != XFSM_TYPE)
    HFST_THROW_MESSAGE(FunctionNotImplementedException, "remove_symbols_from_alphabet");
#if HAVE_XFSM
  this->xfsm_interface.remove_symbols_from_alphabet(this->implementation.xfsm, symbols);
#endif
  (void)symbols;
}


HfstTransducer &HfstTransducer::prune_alphabet(bool force)
{
  hfst::implementations::HfstBasicTransducer * net
    = convert_to_basic_transducer();
  net->prune_alphabet(force);
  return convert_to_hfst_transducer(net);;
}

StringSet HfstTransducer::get_initial_input_symbols() const
{
  switch(type)
    {
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
      return tropical_ofst_interface.get_initial_input_symbols
	(implementation.tropical_ofst);
#endif
    default:
      HFST_THROW_MESSAGE(FunctionNotImplementedException, "get_first_input_symbols");
    }
}

StringSet HfstTransducer::get_first_input_symbols() const
{
    switch(type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        HFST_THROW_MESSAGE(FunctionNotImplementedException, "get_first_input_symbols");
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        return tropical_ofst_interface.get_first_input_symbols
        (implementation.tropical_ofst);
    case LOG_OPENFST_TYPE:
        HFST_THROW_MESSAGE(FunctionNotImplementedException, "get_first_input_symbols");
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        HFST_THROW_MESSAGE(FunctionNotImplementedException, "get_first_input_symbols");
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
        HFST_THROW_MESSAGE(FunctionNotImplementedException, "get_first_input_symbols");
#endif
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    case HFST_OL_TYPE:
    case HFST_OLW_TYPE:
        HFST_THROW_MESSAGE(FunctionNotImplementedException, "get_first_input_symbols");
    default:
        HFST_THROW_MESSAGE(FunctionNotImplementedException, "get_first_input_symbols");
    }
}

StringSet HfstTransducer::get_alphabet() const
{
    switch(type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        return sfst_interface.get_alphabet(implementation.sfst);
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        return tropical_ofst_interface.get_alphabet
        (implementation.tropical_ofst);
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        return log_ofst_interface.get_alphabet(implementation.log_ofst);
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        return foma_interface.get_alphabet(implementation.foma);
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
      return xfsm_interface.get_alphabet(implementation.xfsm);
#endif
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    case HFST_OL_TYPE:
    case HFST_OLW_TYPE:
        return hfst_ol_interface.get_alphabet(implementation.hfst_ol);
    default:
        HFST_THROW_MESSAGE(FunctionNotImplementedException, "get_alphabet");
    }
}

/*
  Only harmonize number-to-symbol-encodings.
  \a another is not modifed, but a modifed copy of it is returned.
*/
HfstTransducer * HfstTransducer::harmonize_symbol_encodings(const HfstTransducer &another)
{
  HfstBasicTransducer another_basic(another);
  HfstBasicTransducer this_basic(*this);
  *this = HfstTransducer(this_basic, this->get_type());
  return new HfstTransducer(another_basic, another.get_type());
}

/*
   Harmonize this transducer with a copy of another.
   another is not modifed, but a modified copy of it is returned.
   Flag diacritics from the alphabet of this transducer are inserted
   to the alphabet of the copy of another, so that they are excluded
   from harmonization.
   If foma is used as implementation type, no harmonization is carried out,
   as foma's functions take care of harmonization. Then NULL is returned.
*/
HfstTransducer * HfstTransducer::harmonize_(const HfstTransducer &another)
{
  using namespace implementations;
    if (this->type != another.type) {
        HFST_THROW(TransducerTypeMismatchException); }

    if (this->anonymous && another.anonymous) {
      HFST_THROW_MESSAGE
    (HfstFatalException, "harmonize_ with anonymous transducers"); }

    HfstTransducer another_copy(another);

    // Prevent flag diacritics from being harmonized by inserting them to
    // the alphabet. FIX?: remove them at the end?
    if (this->get_type() == FOMA_TYPE)
      {
    StringSet this_alphabet    = this->get_alphabet();
    StringSet another_alphabet = another_copy.get_alphabet();
    StringSet add_to_this;
    StringSet add_to_another;

    for (StringSet::const_iterator it = another_alphabet.begin();
     it != another_alphabet.end();
     ++it)
      {
    if (FdOperation::is_diacritic(*it) && this_alphabet.count(*it) == 0)
      {
        add_to_this.insert(*it);
      }
      }
    this->insert_to_alphabet(add_to_this);

    for (StringSet::const_iterator it = this_alphabet.begin();
     it != this_alphabet.end();
     ++it)
      {
    if (FdOperation::is_diacritic(*it) && another_alphabet.count(*it) == 0)
      {
        add_to_another.insert(*it);
      }
      }
    another_copy.insert_to_alphabet(add_to_another);
      }

    switch(this->type)
    {
#if HAVE_FOMA
    case (FOMA_TYPE):
      // no need to harmonize as foma's functions take care of harmonizing
      return new HfstTransducer(another_copy);
      return NULL;
      break;
#endif // HAVE_FOMA
#if HAVE_XFSM
    case (XFSM_TYPE):
      // no need to harmonize as xfsm's functions take care of harmonizing
      return NULL;
      break;
#endif // HAVE_XFSM
#if HAVE_SFST || HAVE_OPENFST
    case (SFST_TYPE):
    case (TROPICAL_OPENFST_TYPE):
#if HAVE_OPENFST_LOG
    case (LOG_OPENFST_TYPE):
#endif
      {
    HfstBasicTransducer * another_basic = another_copy.get_basic_transducer();
    HfstBasicTransducer * this_basic = this->convert_to_basic_transducer();

    this_basic->harmonize(*another_basic);

    this->convert_to_hfst_transducer(this_basic);
    HfstTransducer * another_harmonized
      = new HfstTransducer(*another_basic, this->type);
    delete another_basic;

    return another_harmonized;
    break;
      }
#endif
    case (ERROR_TYPE):
    default:
        HFST_THROW(TransducerHasWrongTypeException);
    }
    return NULL; // make compiler happy
}

/*  Harmonize symbol-to-number encodings and expand unknown and
    identity symbols.

    In the case of foma transducers, does nothing because foma's own functions
    take care of harmonizing. If harmonization is needed,
    FomaTransducer::harmonize can be used instead. */
  void HfstTransducer::harmonize(HfstTransducer &another, bool force/*=false*/)
{
  using namespace implementations;
    if (this->type != another.type) {
        HFST_THROW(TransducerTypeMismatchException); }

    if (this->anonymous && another.anonymous) {
    return; }

    // Prevent flag diacritics from being harmonized by inserting them to
    // the alphabet.
    StringSet this_alphabet    = this->get_alphabet();
    StringSet another_alphabet = another.get_alphabet();

    for (StringSet::const_iterator it = another_alphabet.begin();
     it != another_alphabet.end();
     ++it)
      {
    if (FdOperation::is_diacritic(*it) && this_alphabet.count(*it) == 0)
      {
        this->insert_to_alphabet(*it);
      }
      }

    for (StringSet::const_iterator it = this_alphabet.begin();
     it != this_alphabet.end();
     ++it)
      {
    if (FdOperation::is_diacritic(*it) && another_alphabet.count(*it) == 0)
      {
        another.insert_to_alphabet(*it);
      }
      }

    switch(this->type)
    {
#if HAVE_FOMA
    case (FOMA_TYPE):
        // no need to harmonize as foma's functions take care of harmonizing
      if (!force)
        return;
#endif // HAVE_FOMA
#if HAVE_XFSM
    case (XFSM_TYPE):
        // no need to harmonize as xfsm's functions take care of harmonizing
        break;
#endif // HAVE_XFSM
#if HAVE_SFST || HAVE_OPENFST
    case (SFST_TYPE):
    case (TROPICAL_OPENFST_TYPE):
#if HAVE_OPENFST_LOG
    case (LOG_OPENFST_TYPE):
#endif
      {
    HfstBasicTransducer * this_basic = this->convert_to_basic_transducer();
    HfstBasicTransducer * another_basic =
      another.convert_to_basic_transducer();

    this_basic->harmonize(*another_basic);

    this->convert_to_hfst_transducer(this_basic);
    another.convert_to_hfst_transducer(another_basic);
    break;
      }
#endif
    case (ERROR_TYPE):
    default:
        HFST_THROW(TransducerHasWrongTypeException);
    }
}

// test function
void HfstTransducer::print_alphabet()
{
#if HAVE_SFST
    if (this->type == SFST_TYPE)
      this->sfst_interface.print_alphabet(this->implementation.sfst);
#endif
#if HAVE_OPENFST
    if (this->type == TROPICAL_OPENFST_TYPE)
      this->tropical_ofst_interface.print_alphabet
    (this->implementation.tropical_ofst);
#endif
#if HAVE_FOMA
    if (this->type == FOMA_TYPE)
      {
    HfstBasicTransducer net(*this);
    net.print_alphabet();
      }
#endif
#if HAVE_XFSM
    if (this->type == XFSM_TYPE)
      {
        HFST_THROW_MESSAGE(FunctionNotImplementedException, "print_alphabet");
      }
#endif

    return;
}



// -----------------------------------------------------------------------
//
//                        Lookup functions
//
// -----------------------------------------------------------------------

HfstOneLevelPaths * HfstTransducer::lookup(const StringVector& s,
                                           ssize_t limit, double time_cutoff) const
{
    return lookup_fd(s, limit, time_cutoff);
}

HfstOneLevelPaths * HfstTransducer::lookup(const std::string & s,
                                           ssize_t limit, double time_cutoff) const
{
    return lookup_fd(s, limit, time_cutoff);
}

HfstTwoLevelPaths * HfstTransducer::lookup_pairs(const std::string & s,
                                                 ssize_t limit, double time_cutoff) const
{
    switch(this->type) {
    case (HFST_OL_TYPE):
    case (HFST_OLW_TYPE):
        return this->implementation.hfst_ol->lookup_fd_pairs(s, limit, time_cutoff);
    default:
      HFST_THROW(FunctionNotImplementedException);
    }
}

HfstOneLevelPaths * HfstTransducer::lookup_fd(const StringVector& s,
                          ssize_t limit, double time_cutoff) const
{
    switch(this->type) {

    case (HFST_OL_TYPE):
    case (HFST_OLW_TYPE):
        return this->implementation.hfst_ol->lookup_fd(s, limit, time_cutoff);

    case (ERROR_TYPE):
      HFST_THROW(TransducerHasWrongTypeException);
    default:
      HFST_THROW(FunctionNotImplementedException);
    }
}

HfstOneLevelPaths * HfstTransducer::lookup_fd(const std::string & s,
                         ssize_t limit, double time_cutoff) const
{
    switch(this->type) {

    case (HFST_OL_TYPE):
    case (HFST_OLW_TYPE):
        return this->implementation.hfst_ol->lookup_fd(s, limit, time_cutoff);

    case (ERROR_TYPE):
      HFST_THROW(TransducerHasWrongTypeException);
    default:
      HFST_THROW(FunctionNotImplementedException);

    }
}

HfstOneLevelPaths * HfstTransducer::lookup(const HfstTokenizer& tok,
                       const std::string &s,
                       ssize_t limit, double time_cutoff) const
{
    StringVector sv = tok.tokenize_one_level(s);
    return lookup(sv, limit, time_cutoff);
}

HfstOneLevelPaths * HfstTransducer::lookdown(const StringVector& s,
                         ssize_t limit) const {
    (void)s;
    (void)limit;
    HFST_THROW(FunctionNotImplementedException);
}

HfstOneLevelPaths * HfstTransducer::lookdown_fd(StringVector& s,
                        ssize_t limit) const {
    (void)s;
    (void)limit;
    HFST_THROW(FunctionNotImplementedException);
}

  HfstOneLevelPaths * HfstTransducer::lookdown(const std::string& s,
                         ssize_t limit) const {
    (void)s;
    (void)limit;
    HFST_THROW(FunctionNotImplementedException);
}

  HfstOneLevelPaths * HfstTransducer::lookdown_fd(const std::string& s,
                        ssize_t limit) const {
    (void)s;
    (void)limit;
    HFST_THROW(FunctionNotImplementedException);
}


bool HfstTransducer::is_lookup_infinitely_ambiguous(const StringVector& s)
    const {
    switch(this->type) {
    /* TODO: Convert into HFST_OL(W)_TYPE, if needed. */
    case (HFST_OL_TYPE):
    case (HFST_OLW_TYPE):
    return this->implementation.hfst_ol->
        is_lookup_infinitely_ambiguous(s);
    default:
    (void)s;
    HFST_THROW(FunctionNotImplementedException);
    }
}

bool HfstTransducer::is_lookup_infinitely_ambiguous(const std::string & s)
    const {
    switch(this->type) {
    /* TODO: Convert into HFST_OL(W)_TYPE, if needed. */
    case (HFST_OL_TYPE):
    case (HFST_OLW_TYPE):
    return this->implementation.hfst_ol->
        is_lookup_infinitely_ambiguous(s);
    default:
    (void)s;
    HFST_THROW(FunctionNotImplementedException);
    }
}

bool HfstTransducer::is_lookdown_infinitely_ambiguous
(const StringVector& s) const {
    (void)s;
    HFST_THROW(FunctionNotImplementedException);
}

bool HfstTransducer::is_infinitely_ambiguous()
    const {
    switch(this->type) {
    case (HFST_OL_TYPE):
    case (HFST_OLW_TYPE):
      return this->implementation.hfst_ol->
        is_infinitely_ambiguous();
    case (ERROR_TYPE):
      HFST_THROW(TransducerHasWrongTypeException);
    default:
      hfst::implementations::HfstBasicTransducer net(*this);
      return net.is_infinitely_ambiguous();
    }
}



// -----------------------------------------------------------------------
//
//              Transducer constructors and destructor
//
// -----------------------------------------------------------------------

HfstTransducer::HfstTransducer():
    type(UNSPECIFIED_TYPE),anonymous(false),is_trie(true), name("")
{}


HfstTransducer::HfstTransducer(ImplementationType type):
    type(type),anonymous(false),is_trie(true), name("")
{
    if (! is_implementation_type_available(type))
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);

    switch (type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        implementation.sfst = sfst_interface.create_empty_transducer();
        break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst =
        tropical_ofst_interface.create_empty_transducer();
        this->type = TROPICAL_OPENFST_TYPE;
        break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        implementation.log_ofst =
        log_ofst_interface.create_empty_transducer();
        break;
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        implementation.foma = foma_interface.create_empty_transducer();
        break;
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
        implementation.xfsm = xfsm_interface.create_empty_transducer();
        break;
#endif
        /* Add here your implementation. */
        //#if HAVE_MY_TRANSDUCER_LIBRARY
    //case MY_TRANSDUCER_LIBRARY_TYPE:
        //implementation.my_transducer_library
        //  = my_transducer_library_interface.create_empty_transducer();
        //break;
        //#endif
    case HFST_OL_TYPE:
    case HFST_OLW_TYPE:
        implementation.hfst_ol = hfst_ol_interface.create_empty_transducer
        (type==HFST_OLW_TYPE?true:false);
        break;
    case ERROR_TYPE:
        HFST_THROW(SpecifiedTypeRequiredException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }
}


HfstTransducer::HfstTransducer(const std::string& utf8_str,
                   const HfstTokenizer
                   &multichar_symbol_tokenizer,
                   ImplementationType type):
    type(type),anonymous(false),is_trie(true), name("")
{
    if (! is_implementation_type_available(type))
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);

    if (utf8_str == "")
      HFST_THROW_MESSAGE
    (EmptyStringException,
     "HfstTransducer(const std::string&, const HfstTokenizer&,"
     " ImplementationType)");

    StringPairVector spv =
    multichar_symbol_tokenizer.tokenize(utf8_str);
    switch (type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        implementation.sfst = sfst_interface.define_transducer(spv);
        break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst =
        tropical_ofst_interface.define_transducer(spv);
        this->type = TROPICAL_OPENFST_TYPE;
        break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        implementation.log_ofst =
        log_ofst_interface.define_transducer(spv);
        break;
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        implementation.foma =
        foma_interface.define_transducer(spv);
        break;
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
        implementation.xfsm =
        xfsm_interface.define_transducer(spv);
        break;
#endif
    case ERROR_TYPE:
        HFST_THROW(SpecifiedTypeRequiredException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }
}

HfstTransducer::HfstTransducer(const StringVector & sv,
                               ImplementationType type):
    type(type), anonymous(false), is_trie(false), name("")
{
  StringPairVector spv;
  for (StringVector::const_iterator it = sv.begin(); it != sv.end(); it++)
    {
      spv.push_back(StringPair(*it, *it));
    }
  *this = HfstTransducer(spv, type);
}

HfstTransducer::HfstTransducer(const StringPairVector & spv,
                   ImplementationType type):
    type(type), anonymous(false), is_trie(false), name("")
{
    if (! is_implementation_type_available(type))
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);
    
    for (StringPairVector::const_iterator it = spv.begin();
     it != spv.end(); it++)
      {
    if (it->first == "" || it->second == "")
      HFST_THROW_MESSAGE
        (EmptyStringException,
         "HfstTransducer(const StringPairVector&, ImplementationType)");
      }

    switch (type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        implementation.sfst = sfst_interface.define_transducer(spv);
        this->type = SFST_TYPE;
        break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst =
        tropical_ofst_interface.define_transducer(spv);
        this->type = TROPICAL_OPENFST_TYPE;
        break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        implementation.log_ofst =
        log_ofst_interface.define_transducer(spv);
        this->type = LOG_OPENFST_TYPE;
        break;
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        implementation.foma =
        foma_interface.define_transducer(spv);
        this->type = FOMA_TYPE;
        break;
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
        implementation.xfsm =
        xfsm_interface.define_transducer(spv);
        this->type = XFSM_TYPE;
        break;
#endif
    case ERROR_TYPE:
        HFST_THROW(SpecifiedTypeRequiredException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }
}

HfstTransducer::HfstTransducer(const StringPairSet & sps,
                   ImplementationType type,
                   bool cyclic):
    type(type),anonymous(false),is_trie(false), name("")
{
    if (! is_implementation_type_available(type))
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);

    for (StringPairSet::const_iterator it = sps.begin();
     it != sps.end(); it++)
      {
    if (it->first == "" || it->second == "")
      HFST_THROW_MESSAGE
        (EmptyStringException,
         "HfstTransducer(const StringPairSet&, ImplementationType, bool)");
      }

    switch (type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        implementation.sfst = sfst_interface.define_transducer(sps,cyclic);
        this->type = SFST_TYPE;
        break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst =
        tropical_ofst_interface.define_transducer(sps,cyclic);
        this->type = TROPICAL_OPENFST_TYPE;
        break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        implementation.log_ofst =
        log_ofst_interface.define_transducer(sps,cyclic);
        this->type = LOG_OPENFST_TYPE;
        break;
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        implementation.foma =
        foma_interface.define_transducer(sps,cyclic);
        this->type = FOMA_TYPE;
        break;
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
        implementation.xfsm =
        xfsm_interface.define_transducer(sps,cyclic);
        this->type = XFSM_TYPE;
        break;
#endif
    case ERROR_TYPE:
        HFST_THROW(SpecifiedTypeRequiredException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }
}

HfstTransducer::HfstTransducer(const std::vector<StringPairSet> & spsv,
                   ImplementationType type):
    type(type),anonymous(false),is_trie(false), name("")
{
    if (! is_implementation_type_available(type))
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);

    for (std::vector<StringPairSet>::const_iterator it = spsv.begin();
     it != spsv.end(); it++)
      {
    for (StringPairSet::const_iterator IT = it->begin();
         IT != it->end(); IT++)
      {
        if (IT->first == "" || IT->second == "")
          HFST_THROW_MESSAGE
        (EmptyStringException,
         "HfstTransducer(const std::vector<StringPairSet>&, "
         " ImplementationType)");
      }
      }

    switch (type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        implementation.sfst = sfst_interface.define_transducer(spsv);
        this->type = SFST_TYPE;
        break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst =
        tropical_ofst_interface.define_transducer(spsv);
        this->type = TROPICAL_OPENFST_TYPE;
        break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        implementation.log_ofst =
        log_ofst_interface.define_transducer(spsv);
        this->type = LOG_OPENFST_TYPE;
        break;
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        implementation.foma =
        foma_interface.define_transducer(spsv);
        this->type = FOMA_TYPE;
        break;
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
        implementation.xfsm =
        xfsm_interface.define_transducer(spsv);
        this->type = XFSM_TYPE;
        break;
#endif
    case ERROR_TYPE:
        HFST_THROW(SpecifiedTypeRequiredException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }
}

HfstTransducer::HfstTransducer(const std::string& upper_utf8_str,
                   const std::string& lower_utf8_str,
                   const HfstTokenizer
                   &multichar_symbol_tokenizer,
                   ImplementationType type):
    type(type),anonymous(false),is_trie(true), name("")
{
    if (! is_implementation_type_available(type))
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);

    if (upper_utf8_str == "" ||
    lower_utf8_str == "")
      HFST_THROW_MESSAGE
    (EmptyStringException,
     "HfstTransducer(const std::string&, const std::string&, "
     "const HfstTokenizer&, ImplementationType");

    StringPairVector spv =
    multichar_symbol_tokenizer.tokenize
    (upper_utf8_str,lower_utf8_str);
    switch (type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        implementation.sfst = sfst_interface.define_transducer(spv);
        break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst =
        tropical_ofst_interface.define_transducer(spv);
        this->type = TROPICAL_OPENFST_TYPE;
        break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        implementation.log_ofst =
        log_ofst_interface.define_transducer(spv);
        break;
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        implementation.foma =
        foma_interface.define_transducer(spv);
        break;
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
        implementation.xfsm =
        xfsm_interface.define_transducer(spv);
        break;
#endif
    case ERROR_TYPE:
        HFST_THROW(SpecifiedTypeRequiredException);
    default:
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);
    }
}


HfstTransducer::HfstTransducer(HfstInputStream &in):
    type(in.type), anonymous(false),is_trie(false), name("")
{
  if (! is_lean_implementation_type_available(type)) {
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);
    }

    in.read_transducer(*this);
}

HfstTransducer::HfstTransducer(const HfstTransducer &another):
    type(another.type),anonymous(another.anonymous),
    is_trie(another.is_trie), name("")
{
    if (! is_implementation_type_available(type))
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);

    for (map<string,string>::const_iterator prop = another.props.begin();
         prop != another.props.end();
         ++prop)
      {
        if ((prop->first != "type"))
          {
            this->props[prop->first] = prop->second;
          }
      }
    switch (type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        implementation.sfst = sfst_interface.copy(another.implementation.sfst);
        break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst =
        tropical_ofst_interface.copy(another.implementation.tropical_ofst);
        break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        implementation.log_ofst =
        log_ofst_interface.copy(another.implementation.log_ofst);
        break;
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        implementation.foma = foma_interface.copy(another.implementation.foma);
        break;
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
        implementation.xfsm = xfsm_interface.copy(another.implementation.xfsm);
        break;
#endif
    case HFST_OL_TYPE:
    implementation.hfst_ol
            = another.implementation.hfst_ol->copy
            (another.implementation.hfst_ol, false);
    break;
    case HFST_OLW_TYPE:
    implementation.hfst_ol
            = another.implementation.hfst_ol->copy
            (another.implementation.hfst_ol, true);
    break;
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }
}

HfstTransducer::HfstTransducer
( const hfst::implementations::HfstBasicTransducer &net,
  ImplementationType type):
    type(type),anonymous(false),is_trie(false), name("")
{
    if (! is_lean_implementation_type_available(type))
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);

    switch (type)
    {
#if HAVE_SFST || HAVE_LEAN_SFST
    case SFST_TYPE:
        implementation.sfst =
        ConversionFunctions::hfst_basic_transducer_to_sfst(&net);
        break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst =
      ConversionFunctions::hfst_basic_transducer_to_tropical_ofst(&net);
        break;
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        implementation.log_ofst =
        ConversionFunctions::hfst_basic_transducer_to_log_ofst(&net);
        break;
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        implementation.foma =
        ConversionFunctions::hfst_basic_transducer_to_foma(&net);
        break;
#endif
#if HAVE_XFSM
        // this is slow!
    case XFSM_TYPE:
        implementation.xfsm =
        ConversionFunctions::hfst_basic_transducer_to_xfsm(&net);
        break;
#endif
    case HFST_OL_TYPE:
        implementation.hfst_ol =
        ConversionFunctions::hfst_basic_transducer_to_hfst_ol(&net, false);
        break;
    case HFST_OLW_TYPE:
        implementation.hfst_ol =
        ConversionFunctions::hfst_basic_transducer_to_hfst_ol(&net, true);
        break;
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }
    name = net.name;
}

HfstTransducer::~HfstTransducer(void)
{
    if (! is_lean_implementation_type_available(type))
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);

    switch (type)
    {
#if HAVE_SFST || HAVE_LEAN_SFST
    case SFST_TYPE:
        sfst_interface.delete_transducer(implementation.sfst);
        break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        tropical_ofst_interface.delete_transducer(implementation.tropical_ofst);
        break;
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        log_ofst_interface.delete_transducer(implementation.log_ofst);
        break;
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        foma_interface.delete_foma(implementation.foma);
        break;
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
      // FIX THIS: causes errors...
      //delete implementation.xfsm;
        break;
#endif
    case HFST_OL_TYPE:
    case HFST_OLW_TYPE:
        delete implementation.hfst_ol;
        break;
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }
}


HfstTransducer::HfstTransducer(const std::string &symbol,
                               ImplementationType type):
    type(type),anonymous(false),is_trie(false), name("")
{
    if (! is_implementation_type_available(type))
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);

    HfstTokenizer::check_utf8_correctness(symbol);

    if (symbol == "")
      HFST_THROW_MESSAGE
    (EmptyStringException,
     "HfstTransducer(const std::string&, ImplementationType)");

    switch (this->type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        implementation.sfst = sfst_interface.define_transducer(symbol);
        break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst =
        tropical_ofst_interface.define_transducer(symbol);
        this->type = TROPICAL_OPENFST_TYPE;
        break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        implementation.log_ofst = log_ofst_interface.define_transducer(symbol);
        break;
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        implementation.foma = foma_interface.define_transducer(symbol);
        // should the char* be deleted?
        break;
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
        implementation.xfsm = xfsm_interface.define_transducer(symbol);
        // should the char* be deleted?
        break;
#endif
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }
}

HfstTransducer::HfstTransducer(const std::string &isymbol,
                               const std::string &osymbol,
                               ImplementationType type):
    type(type),anonymous(false),is_trie(false), name("")
{
    if (! is_implementation_type_available(type))
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);

    HfstTokenizer::check_utf8_correctness(isymbol);
    HfstTokenizer::check_utf8_correctness(osymbol);

    if (isymbol == "" || osymbol == "")
      HFST_THROW_MESSAGE
    (EmptyStringException,
     "HfstTransducer(const std::string&, const std::string&, "
     " ImplementationType)");

    switch (this->type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        implementation.sfst
        = sfst_interface.define_transducer(isymbol, osymbol);
        break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst
        = tropical_ofst_interface.define_transducer(isymbol, osymbol);
        this->type = TROPICAL_OPENFST_TYPE;
        break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        implementation.log_ofst
        = log_ofst_interface.define_transducer(isymbol, osymbol);
        break;
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        implementation.foma
        = foma_interface.define_transducer(isymbol, osymbol);
        // should the char*:s be deleted?
        break;
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
        implementation.xfsm
        = xfsm_interface.define_transducer(isymbol, osymbol);
        // should the char*:s be deleted?
        break;
#endif
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }
}


// -----------------------------------------------------------------------
//
//                     Transducer accessors
//
// -----------------------------------------------------------------------

ImplementationType HfstTransducer::get_type(void) const {
    return this->type; }
void HfstTransducer::set_name(const std::string &name) {
  this->set_property(std::string("name"), name);
}
std::string HfstTransducer::get_name() const {
  return this->get_property(std::string("name")); }

void
HfstTransducer::set_property(const std::string& property, const std::string& name)
  {
    HfstTokenizer::check_utf8_correctness(name);
    this->props[property] = name;
    if (property == std::string("name"))
      {
        this->name = name;
      }
  }

std::string HfstTransducer::get_property(const std::string& property) const
  {
    if (this->props.find(property) != this->props.end())
      {
        return this->props.find(property)->second;
      }
    else
      {
        return std::string("");
      }
  }
const std::map<std::string,std::string>&
HfstTransducer::get_properties() const
  {
    return this->props;
  }

// -----------------------------------------------------------------------
//
//                     Properties of a transducer
//
// -----------------------------------------------------------------------

bool HfstTransducer::compare(const HfstTransducer &another, bool harmonize) const
{
    if (this->type != another.type)
    HFST_THROW_MESSAGE(TransducerTypeMismatchException,
               "HfstTransducer::compare");

    HfstTransducer one_copy(*this);
    HfstTransducer another_copy(another);

    /* prevent harmonization, if needed */
    if (! harmonize)
      {
        one_copy.insert_missing_symbols_to_alphabet_from(another_copy);
        another_copy.insert_missing_symbols_to_alphabet_from(one_copy);
      }
    /* always prevent harmonizing special symbols */
    one_copy.insert_missing_symbols_to_alphabet_from(another_copy, true);
    another_copy.insert_missing_symbols_to_alphabet_from(one_copy, true);

    if (this->type != FOMA_TYPE && this->type != XFSM_TYPE)
      {
        HfstTransducer *tmp = one_copy.harmonize_(another_copy);
        another_copy = *tmp;
        delete tmp;
      }

    one_copy.determinize();
    another_copy.determinize();

    switch (one_copy.type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        return one_copy.sfst_interface.are_equivalent(
        one_copy.implementation.sfst,
        another_copy.implementation.sfst);
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        return one_copy.tropical_ofst_interface.are_equivalent(
        one_copy.implementation.tropical_ofst,
        another_copy.implementation.tropical_ofst);
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        return one_copy.log_ofst_interface.are_equivalent(
        one_copy.implementation.log_ofst,
        another_copy.implementation.log_ofst);
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        return one_copy.foma_interface.are_equivalent(
        one_copy.implementation.foma,
        another_copy.implementation.foma);
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
        return one_copy.xfsm_interface.are_equivalent(
        one_copy.implementation.xfsm,
        another_copy.implementation.xfsm);
#endif
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }

}
  
bool HfstTransducer::is_automaton(void) const
{
    switch(type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        return sfst_interface.is_automaton(implementation.sfst);
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        return tropical_ofst_interface.is_automaton(implementation.tropical_ofst);
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        return log_ofst_interface.is_automaton(implementation.log_ofst);
#endif
#endif
#if HAVE_FOMA && HAVE_OPENFST
    case FOMA_TYPE:
      {
        HfstTransducer t(*this);
        t.convert(TROPICAL_OPENFST_TYPE);
        return t.is_automaton();
      }
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
      HFST_THROW(FunctionNotImplementedException);
#endif
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }
}

bool HfstTransducer::is_cyclic(void) const
{
    switch(type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        return sfst_interface.is_cyclic(implementation.sfst);
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        return tropical_ofst_interface.is_cyclic(implementation.tropical_ofst);
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        return log_ofst_interface.is_cyclic(implementation.log_ofst);
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        return foma_interface.is_cyclic(implementation.foma);
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
        return xfsm_interface.is_cyclic(implementation.xfsm);
#endif
    case HFST_OL_TYPE:
    case HFST_OLW_TYPE:
        return hfst_ol_interface.is_cyclic(implementation.hfst_ol);
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }
}

unsigned int HfstTransducer::number_of_states() const
{
#if HAVE_OPENFST
    if (type == TROPICAL_OPENFST_TYPE)
    return this->tropical_ofst_interface.number_of_states
        (this->implementation.tropical_ofst);
#endif
#if HAVE_SFST
    if (type == SFST_TYPE)
    return this->sfst_interface.number_of_states
        (this->implementation.sfst);
#endif
#if HAVE_FOMA
    if (type == FOMA_TYPE)
    return this->foma_interface.number_of_states
      (this->implementation.foma);
#endif
#if HAVE_XFSM
    if (type == XFSM_TYPE)
    return this->xfsm_interface.number_of_states
      (this->implementation.xfsm);
#endif
    return 0;
}

unsigned int HfstTransducer::number_of_arcs() const
{
#if HAVE_OPENFST
    if (type == TROPICAL_OPENFST_TYPE)
    return this->tropical_ofst_interface.number_of_arcs
        (this->implementation.tropical_ofst);
#endif
#if HAVE_SFST
    if (type == SFST_TYPE)
    return this->sfst_interface.number_of_arcs
        (this->implementation.sfst);
#endif
#if HAVE_FOMA
    if (type == FOMA_TYPE)
    return this->foma_interface.number_of_arcs
      (this->implementation.foma);
#endif
#if HAVE_XFSM
    if (type == XFSM_TYPE)
    return this->xfsm_interface.number_of_arcs
      (this->implementation.xfsm);
#endif
    return 0;
}


// -----------------------------------------------------------------------
//
//              Epsilon removal, determinization, minimization
//
// -----------------------------------------------------------------------

// if (required): return ~[(?* FAIL_FLAGS) ~$SUCCEED_FLAGS SELF ?*]
// if (! required): return ~[?* FAIL_FLAGS ~$SUCCEED_FLAGS SELF ?*]
static HfstTransducer * new_filter
  (const HfstTransducer & fail_flags, const HfstTransducer & succeed_flags,
   const HfstTransducer & self, bool required)
{
  ImplementationType type = fail_flags.get_type();
  hfst::xre::XreCompiler comp(type);
  comp.set_expand_definitions(true);
  comp.define("Fail", fail_flags);
  comp.define("Succeed", succeed_flags);
  comp.define("Self", self);
  HfstTransducer * result = NULL;
  if (required)
    result = comp.compile("~[(?* Fail) ~$Succeed Self ?*]");
  else
    result = comp.compile("~[?* Fail ~$Succeed Self ?*]");

  // Should the xre compiler do this?
  result->remove_from_alphabet("Fail");
  result->remove_from_alphabet("Succeed");
  result->remove_from_alphabet("Self");

  return result;
}

// Substitute each symbol '_@FLAG@' with '@FLAG@'
static void substitute_escaped_flags(HfstTransducer * filter)
{
  StringSet alpha = filter->get_alphabet();
  for (StringSet::const_iterator it = alpha.begin(); it != alpha.end(); it++)
    {
      if (it->size() > 1)
        {
          if (it->at(0) == '_' && it->at(1) == '@')
            {
              string str(*it);
              str.erase(0,1);
              filter->substitute(*it, str);
            }
        }
    }
  return;
}

#define FLAG_UNIFY 1
#define FLAG_CLEAR 2
#define FLAG_DISALLOW 4
#define FLAG_NEGATIVE 8
#define FLAG_POSITIVE 16
#define FLAG_REQUIRE 32
#define FLAG_EQUAL 64

#define FLAG_FAIL    1
#define FLAG_SUCCEED 2
#define FLAG_NONE    3


static int flag_build
(int ftype, char *fname, char *fvalue, int fftype, char *ffname, char *ffvalue) {

    if (strcmp(fname,ffname) != 0)
        return FLAG_NONE;

    bool selfnull = false; /* If current flag has no value, e.g. @R.A@ */
    if (strcmp(fvalue, "") == 0)
      selfnull = true;

    int eq = strcmp(fvalue, ffvalue);

    /* U flags */
    if ((ftype == FLAG_UNIFY) && (fftype == FLAG_POSITIVE) && (eq == 0))
        return FLAG_SUCCEED;
    if ((ftype == FLAG_UNIFY) && (fftype == FLAG_CLEAR))
        return FLAG_SUCCEED;
    if ((ftype == FLAG_UNIFY) && (fftype == FLAG_UNIFY) && (eq != 0))
        return FLAG_FAIL;
    if ((ftype == FLAG_UNIFY) && (fftype == FLAG_POSITIVE) && (eq != 0))
        return FLAG_FAIL;
    if ((ftype == FLAG_UNIFY) && (fftype == FLAG_NEGATIVE) && (eq == 0))
        return FLAG_FAIL;

    /* R flag with value = 0 */
    if ((ftype == FLAG_REQUIRE) && (fftype == FLAG_UNIFY) && selfnull)
        return FLAG_SUCCEED;
    if ((ftype == FLAG_REQUIRE) && (fftype == FLAG_POSITIVE) && selfnull)
        return FLAG_SUCCEED;
    if ((ftype == FLAG_REQUIRE) && (fftype == FLAG_NEGATIVE) && selfnull)
        return FLAG_SUCCEED;
    if ((ftype == FLAG_REQUIRE) && (fftype == FLAG_CLEAR) && selfnull)
        return FLAG_FAIL;

    /* R flag with value */
    if ((ftype == FLAG_REQUIRE) && (fftype == FLAG_POSITIVE) && (eq == 0) && !selfnull)
        return FLAG_SUCCEED;
    if ((ftype == FLAG_REQUIRE) && (fftype == FLAG_UNIFY) && (eq == 0) && !selfnull)
        return FLAG_SUCCEED;
    if ((ftype == FLAG_REQUIRE) && (fftype == FLAG_POSITIVE) && (eq != 0) && !selfnull)
        return FLAG_FAIL;
    if ((ftype == FLAG_REQUIRE) && (fftype == FLAG_UNIFY) && (eq != 0) && !selfnull)
        return FLAG_FAIL;
    if ((ftype == FLAG_REQUIRE) && (fftype == FLAG_NEGATIVE) && !selfnull)
        return FLAG_FAIL;
    if ((ftype == FLAG_REQUIRE) && (fftype == FLAG_CLEAR) && !selfnull)
        return FLAG_FAIL;

    /* D flag with value = 0 */
    if ((ftype == FLAG_DISALLOW) && (fftype == FLAG_CLEAR) && selfnull)
      return FLAG_SUCCEED;
    if ((ftype == FLAG_DISALLOW) && (fftype == FLAG_POSITIVE) && selfnull)
      return FLAG_FAIL;
    if ((ftype == FLAG_DISALLOW) && (fftype == FLAG_UNIFY) && selfnull)
      return FLAG_FAIL;
    if ((ftype == FLAG_DISALLOW) && (fftype == FLAG_NEGATIVE) && selfnull)
      return FLAG_FAIL;

    /* D flag with value */
    if ((ftype == FLAG_DISALLOW) && (fftype == FLAG_POSITIVE) && (eq != 0) && !selfnull)
        return FLAG_SUCCEED;
    if ((ftype == FLAG_DISALLOW) && (fftype == FLAG_CLEAR) && !selfnull)
        return FLAG_SUCCEED;
    if ((ftype == FLAG_DISALLOW) && (fftype == FLAG_NEGATIVE ) && (eq == 0) && !selfnull)
        return FLAG_SUCCEED;
    if ((ftype == FLAG_DISALLOW) && (fftype == FLAG_POSITIVE) && (eq == 0) && !selfnull)
        return FLAG_FAIL;
    if ((ftype == FLAG_DISALLOW) && (fftype == FLAG_UNIFY) && (eq == 0) && !selfnull)
        return FLAG_FAIL;
    if ((ftype == FLAG_DISALLOW) && (fftype == FLAG_NEGATIVE ) && (eq != 0) && !selfnull)
        return FLAG_FAIL;

    return FLAG_NONE;
}


static int hfst_operator_to_char(const std::string & op)
{
  if (op[0] == 'U')
    return FLAG_UNIFY;
  if (op[0] == 'C')
    return FLAG_CLEAR;
  if (op[0] == 'D')
    return FLAG_DISALLOW;
  if (op[0] == 'N')
    return FLAG_NEGATIVE;
  if (op[0] == 'P')
    return FLAG_POSITIVE;
  if (op[0] == 'R')
    return FLAG_REQUIRE;
  throw "invalid operator";
}

static int is_valid_flag_combination
(const std::string & flag1, const std::string & flag2)
{
  int operator1 = hfst_operator_to_char(FdOperation::get_operator(flag1));
  char * feature1 = strdup(FdOperation::get_feature(flag1).c_str());
  char * value1 = strdup(FdOperation::get_value(flag1).c_str());

  int operator2 = hfst_operator_to_char(FdOperation::get_operator(flag2));
  char * feature2 = strdup(FdOperation::get_feature(flag2).c_str());
  char * value2 = strdup(FdOperation::get_value(flag2).c_str());

  int result = flag_build(operator1, feature1, value1,
                          operator2, feature2, value2);

  free(feature1); free(value1); free(feature2); free(value2);

  return result;
}


/* @brief Get flag filter for transducer \a transducer.
   @param transducer The transducer that is going to be filtered.
   @param flags The set of all flags in \a transducer.
   @param flag The flag that is filtered, if empty string, all flags are filtered.
   @return The flag filter, if there is nothing to filter, return NULL. */
static HfstTransducer * get_flag_filter
(const HfstTransducer * transducer, const StringSet & flags, const String & flag)
{
  ImplementationType type = transducer->get_type();
  bool flag_found = false;
  HfstTransducer * filter = NULL;

  for (StringSet::const_iterator f = flags.begin();
       f != flags.end(); f++)
    {
      HfstTransducer self("_" + *f, type); // escape flags
      HfstTransducer succeed_flags(type);
      HfstTransducer fail_flags(type);
      
      char op = FdOperation::get_operator(*f)[0];
      if ((flag == "" || FdOperation::get_feature(*f) == flag)
          && (op == 'U' || op == 'R' || op == 'D'))        // Equal flag?
        {
          for (StringSet::const_iterator ff = flags.begin();
               ff != flags.end(); ff++)
            {
              int fstatus = is_valid_flag_combination(*f, *ff);

              if (fstatus == 1) {
                fail_flags.disjunct(HfstTransducer("_" + *ff, type));
                flag_found = true;
              } else if (fstatus == 2) {
                succeed_flags.disjunct(HfstTransducer("_" + *ff, type));
                flag_found = true;
              } else {
                ;
              }

            }
        }
      
      if (flag_found) {
        HfstTransducer * newfilter = new_filter
          (fail_flags, succeed_flags, self, (FdOperation::get_operator(*f)[0] == 'R'));

        // intersect filter with newfilter
        if (filter == NULL)
          filter = newfilter;
        else {
          filter->intersect(*newfilter);
          delete newfilter;
        }
      }
      flag_found = false;
    }

  if (filter != NULL)
    {
      substitute_escaped_flags(filter);  // unescape the flags
      filter->optimize();
    }

  return filter;
}

// Replace arcs in \a transducer that use flag \a flag with epsilon arcs
// and remove \a flag from alphabet of \a transducer. If \a flag is the empty
// string, replace/remove all flags.
static void flag_purge(HfstTransducer & transducer, const std::string & flag)
{
  ImplementationType type = transducer.get_type();
  // slow for xfsm_transducer..
  HfstBasicTransducer net(transducer);
  net.flag_purge(flag);
  transducer = HfstTransducer(net, type);
  return;
}

HfstTransducer &HfstTransducer::eliminate_flags()
{
#if HAVE_FOMA
  if (type == FOMA_TYPE)
    {
      struct fsm * result = this->foma_interface.eliminate_flags(this->implementation.foma);
      this->implementation.foma = result;
      return *this;
    }
#endif
#if HAVE_XFSM
  if (type == XFSM_TYPE)
    {
      this->xfsm_interface.eliminate_flags_xfsm(this->implementation.xfsm);
      return *this;
    }
#endif

  HfstBasicTransducer basic(*this);
  StringSet flags = basic.get_flags();
  HfstTransducer * filter = get_flag_filter(this, flags, "");
  
  if (filter != NULL)
    {
      HfstTransducer filter_copy(*filter);
      filter_copy.compose(*this).compose(*filter);
      delete filter;
      flag_purge(filter_copy, "");
      *this = filter_copy;
    }
  else
    {
      flag_purge(*this, "");
    }

  return this->optimize();
}

HfstTransducer &HfstTransducer::eliminate_flag(const std::string & flag)
{
  HfstBasicTransducer basic(*this);
  StringSet flags = basic.get_flags();
  bool feature_found = false;
  for (StringSet::const_iterator it = flags.begin(); it != flags.end(); it++)
    {
      if (FdOperation::get_feature(*it) == flag)
        {
          feature_found = true;
          break;
        }
    }
  if (! feature_found)
    {
      if (flag.find('.') == std::string::npos)
        HFST_THROW_MESSAGE(HfstException, "HfstTransducer::eliminate_flag: flag feature does not occur in the transducer: " + flag);
      else
        HFST_THROW_MESSAGE(HfstException, "HfstTransducer::eliminate_flag: only the flag feature must be given, no value or operator: " + flag);
    }

#if HAVE_FOMA
  if (type == FOMA_TYPE)
    {
      struct fsm * result = this->foma_interface.eliminate_flag(this->implementation.foma, flag);
      this->implementation.foma = result;
      return *this;
    }
#endif
#if HAVE_XFSM
  if (type == XFSM_TYPE)
    {
      this->xfsm_interface.eliminate_flag_xfsm(this->implementation.xfsm, flag);
      return *this;
    }
#endif

  HfstTransducer * filter = get_flag_filter(this, flags, flag);
  if (filter != NULL)
    {
      HfstTransducer filter_copy(*filter);
      filter_copy.compose(*this).compose(*filter);
      delete filter;
      flag_purge(filter_copy, flag);
      *this = filter_copy;
    }
  else
    {
      flag_purge(*this, flag);
    }

  return this->optimize();
}

HfstTransducer &HfstTransducer::remove_epsilons()
{ is_trie = false;
    return apply(
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::remove_epsilons,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::remove_epsilons,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::remove_epsilons,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::remove_epsilons,
#endif
#if HAVE_XFSM
    NULL,
#endif
    /* Add here your implementation. */
    //#if HAVE_MY_TRANSDUCER_LIBRARY
    //&hfst::implementations::MyTransducerLibraryTransducer::remove_epsilons,
    //#endif
    false ); }

HfstTransducer &HfstTransducer::prune()
{
#if HAVE_OPENFST
  // slow for xfsm type...
  this->convert(TROPICAL_OPENFST_TYPE);
  fst::StdVectorFst * temp = hfst::implementations::TropicalWeightTransducer::prune
    (this->implementation.tropical_ofst);
  this->tropical_ofst_interface.delete_transducer(this->implementation.tropical_ofst);
  this->implementation.tropical_ofst = temp;
  return *this;
#endif
  HFST_THROW(FunctionNotImplementedException);
}

HfstTransducer &HfstTransducer::determinize()
{ is_trie = false;
#if HAVE_XFSM
  if (this->type == XFSM_TYPE) {
    HFST_THROW(FunctionNotImplementedException); }
#endif
    return apply(
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::determinize,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::determinize,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::determinize,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::determinize,
#endif
#if HAVE_XFSM
    NULL,
#endif
    /* Add here your implementation. */
    false ); }

HfstTransducer &HfstTransducer::minimize()
{  is_trie = false;
    return apply(
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::minimize,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::minimize,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::minimize,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::minimize,
#endif
#if HAVE_XFSM
    &hfst::implementations::XfsmTransducer::minimize,
#endif
    /* Add here your implementation. */
    false );
}

HfstTransducer &HfstTransducer::optimize()
{
  if (can_minimize)
    return this->minimize();
  else
    return this->determinize();
}

// -----------------------------------------------------------------------
//
//                        Repeat functions
//
// -----------------------------------------------------------------------

HfstTransducer &HfstTransducer::repeat_star()
{ is_trie = false;
    return apply(
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::repeat_star,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::repeat_star,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::repeat_star,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::repeat_star,
#endif
#if HAVE_XFSM
    NULL,
#endif
    /* Add here your implementation. */
    false ); }

HfstTransducer &HfstTransducer::repeat_plus()
{ is_trie = false;
    return apply(
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::repeat_plus,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::repeat_plus,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::repeat_plus,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::repeat_plus,
#endif
#if HAVE_XFSM
    NULL,
#endif
    /* Add here your implementation. */
    false ); }

HfstTransducer &HfstTransducer::repeat_n(unsigned int n)
{ is_trie = false; // This could be done so that is_trie is preserved
    return apply(
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::repeat_n,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::repeat_n,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::repeat_n,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::repeat_n,
#endif
#if HAVE_XFSM
    NULL,
#endif
    /* Add here your implementation. */
    //#if HAVE_MY_TRANSDUCER_LIBRARY
    //&hfst::implementations::MyTransducerLibraryTransducer::repeat_n,
    //#endif
    n ); }

HfstTransducer &HfstTransducer::repeat_n_plus(unsigned int n)
{ is_trie = false; // This could be done so that is_trie is preserved
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    {
      this->xfsm_interface.repeat_n_plus(this->implementation.xfsm, n);
      return *this;
    }
#endif
    HfstTransducer a(*this);
    return (this->repeat_n(n).concatenate(a.repeat_star()));
}

HfstTransducer &HfstTransducer::repeat_n_minus(unsigned int n)
{ is_trie = false; // This could be done so that is_trie is preserved
    return apply(
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::repeat_le_n,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::repeat_le_n,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::repeat_le_n,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::repeat_le_n,
#endif
#if HAVE_XFSM
    NULL,
#endif
    /* Add here your implementation. */
    n ); }

HfstTransducer &HfstTransducer::repeat_n_to_k(unsigned int n, unsigned int k)
{ is_trie = false; // This could be done so that is_trie is preserved
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    {
      this->xfsm_interface.repeat_n_to_k(this->implementation.xfsm, n, k);
      return *this;
    }
#endif
    HfstTransducer a(*this);
    return (this->repeat_n(n).concatenate(a.repeat_n_minus(k-n)));
}



// -----------------------------------------------------------------------
//
//                      Unary operators
//
// -----------------------------------------------------------------------

HfstTransducer &HfstTransducer::optionalize()
{ is_trie = false; // This could be done so that is_trie is preserved
    return apply(
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::optionalize,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::optionalize,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::optionalize,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::optionalize,
#endif
#if HAVE_XFSM
    &hfst::implementations::XfsmTransducer::optionalize,
#endif
    /* Add here your implementation. */
    false ); }

HfstTransducer &HfstTransducer::invert()
{ is_trie = false; // This could be done so that is_trie is preserved
    return apply(
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::invert,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::invert,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::invert,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::invert,
#endif
#if HAVE_XFSM
    &hfst::implementations::XfsmTransducer::invert,
#endif
    /* Add here your implementation. */
    false ); }

HfstTransducer &HfstTransducer::reverse()
{ is_trie = false; // This could be done so that is_trie is preserved
    return apply (
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::reverse,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::reverse,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::reverse,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::reverse,
#endif
#if HAVE_XFSM
    &hfst::implementations::XfsmTransducer::reverse,
#endif
    /* Add here your implementation. */
    false ); }

HfstTransducer &HfstTransducer::input_project()
{ is_trie = false; // This could be done so that is_trie is preserved
  return apply (
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::extract_input_language,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::extract_input_language,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::extract_input_language,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::extract_input_language,
#endif
#if HAVE_XFSM
    &hfst::implementations::XfsmTransducer::extract_input_language,
#endif
    /* Add here your implementation. */
    false ); }

HfstTransducer &HfstTransducer::output_project()
{ is_trie = false; // This could be done so that is_trie is preserved
  return apply (
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::extract_output_language,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::
    extract_output_language,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::extract_output_language,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::extract_output_language,
#endif
#if HAVE_XFSM
    &hfst::implementations::XfsmTransducer::extract_output_language,
#endif
    /* Add here your implementation. */
    false ); }

HfstTransducer &HfstTransducer::negate()
{ is_trie = false; // This could be done so that is_trie is preserved

  if (! this->is_automaton())
    {
      HFST_THROW_MESSAGE(TransducerIsNotAutomatonException,
			 "HfstTransducer::negate()");
    }
  
  HfstTransducer idstar("@_IDENTITY_SYMBOL_@", this->type);
  // diacritics will not be harmonized in subtract
  StringSet flags = idstar.insert_missing_diacritics_to_alphabet_from(*this);
  for (StringSet::const_iterator it = flags.begin(); it != flags.end(); it++)
    {
      HfstTransducer tr(*it, this->type);
      idstar.disjunct(tr);
    }
  idstar.repeat_star();
  idstar.minimize();
  idstar.subtract(*this, true);
  (*this)=idstar;
  return *this;
}

// -----------------------------------------------------------------------
//
//                        Extract paths
//
// -----------------------------------------------------------------------

std::vector<HfstTransducer*> HfstTransducer::extract_path_transducers()
{
    if (this->type != SFST_TYPE)
        HFST_THROW(FunctionNotImplementedException);

    std::vector<HfstTransducer*> hfst_paths;
#if HAVE_SFST
    std::vector<SFST::Transducer*> sfst_paths
    = sfst_interface.extract_path_transducers(this->implementation.sfst);
    for (std::vector<SFST::Transducer*>::iterator it
         = sfst_paths.begin(); it != sfst_paths.end(); it++)
    {
        HfstTransducer *tr = new HfstTransducer(SFST_TYPE);
        sfst_interface.delete_transducer(tr->implementation.sfst);
        tr->implementation.sfst = *it;
        hfst_paths.push_back(tr);
    }
#endif
    return hfst_paths;
}

void HfstTransducer::extract_paths(ExtractStringsCb& callback, int cycles)
    const
{
    switch (this->type)
    {
#if HAVE_OPENFST
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        hfst::implementations::LogWeightTransducer::extract_paths
        (implementation.log_ofst,callback,cycles,NULL,false);
        break;
#endif
    case TROPICAL_OPENFST_TYPE:
        hfst::implementations::TropicalWeightTransducer::extract_paths
        (implementation.tropical_ofst,callback,cycles,NULL,false);
        break;
#endif
#if HAVE_SFST
    case SFST_TYPE:
        hfst::implementations::SfstTransducer::extract_paths
        (implementation.sfst, callback, cycles,NULL,false);
        break;
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        hfst::implementations::FomaTransducer::extract_paths
        (implementation.foma, callback, cycles,NULL,false);
        break;
#endif
    /* Add here your implementation. */
    case HFST_OL_TYPE:
    case HFST_OLW_TYPE:
        hfst::implementations::HfstOlTransducer::extract_paths
        (implementation.hfst_ol, callback, cycles);
        break;
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }
}
  
void HfstTransducer::extract_paths_fd(ExtractStringsCb& callback,
                      int cycles, bool filter_fd)
    const
{
    switch (this->type)
    {
#if HAVE_OPENFST
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
    {
        FdTable<int64>* t_log_ofst
        = hfst::implementations::LogWeightTransducer::get_flag_diacritics
        (implementation.log_ofst);
        hfst::implementations::LogWeightTransducer::extract_paths
        (implementation.log_ofst,callback,cycles,t_log_ofst,filter_fd);
        delete t_log_ofst;
    }
    break;
#endif
    case TROPICAL_OPENFST_TYPE:
    {
        FdTable<int64>* t_tropical_ofst
        = hfst::implementations::TropicalWeightTransducer::
            get_flag_diacritics(implementation.tropical_ofst);
        hfst::implementations::TropicalWeightTransducer::extract_paths
        (implementation.tropical_ofst,callback,cycles,
         t_tropical_ofst,filter_fd);
        delete t_tropical_ofst;
    }
    break;
#endif
#if HAVE_SFST
    case SFST_TYPE:
    {
        FdTable<SFST::Character>* t_sfst
        = hfst::implementations::SfstTransducer::get_flag_diacritics
        (implementation.sfst);
        hfst::implementations::SfstTransducer::extract_paths
        (implementation.sfst, callback, cycles, t_sfst, filter_fd);
        delete t_sfst;
    }
    break;
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
    {
    FdTable<int>* t_foma
            = hfst::implementations::FomaTransducer::get_flag_diacritics
            (implementation.foma);
    hfst::implementations::FomaTransducer::extract_paths
            (implementation.foma, callback, cycles, t_foma, filter_fd);
    delete t_foma;
    }
    break;
#endif
    /* Add here your implementation. */
    case HFST_OL_TYPE:
    case HFST_OLW_TYPE:
    {
        const FdTable<hfst_ol::SymbolNumber>* t_hfst_ol
        = hfst::implementations::HfstOlTransducer::get_flag_diacritics
        (implementation.hfst_ol);
        hfst::implementations::HfstOlTransducer::extract_paths
        (implementation.hfst_ol,callback,cycles,t_hfst_ol,filter_fd);
        // don't delete t_hfst_ol, it's not a copy of the FdTable but the
        // real thing
    }
    break;
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }
}
  
class ExtractStringsCb_ : public ExtractStringsCb
{
public:
    HfstTwoLevelPaths& paths;
    int max_num;
      
    ExtractStringsCb_(HfstTwoLevelPaths& p, int max):
        paths(p), max_num(max) {}
    RetVal operator()(HfstTwoLevelPath& path, bool final)
    {
        if(final)
          paths.insert(path);
        
        return RetVal((max_num < 1) || (int)paths.size() < max_num, true);
    }
};

int HfstTransducer::longest_path_size(bool obey_flags) const
{
  if (this->is_cyclic())
    {
      HFST_THROW(TransducerIsCyclicException);
    }

  if (! obey_flags)
    {
      HfstBasicTransducer net(*this);
      return net.longest_path_size();
    }

  HfstTwoLevelPaths results;
  bool paths_found = this->extract_longest_paths(results, true /* obey flags */);
  if (! paths_found)
    {
      return -1;
    }
  // else, there is at least one path
  return (int)results.begin()->second.size();
}

static std::string match_any_n_times(unsigned int n, const StringSet & flags)
{
  std::string match_any(" [ ? ");
  for (StringSet::const_iterator it = flags.begin();
       it != flags.end(); it++)
    {
      match_any = match_any + "| \"" + *it + "\" ";
    }
  match_any += " ] ";

  std::string match_length("[");
  for (unsigned int i=0; i < n; i++)
    {
      match_length += match_any;
    }
  match_length += "]";

  return match_length;
}

bool HfstTransducer::extract_longest_paths
(HfstTwoLevelPaths &results, bool obey_flags /*,bool show_flags*/) const
{
  if (this->is_cyclic())
    {
      HFST_THROW(TransducerIsCyclicException);
    }

  HfstBasicTransducer net(*this);
  std::vector<unsigned int> path_lengths = net.path_sizes();
  if (path_lengths.size() == 0)
    return false;

  StringSet flags = net.get_flags();

  // go through each length of accepted paths in descending order
  for (std::vector<unsigned int>::const_iterator length = path_lengths.begin();
       length != path_lengths.end(); length++)
    {
      // create a transducer [ any any ... any any ] where the number of transitions
      // that accept any symbol (including flags) is equal to current length
      // of accepted paths
      std::string match_length = match_any_n_times(*length, flags);

      hfst::xre::XreCompiler xre(this->get_type());
      HfstTransducer * length_tr = xre.compile(match_length.c_str());

      // filter out the paths of current length and extract them
      length_tr->compose(*this);
      length_tr->optimize();
      if (obey_flags)
        {
          length_tr->extract_paths_fd(results);
        }
      else
        {
          length_tr->extract_paths(results);
        }
      delete length_tr;

      // if paths were found
      if (results.size() > 0)
        {
          return true;
        }

    } // lengths of accepted paths gone through

  // no paths found
  return false;
}


void HfstTransducer::extract_shortest_paths(HfstTwoLevelPaths &results) const
{
#if HAVE_OPENFST
  HfstTransducer t(*this);
  t.convert(TROPICAL_OPENFST_TYPE);
  t.n_best(1);
  t.extract_paths(results);
  return;
#endif
  HFST_THROW(FunctionNotImplementedException);
}
  
void HfstTransducer::extract_paths(HfstTwoLevelPaths &results,
                   int max_num, int cycles) const
{
    if(is_cyclic() && max_num < 1 && cycles < 0)
    HFST_THROW_MESSAGE(TransducerIsCyclicException,
               "HfstTransducer::extract_paths");
    
    ExtractStringsCb_ cb(results, max_num);
    extract_paths(cb, cycles);
}
  
void HfstTransducer::extract_paths_fd(HfstTwoLevelPaths &results,
                      int max_num, int cycles,
                      bool filter_fd) const
{
    if(is_cyclic() && max_num < 1 && cycles < 0)
    HFST_THROW_MESSAGE(TransducerIsCyclicException,
               "HfstTransducer::extract_paths_fd");
    
    ExtractStringsCb_ cb(results, max_num);
    extract_paths_fd(cb, cycles, filter_fd);
}

void HfstTransducer::extract_random_paths
(HfstTwoLevelPaths &results, int max_num) const
{
  switch (this->type)
    {
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
    {
      this->tropical_ofst_interface.extract_random_paths
    (this->implementation.tropical_ofst, results, max_num);
    }
    break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
    {
      this->log_ofst_interface.extract_random_paths
    (this->implementation.log_ofst, results, max_num);
    }
    break;
#endif
#endif
#if HAVE_SFST
    case SFST_TYPE:
      {
#if HAVE_OPENFST
        HfstTransducer copy(*this);
        copy.convert(TROPICAL_OPENFST_TYPE);
        copy.tropical_ofst_interface.extract_random_paths
          (copy.implementation.tropical_ofst, results, max_num);
        break;
#endif
        HFST_THROW(FunctionNotImplementedException);
        break;
      }
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
      {
#if HAVE_OPENFST
        HfstTransducer copy(*this);
        copy.convert(TROPICAL_OPENFST_TYPE);
        copy.tropical_ofst_interface.extract_random_paths
          (copy.implementation.tropical_ofst, results, max_num);
        break;
#endif
        HFST_THROW(FunctionNotImplementedException);
        break;
      }
#endif
    /* Add here your implementation. */
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    case HFST_OL_TYPE:
    case HFST_OLW_TYPE:
    default:
      HFST_THROW(FunctionNotImplementedException);
    }
}

void HfstTransducer::extract_random_paths_fd
(HfstTwoLevelPaths &results, int max_num, bool filter_fd) const
{
#if HAVE_OPENFST
  HfstTransducer copy(*this);
  copy.convert(TROPICAL_OPENFST_TYPE);
  copy.tropical_ofst_interface.extract_random_paths_fd
    (copy.implementation.tropical_ofst, results, max_num, filter_fd);
  return;
#endif
  HFST_THROW(FunctionNotImplementedException);
}

HfstTransducer &HfstTransducer::n_best(unsigned int n)
{
    if (! is_implementation_type_available(TROPICAL_OPENFST_TYPE)) {
    (void)n;
    throw ImplementationTypeNotAvailableException("HfstTransducer::n_best implemented only for "
               "TROPICAL_OPENFST_TYPE", __FILE__, __LINE__, type);
    }

#if HAVE_OPENFST
    ImplementationType original_type = this->type;
    if ((original_type == SFST_TYPE) || (original_type == FOMA_TYPE)) {
    this->convert(TROPICAL_OPENFST_TYPE);
    }
#endif

    switch (this->type)
    {
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
    {
    fst::StdVectorFst * temp =
            hfst::implementations::TropicalWeightTransducer::n_best
            (implementation.tropical_ofst,(int)n);
    tropical_ofst_interface.delete_transducer(implementation.tropical_ofst);
    implementation.tropical_ofst = temp;
    break;
    }
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
    {
    hfst::implementations::LogFst * temp =
            hfst::implementations::LogWeightTransducer::n_best
            (implementation.log_ofst,(int)n);
    log_ofst_interface.delete_transducer(implementation.log_ofst);
    implementation.log_ofst = temp;
    break;
    }
#endif
#endif
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    case HFST_OL_TYPE:
    case HFST_OLW_TYPE:
    default:
        HFST_THROW(FunctionNotImplementedException);
        break;
    }
#if HAVE_OPENFST
    this->convert(original_type);
#endif
    return *this;
}


bool HfstTransducer::is_special_symbol(const std::string &symbol)
{
  if (symbol.size() < 4)
    return false;
  if (symbol[0] == '@' && symbol[symbol.size()-1] == '@' &&
      symbol[1] == '_' && symbol[symbol.size()-2] == '_')
    return true;
  return false;
}

StringSet HfstTransducer::insert_missing_diacritics_to_alphabet_from(const HfstTransducer &another)
{
  StringSet this_alphabet = this->get_alphabet();
  StringSet another_alphabet = another.get_alphabet();
  StringSet missing_flags;

  for (StringSet::const_iterator it = another_alphabet.begin();
       it != another_alphabet.end(); it++)
    {
      if (this_alphabet.find(*it) == this_alphabet.end())
        {
          if (FdOperation::is_diacritic(*it))
            missing_flags.insert(*it);
        }
    }
  this->insert_to_alphabet(missing_flags);
  return missing_flags;
}

void HfstTransducer::insert_missing_symbols_to_alphabet_from(const HfstTransducer &another, bool only_special_symbols)
{
  StringSet this_alphabet = this->get_alphabet();
  StringSet another_alphabet = another.get_alphabet();
  StringSet missing_symbols;

  for (StringSet::const_iterator it = another_alphabet.begin();
       it != another_alphabet.end(); it++)
    {
      if (this_alphabet.find(*it) == this_alphabet.end())
        {
          if (! only_special_symbols)
            {
              missing_symbols.insert(*it);
            }
          else
            {
              if (is_special_symbol(*it))
                missing_symbols.insert(*it);
            }
        }
    }
  this->insert_to_alphabet(missing_symbols);
  return;
}

// -----------------------------------------------------------------------
//
//                        Flag diacritics
//
// -----------------------------------------------------------------------

/*
   Check for missing flag diacritics (FG), i.e. FGs that are present in the
   alphabet of \a another but not in the alphabet of this transducer and insert
   them to \a missing_flags. \a return_on_first_miss defines whether function
   returns after first missing FG is found and inserted to \a missing_flags.
   @ retval Whether any missing FGs where found.
*/
bool HfstTransducer::check_for_missing_flags_in
(const HfstTransducer &another,
 StringSet &missing_flags,
 bool return_on_first_miss) const
{
    bool retval=false;
    StringSet this_alphabet = get_alphabet();
    StringSet another_alphabet = another.get_alphabet();

    for (StringSet::const_iterator it = another_alphabet.begin();
         it != another_alphabet.end(); it++)
      {
        if ( FdOperation::is_diacritic(*it) &&
             (this_alphabet.find(*it) == this_alphabet.end()) )
          {
            missing_flags.insert(*it);
            retval = true;
            if (return_on_first_miss)
              { return retval; }
          }
      }
    return retval;
}

void HfstTransducer::insert_freely_missing_flags_from
(const HfstTransducer &another)
{
    StringSet missing_flags;
    if (check_for_missing_flags_in(another, missing_flags,
                                   false /* do not return on first miss */ ))
    {
      HfstBasicTransducer basic(*this);

      for (unsigned int s = 0; s <= (unsigned int)basic.get_max_state(); ++s)
        {
          for (StringSet::const_iterator it = missing_flags.begin();
               it != missing_flags.end(); it++)
            {
              basic.add_transition
                (s, HfstBasicTransition(s,*it,*it,0.0));
            }
        }

      *this = HfstTransducer(basic, this->type);
    }
}

bool has_flags(const HfstTransducer &fst)
{
  StringSet alphabet = fst.get_alphabet();
  for (StringSet::const_iterator it = alphabet.begin();
       it != alphabet.end(); it++)
    {
      if (FdOperation::is_diacritic(*it))
        { return true; }
    }
  return false;
}

bool HfstTransducer::has_flag_diacritics(void) const
{ return has_flags(*this); }

std::string add_suffix_to_feature_name(const std::string &flag_diacritic,
                                       const std::string &suffix)
{
  return
    "@" +
    FdOperation::get_operator(flag_diacritic) +
    "." +
    FdOperation::get_feature(flag_diacritic) + suffix +

    (FdOperation::has_value(flag_diacritic) ?
     "." +
     FdOperation::get_value(flag_diacritic) :
     "") +

    "@";
}

void rename_flag_diacritics(HfstTransducer &fst,const std::string &suffix)
{
  
  HfstBasicTransducer basic_fst(fst);
  HfstBasicTransducer basic_fst_copy;
  (void)basic_fst_copy.add_state(basic_fst.get_max_state());

  hfst::implementations::HfstState s = 0;

  for (HfstBasicTransducer::const_iterator it = basic_fst.begin();
       it != basic_fst.end();
       ++it)
    {
      for (hfst::implementations::HfstBasicTransitions::const_iterator jt =
             it->begin();
           jt != it->end();
           ++jt)
        {
          basic_fst_copy.add_transition
            (s,
             HfstBasicTransition
             (jt->get_target_state(),

              FdOperation::is_diacritic(jt->get_input_symbol())  ?
              add_suffix_to_feature_name(jt->get_input_symbol(),suffix) :
              jt->get_input_symbol(),

              FdOperation::is_diacritic(jt->get_output_symbol())  ?
              add_suffix_to_feature_name(jt->get_output_symbol(),suffix) :
              jt->get_output_symbol(),

              jt->get_weight()));
        }

      if (basic_fst.is_final_state(s))
        { basic_fst_copy.set_final_weight(s,basic_fst.get_final_weight(s)); }

      ++s;
    }
  fst = HfstTransducer(basic_fst_copy,fst.get_type());
}

void HfstTransducer::twosided_flag_diacritics()
{
  HfstBasicTransducer basic_fst(*this);
  HfstBasicTransducer basic_fst_copy;
  (void)basic_fst_copy.add_state(basic_fst.get_max_state());

  hfst::implementations::HfstState s = 0;

  for (HfstBasicTransducer::const_iterator it = basic_fst.begin();
       it != basic_fst.end();
       ++it)
    {
      for (hfst::implementations::HfstBasicTransitions::const_iterator jt =
             it->begin();
           jt != it->end();
           ++jt)
        {
          std::string istr = jt->get_input_symbol();
          std::string ostr = jt->get_output_symbol();
          bool istr_is_flag = FdOperation::is_diacritic(istr);
          bool ostr_is_flag = FdOperation::is_diacritic(ostr);

          bool extra_transition_needed
            = (istr_is_flag || ostr_is_flag) && (istr != ostr);

          if (extra_transition_needed)
            {
              hfst::implementations::HfstState new_state = basic_fst_copy.add_state();

              // flag:foo -> flag:flag 0:foo, foo:flag -> foo:0 flag:flag
              // flag1:flag2 -> flag1:flag1 flag2:flag2

              std::string in = istr;
              std::string out = (istr_is_flag)? istr : hfst::internal_epsilon;

              basic_fst_copy.add_transition
                (s,
                 HfstBasicTransition
                 (new_state, in, out, 0/*?*/));

              in = (ostr_is_flag)? ostr : hfst::internal_epsilon;
              out = ostr;

              basic_fst_copy.add_transition
                (new_state,
                 HfstBasicTransition
                 (jt->get_target_state(), in, out, jt->get_weight()/*?*/));
            }
          else
            {
              basic_fst_copy.add_transition
                (s,
                 HfstBasicTransition
                 (jt->get_target_state(), istr, ostr, jt->get_weight()));
            }
        }

      if (basic_fst.is_final_state(s))
        { basic_fst_copy.set_final_weight(s,basic_fst.get_final_weight(s)); }

      ++s;
    }
  *this = HfstTransducer(basic_fst_copy,this->get_type());
}

std::string encode_flag(const std::string &flag_diacritic)
{
  std::string retval(flag_diacritic);
  retval[0] = '%';
  retval[retval.size()-1] = '%';
  return retval;
}

std::string decode_flag(const std::string &flag_diacritic)
{
  if (flag_diacritic[0] != '%' || flag_diacritic[flag_diacritic.size()-1] != '%')
    return std::string(flag_diacritic);

  std::string retval(flag_diacritic);
  retval[0] = '@';
  retval[retval.size()-1] = '@';
  return retval;
}

void encode_flag_diacritics(HfstTransducer &fst)
{
  
  HfstBasicTransducer basic_fst(fst);
  HfstBasicTransducer basic_fst_copy;
  (void)basic_fst_copy.add_state(basic_fst.get_max_state());

  hfst::implementations::HfstState s = 0;

  for (HfstBasicTransducer::const_iterator it = basic_fst.begin();
       it != basic_fst.end();
       it++)
    {
      for (hfst::implementations::HfstBasicTransitions::const_iterator jt =
             it->begin();
           jt != it->end();
           jt++)
        {
          basic_fst_copy.add_transition
            (s,
             HfstBasicTransition
             (jt->get_target_state(),

              FdOperation::is_diacritic(jt->get_input_symbol())  ?
              encode_flag(jt->get_input_symbol()) :
              jt->get_input_symbol(),

              FdOperation::is_diacritic(jt->get_output_symbol())  ?
              encode_flag(jt->get_output_symbol()) :
              jt->get_output_symbol(),

              jt->get_weight()));
        }

      if (basic_fst.is_final_state(s))
        { basic_fst_copy.set_final_weight(s,basic_fst.get_final_weight(s)); }

      s++;
    }

  // copy alphabet, encode all flags
  StringSet alpha = basic_fst.get_alphabet();
  for (StringSet::const_iterator it = alpha.begin(); it != alpha.end(); it++)
    {
      if (it->size() > 4)
        {
          if ((it->at(0) == '%') && (it->at(it->size()-1) == '%'))
            {
              std::string str(*it);
              str[0] = '@';
              str[str.size()-1] = '@';
              if (FdOperation::is_diacritic(str))
                {
                  std::string msg = "error: reserved symbol '" + str + "' detected";
                  throw msg.c_str();
                }
            }
        }
      String symbol = *it;
      if (FdOperation::is_diacritic(symbol))
        symbol = encode_flag(symbol);
      basic_fst_copy.add_symbol_to_alphabet(symbol);
    }

  fst = HfstTransducer(basic_fst_copy,fst.get_type());
}

void decode_flag_diacritics(HfstTransducer &fst)
{
  
  HfstBasicTransducer basic_fst(fst);
  HfstBasicTransducer basic_fst_copy;
  (void)basic_fst_copy.add_state(basic_fst.get_max_state());

  hfst::implementations::HfstState s = 0;

  for (HfstBasicTransducer::const_iterator it = basic_fst.begin();
       it != basic_fst.end();
       it++)
    {
      for (hfst::implementations::HfstBasicTransitions::const_iterator jt =
             it->begin();
           jt != it->end();
           jt++)
        {
          std::string istr = decode_flag(jt->get_input_symbol());
          if (!FdOperation::is_diacritic(istr))
            istr = jt->get_input_symbol();
          
          std::string ostr = decode_flag(jt->get_output_symbol());
          if (!FdOperation::is_diacritic(ostr))
            ostr = jt->get_output_symbol();

          basic_fst_copy.add_transition
            (s,
             HfstBasicTransition
             (jt->get_target_state(),
              istr,
              ostr,
              jt->get_weight()));
        }

      if (basic_fst.is_final_state(s))
        { basic_fst_copy.set_final_weight(s,basic_fst.get_final_weight(s)); }

      s++;
    }

  // copy alphabet, decode all flags
  StringSet alpha = basic_fst.get_alphabet();
  for (StringSet::const_iterator it = alpha.begin(); it != alpha.end(); it++)
    {
      std::string symbol = decode_flag(*it);
      if (!FdOperation::is_diacritic(symbol))
        symbol = *it;
      basic_fst_copy.add_symbol_to_alphabet(symbol);
    }

  fst = HfstTransducer(basic_fst_copy,fst.get_type());
}


// Return true if the flag in flag_diacritic ends in suffix and false
// otherwise. E.g. if flag_diacritic = "@D.NeedNoun_1.ON@ and suffix =
// "_1", return true.
bool is_flag_suffix
(const std::string &suffix, const std::string &flag_diacritic)
{
  size_t flag_end_pos = flag_diacritic.find_last_of('.');

  if (flag_end_pos == std::string::npos)
    { return false; }

  if (flag_end_pos < suffix.size())
    { return false; }

  if (flag_diacritic.substr(flag_end_pos - suffix.size(),suffix.size())
      != suffix)
    { return false; }

  return true;
}

void HfstTransducer::harmonize_flag_diacritics(HfstTransducer &another,
                                               bool insert_renamed_flags)
{
  if (this->type != another.type)
    HFST_THROW(TransducerTypeMismatchException);

  bool this_has_flag_diacritics    = has_flags(*this);
  bool another_has_flag_diacritics = has_flags(another);

  if (this_has_flag_diacritics && another_has_flag_diacritics)
    {
      rename_flag_diacritics(*this,"_1");
      rename_flag_diacritics(another,"_2");

      if (insert_renamed_flags)
        {
          this->insert_freely_missing_flags_from(another);
          another.insert_freely_missing_flags_from(*this);
          this->remove_illegal_flag_paths();
        }
    }
  else if (this_has_flag_diacritics && insert_renamed_flags)
    { another.insert_freely_missing_flags_from(*this); }
  else if (another_has_flag_diacritics && insert_renamed_flags)
    { this->insert_freely_missing_flags_from(another); }
}

bool HfstTransducer::check_for_missing_flags_in
(const HfstTransducer &another) const
{
    StringSet foo; /* An obligatory argument that is not used. */
    return check_for_missing_flags_in(another, foo,
                                      true /* return on first miss */);
}


static bool substitute_input_flag_with_epsilon(const StringPair &sp, StringPairSet &sps)
{
  if (FdOperation::is_diacritic(sp.first))
    {
      StringPair new_pair(hfst::internal_epsilon, sp.second);
      sps.insert(new_pair);
      return true;
    }
  return false;
}

static bool substitute_output_flag_with_epsilon(const StringPair &sp, StringPairSet &sps)
{
  if (FdOperation::is_diacritic(sp.second))
    {
      StringPair new_pair(sp.first, hfst::internal_epsilon);
      sps.insert(new_pair);
      return true;
    }
  return false;
}

static bool substitute_one_sided_flags(const StringPair &sp, StringPairSet &sps)
{
  if (FdOperation::is_diacritic(sp.first) && (sp.second == hfst::internal_epsilon))
    {
      StringPair new_pair(sp.first, sp.first);
      sps.insert(new_pair);
      return true;
    }
  if (FdOperation::is_diacritic(sp.second) && (sp.first == hfst::internal_epsilon))
    {
      StringPair new_pair(sp.second, sp.second);
      sps.insert(new_pair);
      return true;
    }
  return false;
}



// -----------------------------------------------------------------------
//
//                        Insert freely
//
// -----------------------------------------------------------------------

HfstTransducer &HfstTransducer::insert_freely
(const StringPair &symbol_pair, bool harmonize)
{
    HfstTokenizer::check_utf8_correctness(symbol_pair.first);
    HfstTokenizer::check_utf8_correctness(symbol_pair.second);

    if (symbol_pair.first == "" || symbol_pair.second == "")
      HFST_THROW_MESSAGE
    (EmptyStringException,
     "insert_freely(const StringPair&)");

    HfstTransducer tr(symbol_pair.first, symbol_pair.second, this->get_type());
    return this->insert_freely(tr, harmonize);
}

HfstTransducer &HfstTransducer::insert_freely
(const HfstTransducer &tr, bool harmonize)
{
    if (this->type != tr.type)
    HFST_THROW_MESSAGE(TransducerTypeMismatchException,
               "HfstTransducer::insert_freely");

    // Segfaults in xfst command line tool...
#if HAVE_XFSM
    if (this->type == XFSM_TYPE)
      {
        this->xfsm_interface.insert_freely(this->implementation.xfsm, tr.implementation.xfsm);
        return *this;
      }
#endif

    /* In this function, this transducer must always be harmonized
       according to tr, not the other way round. */
    HfstTransducer * tr_harmonized = NULL;
    if (harmonize)
      {
        bool harm = harmonize_smaller;
        harmonize_smaller=false;
        tr_harmonized = this->harmonize_(tr);
        harmonize_smaller=harm;
      }

    if (tr_harmonized == NULL) { // foma or no harmonization
      tr_harmonized = new HfstTransducer(tr);
    }

    switch (this->type)
    {
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
    {
    hfst::implementations::HfstBasicTransducer * net =
            ConversionFunctions::tropical_ofst_to_hfst_basic_transducer
            (implementation.tropical_ofst);
    tropical_ofst_interface.delete_transducer(implementation.tropical_ofst);
          
    hfst::implementations::HfstBasicTransducer * substituting_net =
            ConversionFunctions::tropical_ofst_to_hfst_basic_transducer
            (tr_harmonized->implementation.tropical_ofst);
    delete tr_harmonized;
          
    net->insert_freely(*substituting_net);
    delete substituting_net;
    implementation.tropical_ofst =
            ConversionFunctions::hfst_basic_transducer_to_tropical_ofst(net);
    delete net;
    return *this;
    break;
    }
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
    {
    hfst::implementations::HfstBasicTransducer * net =
            ConversionFunctions::log_ofst_to_hfst_basic_transducer
            (implementation.log_ofst);
    log_ofst_interface.delete_transducer(implementation.log_ofst);
          
    hfst::implementations::HfstBasicTransducer * substituting_net =
            ConversionFunctions::log_ofst_to_hfst_basic_transducer
            (tr_harmonized->implementation.log_ofst);
    delete tr_harmonized;

    net->insert_freely(*substituting_net);
    delete substituting_net;
    implementation.log_ofst =
            ConversionFunctions::hfst_basic_transducer_to_log_ofst(net);
    delete net;
    return *this;
    break;
    }
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
    {
    // HfstTransducer::harmonize does nothing to a foma transducer,
    // because foma's own functions take care of harmonizing.
    // Now we need to harmonize because we are using internal transducers.
      if (harmonize)
        {
          this->foma_interface.harmonize
            (implementation.foma,tr_harmonized->implementation.foma);
        }

    hfst::implementations::HfstBasicTransducer * net =
            ConversionFunctions::foma_to_hfst_basic_transducer
            (implementation.foma);
    this->foma_interface.delete_foma(implementation.foma);
          
    hfst::implementations::HfstBasicTransducer * substituting_net =
            ConversionFunctions::foma_to_hfst_basic_transducer
            (tr_harmonized->implementation.foma);
    delete tr_harmonized;

    net->insert_freely(*substituting_net);
    delete substituting_net;
    implementation.foma =
            ConversionFunctions::hfst_basic_transducer_to_foma(net);
    delete net;
    return *this;
    break;
    }
#endif
#if HAVE_SFST
    case SFST_TYPE:
    {
    hfst::implementations::HfstBasicTransducer * net =
            ConversionFunctions::sfst_to_hfst_basic_transducer
            (implementation.sfst);
    sfst_interface.delete_transducer(implementation.sfst);
          
    hfst::implementations::HfstBasicTransducer * substituting_net =
            ConversionFunctions::sfst_to_hfst_basic_transducer
            (tr_harmonized->implementation.sfst);
    delete tr_harmonized;

    net->insert_freely(*substituting_net);
    delete substituting_net;
    implementation.sfst =
            ConversionFunctions::hfst_basic_transducer_to_sfst(net);
    delete net;
    return *this;
    break;
    }
#endif
    /* Add here your implementation. */
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }

    return *this;
}

// -----------------------------------------------------------------------
//
//                        Substitution functions
//
// -----------------------------------------------------------------------

HfstTransducer &HfstTransducer::substitute
(bool (*func)(const StringPair &sp, StringPairSet &sps))
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
  hfst::implementations::HfstBasicTransducer * net
    = convert_to_basic_transducer();
  net->substitute(func);
  return convert_to_hfst_transducer(net);
}

HfstTransducer &HfstTransducer::substitute
(const std::string &old_symbol, const std::string &new_symbol,
 bool input_side, bool output_side)
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
  // empty strings are not accepted
  if (old_symbol == "" || new_symbol == "")
    HFST_THROW_MESSAGE
      (EmptyStringException,
       "substitute(const std::string&, const std::string&, bool, bool)");
  
  // if there are implementations available, use them

#if HAVE_SFST
  // do not use until substituted symbols are correctly erased from the alphabet
  if (false && (this->type == SFST_TYPE && input_side && output_side))
    {
      hfst::implementations::Transducer * tmp =
    this->sfst_interface.substitute
    (implementation.sfst, old_symbol, new_symbol);
      sfst_interface.delete_transducer(implementation.sfst);
      implementation.sfst = tmp;
      return *this;
    }
#endif
#if HAVE_OPENFST
  // do not use until substituted symbols are correctly erased from the alphabet
  if (false && (this->type == TROPICAL_OPENFST_TYPE && input_side && output_side))
      {
    fst::StdVectorFst * tmp =
      this->tropical_ofst_interface.substitute
      (implementation.tropical_ofst, old_symbol, new_symbol);
    tropical_ofst_interface.delete_transducer(implementation.tropical_ofst);
    implementation.tropical_ofst = tmp;
    return *this;
      }
#if HAVE_OPENFST_LOG
    if (this->type == LOG_OPENFST_TYPE && input_side && output_side)
      {
    hfst::implementations::LogFst * tmp =
      this->log_ofst_interface.substitute
      (implementation.log_ofst, old_symbol, new_symbol);
    log_ofst_interface.delete_transducer(implementation.log_ofst);
    implementation.log_ofst = tmp;
    return *this;
      }
#endif
#endif
    
    // use the default HfstBasicTransducer function
    hfst::implementations::HfstBasicTransducer * net =
      convert_to_basic_transducer();
    net->substitute(old_symbol,
                    new_symbol,
                    input_side, output_side);
    return convert_to_hfst_transducer(net);
}


HfstTransducer &HfstTransducer::substitute
(const StringPair &old_symbol_pair,
 const StringPair &new_symbol_pair)
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
  // empty strings are not accepted
  if (old_symbol_pair.first == "" || old_symbol_pair.second == "" ||
      new_symbol_pair.first == "" || new_symbol_pair.second == "")
    HFST_THROW_MESSAGE
      (EmptyStringException,
       "substitute(const StringPair&, const StringPair&)");
  
  hfst::implementations::HfstBasicTransducer * net
    = convert_to_basic_transducer();
  net->substitute(old_symbol_pair, new_symbol_pair);
  convert_to_hfst_transducer(net);
  return *this;
}

HfstTransducer &HfstTransducer::substitute
(const StringPair &old_symbol_pair,
 const StringPairSet &new_symbol_pair_set)
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
  if(old_symbol_pair.first == "" || old_symbol_pair.second == "")
    HFST_THROW_MESSAGE
      (EmptyStringException,
       "substitute(const StringPair&, const StringPairSet&");

  hfst::implementations::HfstBasicTransducer * net
    = convert_to_basic_transducer();
  net->substitute(old_symbol_pair, new_symbol_pair_set);
  return convert_to_hfst_transducer(net);

}

HfstTransducer & HfstTransducer::substitute_symbol(const std::string &old_symbol, const std::string &new_symbol, bool input_side, bool output_side)
{ return this->substitute(old_symbol, new_symbol, input_side, output_side); }
HfstTransducer & HfstTransducer::substitute_symbol_pair(const StringPair &old_symbol_pair, const StringPair &new_symbol_pair)
{ return this->substitute(old_symbol_pair, new_symbol_pair); }
HfstTransducer & HfstTransducer::substitute_symbol_pair_with_set(const StringPair &old_symbol_pair, const hfst::StringPairSet &new_symbol_pair_set)
{ return this->substitute(old_symbol_pair, new_symbol_pair_set); }
HfstTransducer & HfstTransducer::substitute_symbol_pair_with_transducer(const StringPair &symbol_pair, HfstTransducer &transducer, bool harmonize)
{ return this->substitute(symbol_pair, transducer, harmonize); }

HfstTransducer &HfstTransducer::substitute_symbols
(const HfstSymbolSubstitutions &substitutions)
{ return this->substitute(substitutions); }

HfstTransducer &HfstTransducer::substitute
(const HfstSymbolSubstitutions &substitutions)
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
  hfst::implementations::HfstBasicTransducer * net
    = convert_to_basic_transducer();

  try  {
    net->substitute(substitutions);
  }
  catch (const FunctionNotImplementedException & e) {
    (void)e;
    for (HfstSymbolSubstitutions::const_iterator it =
           substitutions.begin(); it != substitutions.end(); it++) {
      net->substitute(it->first, it->second, true, true);
    }
  }
  
  return convert_to_hfst_transducer(net);
}

HfstTransducer &HfstTransducer::substitute_symbol_pairs
(const HfstSymbolPairSubstitutions &substitutions)
{
  return this->substitute(substitutions);
}

HfstTransducer &HfstTransducer::substitute
(const HfstSymbolPairSubstitutions &substitutions)
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
  hfst::implementations::HfstBasicTransducer * net
    = convert_to_basic_transducer();
  net->substitute(substitutions);
  return convert_to_hfst_transducer(net);
}

HfstTransducer &HfstTransducer::substitute
(const StringPair &symbol_pair,
 HfstTransducer &transducer, bool harmonize)
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
    if (this->type != transducer.type) {
    HFST_THROW_MESSAGE(TransducerTypeMismatchException,
               "HfstTransducer::substitute"); }

    if (symbol_pair.first == "" || symbol_pair.second == "")
      HFST_THROW_MESSAGE
    (EmptyStringException,
     "substitute(const StringPair&, HfstTransducer&)");

#if HAVE_SFST && HAVE_OPENFST
    if (this->type == SFST_TYPE)
      {
        HfstTransducer this_copy(*this);
        this_copy.convert(TROPICAL_OPENFST_TYPE);
        HfstTransducer transducer_copy(transducer);
        transducer_copy.convert(TROPICAL_OPENFST_TYPE);
        HfstTransducer retval = this_copy.substitute(symbol_pair, transducer_copy, harmonize);
        retval.convert(SFST_TYPE);
        *this = retval;
        return *this;
      }
#endif

    HfstTransducer pairTransducer(symbol_pair.first, symbol_pair.second, this->type);
    if (! harmonize)
      {
        this->insert_missing_symbols_to_alphabet_from(pairTransducer);
        pairTransducer.insert_missing_symbols_to_alphabet_from(*this);
      }
    this->insert_missing_symbols_to_alphabet_from(pairTransducer, true);
    pairTransducer.insert_missing_symbols_to_alphabet_from(*this, true);

    this->harmonize(pairTransducer);

    if (! harmonize)
      {
        this->insert_missing_symbols_to_alphabet_from(transducer);
        transducer.insert_missing_symbols_to_alphabet_from(*this);
      }
    this->insert_missing_symbols_to_alphabet_from(transducer, true);
    transducer.insert_missing_symbols_to_alphabet_from(*this, true);

    bool harm = harmonize_smaller;
    harmonize_smaller=false;
    this->harmonize(transducer);
    harmonize_smaller=harm;

#if HAVE_FOMA
    if (this->type == FOMA_TYPE)
    {
        // HfstTransducer::harmonize does nothing to a foma transducer,
        // because foma's own functions take care of harmonizing.
        // Now we need to harmonize because we are using internal transducers.
        this->foma_interface.harmonize
        (implementation.foma,transducer.implementation.foma);

        hfst::implementations::HfstBasicTransducer * net =
        ConversionFunctions::foma_to_hfst_basic_transducer
        (implementation.foma);
        this->foma_interface.delete_foma(implementation.foma);

        hfst::implementations::HfstBasicTransducer * substituting_net =
        ConversionFunctions::foma_to_hfst_basic_transducer
        (transducer.implementation.foma);

        net->substitute(symbol_pair, *substituting_net);
        delete substituting_net;
        implementation.foma =
        ConversionFunctions::hfst_basic_transducer_to_foma(net);
        delete net;
        return *this;
    }
#endif
#if HAVE_OPENFST
    if (this->type == TROPICAL_OPENFST_TYPE)
    {
      this->tropical_ofst_interface.substitute
        (implementation.tropical_ofst,
         symbol_pair,transducer.implementation.tropical_ofst);

      return *this;
    }
#if HAVE_OPENFST_LOG
    if (this->type == LOG_OPENFST_TYPE)
    {
        this->log_ofst_interface.substitute
        (implementation.log_ofst,
         symbol_pair,transducer.implementation.log_ofst);
        return *this;
    }
#endif
#endif
    if (this->type == ERROR_TYPE) {
    HFST_THROW(TransducerHasWrongTypeException);
    }

    HFST_THROW(FunctionNotImplementedException);
}


// -----------------------------------------------------------------------
//
//                        Weight handling
//
// -----------------------------------------------------------------------

HfstTransducer &HfstTransducer::set_final_weights(float weight, bool increment)
{
#if HAVE_OPENFST
    if (this->type == TROPICAL_OPENFST_TYPE) {
    implementation.tropical_ofst  =
        this->tropical_ofst_interface.set_final_weights
      (this->implementation.tropical_ofst, weight, increment);
    return *this;
    }
#if HAVE_OPENFST_LOG
    if (this->type == LOG_OPENFST_TYPE) {
    implementation.log_ofst  =
        this->log_ofst_interface.set_final_weights
        (this->implementation.log_ofst, weight);
    return *this;
    }
#endif
#endif
    (void)weight;
    return *this;
}

HfstTransducer &HfstTransducer::push_labels(PushType push_type)
{
#if HAVE_OPENFST
    bool to_initial_state = (push_type == TO_INITIAL_STATE);
    if (this->type == TROPICAL_OPENFST_TYPE)
    {
        hfst::implementations::StdVectorFst * tmp  =
        this->tropical_ofst_interface.push_labels
        (this->implementation.tropical_ofst, to_initial_state);
        this->tropical_ofst_interface.delete_transducer(this->implementation.tropical_ofst);
        this->implementation.tropical_ofst = tmp;
        return *this;
    }
#if HAVE_OPENFST_LOG
    if (this->type == LOG_OPENFST_TYPE)
    {
        hfst::implementations::LogFst * tmp =
        this->log_ofst_interface.push_labels
        (this->implementation.log_ofst, to_initial_state);
        this->log_ofst_interface.delete_transducer(this->implementation.log_ofst);
        this->implementation.log_ofst = tmp;
        return *this;
    }
#endif
#endif
    (void)push_type;
    return *this;
}

HfstTransducer &HfstTransducer::push_weights(PushType push_type)
{
#if HAVE_OPENFST
    bool to_initial_state = (push_type == TO_INITIAL_STATE);
    if (this->type == TROPICAL_OPENFST_TYPE)
    {
        hfst::implementations::StdVectorFst * tmp  =
        this->tropical_ofst_interface.push_weights
        (this->implementation.tropical_ofst, to_initial_state);
        this->tropical_ofst_interface.delete_transducer(this->implementation.tropical_ofst);
        this->implementation.tropical_ofst = tmp;
        return *this;
    }
#if HAVE_OPENFST_LOG
    if (this->type == LOG_OPENFST_TYPE)
    {
        hfst::implementations::LogFst * tmp =
        this->log_ofst_interface.push_weights
        (this->implementation.log_ofst, to_initial_state);
        this->log_ofst_interface.delete_transducer(this->implementation.log_ofst);
        this->implementation.log_ofst = tmp;
        return *this;
    }
#endif
#endif
    (void)push_type;
    return *this;
}


HfstTransducer &HfstTransducer::transform_weights(float (*func)(float))
{
#if HAVE_OPENFST
    if (this->type == TROPICAL_OPENFST_TYPE) {
    implementation.tropical_ofst  =
        this->tropical_ofst_interface.transform_weights
        (this->implementation.tropical_ofst, func);
    return *this;
    }
#if HAVE_OPENFST_LOG
    if (this->type == LOG_OPENFST_TYPE) {
    implementation.log_ofst  =
        this->log_ofst_interface.transform_weights
        (this->implementation.log_ofst, func);
    return *this;
    }
#endif
#endif
    (void)func;
    return *this;
}

bool HfstTransducer::has_weights() const
{
#if HAVE_OPENFST
  if (this->type == TROPICAL_OPENFST_TYPE)
    {
      return this->tropical_ofst_interface.has_weights(implementation.tropical_ofst);
    }
#if HAVE_OPENFST_LOG
    if (this->type == LOG_OPENFST_TYPE)
      {
	HFST_THROW(FunctionNotImplementedException);
      }
#endif
#endif
    return false;
}


// -----------------------------------------------------------------------
//
//                        Binary operators
//
// -----------------------------------------------------------------------

bool substitute_single_identity_with_the_other_symbol
(const StringPair &sp, StringPairSet &sps)
{
    std::string isymbol = sp.first;
    std::string osymbol = sp.second;
    
    if (isymbol.compare("@_IDENTITY_SYMBOL_@") == 0 &&
        (osymbol.compare("@_IDENTITY_SYMBOL_@") != 0)) {
    isymbol = std::string("@_UNKNOWN_SYMBOL_@");
    sps.insert(StringPair(isymbol, osymbol));
    return true;
    }
    else if (osymbol.compare("@_IDENTITY_SYMBOL_@") == 0 &&
             (isymbol.compare("@_IDENTITY_SYMBOL_@") != 0)) {
    osymbol = std::string("@_UNKNOWN_SYMBOL_@");
    sps.insert(StringPair(isymbol, osymbol));
    return true;
    }
    else
    return false;
}

bool substitute_unknown_identity_pairs
(const StringPair &sp, StringPairSet &sps)
{
    std::string isymbol = sp.first;
    std::string osymbol = sp.second;

    if (isymbol.compare("@_UNKNOWN_SYMBOL_@") == 0 &&
        osymbol.compare("@_IDENTITY_SYMBOL_@") == 0) {
    isymbol = std::string("@_IDENTITY_SYMBOL_@");
    osymbol = std::string("@_IDENTITY_SYMBOL_@");
    sps.insert(StringPair(isymbol, osymbol));
    return true;
    }
    return false;
}


HfstTransducer &HfstTransducer::merge
(const HfstTransducer &another, const struct hfst::xre::XreConstructorArguments & args)
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
  HfstBasicTransducer this_basic(*this);
  HfstBasicTransducer another_basic(another);
  std::set<std::string> markers_added;
  HfstBasicTransducer result = hfst::implementations::HfstBasicTransducer::merge(this_basic, another_basic, args.list_definitions, markers_added);
  HfstTransducer initial_merge(result, this->get_type());
  initial_merge.optimize();

  // filter non-optimal paths
  // [ ? | #V ?:? ]* %#V:V ?:0 [ ? | #V ?:? | %#V:V ?:0 ]*
  hfst::xre::XreCompiler xre_(args);
  xre_.set_verbosity(false);

  for (std::set<std::string>::const_iterator it = markers_added.begin(); it != markers_added.end(); it++)
    {
      std::string marker = *it;
      std::string symbol(1, it->at(1)); // @X@ -> X
      std::string worsener_string("[ ? | \"" + marker +  "\" ?:? ]* \"" + marker + "\":" + symbol + " ?:0 [ ? | \"" + marker + "\" ?:? | \"" + marker + "\":" + symbol + " ?:0 ]* ;");

      HfstTransducer * worsener = xre_.compile(worsener_string);
      assert(worsener != NULL);
      worsener->optimize();
      HfstTransducer cp(initial_merge);
      cp.compose(*worsener).output_project().optimize();
      delete worsener;

      initial_merge.subtract(cp).optimize();
      initial_merge.substitute(marker, internal_epsilon);

      HfstBasicTransducer fsm(initial_merge);
      StringSet symbols = fsm.symbols_used();
      if (symbols.find(symbol) == symbols.end())
        {
          initial_merge.remove_from_alphabet(symbol);
        }
    }

  *this = initial_merge;
  return *this;
}

HfstTransducer &HfstTransducer::compose
(const HfstTransducer &another,
 bool harmonize)
{ is_trie = false;

  if (this->type != another.type)
    HFST_THROW(TransducerTypeMismatchException);

    HfstTransducer * another_copy = new HfstTransducer(another);

    /* If we want flag diacritcs to be handled in the same way as epsilons
       in composition, we substitute output flags of first transducer with
       epsilons and input flags of second transducer with epsilons. NOTE:
       we assume that flag diacritics are always used as identities, i.e.
       @FLAG1@:foo, foo:@FLAG2@ or @FLAG1@:@FLAG2@ are not allowed in
       transitions.

       In XFSM, this is controlled through a variable that has been set
       already when set_flag_is_epsilon_in_composition() has been called */
    if (flag_is_epsilon_in_composition && this->type != XFSM_TYPE)
      {
        try
          {
            this->substitute(&substitute_output_flag_with_epsilon);
            another_copy->substitute(&substitute_input_flag_with_epsilon);
          }
        catch (const char * msg)
          {
            (void)msg;
            HFST_THROW(FlagDiacriticsAreNotIdentitiesException);
          }
      }

    // Variables possibly needed next.
    StringSet diacritics_added_from_another_to_this;
    StringSet diacritics_added_from_this_to_another;

    /* If we want flag diacritics to match with identities and unknowns (as
       Xerox does), we encode flags as normal symbols before composition. We do
       not need to do this for XFSM transducers, as this is already the default
       behavior.

       If we do not want flags to match with identities and unknowns (the default
       for non-XFSM transducers) and the transducer is of XFSM_TYPE, we add flags
       of first transducer to alphabet of second transducer if they are not yet
       there and vice versa before composition. */
    if (xerox_composition)
      {
        if (this->type != XFSM_TYPE)
          {
            encode_flag_diacritics(*this);
            encode_flag_diacritics(*another_copy);
          }
      }
    else
      {
        if (this->type == XFSM_TYPE)
          {
            diacritics_added_from_another_to_this
              = this->insert_missing_diacritics_to_alphabet_from(*another_copy);
            diacritics_added_from_this_to_another
              = another_copy->insert_missing_diacritics_to_alphabet_from(*this);
          }
      }


    /* Prevent harmonization (i.e. matching unknown symbols), if requested. */
    if (! harmonize)
      {
        this->insert_missing_symbols_to_alphabet_from(*another_copy);
        another_copy->insert_missing_symbols_to_alphabet_from(*this);
      }

    // hfst-lexc's @_REG.Root_#_@ ??
    /* Special symbols are never harmonized, add them to alphabets of
       both transducers to prevent them from being matched with unknowns
       and identities. */
    this->insert_missing_symbols_to_alphabet_from(*another_copy, true);
    another_copy->insert_missing_symbols_to_alphabet_from(*this, true);

    // Harmonize, FOMA and XFSM take care of this by default.
    if (this->type != FOMA_TYPE && this->type != XFSM_TYPE)
      {
        HfstTransducer * tmp =
          this->harmonize_(const_cast<HfstTransducer&>(*another_copy));
        delete another_copy;
        another_copy = tmp;
      }

    /* Take care of unknown and identity symbols being handled right in
       composition, FOMA and XFSM take care of this by default. */
    if ( (this->type != FOMA_TYPE && this->type != XFSM_TYPE) && unknown_symbols_in_use)
      {
        this->substitute("@_IDENTITY_SYMBOL_@","@_UNKNOWN_SYMBOL_@",false,true);
        const_cast<HfstTransducer*>(another_copy)->substitute
          ("@_IDENTITY_SYMBOL_@","@_UNKNOWN_SYMBOL_@",true,false);
      }

    switch (this->type)
    {
#if HAVE_SFST
    case SFST_TYPE:
    {
    SFST::Transducer * sfst_temp =
      this->sfst_interface.compose
      (implementation.sfst,another_copy->implementation.sfst);
    sfst_interface.delete_transducer(implementation.sfst);
    implementation.sfst = sfst_temp;
    break;
    }
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
    {
    fst::StdVectorFst * tropical_ofst_temp =
            this->tropical_ofst_interface.compose
        (this->implementation.tropical_ofst,
         another_copy->implementation.tropical_ofst);
    tropical_ofst_interface.delete_transducer(implementation.tropical_ofst);
    implementation.tropical_ofst = tropical_ofst_temp;
    break;
    }
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
    {
    hfst::implementations::LogFst * log_ofst_temp =
            this->log_ofst_interface.compose
      (implementation.log_ofst,
       another_copy->implementation.log_ofst);
    log_ofst_interface.delete_transducer(implementation.log_ofst);
    implementation.log_ofst = log_ofst_temp;
    break;
    }
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
    {
    fsm * foma_temp =
            this->foma_interface.compose
            (implementation.foma,another_copy->implementation.foma);
    this->foma_interface.delete_foma(implementation.foma);
    implementation.foma = foma_temp;
    break;
    }
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
      {
        this->implementation.xfsm =
          this->xfsm_interface.compose(this->implementation.xfsm,
                                       another_copy->implementation.xfsm);
        break;
      }
#endif
    #if HAVE_HFSTOL
    case HFST_OL_TYPE:
    case HFST_OLW_TYPE:
        HFST_THROW(HfstTransducerTypeMismatchException);
        // This is the exception the tool wants to hear
    #endif
    case ERROR_TYPE:
        HFST_THROW(TransducerHasWrongTypeException);
    default:
        HFST_THROW(FunctionNotImplementedException);
    }

    // Revert changes made before composition

    if (xerox_composition)
      {
        if (this->type != XFSM_TYPE)
          {
            decode_flag_diacritics(*this);
            decode_flag_diacritics(*another_copy);
          }
      }
    else
      {
        if (this->type == XFSM_TYPE)
          {
            this->remove_symbols_from_alphabet(diacritics_added_from_another_to_this);
            another_copy->remove_symbols_from_alphabet(diacritics_added_from_this_to_another);
          }
      }

    if (flag_is_epsilon_in_composition && this->type != XFSM_TYPE)
      {
        this->substitute(&substitute_one_sided_flags);
      }

    if ( (this->type != FOMA_TYPE && this->type != XFSM_TYPE) && unknown_symbols_in_use)
    {
        this->substitute(&substitute_single_identity_with_the_other_symbol);
        (const_cast<HfstTransducer*>(another_copy))->
       substitute(&substitute_unknown_identity_pairs);
    }
    delete another_copy;

    return *this;
}

// Composition with this transducer restricts _1_flags ($X.Y_1.Z$) so
// they can't succeed _2_flags ($X.Y_2.Z$) immediately. Used for
// filtering illegal combinations of flag diacritics after binary
// operations.
HfstTransducer get_flag_path_restriction(const StringSet &_1_flags,
                                         const StringSet &_2_flags,
                                         ImplementationType type)
{
  // Two state fst with borh states final.
  HfstBasicTransducer basic_restriction;
  basic_restriction.add_state();
  implementations::HfstState start_state = 0;
  implementations::HfstState seen_2_state = 1;

  basic_restriction.set_final_weight(start_state, 0.0);
  basic_restriction.set_final_weight(seen_2_state, 0.0);

  basic_restriction.add_transition
    (start_state,
     HfstBasicTransition(start_state,
                         internal_identity,
                         internal_identity,
                         0.0));

  basic_restriction.add_transition
    (seen_2_state,
     HfstBasicTransition(start_state,
                         internal_identity,
                         internal_identity,
                         0.0));

  // All _1_flags are allowed as long as no _2_flags with no
  // intervening symbols were observed.
  for (StringSet::const_iterator it = _1_flags.begin();
       it != _1_flags.end();
       ++it)
    {
      std::string dollar_flag = *it;
      dollar_flag.at(0) = '$';
      dollar_flag.at(dollar_flag.size() - 1) = '$';

      basic_restriction.add_transition
        (start_state,
         HfstBasicTransition(start_state,
                             dollar_flag,
                             dollar_flag,
                             0.0));
    }

  // If _2_flags are observed, _1_flags are illegal before an
  // intervening regular symbol is seen.
  for (StringSet::const_iterator it = _2_flags.begin();
       it != _2_flags.end();
       ++it)
    {
      std::string dollar_flag = *it;
      dollar_flag.at(0) = '$';
      dollar_flag.at(dollar_flag.size() - 1) = '$';

      basic_restriction.add_transition
        (start_state,
         HfstBasicTransition(seen_2_state,
                             dollar_flag,
                             dollar_flag,
                             0.0));

      basic_restriction.add_transition
        (seen_2_state,
         HfstBasicTransition(seen_2_state,
                             dollar_flag,
                             dollar_flag,
                             0.0));
    }

  HfstTransducer restriction(basic_restriction,
                             type);

  return restriction;
}

HfstTransducer &HfstTransducer::remove_illegal_flag_paths(void)
{
  StringSet alphabet = this->get_alphabet();
  StringSet _1_flags;
  StringSet _2_flags;

  // Gather _1 and _2 flag diacritics.
  for (StringSet::const_iterator it = alphabet.begin();
       it != alphabet.end();
       ++it)
    {
      if (! FdOperation::is_diacritic(*it))
        { continue; }

      if (it->find("_1.") != std::string::npos)
        {
          _1_flags.insert(*it);
        }

      if (it->find("_2.") != std::string::npos)
        {
          _2_flags.insert(*it);
        }
    }

  // if there aren't both _1 and _2 flag diaciritcs, there can be no
  // illegal paths.
  if (_1_flags.empty() || _2_flags.empty())
    { return *this; }

  // Rename @...@ flags to $...$ flags and compile restriction.
  HfstSymbolSubstitutions subst;
  HfstSymbolSubstitutions back_subst;
  
  for (StringSet::const_iterator it = _1_flags.begin();
       it != _1_flags.end();
       ++it)
    {
      std::string at_flag = *it;
      std::string dollar_flag = *it;
      dollar_flag.at(0) = '$';
      dollar_flag.at(dollar_flag.size() - 1) = '$';

      subst[at_flag] = dollar_flag;
      back_subst[dollar_flag] = at_flag;
    }

  for (StringSet::const_iterator it = _2_flags.begin();
       it != _2_flags.end();
       ++it)
    {
      std::string at_flag = *it;
      std::string dollar_flag = *it;
      dollar_flag.at(0) = '$';
      dollar_flag.at(dollar_flag.size() - 1) = '$';

      subst[at_flag] = dollar_flag;
      back_subst[dollar_flag] = at_flag;
    }

  this->substitute(subst);

  HfstTransducer restriction = get_flag_path_restriction(_1_flags,
                                                         _2_flags,
                                                         this->type);


  // Apply restrictions.
  this->compose(restriction);

  // Rename $...$ flags back to @...@ flags.
  this->substitute(back_subst);

  return *this;
}

HfstTransducer &HfstTransducer::lenient_composition( const HfstTransducer &another, bool /*harmonize*/)
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
    if ( this->type != another.type )
    {
        HFST_THROW_MESSAGE(HfstTransducerTypeMismatchException, "HfstTransducer::lenient_composition");
    }

    HfstTransducer retval(*this);
    // true is a dummy variable, false means do not encode epsilons
    retval.compose(another).optimize().priority_union(*this).optimize();

    *this = retval;
    return *this;

}


HfstTransducer &HfstTransducer::cross_product( const HfstTransducer &another, bool /*harmonize*/)
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
    if ( this->type != another.type )
    {
        HFST_THROW_MESSAGE(HfstTransducerTypeMismatchException, "HfstTransducer::cross_product");
    }

    HfstTransducer automata1(*this);
    HfstTransducer automata2(another);

    // Check if both input transducers are automata
    HfstTransducer t1_proj(automata1);
    t1_proj.input_project();
    HfstTransducer t2_proj(automata2);
    t2_proj.input_project();

    if ( ! t1_proj.compare(automata1) || ! t2_proj.compare(automata2) )
    {
            HFST_THROW_MESSAGE(TransducersAreNotAutomataException, "HfstTransducer::cross_product");
    }

    // Put MARK all over lower part of automata1 and upper part of automata2,
    // and then compose them
    // Also, there should be created padding after strings, on both sides

    automata1.insert_to_alphabet("@_MARK_@");
    automata2.insert_to_alphabet("@_MARK_@");

    HfstTokenizer TOK;
    TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");
    TOK.add_multichar_symbol("@_UNKNOWN_SYMBOL_@");
    TOK.add_multichar_symbol("@_MARK_@");


    // EpsilonToMark and MarkToEpsilon are paddings (if strings are not the same size)
    HfstTransducer UnknownToMark("@_UNKNOWN_SYMBOL_@", "@_MARK_@", TOK, type);
    HfstTransducer EpsilonToMark("@_EPSILON_SYMBOL_@", "@_MARK_@", TOK, type);

    HfstTransducer MarkToUnknown(UnknownToMark);
    MarkToUnknown.invert();
    HfstTransducer MarkToEpsilon(EpsilonToMark);
    MarkToEpsilon.invert();

    UnknownToMark.repeat_star().minimize(); // minimization is safe
    EpsilonToMark.repeat_star().minimize(); // minimization is safe
    MarkToUnknown.repeat_star().minimize(); // minimization is safe
    MarkToEpsilon.repeat_star().minimize(); // minimization is safe

    HfstTransducer a1(automata1);
    a1.compose(UnknownToMark).optimize().concatenate(EpsilonToMark).optimize();

    HfstTransducer b1(MarkToUnknown);
    b1.compose(automata2).optimize().concatenate(MarkToEpsilon).optimize();

    HfstTransducer retval(a1);
    retval.compose(b1).optimize();

    // Expand ?:? transitions to ?:?|?
    StringPairSet id_or_unk;
    id_or_unk.insert(StringPair("@_UNKNOWN_SYMBOL_@", "@_UNKNOWN_SYMBOL_@"));
    id_or_unk.insert(StringPair("@_IDENTITY_SYMBOL_@", "@_IDENTITY_SYMBOL_@"));
    retval.substitute(StringPair("@_UNKNOWN_SYMBOL_@", "@_UNKNOWN_SYMBOL_@"), id_or_unk);

    retval.remove_from_alphabet("@_MARK_@");

    *this = retval;
    return *this;

}

//
// -------------------- Shuffle functions --------------------
//

// A flag to indicate that there was an error during shuffle.
static bool shuffle_failed=false;
// Possible cases for function code_symbols_for_shuffle.
enum ShuffleCoding { ENCODE_FIRST_SHUFFLE_ARGUMENT,
                     ENCODE_SECOND_SHUFFLE_ARGUMENT,
                     DECODE_AFTER_SHUFFLE };
// The current case in function code_symbols_for_shuffle.
static ShuffleCoding shuffle_coding_case;

// A function that is given as a parameter to substitute function
// during the shuffle operation. The purpose of this function is (1)
// to encode symbols in the two argument transducers so that no symbol
// is present at both transducers or (2) to decode the symbols
// in the shuffled transducer back to the original ones.
bool code_symbols_for_shuffle(const StringPair &sp, StringPairSet &sps)
{
  // not automaton, shuffle fails
  if (sp.first != sp.second) {
    shuffle_failed=true;
    return false;
  }
  // special symbols are not coded, except identities
  if (is_epsilon(sp.first) ||
      is_unknown(sp.first)) {
    return false;
  }
  switch (shuffle_coding_case)
    {
      // substitute each symbol foo in the first argument transducer
      // with a symbol @1foo
    case ENCODE_FIRST_SHUFFLE_ARGUMENT:
        {
          std::string symbol_escaped = "@1" + sp.first;
          StringPair new_sp(symbol_escaped, symbol_escaped);
          sps.insert(new_sp);
          break;
        }
        // substitute each symbol bar in the second argument transducer
        // with a symbol @2bar
    case ENCODE_SECOND_SHUFFLE_ARGUMENT:
        {
          std::string symbol_escaped = "@2" + sp.first;
          StringPair new_sp(symbol_escaped, symbol_escaped);
          sps.insert(new_sp);
          break;
        }
        // substitute each symbol @1foo or @2bar in the shuffled transducer
        // with the original foo or bar.
    case DECODE_AFTER_SHUFFLE:
        {
          std::string symbol_unescaped = sp.first.substr(2);
          StringPair new_sp(symbol_unescaped, symbol_unescaped);
          sps.insert(new_sp);
          break;
        }
    default:
      assert(false);
    }
  
  return true;
}

HfstTransducer &HfstTransducer::shuffle(const HfstTransducer &another, bool)
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
  if (this->type != another.type)
    HFST_THROW_MESSAGE(TransducerTypeMismatchException,
                       "HfstTransducer::shuffle(const HfstTransducer&)");

  // We use HfstBasicTransducers for efficiency
  HfstBasicTransducer this_basic(*this);
  HfstBasicTransducer another_basic(another);

  // Expand (unknowns and) identities
  this_basic.harmonize(another_basic);

  // Find out the original alphabets of both transducers
  StringSet this_alphabet = this_basic.get_alphabet();
  StringSet another_alphabet = another_basic.get_alphabet();

  // Encode first transducer, i.e. prefix each symbol with "@1"
  shuffle_coding_case=ENCODE_FIRST_SHUFFLE_ARGUMENT;
  this_basic.substitute(&code_symbols_for_shuffle);
  // also remember to remove the unprefixed symbols from the alphabet
  this_basic.remove_symbols_from_alphabet(this_alphabet);

  // Encode second transducer, i.e. prefix each symbol with "@2"
  shuffle_coding_case=ENCODE_SECOND_SHUFFLE_ARGUMENT;
  another_basic.substitute(&code_symbols_for_shuffle);
  // also remember to remove the unprefixed symbols from the alphabet
  another_basic.remove_symbols_from_alphabet(another_alphabet);

  // See if shuffle failed, i.e. either transducer is not an automaton
  if (shuffle_failed) {
    shuffle_failed=false;
    HFST_THROW_MESSAGE(TransducersAreNotAutomataException,
                       "HfstTransducer::shuffle(const HfstTransducer&)");
  }

  // The new alphabets of transducers where each symbol is prefixed
  // with "@1" or "@2"
  this_alphabet = this_basic.get_alphabet();
  another_alphabet = another_basic.get_alphabet();

  // Transform alphabets of transducers into string pair sets for function
  // insert_freely
  StringPairSet this_alphabet_pairset;
  for (StringSet::const_iterator it = this_alphabet.begin();
       it != this_alphabet.end(); it++) {
    this_alphabet_pairset.insert(StringPair(*it, *it));
  }
  StringPairSet another_alphabet_pairset;
  for (StringSet::const_iterator it = another_alphabet.begin();
       it != another_alphabet.end(); it++) {
    another_alphabet_pairset.insert(StringPair(*it, *it));
  }

  // Freely insert any number of any symbol in the first transducer
  // to the second transducer and vice versa
  this_basic.insert_freely(another_alphabet_pairset, 0);
  another_basic.insert_freely(this_alphabet_pairset, 0);

  // We use HfstTransducers for intersection
  HfstTransducer this1(this_basic, this->get_type());
  HfstTransducer another1(another_basic, another.get_type());

  this1.intersect(another1);
  this1.optimize();
  
  // We use HfstBasicTransducers again
  HfstBasicTransducer this1_basic(this1);

  // Decode the shuffled transducer, i.e. remove the prefixes
  // "@1" and "@2" from symbols
  shuffle_coding_case=DECODE_AFTER_SHUFFLE;
  this1_basic.substitute(&code_symbols_for_shuffle);
  // also remember to remove the prefixed symbols from the alphabet
  this1_basic.remove_symbols_from_alphabet(this_alphabet);
  this1_basic.remove_symbols_from_alphabet(another_alphabet);

  // Convert once again to HfstTransducer
  HfstTransducer this_finally(this1_basic, this->get_type());
  this->operator=(this_finally);

  return *this;
}

// ---------------------- Shuffle functions end --------------------


// Q .P. R = Q | [~[Q .u] .o. R ]
// .u is input project
HfstTransducer &HfstTransducer::priority_union (const HfstTransducer &another)
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
    if ( this->type != another.type )
    {
        HFST_THROW_MESSAGE(HfstTransducerTypeMismatchException,
                   "HfstTransducer::priority_union");
    }
    HfstTransducer t1(*this);
    HfstTransducer t2(another);

    HfstTransducer t1upper(t1);
    t1upper.input_project().optimize();
    
    HfstTransducer complement(t1upper);
    complement.negate().prune_alphabet(false);

    complement.compose(t2, true).optimize();

    HfstTransducer retval(t1);
    retval.disjunct(complement).optimize();
    
    *this = retval;
    return *this;
}

HfstTransducer &HfstTransducer::compose_intersect
(const HfstTransducerVector &v, bool invert, bool)
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
  // Foma transducers don't harmonize porperly. If the input is foma
  // transducers, convert to openfst type.
  bool convert_to_openfst = false;
  if (get_type() == FOMA_TYPE)
    {
      convert_to_openfst = true;
      this->convert(TROPICAL_OPENFST_TYPE);
    }
  
  // The intersection of an empty set of rules is the empty language,
  // which makes the result empty.
  if (v.empty())
    { *this = HfstTransducer(type); }
  
  const HfstTransducer &first = *v.begin();
  
  // If rule transducers contain word boundaries, add word boundaries to
  // the lexicon unless the lexicon already contains them.
  std::set<std::string> rule_alphabet = first.get_alphabet();

  if (rule_alphabet.find("@#@") != rule_alphabet.end())
    {
      std::set<std::string> lexicon_alphabet = get_alphabet();
      HfstTokenizer tokenizer;
      tokenizer.add_multichar_symbol("@#@");
      tokenizer.add_multichar_symbol(internal_epsilon);
      HfstTransducer wb(internal_epsilon,"@#@",tokenizer,type);
      HfstTransducer wb_copy(wb);

      // Add the word boundary symbol to the alphabet so harmonization
      // won't touch it.
      HfstBasicTransducer basic_this(*this);
      basic_this.add_symbol_to_alphabet("@#@");
      *this = HfstTransducer(basic_this,this->get_type());

      wb.concatenate(*this).concatenate(wb_copy).optimize();
      *this = wb;
    }

    HfstTransducer rule_1 = v.at(0);
    
    if (convert_to_openfst)
      { rule_1.convert(TROPICAL_OPENFST_TYPE); }

    HfstTransducer * harmonized_lexicon = rule_1.harmonize_(*this);

    if (harmonized_lexicon == NULL)
      { harmonized_lexicon = new HfstTransducer(*this); }

    if (invert)
      {
        harmonized_lexicon->invert();
        harmonized_lexicon->substitute(StringPair("@#@",internal_epsilon),
                                       StringPair(internal_epsilon,"@#@"));
      }

    harmonized_lexicon->substitute(internal_identity,"||_IDENTITY_SYMBOL_||");
    harmonized_lexicon->substitute(internal_unknown,"||_UNKNOWN_SYMBOL_||");

    if (v.size() == 1)
    {
      HfstTransducer rule_fst = v.at(0);
      if (convert_to_openfst)
        { rule_fst.convert(TROPICAL_OPENFST_TYPE); }

      if (invert)
        {
          rule_fst.invert();
          rule_fst.substitute(StringPair(internal_epsilon,"@#@"),
                              StringPair("@#@",internal_epsilon));
        }
      
      // In case there is only onw rule, compose with that.
      implementations::ComposeIntersectRule rule(rule_fst);
      // Create a ComposeIntersectLexicon from *this.
      
      //implementations::ComposeIntersectLexicon lexicon(*this);
      implementations::ComposeIntersectLexicon lexicon(*harmonized_lexicon);
      
      hfst::implementations::HfstBasicTransducer res =
        lexicon.compose_with_rules(&rule);
      
      res.prune_alphabet();
      *this = HfstTransducer(res,type);
    }
    else
      {

        // In case there are many rules, build a ComposeIntersectRulePair
        // recursively and compose with that.
        
        HfstTransducer first_rule_fst = v.at(0);
        if (convert_to_openfst)
          { first_rule_fst.convert(TROPICAL_OPENFST_TYPE); }

        if (invert)
          {
            first_rule_fst.invert();
            first_rule_fst.substitute(StringPair(internal_epsilon,"@#@"),
                                      StringPair("@#@",internal_epsilon));
          }

        HfstTransducer second_rule_fst = v.at(1);
        if (convert_to_openfst)
          { second_rule_fst.convert(TROPICAL_OPENFST_TYPE); }

        if (invert)
          {
            second_rule_fst.invert();
            second_rule_fst.substitute(StringPair(internal_epsilon,"@#@"),
                                       StringPair("@#@",internal_epsilon));
          }

        std::vector<implementations::ComposeIntersectRule*> rule_vector;
        implementations::ComposeIntersectRule * first_rule =
          new implementations::ComposeIntersectRule(first_rule_fst);
        implementations::ComposeIntersectRule * second_rule =
          new implementations::ComposeIntersectRule(second_rule_fst);
        
        implementations::ComposeIntersectRulePair * rules =
          new implementations::ComposeIntersectRulePair
          (first_rule,second_rule);
        
        for (HfstTransducerVector::const_iterator it = v.begin() + 2;
             it != v.end();
             ++it)
          {
            HfstTransducer rule_fst(*it);
            if (convert_to_openfst)
              { rule_fst.convert(TROPICAL_OPENFST_TYPE); }

            if (invert)
              {
                rule_fst.invert();
                rule_fst.substitute(StringPair(internal_epsilon,"@#@"),
                                    StringPair("@#@",internal_epsilon));
              }
        
            rules = new implementations::ComposeIntersectRulePair
              (new implementations::ComposeIntersectRule(rule_fst),rules);
          }
        // Create a ComposeIntersectLexicon from *this.
        implementations::ComposeIntersectLexicon lexicon(*harmonized_lexicon);
        hfst::implementations::HfstBasicTransducer res =
          lexicon.compose_with_rules(rules);
        
        res.prune_alphabet();
        *this = HfstTransducer(res,type);
        
        if (invert)
          { this->invert(); }

        delete rules;
      }
    
    delete harmonized_lexicon;
    
    this->substitute("||_IDENTITY_SYMBOL_||",internal_identity);
    this->substitute("||_UNKNOWN_SYMBOL_||",internal_unknown);

    if (convert_to_openfst)
      { this->convert(FOMA_TYPE); }

    return *this;
}

HfstTransducer &HfstTransducer::concatenate
(const HfstTransducer &another, bool harmonize)
{ is_trie = false; // This could be done so that is_trie is preserved
    return apply
    (
#if HAVE_SFST
        &hfst::implementations::SfstTransducer::concatenate,
#endif
#if HAVE_OPENFST
        &hfst::implementations::TropicalWeightTransducer::concatenate,
#if HAVE_OPENFST_LOG
        &hfst::implementations::LogWeightTransducer::concatenate,
#endif
#endif
#if HAVE_FOMA
        &hfst::implementations::FomaTransducer::concatenate,
#endif
#if HAVE_XFSM
        &hfst::implementations::XfsmTransducer::concatenate,
#endif
        /* Add here your implementation. */
        //#if HAVE_MY_TRANSDUCER_LIBRARY
        //&hfst::implementations::MyTransducerLibraryTransducer::concatenate,
        //#endif
        const_cast<HfstTransducer&>(another), harmonize);
}



HfstTransducer &HfstTransducer::disjunct(const StringPairVector &spv)
{
    switch (this->type)
    {
#if HAVE_SFST
    case SFST_TYPE:
        sfst_interface.disjunct(implementation.sfst, spv);
        break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        tropical_ofst_interface.disjunct(implementation.tropical_ofst, spv);
        break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        HFST_THROW(FunctionNotImplementedException);
        break;
#endif
#endif
    case FOMA_TYPE:
        HFST_THROW(FunctionNotImplementedException);
        break;
    /* Add here your implementation. */
    default:
        assert(false);
    }
    return *this;
}

// TODO...
HfstTransducer &HfstTransducer::disjunct_as_tries(HfstTransducer &another,
                          ImplementationType type)
{
    convert(type);
    if (type != another.type)
    { another = HfstTransducer(another).convert(type); }

    switch (this->type)
    {
    case SFST_TYPE:
        HFST_THROW(FunctionNotImplementedException);
        break;
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        HFST_THROW(FunctionNotImplementedException);
        break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        HFST_THROW(FunctionNotImplementedException);
        break;
#endif
#endif
    case FOMA_TYPE:
        HFST_THROW(FunctionNotImplementedException);
        break;
    default:
        assert(false);
    }
    return *this;
}

HfstTransducer &HfstTransducer::disjunct
(const HfstTransducer &another, bool harmonize)
{
    is_trie = false;
    return apply(
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::disjunct,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::disjunct,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::disjunct,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::disjunct,
#endif
#if HAVE_XFSM
    &hfst::implementations::XfsmTransducer::disjunct,
#endif
    /* Add here your implementation. */
    const_cast<HfstTransducer&>(another), harmonize); }

HfstTransducer &HfstTransducer::intersect
(const HfstTransducer &another, bool harmonize)
{ is_trie = false; // This could be done so that is_trie is preserved
    return apply(
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::intersect,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::intersect,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::intersect,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::intersect,
#endif
#if HAVE_XFSM
    &hfst::implementations::XfsmTransducer::intersect,
#endif
    /* Add here your implementation. */
    const_cast<HfstTransducer&>(another), harmonize); }

HfstTransducer &HfstTransducer::subtract
(const HfstTransducer &another, bool harmonize)
{ is_trie = false; // This could be done so that is_trie is preserved
    return apply(
#if HAVE_SFST
    &hfst::implementations::SfstTransducer::subtract,
#endif
#if HAVE_OPENFST
    &hfst::implementations::TropicalWeightTransducer::subtract,
#if HAVE_OPENFST_LOG
    &hfst::implementations::LogWeightTransducer::subtract,
#endif
#endif
#if HAVE_FOMA
    &hfst::implementations::FomaTransducer::subtract,
#endif
#if HAVE_XFSM
    &hfst::implementations::XfsmTransducer::subtract,
#endif
    /* Add here your implementation. */
    const_cast<HfstTransducer&>(another), harmonize); }
  

// -----------------------------------------------------------------------
//
//                       Conversion functions
//
// -----------------------------------------------------------------------

implementations::HfstBasicTransducer * HfstTransducer::
get_basic_transducer() const
{
#if HAVE_SFST || HAVE_LEAN_SFST
    if (this->type == SFST_TYPE)
      {
        hfst::implementations::HfstBasicTransducer * net =
      ConversionFunctions::sfst_to_hfst_basic_transducer
      (implementation.sfst);
    return net;
      }
#endif
#if HAVE_OPENFST
    if (this->type == TROPICAL_OPENFST_TYPE)
      {
                hfst::implementations::HfstBasicTransducer * net =
      ConversionFunctions::tropical_ofst_to_hfst_basic_transducer
      (implementation.tropical_ofst);
    return net;
      }
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
    if (this->type == LOG_OPENFST_TYPE)
      {
        hfst::implementations::HfstBasicTransducer * net =
      ConversionFunctions::log_ofst_to_hfst_basic_transducer
      (implementation.log_ofst);
    return net;
      }
#endif
#endif
#if HAVE_FOMA
    if (this->type == FOMA_TYPE)
      {
        hfst::implementations::HfstBasicTransducer * net =
      ConversionFunctions::foma_to_hfst_basic_transducer
      (implementation.foma);
    return net;
      }
#endif
    if (this->type == ERROR_TYPE) {
    HFST_THROW(TransducerHasWrongTypeException);
    }
    HFST_THROW(FunctionNotImplementedException);
}


implementations::HfstBasicTransducer * HfstTransducer::
convert_to_basic_transducer()
{
#if HAVE_SFST || HAVE_LEAN_SFST
    if (this->type == SFST_TYPE)
      {
        hfst::implementations::HfstBasicTransducer * net =
      ConversionFunctions::sfst_to_hfst_basic_transducer
      (implementation.sfst);
        sfst_interface.delete_transducer(implementation.sfst);
    return net;
      }
#endif
#if HAVE_OPENFST
    if (this->type == TROPICAL_OPENFST_TYPE)
      {
        hfst::implementations::HfstBasicTransducer * net =
      ConversionFunctions::tropical_ofst_to_hfst_basic_transducer
      (implementation.tropical_ofst);
        tropical_ofst_interface.delete_transducer(implementation.tropical_ofst);
    return net;
      }
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
    if (this->type == LOG_OPENFST_TYPE)
      {
        hfst::implementations::HfstBasicTransducer * net =
      ConversionFunctions::log_ofst_to_hfst_basic_transducer
      (implementation.log_ofst);
        log_ofst_interface.delete_transducer(implementation.log_ofst);
    return net;
      }
#endif
#endif
#if HAVE_FOMA
    if (this->type == FOMA_TYPE)
      {
        hfst::implementations::HfstBasicTransducer * net =
      ConversionFunctions::foma_to_hfst_basic_transducer
      (implementation.foma);
        foma_interface.delete_foma(implementation.foma);
    return net;
      }
#endif
    if (this->type == ERROR_TYPE) {
    HFST_THROW(TransducerHasWrongTypeException);
    }
    HFST_THROW(FunctionNotImplementedException);
}


HfstTransducer &HfstTransducer::
convert_to_hfst_transducer(implementations::HfstBasicTransducer *t)
{
#if HAVE_SFST || HAVE_LEAN_SFST
    if (this->type == SFST_TYPE)
      {
        implementation.sfst =
        ConversionFunctions::hfst_basic_transducer_to_sfst(t);
        name = t->name;
        delete t;
    return *this;
      }
#endif
#if HAVE_OPENFST
    if (this->type == TROPICAL_OPENFST_TYPE)
      {
        implementation.tropical_ofst =
        ConversionFunctions::hfst_basic_transducer_to_tropical_ofst(t);
        name = t->name;
        delete t;
    return *this;
      }
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
    if (this->type == LOG_OPENFST_TYPE)
      {
        implementation.log_ofst =
        ConversionFunctions::hfst_basic_transducer_to_log_ofst(t);
        name = t->name;
        delete t;
    return *this;
      }
#endif
#endif
#if HAVE_FOMA
    if (this->type == FOMA_TYPE)
      {
        implementation.foma =
        ConversionFunctions::hfst_basic_transducer_to_foma(t);
        name = t->name;
        delete t;
    return *this;
      }
#endif
    if (this->type == ERROR_TYPE) {
    HFST_THROW(TransducerHasWrongTypeException);
    }
    HFST_THROW(FunctionNotImplementedException);
}

HfstTransducer &HfstTransducer::convert(const HfstTransducer &t,
                    ImplementationType type)
{
    if (type == ERROR_TYPE)
    {
    HFST_THROW_MESSAGE
        (SpecifiedTypeRequiredException, "HfstTransducer::convert");
    }
    if (type == t.type)
    { return *(new HfstTransducer(t)); }
    if (! is_lean_implementation_type_available(type)) {
      throw ImplementationTypeNotAvailableException("HfstTransducer::convert", __FILE__, __LINE__, type);
    }

    hfst::implementations::HfstBasicTransducer net(t);

    HfstTransducer * retval = new HfstTransducer(net, type);

    return *retval;
}


/* ERROR_TYPE or UNSPECIFIED_TYPE returns true, so they must be handled
   separately */
bool HfstTransducer::is_implementation_type_available
(ImplementationType type) {
#if !HAVE_FOMA
    if (type == FOMA_TYPE)
      return false;
#endif
#if !HAVE_SFST
    if (type == SFST_TYPE)
      return false;
#endif
#if !HAVE_OPENFST
    if (type == TROPICAL_OPENFST_TYPE || type == LOG_OPENFST_TYPE)
      return false;
#endif
#if !HAVE_OPENFST_LOG
    if (type == LOG_OPENFST_TYPE)
      return false;
#endif
#if !HAVE_XFSM
    if (type == XFSM_TYPE)
      return false;
#endif
    /* Add here your implementation. */
    //#if !HAVE_MY_TRANSDUCER_LIBRARY
    //if (type == MY_TRANSDUCER_LIBRARY_TYPE)
    //  return false;
    //#endif
    (void)type;
    return true;
}

bool HfstTransducer::is_lean_implementation_type_available
(ImplementationType type) {
#if !HAVE_FOMA
    if (type == FOMA_TYPE)
      return false;
#endif
#if !HAVE_SFST
#if !HAVE_LEAN_SFST
    if (type == SFST_TYPE)
      return false;
#endif
#endif
#if !HAVE_OPENFST
    if (type == TROPICAL_OPENFST_TYPE || type == LOG_OPENFST_TYPE)
      return false;
#endif
#if !HAVE_OPENFST_LOG
#if !HAVE_LEAN_OPENFST_LOG
    if (type == LOG_OPENFST_TYPE)
      return false;
#endif
#endif
#if !HAVE_XFSM
    if (type == XFSM_TYPE)
      return false;
#endif
    (void)type;
    return true;
}  
  
HfstTransducer &HfstTransducer::convert(ImplementationType type,
                    std::string options)
{
  if (! is_lean_implementation_type_available(this->type)) {
    HFST_THROW_MESSAGE(HfstFatalException,
                       "HfstTransducer::convert: the original type "
                       "of the transducer is not available!");
  }

    if (type == ERROR_TYPE)
    {
        HFST_THROW_MESSAGE(SpecifiedTypeRequiredException,
                           "HfstTransducer::convert"); }
    if (type == this->type)
    { return *this; }
    if (! is_lean_implementation_type_available(type)) {
      throw ImplementationTypeNotAvailableException("HfstTransducer::convert", __FILE__, __LINE__, type);
    }

    hfst::implementations::HfstBasicTransducer * internal=NULL;
    switch (this->type)
      {
#if HAVE_FOMA
    case FOMA_TYPE:
      internal =
        ConversionFunctions::foma_to_hfst_basic_transducer
        (implementation.foma);
      foma_interface.delete_foma(implementation.foma);
      break;
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
      internal =
        ConversionFunctions::xfsm_to_hfst_basic_transducer
        (implementation.xfsm);
      //delete implementation.xfsm;
      break;
#endif
        /* Add here your implementation. */
            //#if HAVE_MY_TRANSDUCER_LIBRARY
            //case MY_TRANSDUCER_LIBRARY_TYPE:
            //internal =
            // ConversionFunctions::
            //   my_transducer_library_transducer_to_hfst_basic_transducer
            //     (implementation.my_transducer_library);
            //delete(implementation.my_transducer_library);
            //break;
            //#endif
#if HAVE_SFST || HAVE_LEAN_SFST
    case SFST_TYPE:
      internal =
        ConversionFunctions::sfst_to_hfst_basic_transducer
        (implementation.sfst);
      sfst_interface.delete_transducer(implementation.sfst);
      break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
      internal =
        ConversionFunctions::tropical_ofst_to_hfst_basic_transducer
        (implementation.tropical_ofst);
      assert(internal != NULL);
      tropical_ofst_interface.delete_transducer(implementation.tropical_ofst);
      break;
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        internal =
        ConversionFunctions::log_ofst_to_hfst_basic_transducer
          (implementation.log_ofst);
        log_ofst_interface.delete_transducer(implementation.log_ofst);
        break;
#endif
      case HFST_OL_TYPE:
      case HFST_OLW_TYPE:
        internal =
          ConversionFunctions::hfst_ol_to_hfst_basic_transducer
          (implementation.hfst_ol);
        delete implementation.hfst_ol;
        break;
#endif
    case ERROR_TYPE:
    default:
      (void)internal;
      HFST_THROW(TransducerHasWrongTypeException);
      break;
    }
    this->type = type;
    switch (this->type)
    {
#if HAVE_SFST || HAVE_LEAN_SFST
    case SFST_TYPE:
      implementation.sfst =
        ConversionFunctions::hfst_basic_transducer_to_sfst(internal);
      delete internal;
      break;
#endif
        /* Add here your implementation. */
            //#if HAVE_MY_TRANSDUCER_LIBRARY
            //case MY_TRANSDUCER_LIBRARY_TYPE:
            //implementation.my_transducer_library =
            // ConversionFunctions::
            //   hfst_basic_transducer_to_my_transducer_library_transducer
            //     (internal);
            //delete internal;
            //break;
            //#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
      implementation.tropical_ofst =
        ConversionFunctions::hfst_basic_transducer_to_tropical_ofst
        (internal);
      delete internal;
      break;
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
    case LOG_OPENFST_TYPE:
      implementation.log_ofst =
        ConversionFunctions::hfst_basic_transducer_to_log_ofst(internal);
      delete internal;
      break;
#endif
    case HFST_OL_TYPE:
    case HFST_OLW_TYPE:
      implementation.hfst_ol =
        ConversionFunctions::hfst_basic_transducer_to_hfst_ol
        (internal, this->type==HFST_OLW_TYPE?true:false, options);
      delete internal;
      break;
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
      implementation.foma =
        ConversionFunctions::hfst_basic_transducer_to_foma(internal);
      delete internal;
      break;
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
      implementation.xfsm =
        ConversionFunctions::hfst_basic_transducer_to_xfsm(internal);
      delete internal;
      break;
#endif
        case ERROR_TYPE:
    default:
      HFST_THROW(TransducerHasWrongTypeException);
    }

    return *this;
}

void HfstTransducer::write_in_att_format
(const std::string &filename, bool print_weights) const
{
  FILE * ofile = hfst::hfst_fopen(filename.c_str(), "wb");
    if (ofile == NULL) {
    std::string message(filename);
    HFST_THROW_MESSAGE(StreamCannotBeWrittenException, message);
    }
    write_in_att_format(ofile,print_weights);
    fclose(ofile);
}

void HfstTransducer::write_in_att_format_number
(FILE * ofile, bool print_weights) const
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
  hfst::implementations::HfstBasicTransducer net(*this);
  net.write_in_att_format_number(ofile, print_weights);
}

void HfstTransducer::write_in_att_format
(char * buffer, bool print_weights) const
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
  hfst::implementations::HfstBasicTransducer net(*this);
  net.write_in_att_format(buffer, print_weights);
}


void HfstTransducer::write_in_att_format
(FILE * ofile, bool print_weights) const
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
    // Implemented only for internal transducer format.
    hfst::implementations::HfstBasicTransducer net(*this);
    net.write_in_att_format(ofile, print_weights);
}

/* Implemented only for XFSM_TYPE. */
void HfstTransducer::write_xfsm_transducer_in_att_format(const char * filename) const
{
  if (type != XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#if HAVE_XFSM
  hfst::implementations::XfsmTransducer::write_in_att_format
    (const_cast<NETptr>(this->implementation.xfsm), filename);
#endif
  (void)filename;
}

/* Implemented only for XFSM_TYPE. */
void HfstTransducer::write_xfsm_transducer_in_prolog_format(const char * filename) const
{
  if (type != XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#if HAVE_XFSM
  hfst::implementations::XfsmTransducer::write_in_prolog_format
    (const_cast<NETptr>(this->implementation.xfsm), filename);
#endif
  (void)filename;
}

void HfstTransducer::write_in_prolog_format(FILE * file, const std::string & name,
                                            bool write_weights)
{
  /* For big transducers, converting from xfsm is slow. */
  if (type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
  HfstBasicTransducer fsm(*this);
  fsm.write_in_prolog_format(file, name, write_weights);
}

HfstTransducer * HfstTransducer::prolog_file_to_xfsm_transducer(const char * filename)
{
#if HAVE_XFSM
  HfstTransducer * retval = new HfstTransducer(hfst::XFSM_TYPE);
  retval->implementation.xfsm = XfsmTransducer::prolog_file_to_xfsm_transducer(filename);
  return retval;
#else
  (void)filename;
  HFST_THROW(FunctionNotImplementedException);
#endif
}

HfstTransducer::HfstTransducer(FILE * ifile,
                               ImplementationType type,
                               const std::string &epsilon_symbol):
    type(type),anonymous(false),is_trie(false), name("")
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
  unsigned int linecount=0;

    if (! is_lean_implementation_type_available(type))
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);

    HfstTokenizer::check_utf8_correctness(epsilon_symbol);

    // Implemented only for internal transducer format.
    hfst::implementations::HfstBasicTransducer net =
      /*hfst::implementations::HfstTransitionGraph<hfst::implementations::
        HfstTropicalTransducerTransitionData>::*/
      HfstBasicTransducer::read_in_att_format(ifile, std::string(epsilon_symbol), linecount);

    // Conversion is done here.
    switch (type)
    {
#if HAVE_SFST || HAVE_LEAN_SFST
    case SFST_TYPE:
        implementation.sfst =
        ConversionFunctions::hfst_basic_transducer_to_sfst(&net);
        break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst
        = ConversionFunctions::hfst_basic_transducer_to_tropical_ofst(&net);
          
        break;
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        implementation.log_ofst
        = ConversionFunctions::hfst_basic_transducer_to_log_ofst(&net);
        break;
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        implementation.foma =
        ConversionFunctions::hfst_basic_transducer_to_foma(&net);
        break;
#endif
#if HAVE_HFSTOL
    case HFST_OL_TYPE:
    implementation.hfst_ol
            = ConversionFunctions::hfst_basic_transducer_to_hfst_ol
            (&net, false);
    break;
    case HFST_OLW_TYPE:
    implementation.hfst_ol
            = ConversionFunctions::hfst_basic_transducer_to_hfst_ol(&net, true);
    break;
#endif
    /* Add here your implementation. */
        //#if HAVE_MY_TRANSDUCER_LIBRARY
        //case MY_TRANSDUCER_LIBRARY_TYPE:
        //implementation.my_transducer_library =
        //  ConversionFunctions::
        //    hfst_basic_transducer_to_my_transducer_library_transducer(&net);
        //break;
        //#endif
    case ERROR_TYPE:
        HFST_THROW(SpecifiedTypeRequiredException);
    default:
        HFST_THROW(TransducerHasWrongTypeException);
    }
}

HfstTransducer::HfstTransducer(FILE * ifile,
                               ImplementationType type,
                               const std::string &epsilon_symbol,
                               unsigned int & linecount):
    type(type),anonymous(false),is_trie(false), name("")
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif

    if (! is_lean_implementation_type_available(type))
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);

    HfstTokenizer::check_utf8_correctness(epsilon_symbol);

    // Implemented only for internal transducer format.
    hfst::implementations::HfstBasicTransducer net =
      /*hfst::implementations::HfstTransitionGraph<hfst::implementations::
        HfstTropicalTransducerTransitionData>::*/
      HfstBasicTransducer::read_in_att_format(ifile, std::string(epsilon_symbol), linecount);

    // Conversion is done here.
    switch (type)
    {
#if HAVE_SFST || HAVE_LEAN_SFST
    case SFST_TYPE:
        implementation.sfst =
        ConversionFunctions::hfst_basic_transducer_to_sfst(&net);
        break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst
        = ConversionFunctions::hfst_basic_transducer_to_tropical_ofst(&net);
        break;
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
    case LOG_OPENFST_TYPE:
        implementation.log_ofst
        = ConversionFunctions::hfst_basic_transducer_to_log_ofst(&net);
        break;
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
        implementation.foma =
        ConversionFunctions::hfst_basic_transducer_to_foma(&net);
        break;
#endif
#if HAVE_HFSTOL
    case HFST_OL_TYPE:
    implementation.hfst_ol
            = ConversionFunctions::hfst_basic_transducer_to_hfst_ol
            (&net, false);
    break;
    case HFST_OLW_TYPE:
    implementation.hfst_ol
            = ConversionFunctions::hfst_basic_transducer_to_hfst_ol(&net, true);
    break;
#endif
    /* Add here your implementation. */
        //#if HAVE_MY_TRANSDUCER_LIBRARY
        //case MY_TRANSDUCER_LIBRARY_TYPE:
        //implementation.my_transducer_library =
        //  ConversionFunctions::
        //    hfst_basic_transducer_to_my_transducer_library_transducer(&net);
        //break;
        //#endif
    case ERROR_TYPE:
        HFST_THROW(SpecifiedTypeRequiredException);
    default:
        HFST_THROW(TransducerHasWrongTypeException);
    }
}



HfstTransducer &HfstTransducer::read_in_att_format
(const std::string &filename, ImplementationType type,
 const std::string &epsilon_symbol)
{
  if (type == XFSM_TYPE)
    { HFST_THROW(FunctionNotImplementedException); }
  FILE * ifile = hfst::hfst_fopen(filename.c_str(), "rb");
  if (ifile == NULL) {
    std::string message(filename);
    HFST_THROW_MESSAGE(StreamNotReadableException, message);
  }
  HfstTokenizer::check_utf8_correctness(epsilon_symbol);
  
  HfstTransducer &retval = read_in_att_format(ifile, type, epsilon_symbol);
  fclose(ifile);
  return retval;
}

HfstTransducer &HfstTransducer::read_in_att_format
(FILE * ifile, ImplementationType type, const std::string &epsilon_symbol)
{
  if (type == XFSM_TYPE)
    { HFST_THROW(FunctionNotImplementedException); }

    if (! is_implementation_type_available(type))
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);

    HfstTokenizer::check_utf8_correctness(epsilon_symbol);

    unsigned int foo = 0;
    hfst::implementations::HfstBasicTransducer net =
      /*hfst::implementations::HfstTransitionGraph<hfst::implementations::
        HfstTropicalTransducerTransitionData>*/
      HfstBasicTransducer::read_in_att_format(ifile, std::string(epsilon_symbol), foo);
    HfstTransducer *retval = new HfstTransducer(net,type);
    (void)foo;
    return *retval;
}



// -----------------------------------------------------------------------
//
//                     Miscellaneous functions
//
// -----------------------------------------------------------------------


HfstTransducer HfstTransducer::universal_pair( ImplementationType type )
{
        using namespace implementations;
        HfstBasicTransducer bt;
        bt.add_transition(0, HfstBasicTransition(1, "@_IDENTITY_SYMBOL_@", "@_IDENTITY_SYMBOL_@", 0) );
        bt.add_transition(0, HfstBasicTransition(1, "@_UNKNOWN_SYMBOL_@", "@_UNKNOWN_SYMBOL_@", 0) );
        bt.add_transition(0, HfstBasicTransition(1, "@_UNKNOWN_SYMBOL_@", "@_EPSILON_SYMBOL_@", 0) );
        bt.add_transition(0, HfstBasicTransition(1, "@_EPSILON_SYMBOL_@", "@_UNKNOWN_SYMBOL_@", 0) );
        bt.set_final_weight(1, 0);

        HfstTransducer Retval(bt, type);

    return Retval;
}

HfstTransducer HfstTransducer::identity_pair( ImplementationType type )
{
        using namespace implementations;
        HfstBasicTransducer bt;
        bt.add_transition(0, HfstBasicTransition(1, "@_IDENTITY_SYMBOL_@", "@_IDENTITY_SYMBOL_@", 0) );
        bt.set_final_weight(1, 0);

        HfstTransducer Retval(bt, type);

        return Retval;
}

HfstTransducer &HfstTransducer::assign(const HfstTransducer &another)
{
  return this->operator=(another);
}

HfstTransducer &HfstTransducer::operator=(const HfstTransducer &another)
{
#if HAVE_XFSM
  if (this->type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
    // Check for self-assignment.
    if (&another == this)
    { return *this; }
  
    if (this->type != UNSPECIFIED_TYPE &&
    this->type != another.type) {
      HFST_THROW_MESSAGE
    (TransducerTypeMismatchException, "HfstTransducer::operator=");
    }

    // set some features
    anonymous = another.anonymous;
    is_trie = another.is_trie;
    this->set_name(another.get_name());

    // Delete old transducer.
    switch (this->type)
    {
#if HAVE_FOMA
    case FOMA_TYPE:
      foma_interface.delete_foma(implementation.foma);
    break;
#endif
#if HAVE_SFST
    case SFST_TYPE:
      sfst_interface.delete_transducer(implementation.sfst);
    break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
    tropical_ofst_interface.delete_transducer(implementation.tropical_ofst);
    break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
    log_ofst_interface.delete_transducer(implementation.log_ofst);
    break;
#endif
#endif
    case HFST_OL_TYPE:
    case HFST_OLW_TYPE:
      delete implementation.hfst_ol;
      break;
    /* Add here your implementation. */
    //#if HAVE_MY_TRANSDUCER_LIBRARY
    //case MY_TRANSDUCER_LIBRARY_TYPE:
    //delete implementation.my_transducer_library;
    //break;
    //#endif
    case UNSPECIFIED_TYPE:
      break;
    case ERROR_TYPE:
    default:
    HFST_THROW(TransducerHasWrongTypeException);
    }
    
    // Set new transducer.
  
    // Sfst doesn't have a const copy constructor, so we need to do a
    // const_cast here. Shouldn't be a problem...
    HfstTransducer &another_1 = const_cast<HfstTransducer&>(another);
    type = another.type;
    switch (type)
    {
#if HAVE_FOMA
    case FOMA_TYPE:
    implementation.foma =
        foma_interface.copy(another_1.implementation.foma);
    break;
#endif
#if HAVE_SFST
    case SFST_TYPE:
    implementation.sfst =
        sfst_interface.copy(another_1.implementation.sfst);
    break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
    implementation.tropical_ofst =
        tropical_ofst_interface.copy(another_1.implementation.tropical_ofst);
    break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
    implementation.log_ofst =
        log_ofst_interface.copy(another_1.implementation.log_ofst);
    break;
#endif
#endif
    case HFST_OL_TYPE:
      implementation.hfst_ol
        = another_1.implementation.hfst_ol->
        copy(another_1.implementation.hfst_ol, false);
      break;
    case HFST_OLW_TYPE:
      implementation.hfst_ol
        = another_1.implementation.hfst_ol->
        copy(another_1.implementation.hfst_ol, true);
      break;
    /* Add here your implementation. */
    default:
    (void)1;
    }
    return *this;

}


HfstTokenizer HfstTransducer::create_tokenizer()
{
    HfstTokenizer tok;

    if (this->type == SFST_TYPE)
    {
        StringPairSet sps = this->get_symbol_pairs();
        for (StringPairSet::const_iterator it = sps.begin();
             it != sps.end(); it++)
    {
            if (it->first.size() > 1)
        tok.add_multichar_symbol(it->first);
            if (it->second.size() > 1)
        tok.add_multichar_symbol(it->second);
    }
    }
    else
    {
        hfst::implementations::HfstBasicTransducer t(*this);
        t.prune_alphabet();
        StringSet alpha = t.get_alphabet();
        for (StringSet::iterator it = alpha.begin();
             it != alpha.end(); it++)
    {
            if (it->size() > 1)
        tok.add_multichar_symbol(*it);
    }
    }

    return tok;
}

HfstTransducer HfstTransducer::read_lexc(const std::string &filename,
                                         ImplementationType type,
                                         bool verbose) {
  HfstTransducer * ptr = read_lexc_ptr(filename, type, verbose);
  HfstTransducer retval(*ptr);
  delete ptr;
  return retval;
}

HfstTransducer * HfstTransducer::read_lexc_ptr(const std::string &filename,
                                               ImplementationType type,
                                               bool verbose)
{
  if (type == XFSM_TYPE)
    { HFST_THROW(FunctionNotImplementedException); }

  (void)filename;
  if (! is_implementation_type_available(type))
    throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);
  
  HfstTransducer * retval = new HfstTransducer();
  
  switch (type)
    {
#if HAVE_FOMA
    case FOMA_TYPE:
#if GENERATE_LEXC_WRAPPER
      retval->implementation.foma = foma_interface.read_lexc(filename, verbose);
      retval->type=FOMA_TYPE;
      break;
#endif // GENERATE_LEXC_WRAPPER
#endif // HAVE_FOMA
#if HAVE_SFST
    case SFST_TYPE:
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
#endif
#endif
#if HAVE_SFST || HAVE_OPENFST
      {
        hfst::lexc::LexcCompiler compiler(type);
        compiler.setVerbosity(verbose);
        compiler.parse(filename.c_str());
        retval = compiler.compileLexical();
        return retval;
        break;
      }
#endif
    /* Add here your implementation. */
    //#if HAVE_MY_TRANSDUCER_LIBRARY
    //case MY_TRANSDUCER_LIBRARY_TYPE:
    // ...
    //break;
    //#endif
    case ERROR_TYPE:
    default:
      HFST_THROW(TransducerHasWrongTypeException);
    }
  return retval;
}

std::ostream & redirect(std::ostream &out, const HfstTransducer & t)
{
  return operator<<(out, t);
}

std::ostream & operator<<
(std::ostream &out, const HfstTransducer & t)
{
#if HAVE_XFSM
  if (t.type == XFSM_TYPE)
    HFST_THROW(FunctionNotImplementedException);
#endif
    // Implemented only for internal transducer format.
    hfst::implementations::HfstBasicTransducer net(t);
    bool write_weights;
    if (t.type == SFST_TYPE || t.type == FOMA_TYPE)
    write_weights=false;
    else
    write_weights=true;
    net.write_in_att_format(out, write_weights);
    return out;
}

}

#else // MAIN_TEST was defined

#include <iostream>
using namespace hfst;
using namespace implementations;

// Cross product unit tests
void cross_product_subtest1( ImplementationType type )
{
        HfstTokenizer TOK;

        HfstTransducer tmp1("dog", TOK, type);
        HfstTransducer tmp2("cat", TOK, type);
        HfstTransducer input1(tmp1);
        input1.disjunct(tmp2).minimize();

        HfstTransducer tmp11("chien", TOK, type);
        HfstTransducer tmp22("chat", TOK, type);
        HfstTransducer input2(tmp11);
        input2.disjunct(tmp22).minimize();


        HfstTransducer cp(input1);
        cp.cross_product(input2);

        HfstTransducer r1("cat", "chien", TOK, type);
        HfstTransducer r2("cat", "chat", TOK, type);
        HfstTransducer r3("dog", "chien", TOK, type);
        HfstTransducer r4("dog", "chat", TOK, type);
        HfstTransducer result(r1);
        result.disjunct(r2).disjunct(r3).disjunct(r4).minimize();

        assert(cp.compare(result));
}

void cross_product_subtest2( ImplementationType type )
{
        HfstTokenizer TOK;
        TOK.add_multichar_symbol("@_UNKNOWN_SYMBOL_@");

        HfstTransducer input1( HfstTransducer::identity_pair(type) );

        HfstTransducer input2("a", TOK, type);

        HfstTransducer cp(input1);
        cp.cross_product(input2);


        HfstTransducer r1("a", TOK, type);
        HfstTransducer r2("@_UNKNOWN_SYMBOL_@", "a", TOK, type);
        HfstTransducer result(r1);
        result.disjunct(r2).minimize();
        assert(cp.compare(result));
}

void cross_product_subtest3( ImplementationType type )
{
        HfstTokenizer TOK;
        TOK.add_multichar_symbol("@_UNKNOWN_SYMBOL_@");
        TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

        HfstTransducer input1( HfstTransducer::identity_pair(type) );
        input1.repeat_star().minimize();

        HfstTransducer input2("a", TOK, type);

        HfstTransducer cp(input1);
        cp.cross_product(input2);


        HfstTransducer r1("a", TOK, type);
        HfstTransducer r2("@_UNKNOWN_SYMBOL_@", "a", TOK, type);
        HfstTransducer r3("a", "@_EPSILON_SYMBOL_@", TOK, type);
        HfstTransducer r4("@_UNKNOWN_SYMBOL_@", "@_EPSILON_SYMBOL_@", TOK, type);
        HfstTransducer r5("@_EPSILON_SYMBOL_@", "a", TOK, type);
        r3.disjunct(r4).minimize().repeat_star();
        r1.disjunct(r2).concatenate(r3).minimize();

        HfstTransducer result(r5);
        result.disjunct(r1).minimize();
        assert(cp.compare(result));
}
void cross_product_subtest4( ImplementationType type )
{

        HfstTokenizer TOK;
        TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

        HfstTransducer input1("b", TOK, type);
        HfstTransducer input2("a", TOK, type);
        input2.repeat_star().minimize();

        HfstTransducer cp(input1);
        cp.cross_product(input2);

        HfstTransducer r1("b", "a", TOK, type);
        HfstTransducer r2("@_EPSILON_SYMBOL_@", "a", TOK, type);
        r2.repeat_star().minimize();
        r1.concatenate(r2);
        HfstTransducer result("b", "@_EPSILON_SYMBOL_@", TOK, type);
        result.disjunct(r1).minimize();

        assert(cp.compare(result));
}


// Priority union unit tests
void priority_union_test ( ImplementationType type )
{
    HfstTokenizer TOK;
    TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

    HfstTransducer input1("a", "X", TOK, type);
    HfstTransducer input2("b", "X", TOK, type);

    HfstTransducer input3("b", "N", TOK, type);
    HfstTransducer input4("c", "X", TOK, type);


    HfstTransducer t1(input1);
    t1.disjunct(input2).minimize();

    HfstTransducer t2(input3);
    t2.disjunct(input4).minimize();

    HfstTransducer result1a(t1);
    result1a.disjunct(input4).minimize();


    HfstTransducer testTr1(t1);
    assert ( testTr1.priority_union( t2 ).compare( result1a ) );


    HfstBasicTransducer                btEmpty,
                                                    btEmptyString,
                                                    epsilon,
                                                    bt1,
                                                    bt2,
                                                    bt3,
                                                    bt2withoutPriority,
                                                    btIdentity,
                                                    btUnknown,
                                                    btEpsilon,
                                                    btResult1,
                                                    btResult2,
                                                    btResult3,
                                                    btResult4,
                                                    btResult5,
                                                    btResult6,
                                                    btResult7;

    // Empty string
    btEmptyString.set_final_weight(0, 3);

    // First test transducer
    bt1.add_transition(0, HfstBasicTransition(1, "a", "a", 1) );
    bt1.add_transition(0, HfstBasicTransition(1, "b", "b", 2) );
    bt1.add_transition(1, HfstBasicTransition(2, "@_EPSILON_SYMBOL_@", "1", 3) );
    bt1.set_final_weight(2, 5);

    // Second test transducer
    bt2.add_transition(0, HfstBasicTransition(1, "c", "C", 10) );
    bt2.add_transition(0, HfstBasicTransition(1, "b", "B", 20) );
    bt2.add_transition(1, HfstBasicTransition(2, "@_EPSILON_SYMBOL_@", "1", 30) );
    bt2.set_final_weight(2, 50);

    // Second test transducer without priority string
    bt2withoutPriority.add_transition(0, HfstBasicTransition(1, "c", "C", 10) );
    bt2withoutPriority.add_transition(1, HfstBasicTransition(2, "@_EPSILON_SYMBOL_@", "1", 30) );
    bt2withoutPriority.set_final_weight(2, 50);

    // Third test transducer
    bt3.add_transition(0, HfstBasicTransition(1, "a", "b", 55) );
    bt3.add_transition(0, HfstBasicTransition(1, "b", "b", 65) );
    bt3.set_final_weight(1, 75);

    // Identity to Identity
    btIdentity.add_transition(0, HfstBasicTransition(0, "@_IDENTITY_SYMBOL_@", "@_IDENTITY_SYMBOL_@", 100) );
    btIdentity.set_final_weight(0, 100);

    // Unknown to unknown
    btUnknown.add_transition(0, HfstBasicTransition(0, "@_UNKNOWN_SYMBOL_@", "@_UNKNOWN_SYMBOL_@", 200) );
    btUnknown.set_final_weight(0, 200);

    // Epsilon
    btEpsilon.add_transition(0, HfstBasicTransition(0, "@_EPSILON_SYMBOL_@", "@_EPSILON_SYMBOL_@", 300) );
    btEpsilon.set_final_weight(0, 300);

    // Result 1 ... tr1 .P. emptyString
    //                        ...        emptyString .p. tr1
    btResult1.add_transition(0, HfstBasicTransition(1, "a", "a", 1) );
    btResult1.add_transition(0, HfstBasicTransition(1, "b", "b", 2) );
    btResult1.add_transition(1, HfstBasicTransition(2, "@_EPSILON_SYMBOL_@", "1", 3) );
    btResult1.set_final_weight(2, 5);
    btResult1.set_final_weight(0, 3);

    // Result 2 ... tr1 .P. tr2
      btResult2.add_transition(0, HfstBasicTransition(1, "c", "C", 10) );
    btResult2.add_transition(0, HfstBasicTransition(1, "b", "b", 20) );
    btResult2.add_transition(0, HfstBasicTransition(2, "a", "a", 1) );
    btResult2.add_transition(1, HfstBasicTransition(3, "@_EPSILON_SYMBOL_@", "1", 30) );
    btResult2.add_transition(2, HfstBasicTransition(4, "@_EPSILON_SYMBOL_@", "1", 3) );
    btResult2.set_final_weight(3, 50);
    btResult2.set_final_weight(4, 5);

    // Result 3 ... tr1 .P. tr2 without priority
    btResult3.add_transition(0, HfstBasicTransition(1, "c", "C", 10) );
    btResult3.add_transition(0, HfstBasicTransition(2, "b", "b", 2) );
    btResult3.add_transition(0, HfstBasicTransition(2, "a", "a", 1) );
    btResult3.add_transition(1, HfstBasicTransition(3, "@_EPSILON_SYMBOL_@", "1", 30) );
    btResult3.add_transition(2, HfstBasicTransition(4, "@_EPSILON_SYMBOL_@", "1", 3) );
    btResult3.set_final_weight(3, 50);
    btResult3.set_final_weight(4, 5);

    // Result 4 ... tr1 .P. trIdentity
    btResult4.add_transition(0, HfstBasicTransition(0, "@_IDENTITY_SYMBOL_@", "@_IDENTITY_SYMBOL_@", 100) );
    btResult4.add_transition(0, HfstBasicTransition(0, "b", "b", 100) );
    btResult4.add_transition(0, HfstBasicTransition(0, "a", "a", 100) );
    btResult4.add_transition(0, HfstBasicTransition(0, "1", "1", 100) );
    btResult4.set_final_weight(0, 100);

    // Result 5 ... trIdentity .P. tr3
    btResult5.add_transition(0, HfstBasicTransition(1, "a", "b", 130) );
    btResult5.add_transition(0, HfstBasicTransition(2, "b", "b", 140) );
    btResult5.add_transition(0, HfstBasicTransition(3, "@_IDENTITY_SYMBOL_@", "@_IDENTITY_SYMBOL_@", 200) );
    btResult5.add_transition(0, HfstBasicTransition(4, "a", "a", 300) );
    btResult5.add_transition(2, HfstBasicTransition(3, "b", "b", 160) );
    btResult5.add_transition(2, HfstBasicTransition(3, "a", "a", 160) );
    btResult5.add_transition(2, HfstBasicTransition(3, "@_IDENTITY_SYMBOL_@", "@_IDENTITY_SYMBOL_@", 160) );
    btResult5.add_transition(3, HfstBasicTransition(3, "b", "b", 100) );
    btResult5.add_transition(3, HfstBasicTransition(3, "a", "a", 100) );
    btResult5.add_transition(3, HfstBasicTransition(3, "@_IDENTITY_SYMBOL_@", "@_IDENTITY_SYMBOL_@", 100) );
    btResult5.add_transition(4, HfstBasicTransition(3, "b", "b", 0) );
    btResult5.add_transition(4, HfstBasicTransition(3, "a", "a", 0) );
    btResult5.add_transition(4, HfstBasicTransition(3, "@_IDENTITY_SYMBOL_@", "@_IDENTITY_SYMBOL_@", 0) );
    btResult5.set_final_weight(0, 100);
    btResult5.set_final_weight(1, 0);
    btResult5.set_final_weight(2, 0);
    btResult5.set_final_weight(3, 0);

    // Result 6 ... tr3 .P. trUnknown
    btResult6.add_transition(0, HfstBasicTransition(0, "@_UNKNOWN_SYMBOL_@", "@_UNKNOWN_SYMBOL_@", 200) );
    btResult6.add_transition(0, HfstBasicTransition(0, "@_UNKNOWN_SYMBOL_@", "b", 200) );
    btResult6.add_transition(0, HfstBasicTransition(0, "b", "@_UNKNOWN_SYMBOL_@", 200) );
    btResult6.add_transition(0, HfstBasicTransition(0, "b", "a", 200) );
    btResult6.add_transition(0, HfstBasicTransition(0, "@_UNKNOWN_SYMBOL_@", "a", 200) );
    btResult6.add_transition(0, HfstBasicTransition(0, "a", "@_UNKNOWN_SYMBOL_@", 200) );
    btResult6.add_transition(0, HfstBasicTransition(0, "a", "b", 200) );
    btResult6.set_final_weight(0, 200);

    // Result 7 ... trUnknown .P. tr3
    btResult7.add_transition(0, HfstBasicTransition(1, "a", "b", 130) );
    btResult7.add_transition(0, HfstBasicTransition(2, "b", "b", 140) );
    btResult7.add_transition(0, HfstBasicTransition(3, "b", "a", 600) );
    btResult7.add_transition(0, HfstBasicTransition(4, "@_UNKNOWN_SYMBOL_@", "a", 400) );
    btResult7.add_transition(0, HfstBasicTransition(4, "@_UNKNOWN_SYMBOL_@", "b", 400) );
    btResult7.add_transition(0, HfstBasicTransition(4, "@_UNKNOWN_SYMBOL_@", "@_UNKNOWN_SYMBOL_@", 400) );
    btResult7.add_transition(0, HfstBasicTransition(3, "b", "@_UNKNOWN_SYMBOL_@", 600) );
    btResult7.add_transition(0, HfstBasicTransition(3, "a", "@_UNKNOWN_SYMBOL_@", 600) );
    btResult7.add_transition(1, HfstBasicTransition(4, "a", "b", 470) );
    btResult7.add_transition(1, HfstBasicTransition(4, "b", "a", 470) );
    btResult7.add_transition(1, HfstBasicTransition(4, "@_UNKNOWN_SYMBOL_@", "a", 470) );
    btResult7.add_transition(1, HfstBasicTransition(4, "@_UNKNOWN_SYMBOL_@", "b", 470) );
    btResult7.add_transition(1, HfstBasicTransition(4, "@_UNKNOWN_SYMBOL_@", "@_UNKNOWN_SYMBOL_@", 470) );
    btResult7.add_transition(1, HfstBasicTransition(4, "b", "@_UNKNOWN_SYMBOL_@", 470) );
    btResult7.add_transition(1, HfstBasicTransition(4, "a", "@_UNKNOWN_SYMBOL_@", 470) );
    btResult7.add_transition(3, HfstBasicTransition(4, "a", "b", 0) );
    btResult7.add_transition(3, HfstBasicTransition(4, "b", "a", 0) );
    btResult7.add_transition(3, HfstBasicTransition(4, "@_UNKNOWN_SYMBOL_@", "a", 0) );
    btResult7.add_transition(3, HfstBasicTransition(4, "@_UNKNOWN_SYMBOL_@", "b", 0) );
    btResult7.add_transition(3, HfstBasicTransition(4, "@_UNKNOWN_SYMBOL_@", "@_UNKNOWN_SYMBOL_@", 0) );
    btResult7.add_transition(3, HfstBasicTransition(4, "b", "@_UNKNOWN_SYMBOL_@", 0) );
    btResult7.add_transition(3, HfstBasicTransition(4, "a", "@_UNKNOWN_SYMBOL_@", 0) );
    btResult7.add_transition(4, HfstBasicTransition(4, "a", "b", 200) );
    btResult7.add_transition(4, HfstBasicTransition(4, "b", "a", 200) );
    btResult7.add_transition(4, HfstBasicTransition(4, "@_UNKNOWN_SYMBOL_@", "a", 200) );
    btResult7.add_transition(4, HfstBasicTransition(4, "@_UNKNOWN_SYMBOL_@", "b", 200) );
    btResult7.add_transition(4, HfstBasicTransition(4, "@_UNKNOWN_SYMBOL_@", "@_UNKNOWN_SYMBOL_@", 200) );
    btResult7.add_transition(4, HfstBasicTransition(4, "b", "@_UNKNOWN_SYMBOL_@", 200) );
    btResult7.add_transition(4, HfstBasicTransition(4, "a", "@_UNKNOWN_SYMBOL_@", 200) );
    btResult7.set_final_weight(0, 200);
    btResult7.set_final_weight(1, 0);
    btResult7.set_final_weight(2, 0);
    btResult7.set_final_weight(4, 0);


    // Transforming basic transducers to a TYPE transducer
    HfstTransducer trEmpty(btEmpty, type);
    HfstTransducer trEmptyString(btEmptyString, type);
    HfstTransducer tr1(bt1, type);
    HfstTransducer tr2(bt2, type);
    HfstTransducer tr3(bt3, type);
    HfstTransducer tr2withoutPriority(bt2withoutPriority, type);
    HfstTransducer trIdentity(btIdentity, type);
    HfstTransducer trUnknown(btUnknown, type);
    HfstTransducer trEpsilon(btEpsilon, type);
    HfstTransducer result1(btResult1, type);
    HfstTransducer result2(btResult2, type);
    HfstTransducer result3(btResult3, type);
    HfstTransducer result4(btResult4, type);
    HfstTransducer result5(btResult5, type);
    HfstTransducer result6(btResult6, type);
    HfstTransducer result7(btResult7, type);

    // emptyLang .P. emptyLang
    HfstTransducer testTr = trEmpty;
    assert ( testTr.priority_union( trEmpty ).compare( trEmpty ) );
    // emptyString .P. emptyString
    testTr = trEmptyString;
    assert ( testTr.priority_union( trEmptyString ).compare( trEmptyString ) );
    // transducer .P. emptyString
    testTr = tr1;
    assert ( testTr.priority_union( trEmptyString ).compare( result1 ) );
    // emptyString .P. transducer
    testTr = trEmptyString;
    assert ( testTr.priority_union( tr1 ).compare( result1 ) );
    
    
    
    // normal transducer .P. normal transducer
    testTr = tr1;

    // TODO the result is wrong, change it! (wrong because of the wights shifting)
    //assert ( testTr.priority_union( tr2 ).compare( result2 ) );

    // normal transducer .P. normal transducer without priority string
    testTr = tr1;
    assert (  testTr.priority_union( tr2withoutPriority ).compare( result3 ) );
    // normal transducer .P. universal language
    testTr = tr1;

    if (false) { // DEBUG
      std::cerr << testTr.priority_union( trIdentity ).
    push_weights(TO_FINAL_STATE).minimize()
        << "--\n"
        << result4
        << std::endl;
    }

    //TODO - results are worng, change them! (wrong because of the wights shifting)
    /*
    //assert ( testTr.priority_union( trIdentity ).compare( result4 ) ); // FAIL
    // identity .p. normal transducer
    testTr = trIdentity;
    assert (  testTr.priority_union( tr3 ).compare( result5 ) );
    
    // normal .p. unknown
    testTr = tr3;
    
    //assert (  testTr.priority_union( trUnknown ).compare( result6 ) ); FAIL
    // unknown .p. normal
    testTr = trUnknown;
    assert ( testTr.priority_union( tr3 ).compare( result7 ) );
     */
}

// Universal pair test function
void universal_pair_test ( ImplementationType type )
{
    HfstBasicTransducer     bt,
                            bt2,
                            bt3,
                            btResult1,
                            btResult2,
                            btResult3,
                            btResult4,
                            btEmpty;

    // Test transducer a:a
    bt.add_transition(0, HfstBasicTransition(1, "a", "a", 0) );
    bt.set_final_weight(1, 0);
    // Test transducer b:b
    bt2.add_transition(0, HfstBasicTransition(1, "a", "b", 0) );
    bt2.set_final_weight(1, 0);
    // Test transducer aa:bb
    bt3.add_transition(0, HfstBasicTransition(1, "a", "b", 0) );
    bt3.add_transition(1, HfstBasicTransition(2, "a", "b", 0) );
    bt3.set_final_weight(2, 0);

    // Result 1 ( a:a .o. universal pair )
    btResult1.add_transition(0, HfstBasicTransition(1, "a", "a", 0) );
    btResult1.add_transition(0, HfstBasicTransition(1, "a", "@_UNKNOWN_SYMBOL_@", 0) );
    btResult1.add_transition(0, HfstBasicTransition(1, "a", "@_EPSILON_SYMBOL_@",  0) );
    btResult1.set_final_weight(1, 0);
    // Result 2 ( universal pair .o. a:a )
    btResult2.add_transition(0, HfstBasicTransition(1, "a", "a", 0) );
    btResult2.add_transition(0, HfstBasicTransition(1, "@_UNKNOWN_SYMBOL_@", "a", 0) );
    btResult2.add_transition(0, HfstBasicTransition(1, "@_EPSILON_SYMBOL_@", "a", 0) );
    btResult2.set_final_weight(1, 0);
    // Result 3 ( a:b .o. universal pair )
    btResult3.add_transition(0, HfstBasicTransition(1, "a", "b", 0) );
    btResult3.add_transition(0, HfstBasicTransition(1, "a", "a", 0) );
    btResult3.add_transition(0, HfstBasicTransition(1, "a", "@_UNKNOWN_SYMBOL_@", 0) );
    btResult3.add_transition(0, HfstBasicTransition(1, "a", "@_EPSILON_SYMBOL_@", 0) );
    btResult3.set_final_weight(1, 0);
    // Result 4 ( universal pair .o. a:b )
    btResult4.add_transition(0, HfstBasicTransition(1, "a", "b", 0) );
    btResult4.add_transition(0, HfstBasicTransition(1, "b", "b", 0) );
    btResult4.add_transition(0, HfstBasicTransition(1, "@_UNKNOWN_SYMBOL_@", "b", 0) );
    btResult4.add_transition(0, HfstBasicTransition(1, "@_EPSILON_SYMBOL_@", "b", 0) );
    btResult4.set_final_weight(1, 0);

    HfstTransducer tr1(bt, type);
    HfstTransducer tr2(bt2, type);
    HfstTransducer tr3(bt3, type);
    HfstTransducer result1(btResult1, type);
    HfstTransducer result2(btResult2, type);
    HfstTransducer result3(btResult3, type);
    HfstTransducer result4(btResult4, type);
    HfstTransducer empty(btEmpty, type);

    HfstTransducer un = HfstTransducer::universal_pair(type);

    // Universal pair is tested by composing it with test transducers
    // a:a .o. universal pair
    HfstTransducer tmp = tr1;
    assert ( tmp.compose(un).compare( result1 ) );
    // universal pair .o. a:a
    tmp = un;
    assert ( tmp.compose(tr1).compare( result2 ) );
    // a:b .o. universal pair
    tmp = tr2;
    assert ( tmp.compose(un).compare( result3 ) );
    // universal pair .o. a:b
    tmp = un;
    assert ( tmp.compose(tr2).compare( result4 ) );
    // aa:bb .o. universal pair
    tmp = tr3;
    assert ( tmp.compose(un).compare( empty ) );
    // universal pair .o. aa:bb
    tmp = un;
    assert ( tmp.compose(tr3).compare( empty ) );

}
void lenient_composition_test ( ImplementationType type )
{
    HfstTokenizer TOK;

    HfstTransducer input1("a", "X", TOK, type);
    HfstTransducer input2("b", "X", TOK, type);

    HfstTransducer input3("b", "N", TOK, type);
    HfstTransducer input4("c", "X", TOK, type);


    HfstTransducer t1(input1);
    t1.disjunct(input2).minimize();

    HfstTransducer t2(input3);
    t2.disjunct(input4).minimize();

    HfstTransducer testTr1(t1);
    assert ( testTr1.lenient_composition( t2 ).compare( t1 ) );

}

int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":" << std::endl;
    
    ImplementationType types[] = {SFST_TYPE,
                                  TROPICAL_OPENFST_TYPE,
                                  FOMA_TYPE};
    unsigned int NUMBER_OF_TYPES=3;

    for (unsigned int i=0; i < NUMBER_OF_TYPES; i++)
    {
      if (! HfstTransducer::is_implementation_type_available(types[i]))
        continue;

        // One case that fails with FOMA_TYPE
        HfstTransducer a("a", types[i]);
        a.repeat_n(2);

        // Test alphabet after substitute

        HfstTransducer t("a", "b", types[i]);
        t.substitute("a", "c");
        StringSet alpha = t.get_alphabet();
        assert(alpha.find("b") != alpha.end());
        assert(alpha.find("c") != alpha.end());
        // TODO: which is correct?
        //assert(alpha.find("a") != alpha.end());

        HfstTransducer t1("a", "b", types[i]);
        HfstTransducer t2("a", "c", types[i]);
        t1.substitute(StringPair("a", "b"), t2);

        alpha = t1.get_alphabet();
        assert(alpha.find("a") != alpha.end());
        assert(alpha.find("c") != alpha.end());
        assert(alpha.find("b") != alpha.end());

        // Test the const arguments are really const
        HfstTransducer ab("a", "b", types[i]);
        HfstTransducer ac("b", "c", types[i]);
        HfstTransducer ab_(ab);
        HfstTransducer ac_(ac);
    
        ab_.compare(ac_);
        assert(ab_.get_alphabet() == ab.get_alphabet());
        assert(ac_.get_alphabet() == ac.get_alphabet());


        // Test removing symbols
        {
          HfstTransducer TR("a", "b", types[i]);
          HfstTransducer TR_COPY(TR);
          TR.remove_from_alphabet("c");
          TR.remove_from_alphabet("d");
          assert(TR.compare(TR_COPY));
          TR.remove_from_alphabet("c");
          TR.remove_from_alphabet("d");
        }

        HfstTransducer &compose(const HfstTransducer &another);

        HfstTransducer &compose_intersect(const HfstTransducerVector &v);

        HfstTransducer &concatenate(const HfstTransducer &another);

        HfstTransducer &disjunct(const HfstTransducer &another);

        HfstTransducer &disjunct(const StringPairVector &spv);

        HfstTransducer &intersect(const HfstTransducer &another);

        HfstTransducer &subtract(const HfstTransducer &another);

        HfstTransducer &insert_freely(const HfstTransducer &tr);

        HfstTransducer &substitute(const StringPair &symbol_pair,
                       HfstTransducer &transducer);
    
        // priority_union unit tests (also tested in hfst-xfst tests)
        priority_union_test( types[i] );

        // lenient_composition unit tests
       lenient_composition_test( types[i] );


        // cross_product unit test
        cross_product_subtest1( types[i] );
        cross_product_subtest2( types[i] );
        cross_product_subtest3( types[i] );
        cross_product_subtest4( types[i] );

        // universal pair unit tests
        universal_pair_test( types[i] );

        void insert_freely_missing_flags_from
          (const HfstTransducer &another);

        // Flag diacritic harmonization test
        HfstTokenizer flag_tokenizer;
        flag_tokenizer.add_multichar_symbol("@P.Char.ON@");
        flag_tokenizer.add_multichar_symbol("@R.Char.ON@");

        HfstTransducer any_a("A",types[i]);
        HfstTransducer any_b("B",types[i]);
        HfstTransducer any_c("C",types[i]);
        any_a.disjunct(any_b).disjunct(any_c).minimize();
        HfstTransducer any_symbol(any_a);
        HfstTransducer any(any_symbol);
        any.repeat_star();

        HfstTransducer a_paths("A" "@P.Char.ON@",
                               flag_tokenizer,
                               types[i]);
        a_paths.concatenate(any);
        HfstTransducer a_end("@R.Char.ON@" "A",
                             flag_tokenizer,
                             types[i]);
        a_paths.concatenate(a_end).minimize();

        HfstTransducer a_paths_copy(a_paths);

        a_paths_copy.convert(HFST_OLW_TYPE);

        HfstOneLevelPaths * results =
          a_paths_copy.lookup_fd(flag_tokenizer.tokenize_one_level("ABCBA"));
        assert(results->size() == 1);
        assert(hfst::symbols::remove_flags(results->begin()->second) ==
               flag_tokenizer.tokenize_one_level("ABCBA"));
        delete results;

        results =
          a_paths_copy.lookup_fd(flag_tokenizer.tokenize_one_level("ABCAA"));
        assert(results->size() == 1);
        assert(hfst::symbols::remove_flags(results->begin()->second) ==
               flag_tokenizer.tokenize_one_level("ABCAA"));
        delete results;

        HfstTransducer b_paths(any_symbol);
        HfstTransducer b_paths_("B"
                                "@P.Char.ON@",
                                flag_tokenizer,
                                types[i]);
        b_paths.concatenate(b_paths_);
        b_paths.concatenate(any);
        HfstTransducer b_end("@R.Char.ON@" "B",
                             flag_tokenizer,
                             types[i]);
        b_end.concatenate(any_symbol);
        b_paths.concatenate(b_end).minimize();

        HfstTransducer b_paths_copy(b_paths);
        b_paths_copy.convert(HFST_OLW_TYPE);

        results =
          b_paths_copy.lookup_fd(flag_tokenizer.tokenize_one_level("ABCBA"));
        assert(results->size() == 1);
        assert(hfst::symbols::remove_flags(results->begin()->second) ==
               flag_tokenizer.tokenize_one_level("ABCBA"));
        delete results;

        results =
          b_paths_copy.lookup_fd(flag_tokenizer.tokenize_one_level("ABCBB"));
        assert(results->size() == 1);
        assert(hfst::symbols::remove_flags(results->begin()->second) ==
               flag_tokenizer.tokenize_one_level("ABCBB"));
        delete results;

        a_paths.harmonize_flag_diacritics(b_paths);

        a_paths.intersect(b_paths).minimize();

        a_paths.convert(HFST_OLW_TYPE);
        
        HfstOneLevelPaths * one_result =
          a_paths.lookup_fd(flag_tokenizer.tokenize_one_level("ABCBA"));
        assert(one_result->size() == 1);
        assert(hfst::symbols::remove_flags(one_result->begin()->second) ==
               flag_tokenizer.tokenize_one_level("ABCBA"));
        delete one_result;

        HfstOneLevelPaths * no_results =
          a_paths.lookup_fd(flag_tokenizer.tokenize_one_level("ABCBB"));
        assert(no_results->size() == 0);
        delete no_results;

        no_results =
          a_paths.lookup_fd(flag_tokenizer.tokenize_one_level("ABCAA"));
        assert(no_results->size() == 0);
        delete no_results;

        no_results =
          a_paths.lookup_fd(flag_tokenizer.tokenize_one_level("ABCCC"));
        assert(no_results->size() == 0);
        delete no_results;
    }

    
    std::cout << "ok" << std::endl;
    return 0;
}

#endif // MAIN_TEST

