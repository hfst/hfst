// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#ifndef _HFST_DATA_TYPES_H_
#define _HFST_DATA_TYPES_H_

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <set>


/** @file HfstDataTypes.h
    \brief Datatypes that are needed when using the HFST API. */

namespace hfst
{

  class HfstTransducer;
  class HfstGrammar;
  class HfstInputStream;
  class HfstOutputStream;

  /// @brief a vector of transducers for methods applying a cascade of automata
  typedef std::vector<HfstTransducer> HfstTransducerVector;

  // ENUMS AND TYPEDEFS...

  /** \brief The type of an HfstTransducer. */
  enum ImplementationType
  {
    SFST_TYPE, /**< An SFST transducer, unweighted. */
    TROPICAL_OPENFST_TYPE, /**< An OpenFst transducer with tropical weights. */
    LOG_OPENFST_TYPE, /**< An OpenFst transducer with logarithmic weights. */
    FOMA_TYPE, /**< A foma transducer, unweighted. */
    XFSM_TYPE, /**< An xfsm transducer, unweighted (mostly for testing purposes). */
    /* Add an enumerator for your transducer type here. */
    //MY_TRANSDUCER_LIBRARY_TYPE,
    HFST_OL_TYPE, /**< An HFST optimized lookup transducer, unweighted */
    HFST_OLW_TYPE, /**< An HFST optimized lookup transducer with weights */
    HFST2_TYPE, /**< HFST2 header present, conversion required */
    UNSPECIFIED_TYPE, /**< Format left open by e.g. default constructor */
    ERROR_TYPE /**< Type not recognised.
                  This type might be returned by a function
                  if an error occurs. */
  };

  const char * implementation_type_to_string(ImplementationType type);
  const char * implementation_type_to_format(ImplementationType type);

  /** \brief The type of a push operation.
      @see HfstTransducer::push_weights */
  enum PushType
  {
    TO_INITIAL_STATE /**< Push weights towards initial state. */,
    TO_FINAL_STATE /**< Push weights towards final state(s). */
  };

  //! @brief A vector of transducers
  //!
  //! Used by compose_intersect.
  typedef std::vector<HfstTransducer> HfstTransducerVector;
 
  //! @brief A pair of transducers
  //!
  //! Used by functions in namespace \link hfst::rules rules\endlink
  typedef std::pair <HfstTransducer,HfstTransducer> HfstTransducerPair;
  //! @brief A vector of transducer pairs
  //!
  //! Used by functions in namespace \link hfst::rules rules\endlink
  typedef std::vector <HfstTransducerPair> HfstTransducerPairVector;

  //! @brief A vector of strings
  //!
  //! Used by typedef HfstOneLevelPath.
  typedef std::vector<std::string> StringVector;
  //! @brief A path of one level of arcs with collected weight.
  //!
  //! Used as the source and result data type for lookups and downs.
  //! In the first case, the float is ignored. In the latter case,
  //! the float represents the weight of the path.
  //!
  //! @see HfstTokenizer::lookup_tokenize
  //! @see HfstTransducer::lookup
  typedef std::pair<float, StringVector> HfstOneLevelPath;
  //! @brief A set of simple paths.
  //!
  //! Used as return type of lookup with multiple, unique results.
  typedef std::set<HfstOneLevelPath> HfstOneLevelPaths;

  //! @brief A vector of string pairs
  //!
  //! Used by typedef HfstTwoLevelPath.
  typedef std::vector<std::pair<std::string,std::string> > StringPairVector;
  //! @brief A path of two level of arcs with collected weight.
  typedef std::pair<float, StringPairVector> HfstTwoLevelPath;
  //! @brief A set of two-level weighted paths.
  typedef std::set<HfstTwoLevelPath> HfstTwoLevelPaths;

  namespace implementations {
    template <class C> class HfstTransitionGraph;
    class HfstTropicalTransducerTransitionData;
    //typedef HfstTransitionGraph<HfstTropicalTransducerTransitionData>
    class HfstBasicTransducer;
      //class HfstFastTransitionData;
      //typedef HfstTransitionGraph<HfstFastTransitionData>
      //HfstFastTransducer;
    typedef unsigned int HfstState;

    template <class C> class HfstTransition;
    //typedef HfstTransition<HfstTropicalTransducerTransitionData>
    class  HfstBasicTransition;
    //typedef HfstTransition<HfstFastTransitionData>
    //  HfstFastTransition;

  }

  // ...ENUMS AND TYPEDEFS


}
// vim: set ft=cpp.doxygen:
#endif
