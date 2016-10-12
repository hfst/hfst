// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#ifndef _CONVERT_TRANSDUCER_H_
#define _CONVERT_TRANSDUCER_H_

#if HAVE_CONFIG_H
#  include <config.h>
#endif // HAVE_CONFIG_H

#include <map>
#include <iostream>
#include <vector>

#if HAVE_OPENFST
#if OPENFST_VERSION_1_5_4
  #include "back-ends/openfst/src/include/fst/fst-decl.h"
  namespace fst { typedef fst::VectorFst<LogArc> LogFst; }
#else
namespace fst
{
  template <class W> class TropicalWeightTpl;
  typedef TropicalWeightTpl<float> TropicalWeight;

  template <class W> class LogWeightTpl;
  typedef LogWeightTpl<float> LogWeight;

  template <class W> class ArcTpl;
  typedef ArcTpl<TropicalWeight> StdArc;
  typedef ArcTpl<LogWeight> LogArc;

  template <class A> class VectorFst;
  typedef VectorFst<StdArc> StdVectorFst;
  typedef VectorFst<LogArc> LogFst;
}
#endif // OPENFST_VERSION_1_5_4
#endif // HAVE_OPENFST

/*#if HAVE_SFST
#include "back-ends/sfst/fst.h"
#endif // HAVE_SFST

#if HAVE_FOMA
#ifndef _FOMALIB_H_
#define _FOMALIB_H_
#include "back-ends/foma/fomalib.h"
#endif // _FOMALIB_H_
#endif // HAVE_FOMA*/

#if HAVE_XFSM
#include "xfsm/xfsm_api.h"
#endif

/* Add here your transducer library header (and possibly a guard). */
//#if HAVE_MY_TRANSDUCER_LIBRARY
//#ifndef _MY_TRANSDUCER_LIBRARY_LIB_H_
//#define _MY_TRANSDUCER_LIBRARY_LIB_H_
//#include "my_transducer_library/MyTransducerLibrary.h"
//#endif // _MY_TRANSDUCER_LIBRARY_LIB_H_
//#endif // HAVE_MY_TRANSDUCER_LIBRARY

#include "../HfstExceptionDefs.h"
#include "optimized-lookup/transducer.h"
//#include "HfstConstantTransducer.h"

namespace SFST { class Node; class Transducer; }
struct fsm;

#include "../HfstDataTypes.h"

/** @file ConvertTransducerFormat.h
    \brief Declarations of functions for converting between
    transducer backend formats. */

namespace hfst {

namespace implementations {

#if HAVE_OPENFST
  typedef /*fst::StdArc::StateId*/ unsigned int StateId;
  //typedef fst::ArcIterator<fst::StdVectorFst> StdArcIterator;
#endif


  class ConversionFunctions {

  public:

    typedef std::map<std::string, unsigned int> String2NumberMap;
    typedef std::vector<unsigned int> NumberVector;

    /* A number-to-string vector common to all transducers during a session. */
    static StringVector number_to_string_vector;

    /* A string-to-number map common to all transducers during a session. */
    static String2NumberMap string_to_number_map;

    /* Get the string that is represented by \a number in the number-to-string
       vector. If \a number is not found, return the empty string. */
    static std::string get_string(unsigned int number);

    /* Get the number that represents \a str in the string-to-number map.
       If \a str is not found, add it to the next free index. */
    static unsigned int get_number(const std::string &str);

    /* Get a vector that tells how a transducer that follows
       the number-to-symbol encoding of \a coding should be harmonized so that
       it will follow the one of number_to_string_vector. */
    static NumberVector get_harmonization_vector
      (const StringVector &coding_vector);

    static HfstBasicTransducer * hfst_transducer_to_hfst_basic_transducer
      (const hfst::HfstTransducer &t);

#if HAVE_SFST
  static void sfst_to_hfst_basic_transducer
    ( SFST::Node *node,
      HfstBasicTransducer *net,
      std::vector<unsigned int> &harmonization_vector);

  static HfstBasicTransducer * sfst_to_hfst_basic_transducer
    (SFST::Transducer * t);

  static SFST::Transducer * hfst_basic_transducer_to_sfst
    (const HfstBasicTransducer * t);

  /*  static void sfst_to_hfst_fast_transducer
    ( SFST::Node *node,
      HfstFastTransducer *net, NumberVector &harmonization_vector);

  static HfstFastTransducer * sfst_to_hfst_fast_transducer
    (SFST::Transducer * t);

  static SFST::Transducer * hfst_fast_transducer_to_sfst
    (const HfstFastTransducer * t);

  static void sfst_to_hfst_constant_transducer
    ( SFST::Node *node,
      HfstConstantTransducer *net);

  static HfstConstantTransducer * sfst_to_hfst_constant_transducer
    (SFST::Transducer * t);

  static SFST::Transducer * hfst_constant_transducer_to_sfst
  (const HfstConstantTransducer * t); */
#endif // HAVE_SFST
  
#if HAVE_FOMA
  static HfstBasicTransducer * foma_to_hfst_basic_transducer(fsm * t);

