// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#ifndef _HFST_BASIC_TRANSITION_H_
#define _HFST_BASIC_TRANSITION_H_

/** @file HfstBasicTransition.h
    @brief Class HfstBasicTransition */

#include "../HfstDataTypes.h"
#include "HfstTropicalTransducerTransitionData.h"

#include "../hfstdll.h"

namespace hfst {

  namespace implementations {

   /** @brief A transition that consists of a target state and
        transition data represented by class C.

        The easiest way to use this template is to choose the
        the implementation #HfstBasicTransition which is compatible with
        #HfstBasicTransducer.
        
        @see HfstBasicTransition
   */
    class HfstBasicTransition
      {
      protected:
        HfstState target_state; // the state where the transition leads
        HfstTropicalTransducerTransitionData transition_data;      // the actual transition data

        /* Get the number that represents the symbol in the transition data. */
        /*static unsigned int get_symbol_number
          (const HfstTropicalTransducerTransitionData::SymbolType &symbol);*/
        
      public:
        
        /** @brief Create a transition leading to state zero with input and
            output symbols and weight as given by default constructors
            of C::SymbolType and C::WeightType. */
        HFSTDLL HfstBasicTransition();
        
        /** @brief Create a transition leading to state \a s with input symbol
            \a isymbol, output_symbol \a osymbol and weight \a weight. */
        HFSTDLL HfstBasicTransition(HfstState s,
                     HfstTropicalTransducerTransitionData::SymbolType isymbol,
                                    HfstTropicalTransducerTransitionData::SymbolType osymbol,
                                    HfstTropicalTransducerTransitionData::WeightType weight);
        
        HFSTDLL HfstBasicTransition(HfstState s,
                                    unsigned int inumber,
                                    unsigned int onumber,
                                    HfstTropicalTransducerTransitionData::WeightType weight,
                                    bool foo);
        
        /** @brief Create a deep copy of transition \a another. */
        HFSTDLL HfstBasicTransition(const HfstBasicTransition &another);
        
        HFSTDLL ~HfstBasicTransition();
        
        /** @brief Whether this transition is less than transition \a
            another. Needed for storing transitions in a set. */
        HFSTDLL bool operator<(const HfstBasicTransition &another) const;
        
        /** @brief Assign this transition the same value as transition
            \a another. */
        HFSTDLL void operator=(const HfstBasicTransition &another);
        
        /** @brief Get the target state of the transition. */
        HFSTDLL HfstState get_target_state() const;
        
        /** @brief Get the transition data of the transition. */
        HFSTDLL const HfstTropicalTransducerTransitionData & get_transition_data() const;
        
        /** @brief Get the input symbol of the transition. */
        HFSTDLL HfstTropicalTransducerTransitionData::SymbolType get_input_symbol() const;
        
        /** @brief Get the output symbol of the transition. */
        HFSTDLL HfstTropicalTransducerTransitionData::SymbolType get_output_symbol() const;
        
        /* Get the internal input number of the transition. */
        HFSTDLL unsigned int get_input_number() const;
        
        /* Get the internal output number of the transition. */
        HFSTDLL unsigned int get_output_number() const;
        
        /** @brief Get the weight of the transition. */
        HFSTDLL HfstTropicalTransducerTransitionData::WeightType get_weight() const;
        
        /** @brief Set the weight of the transition. */
        HFSTDLL void set_weight(HfstTropicalTransducerTransitionData::WeightType w);
        
        friend class ComposeIntersectFst;
        friend class ComposeIntersectLexicon;
        friend class ComposeIntersectRule;
        friend class ComposeIntersectRulePair;
      };
  }
  
}

#endif // _HFST_BASIC_TRANSITION_H_
