// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "back-ends/sfst/interface.h"
#include "back-ends/sfst/fst.h"

#include "ConvertTransducerFormat.h"
#include "HfstBasicTransducer.h"
#include "HfstTransducer.h"

#include <stdexcept>

#ifndef MAIN_TEST
namespace hfst { namespace implementations
{


  /* -----------------------------------------------------------

      Conversion functions between HfstBasicTransducer and SFST transducer.

     ----------------------------------------------------------- */

#if HAVE_SFST

  SFST::VType VMARK = 10000;

  /* Recursively copy all transitions of \a node to \a net.
     Used by function sfst_to_hfst_basic_transducer(SFST::Transducer * t).

     @param node  The current node in the SFST transducer
     @param net   The HfstBasicTransducer that is being created
     @harmonization_vector A vector that defines how symbol numbers are recoded
  */

  void ConversionFunctions::sfst_to_hfst_basic_transducer
  ( SFST::Node *node,
    HfstBasicTransducer *net,
    std::vector<unsigned int> &harmonization_vector) {
    
    // If node has not been visited before
    if (not node->was_visited(VMARK)) {
      SFST::Arcs *arcs=node->arcs();
      
      // Count the number of transitions in the current state
      // and initialize the transition vector of net for maximum efficiency
      unsigned int number_of_arcs=0;
      for( SFST::ArcsIter p(arcs); p; p++ ) {
        number_of_arcs++;
      }
      net->initialize_transition_vector(node->index, number_of_arcs);
      
      // Go through all transitions and copy them to \a net
      for( SFST::ArcsIter p(arcs); p; p++ ) {
        SFST::Arc *arc=p;
        
        net->add_transition
          (node->index,
           HfstBasicTransition
           (arc->target_node()->index,
            harmonization_vector.at(arc->label().lower_char()),
            harmonization_vector.at(arc->label().upper_char()),
            0, false), false);
      }
      
      if (node->is_final()) {
        net->set_final_weight(node->index,0);
      }
      
      // Call this function recursively for all target nodes
      // of the transitions
      for( SFST::ArcsIter p(arcs); p; p++ ) {
        SFST::Arc *arc=p;
        sfst_to_hfst_basic_transducer
          (arc->target_node(), net, harmonization_vector);
      }
    }
  }



  /* Create an HfstBasicTransducer equivalent to an SFST transducer \a t. */
  
  HfstBasicTransducer * ConversionFunctions::sfst_to_hfst_basic_transducer
  ( SFST::Transducer * t ) {
    
#ifdef DEBUG_CONVERSION
    StringSet alphabet_before;
    SFST::Alphabet::CharMap CM = t->alphabet.get_char_map();
    for (SFST::Alphabet::CharMap::const_iterator it
           = CM.begin(); it != CM.end(); it++)
      {
        if (it->first != 0)
          alphabet_before.insert(std::string(it->second));
        else
          alphabet_before.insert(internal_epsilon);
      }
#endif

    // The resulting HfstBasicTransducer
    HfstBasicTransducer * net = new HfstBasicTransducer();

    // How numbers are recoded in the conversion
    StringVector symbol_vector = SfstTransducer::get_symbol_vector(t);
    // change SFST's internal "<>" to HFST's "@_EPSILON_SYMBOL_@"
    symbol_vector.at(0) = "@_EPSILON_SYMBOL_@";
    std::vector<unsigned int> harmonization_vector
      = HfstTropicalTransducerTransitionData::
      get_harmonization_vector(symbol_vector);
    
    // Initialize the number of nodes in the transducer for efficiency
    std::vector<SFST::Node*> indexing;
    t->nodeindexing(&indexing);
    unsigned int number_of_nodes = (unsigned int)indexing.size();
    net->initialize_state_vector(number_of_nodes);
    
    if (t->root_node()->check_visited(VMARK))
      VMARK++;
    
    // Recursively copy the transitions in each node to the basic transducer
    sfst_to_hfst_basic_transducer
      (t->root_node(), net, harmonization_vector);
    
    // Copy the alphabet
    SFST::Alphabet::CharMap cm = t->alphabet.get_char_map();
    for (SFST::Alphabet::CharMap::const_iterator it
           = cm.begin(); it != cm.end(); it++)
      {
        assert(strcmp(it->second, "") != 0);
        if (it->first != 0) { // The epsilon symbol "<>" is not inserted
          net->alphabet.insert(std::string(it->second));
        }
      }
    
#ifdef DEBUG_CONVERSION
    StringSet alphabet_after = net->get_alphabet();
    assert(alphabet_after == alphabet_before);
#endif
    
    return net;
  }
  
  
  /* Create an SFST::Transducer equivalent to HfstBasicTransducer \a net. */

  SFST::Transducer * ConversionFunctions::
  hfst_basic_transducer_to_sfst(const HfstBasicTransducer * net) {
    
    // Create an SFST transducer and insert HFST special symbols to its alphabet
    SFST::Transducer * t = new SFST::Transducer();
    t->alphabet.add_symbol(internal_unknown.c_str(), 1);
    t->alphabet.add_symbol(internal_identity.c_str(), 2);
    
    // Copy the alphabet
    for (HfstBasicTransducer::HfstAlphabet::iterator it
           = net->alphabet.begin();
         it != net->alphabet.end(); it++) {
      if (not is_epsilon(*it) && not is_unknown(*it) && not is_identity(*it))
        t->alphabet.add_symbol(it->c_str(), net->get_symbol_number(*it));
    }
    
    // How symbols occurring in the basic transducer must be recoded
    // in the SFST transducer
    std::map<std::string, unsigned int> symbol_map = SfstTransducer::get_symbol_map(t);
    // Change from SFST's internal "<>" to HFST's "@_EPSILON_SYMBOL_@"
    symbol_map.erase("<>");
    symbol_map[internal_epsilon] = 0;
    std::vector<unsigned int> harm =
      HfstTropicalTransducerTransitionData::get_reverse_harmonization_vector
      (symbol_map);
    
    // State number-to-SFST Node mapping
    std::vector<SFST::Node*> state_vector;
    state_vector.push_back(t->root_node());
    for (unsigned int i=1; i <= (net->get_max_state()); i++) {
      state_vector.push_back(t->new_node());
    }
    
    // Go through all states
    unsigned int source_state=0;
    for (HfstBasicTransducer::const_iterator it = net->begin();
         it != net->end(); it++)
      {
        // Go through the set of transitions in each state
        for (HfstBasicTransducer::HfstTransitions::const_iterator tr_it
               = it->begin();
             tr_it != it->end(); tr_it++)
          {
            // input and output numbers in the transition
            SFST::Label l
              (harm.at(tr_it->get_input_number()),
               harm.at(tr_it->get_output_number()));
            
            // Copy transition to node
            state_vector[source_state]->add_arc
              (l, state_vector[tr_it->get_target_state()], t);
          }
        source_state++;
      }
    
    // Go through the final states
    for (HfstBasicTransducer::FinalWeightMap::const_iterator it
           = net->final_weight_map.begin();
         it != net->final_weight_map.end(); it++)
      {
        if (it->first >= state_vector.size()) { // should not happen..
          state_vector.push_back(t->new_node());
        }
        state_vector[it->first]->set_final(1);
      }
    
    return t;
  }
  
#endif // HAVE_SFST

  }}
#else // MAIN_TEST was defined

#include <iostream>

int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":" << std::endl;
    
    std::cout << "ok" << std::endl;
    return 0;
}

#endif // MAIN_TEST