  static fsm * hfst_basic_transducer_to_foma
    (const HfstBasicTransducer * t);


  /*  static HfstFastTransducer * foma_to_hfst_fast_transducer(struct fsm * t);

  static struct fsm * hfst_fast_transducer_to_foma
    (const HfstFastTransducer * t);


  static HfstConstantTransducer * foma_to_hfst_constant_transducer
    (struct fsm * t);

  static struct fsm * hfst_constant_transducer_to_foma
  (const HfstConstantTransducer * t); */
#endif // HAVE_FOMA

#if HAVE_XFSM
  static HfstBasicTransducer * xfsm_to_hfst_basic_transducer(NETptr t);
  static NETptr hfst_basic_transducer_to_xfsm(const HfstBasicTransducer * t);
#endif // HAVE_XFSM

#if HAVE_OPENFST
  static HfstBasicTransducer * tropical_ofst_to_hfst_basic_transducer
    (fst::StdVectorFst * t, bool has_hfst_header=true);
  
  static StateId hfst_state_to_state_id
    (HfstState s, std::map<HfstState, StateId> &state_map,
     fst::StdVectorFst * t);

  static fst::StdVectorFst * hfst_basic_transducer_to_tropical_ofst
    (const HfstBasicTransducer * t);


  /*  static HfstFastTransducer * tropical_ofst_to_hfst_fast_transducer
    (fst::StdVectorFst * t, bool has_hfst_header=true);

  static fst::StdVectorFst * hfst_fast_transducer_to_tropical_ofst
    (const HfstFastTransducer * t);



  static HfstConstantTransducer * tropical_ofst_to_hfst_constant_transducer
    (fst::StdVectorFst * t, bool has_hfst_header=true);

  static fst::StdVectorFst * hfst_constant_transducer_to_tropical_ofst
  (const HfstConstantTransducer * t); */

#if HAVE_OPENFST_LOG
  static HfstBasicTransducer * log_ofst_to_hfst_basic_transducer
    (fst::LogFst * t, bool had_hfst_header=true);
  
  static StateId hfst_state_to_state_id
    (HfstState s, std::map<HfstState, StateId> &state_map,
     fst::LogFst * t);

  static fst::LogFst * hfst_basic_transducer_to_log_ofst
    (const HfstBasicTransducer * t);


  /*  static HfstFastTransducer * log_ofst_to_hfst_fast_transducer
    (LogFst * t, bool has_hfst_header=true);

  static LogFst * hfst_fast_transducer_to_log_ofst
    (const HfstFastTransducer * t);


  static HfstConstantTransducer * log_ofst_to_hfst_constant_transducer
    (LogFst * t, bool had_hfst_header=true);

  static LogFst * hfst_constant_transducer_to_log_ofst
  (const HfstConstantTransducer * t); */

#endif

#endif // HAVE_OPENFST
  
  
  static HfstBasicTransducer * hfst_ol_to_hfst_basic_transducer
    (hfst_ol::Transducer * t);

  static hfst_ol::Transducer * hfst_basic_transducer_to_hfst_ol
      (const HfstBasicTransducer * t, bool weighted,
       std::string options="", HfstTransducer * harmonizer = NULL);

  // A way to smuggle a hfst_ol backend into a HfstTransducer wrapper
  static HfstTransducer * hfst_ol_to_hfst_transducer(hfst_ol::Transducer * t);

  // And the reverse
  static hfst_ol::Transducer * hfst_transducer_to_hfst_ol(HfstTransducer * t);

  /* Define here the functions that convert between HfstBasicTransducer and
     your transducer class. */
  //#if HAVE_MY_TRANSDUCER_LIBRARY
  //static HfstBasicTransducer *
  //  my_transducer_library_transducer_to_hfst_basic_transducer
  //    (my_namespace::MyFst * t);
  //
  //static my_namespace::MyFst *
  //  hfst_basic_transducer_to_my_transducer_library_transducer
  //    (const HfstBasicTransducer * t);
  //#endif // HAVE_MY_TRANSDUCER_LIBRARY

    friend class StringVectorInitializer;
    friend class String2NumberMapInitializer;
  
  };

    // Initialization of static members in class
    // ConvertTransducerFormat..
    class StringVectorInitializer {
    public:
      StringVectorInitializer
        (StringVector &vector) {
        vector.push_back(std::string("@_EPSILON_SYMBOL_@"));
    vector.push_back(std::string("@_UNKNOWN_SYMBOL_@"));
        vector.push_back(std::string("@_IDENTITY_SYMBOL_@"));
      }
    };

    class String2NumberMapInitializer {
    public:
      String2NumberMapInitializer
        (ConversionFunctions::String2NumberMap &map) {
        map["@_EPSILON_SYMBOL_@"] = 0;
        map["@_UNKNOWN_SYMBOL_@"] = 1;
        map["@_IDENTITY_SYMBOL_@"] = 2;
      }
    };

} }
#endif // _CONVERT_TRANSDUCER_H_

