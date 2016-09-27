// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include "HfstBasicTransition.h"

namespace hfst {

  namespace implementations {

    /*unsigned int HfstBasicTransition::get_symbol_number
    (const HfstTropicalTransducerTransitionData::SymbolType &symbol) 
    {
      return HfstTropicalTransducerTransitionData::get_symbol_number(symbol);
      }*/

    HfstBasicTransition::HfstBasicTransition(): target_state(0) {};

    HfstBasicTransition::HfstBasicTransition(HfstState s,
                     HfstTropicalTransducerTransitionData::SymbolType isymbol,
                     HfstTropicalTransducerTransitionData::SymbolType osymbol,
                     HfstTropicalTransducerTransitionData::WeightType weight):
        target_state(s), transition_data(isymbol, osymbol, weight)
    {};
        
    HfstBasicTransition::HfstBasicTransition(HfstState s,
                                             unsigned int inumber,
                                             unsigned int onumber,
                                             HfstTropicalTransducerTransitionData::WeightType weight,
                                             bool foo):
      target_state(s), transition_data(inumber, onumber, weight)
    { (void)foo; };
        
    HfstBasicTransition::HfstBasicTransition(const HfstBasicTransition &another):
      target_state(another.target_state),
      transition_data(another.transition_data)
    {};
    
    HfstBasicTransition::~HfstBasicTransition() {};
    
    bool HfstBasicTransition::operator<(const HfstBasicTransition &another) const {
      if (target_state == another.target_state)
        return (transition_data < another.transition_data);
      return (target_state < another.target_state);
    }

    void HfstBasicTransition::operator=(const HfstBasicTransition &another) {
      target_state = another.target_state;
      transition_data = another.transition_data;
    }
    
    HfstState HfstBasicTransition::get_target_state() const {
      return target_state;
    }
    
    const HfstTropicalTransducerTransitionData & HfstBasicTransition::get_transition_data() const {
      return transition_data;
    }
    
    HfstTropicalTransducerTransitionData::SymbolType HfstBasicTransition::get_input_symbol() const {
      return transition_data.get_input_symbol();
    }
    
    HfstTropicalTransducerTransitionData::SymbolType HfstBasicTransition::get_output_symbol() const {
      return transition_data.get_output_symbol();
    }
    
    unsigned int HfstBasicTransition::get_input_number() const {
      return transition_data.get_input_number();
    }
    
    unsigned int HfstBasicTransition::get_output_number() const {
      return transition_data.get_output_number();
    }
    
    HfstTropicalTransducerTransitionData::WeightType HfstBasicTransition::get_weight() const {
      return transition_data.get_weight();
    }
    
    void HfstBasicTransition::set_weight(HfstTropicalTransducerTransitionData::WeightType w) {
      transition_data.set_weight(w);
    }
  }
  
}

