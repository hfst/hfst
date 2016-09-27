
// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#ifndef _HFST_TROPICAL_TRANSDUCER_TRANSITION_DATA_H_
#define _HFST_TROPICAL_TRANSDUCER_TRANSITION_DATA_H_


#include <string>
#include <map>
#include <set>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <vector>
#include "../HfstExceptionDefs.h"

#include "../hfstdll.h"

namespace hfst {

  namespace implementations {

    struct string_comparison {
      bool operator() (const std::string &str1, const std::string &str2) const {
        return (str1.compare(str2) < 0);
      }
    };
    
    /** @brief One implementation of template class C in
        HfstTransition.
        
        A HfstTropicalTransducerTransitionData has an input symbol and an
        output symbol of type SymbolType (string) and a weight of type
        WeightType (float).

        \internal Actually a HfstTropicalTransducerTransitionData has an
        input and an output number of type unsigned int, but this
        implementation is hidden from the user.
        The class has two static maps and functions that take care of conversion
        between strings and internal numbers.
        
        @see HfstTransition HfstBasicTransition */
    class HfstTropicalTransducerTransitionData {
    public:
      /** @brief The input and output symbol type. */
      typedef std::string SymbolType;
      /** @brief The weight type. */
      typedef float WeightType;
      /** @brief A set of symbols. */
      typedef std::set<SymbolType> SymbolTypeSet;
      
      typedef std::vector<SymbolType>
        Number2SymbolVector;
      typedef std::map<SymbolType, unsigned int, string_comparison>
        Symbol2NumberMap;

      HFSTDLL static SymbolType get_epsilon();
      HFSTDLL static SymbolType get_unknown();
      HFSTDLL static SymbolType get_identity();
      
    public: /* FIXME: Should be private. */
      /* Maps that contain information of the mappings between strings
         and numbers */
      HFSTDLL static Number2SymbolVector number2symbol_map;
      HFSTDLL static Symbol2NumberMap symbol2number_map;
      /* The biggest number in use. */
      HFSTDLL static unsigned int max_number;

      /* Get the biggest number used to represent a symbol. */
      HFSTDLL static unsigned int get_max_number();

      /*
         Get a vector that defines how numbers of a transducer must
         be changed, i.e. harmonized, so that it follows the same
         number-to-string encoding as all transducers that use the datatype
         HfstTropicalTransducerTransitionData.

         \a symbols defines how numbers are mapped to strings in the
         original transducer so that each index in \a symbols
         is the number that corresponds to the string at that index.
         An empty string at an index means that the index is not
         used in the original transducer.

         The result is a vector whose each index is the number that
         must be replaced by the number at that index when a
         transducer is harmonized. If an index is not used in the
         transducer, the result will contain a zero at that index.
      */
      static std::vector<unsigned int> get_harmonization_vector
        (const std::vector<SymbolType> &symbols);

      static std::vector<unsigned int> get_reverse_harmonization_vector
        (const std::map<SymbolType, unsigned int> &symbols);

    protected:
      /* Get the symbol that is mapped as \a number */
      static const std::string &get_symbol(unsigned int number);

      /* Get the number that is used to represent \a symbol */
      static unsigned int get_number(const std::string &symbol);

      //private: TEST
    public:
      /* The actual transition data */
      unsigned int input_number;
      unsigned int output_number;
      WeightType weight;

    public:
      HFSTDLL void print_transition_data();

    public:

      /** @brief Create a HfstTropicalTransducerTransitionData with
          epsilon input and output strings and weight zero. */
      HFSTDLL HfstTropicalTransducerTransitionData();
      
      /** @brief Create a deep copy of HfstTropicalTransducerTransitionData
          \a data. */
      HFSTDLL HfstTropicalTransducerTransitionData
        (const HfstTropicalTransducerTransitionData &data);

      /** @brief Create a HfstTropicalTransducerTransitionData with
          input symbol \a isymbol, output symbol \a osymbol
          and weight \a weight. */
      HFSTDLL HfstTropicalTransducerTransitionData(SymbolType isymbol,
                                                   SymbolType osymbol,
                                                   WeightType weight);

      HFSTDLL HfstTropicalTransducerTransitionData
        (unsigned int inumber,
         unsigned int onumber,
         WeightType weight);

      /** @brief Get the input symbol. */
      HFSTDLL const SymbolType &get_input_symbol() const;

      /** @brief Get the output symbol. */
      HFSTDLL const SymbolType &get_output_symbol() const;

      HFSTDLL unsigned int get_input_number() const;

      HFSTDLL unsigned int get_output_number() const;
      
      /** @brief Get the weight. */
      HFSTDLL WeightType get_weight() const;

      /** @brief Set the weight. */
      HFSTDLL void set_weight(WeightType w);

      /* Are these needed? */
      HFSTDLL static bool is_epsilon(const SymbolType &symbol);
      HFSTDLL static bool is_unknown(const SymbolType &symbol);
      HFSTDLL static bool is_identity(const SymbolType &symbol);
      HFSTDLL static bool is_valid_symbol(const SymbolType &symbol);
      HFSTDLL static SymbolType get_marker(const SymbolTypeSet &sts);

      /** @brief Whether this transition is less than transition
          \a another.
          
          /internal is it too slow if string comparison is used instead?
      */
      HFSTDLL bool operator<(const HfstTropicalTransducerTransitionData &another) const;
      // same as operator< but weight is ignored
      HFSTDLL bool less_than_ignore_weight(const HfstTropicalTransducerTransitionData &another) const;
      HFSTDLL void operator=(const HfstTropicalTransducerTransitionData &another);
      
      friend class Number2SymbolVectorInitializer;
      friend class Symbol2NumberMapInitializer;

      friend class ComposeIntersectFst;
      friend class ComposeIntersectLexicon;
      friend class ComposeIntersectRule;
      friend class ComposeIntersectRulePair;
      friend class HfstBasicTransducer;

    };

    // Initialization of static members in class
    // HfstTropicalTransducerTransitionData..
    class Number2SymbolVectorInitializer {
    public:
      HFSTDLL Number2SymbolVectorInitializer
        (HfstTropicalTransducerTransitionData::Number2SymbolVector &vect);
    };

    class Symbol2NumberMapInitializer {
    public:
      HFSTDLL Symbol2NumberMapInitializer
        (HfstTropicalTransducerTransitionData::Symbol2NumberMap &map);
    };

  } // namespace implementations

} // namespace hfst

#endif // #ifndef _HFST_TROPICAL_TRANSDUCER_TRANSITION_DATA_H_
