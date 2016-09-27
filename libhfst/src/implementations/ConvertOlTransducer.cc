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

#include <algorithm>

#include "ConvertTransducerFormat.h"
#include "optimized-lookup/convert.h"
#include "HfstBasicTransducer.h"
#include "HfstTransducer.h"

#ifndef MAIN_TEST
namespace hfst { namespace implementations
{

  /* -----------------------------------------------------------

      Conversion functions between HfstBasicTransducer and
      optimized lookup transducers

      ---------------------------------------------------------- */

/* An auxiliary function. */
unsigned int hfst_ol_to_hfst_basic_add_state
(hfst_ol::Transducer * t,
 HfstBasicTransducer * basic,
 hfst_ol::HfstOlToBasicStateMap & state_map,
 bool weighted,
 hfst_ol::TransitionTableIndex index,
                                             unsigned int state_number)
{
  unsigned int new_state = state_number;
  state_map[index] = new_state;
  
  if(hfst_ol::indexes_transition_index_table(index))
  {
    const hfst_ol::TransitionIndex& transition_index = t->get_index(index);
    
    if(transition_index.final())
    {
        basic->add_state(new_state);
        basic->set_final_weight(new_state,
                                weighted ?
                                dynamic_cast<const hfst_ol::TransitionWIndex&>
                                (transition_index).final_weight() :
                                0.0);
    }
  }
  else // indexes transition table
  {
    const hfst_ol::Transition& transition = t->get_transition(index);
    
    if(transition.final())
    {
        basic->add_state(new_state);
        basic->set_final_weight(new_state,
                                weighted ?
                                dynamic_cast<const hfst_ol::TransitionW&>
                                (transition).get_weight() :
                                0.0);
    }
  }
  return new_state;
}


  /* Create an HfstBasicTransducer equivalent to hfst_ol::Transducer \a t . */
  HfstBasicTransducer * ConversionFunctions::
  hfst_ol_to_hfst_basic_transducer(hfst_ol::Transducer * t)
  {
      HfstBasicTransducer * basic = new HfstBasicTransducer();
      bool weighted = t->get_header().probe_flag(hfst_ol::Weighted);
      const hfst_ol::SymbolTable& symbols
        = t->get_alphabet().get_symbol_table();
      
      
      /* This contains indices to either (1) the start of a set of entries
         in the transition index table, or (2) the boundary before a set
         of entries in the transition table; in this case, the following
         entries will all have the same input symbol. In either case
         the index represents a state and may be final The will already be
         an entry in state_map for each value in agenda */
      std::vector<hfst_ol::TransitionTableIndex> agenda;
      hfst_ol::HfstOlToBasicStateMap state_map;
      unsigned int state_number=0;
      
      hfst_ol_to_hfst_basic_add_state
        (t, basic, state_map, weighted, 0, state_number);
      agenda.push_back(0);
      while(!agenda.empty())
      {
          hfst_ol::TransitionTableIndex current_index = agenda.back();
          agenda.pop_back();
          
          unsigned int current_state = state_map[current_index];
          
          hfst_ol::TransitionTableIndexSet transitions
            = t->get_transitions_from_state(current_index);
          for(hfst_ol::TransitionTableIndexSet::const_iterator it
                =transitions.begin();it!=transitions.end();it++)
          {
              const hfst_ol::Transition& transition = t->get_transition(*it);
              
              if(state_map.find(transition.get_target()) == state_map.end())
              {
                  state_number++;
                  hfst_ol_to_hfst_basic_add_state
                    (t, basic, state_map, weighted,
                     transition.get_target(), state_number);
                  agenda.push_back(transition.get_target());
              }
              basic->add_transition
                (current_state,
                 HfstBasicTransition
                 (state_map[transition.get_target()],
                  symbols[transition.get_input_symbol()],
                  symbols[transition.get_output_symbol()],
                  weighted ? dynamic_cast<const hfst_ol::TransitionW&>
                  (transition).get_weight() : 0 ));
          }
      }
      
      return basic;
      
  }

typedef std::set<std::string> StringSet;
using hfst_ol::SymbolNumber;
using hfst_ol::NO_SYMBOL_NUMBER;

void get_states_and_symbols(
    const HfstBasicTransducer * t,
    std::vector<hfst_ol::StatePlaceholder> & state_placeholders,
    hfst_ol::SymbolTable & symbol_table,
    SymbolNumber & seen_input_symbols,
    std::set<SymbolNumber> & flag_symbols,
    hfst_ol::Transducer * harmonizer)
{
    // Symbols must be in the following order in an optimized-lookup
    // transducer:
    // 1) epsilon
    // 2) other input symbols
    // 3) symbols that aren't used as input symbols
    // Flag diacritics must be indexed as if they were symbol #0
    // (epsilon) and otherwise have a proper unique number,
    // but they can appear anywhere in the alphabet.
    
    // In this case we implement an optimisation where flag diacritics
    // appear at the end of the alphabet. This allows us to ignore
    // them for indexing purposes, which potentially makes the index
    // table smaller and faster to pack.
    
    // We also gather information about possible gaps in the state numbering,
    // because we want it to be contiguous from now on.

    // If we have been told to use a certain symbol table,
    // there's no need to keep track of symbols here.
    StringSet * input_symbols = new StringSet();
    StringSet * flag_diacritics = new StringSet();
    StringSet * other_symbols = new StringSet();
    
    unsigned int first_transition = 0;
    unsigned int state_number = 0;
    for (HfstBasicTransducer::const_iterator it = t->begin();
         it != t->end(); ++it) {
        hfst_ol::Weight final_w = 0.0;
        if (t->is_final_state(state_number)) {
            final_w = t->get_final_weight(state_number);
        }
        state_placeholders.push_back(hfst_ol::StatePlaceholder(
                                         state_number,
                                         t->is_final_state(state_number),
                                         first_transition,
                                         final_w));
        ++first_transition; // there's a padding entry between states
        for (HfstBasicTransducer::HfstTransitions::const_iterator tr_it
                 = it->begin();
             tr_it != it->end(); ++tr_it) {
            ++first_transition;
            // If we don't already have a symbol table, collect symbols
            if (harmonizer == NULL) {
                if (FdOperation::is_diacritic(tr_it->get_input_symbol()) ||
                    hfst_ol::PmatchAlphabet::is_insertion(
                        tr_it->get_input_symbol())) {
                    flag_diacritics->insert(tr_it->get_input_symbol());
                } else {
                    input_symbols->insert(tr_it->get_input_symbol());
                }
                other_symbols->insert(tr_it->get_output_symbol());
            }
        }
        ++state_number;
    }

    std::map<std::string, SymbolNumber> string_symbol_map;

    // Collect symbols if we need to
    if (harmonizer == NULL) {
        
        // 1) epsilon
        string_symbol_map[internal_epsilon] = symbol_table.size();
        symbol_table.push_back(internal_epsilon);
        
        // 2) input symbols
        for (std::set<std::string>::iterator it = input_symbols->begin();
             it != input_symbols->end(); ++it) {
            if (!is_epsilon(*it)) {
                string_symbol_map[*it] = symbol_table.size();
                symbol_table.push_back(*it);
                ++seen_input_symbols;
            }
        }
        
        // 3) Flag diacritics
        for (std::set<std::string>::iterator it = flag_diacritics->begin();
             it != flag_diacritics->end(); ++it) {
            if (!is_epsilon(*it)) {
                string_symbol_map[*it] = symbol_table.size();
                // TODO: cl.exe: conversion from 'size_t' to 'char16_t'
                flag_symbols.insert((unsigned short)symbol_table.size());
                symbol_table.push_back(*it);
                // don't increment seen_input_symbols - we use it for
                // indexing
            }
        }
        
        // 4) non-input symbols
        for (std::set<std::string>::iterator it = other_symbols->begin();
             it != other_symbols->end(); ++it) {
            if (!is_epsilon(*it) && input_symbols->count(*it) == 0 &&
              flag_diacritics->count(*it) == 0) {
                string_symbol_map[*it] = symbol_table.size();
                symbol_table.push_back(*it);
            }
        }
        
    } else {
        symbol_table = harmonizer->get_symbol_table();
        string_symbol_map = harmonizer->get_alphabet().build_string_symbol_map();
        seen_input_symbols = harmonizer->get_header().input_symbol_count();
        for (SymbolNumber i = 0; i < symbol_table.size(); ++i) {
            if (harmonizer->get_alphabet().is_flag_diacritic(i) ||
                hfst_ol::PmatchAlphabet::is_insertion(symbol_table[i])) {
                flag_symbols.insert(i);
            }
        }
    }
    delete input_symbols;
    delete flag_diacritics;
    delete other_symbols;

    // Do a second pass over the transitions, figuring out everything
    // about the states except starting indices

    state_number = 0;
    for (HfstBasicTransducer::const_iterator it = t->begin();
         it != t->end(); ++it) {
        for (HfstBasicTransducer::HfstTransitions::const_iterator tr_it
               = it->begin();
             tr_it != it->end(); ++tr_it) {
            // add input in case we're seeing it the first time
            state_placeholders[state_number].add_input(
                string_symbol_map[tr_it->get_input_symbol()],
                flag_symbols);
            unsigned int target = tr_it->get_target_state();
            hfst_ol::TransitionPlaceholder trans(
                target,
                string_symbol_map[tr_it->get_input_symbol()],
                string_symbol_map[tr_it->get_output_symbol()],
                tr_it->get_weight());
            SymbolNumber input_sym = string_symbol_map[tr_it->get_input_symbol()];
            state_placeholders[state_number].add_transition(trans);
        }
        ++state_number;
    }
}

  /* Create an hfst_ol::Transducer equivalent to HfstBasicTransducer \a t.
     \a weighted defined whether the created transducer is weighted. */
  hfst_ol::Transducer * ConversionFunctions::
  hfst_basic_transducer_to_hfst_ol
  (const HfstBasicTransducer * t, bool weighted, std::string options,
   HfstTransducer * harmonizer)
  {
      const float packing_aggression = 0.85;
      const int floor_jump_threshold = 4; // a packing aggression parameter
      bool quick = options == "quick";
      // The transition array is indexed starting from this constant
      const unsigned int TA_OFFSET = 2147483648u;

      // If we got a harmonizer, we
      // unpack the raw optimized-lookup backend from it
      hfst_ol::Transducer * harmonizer_ol = NULL;
      if (harmonizer != NULL) {
          harmonizer_ol = harmonizer->implementation.hfst_ol;
      }
      
      std::vector<hfst_ol::StatePlaceholder> state_placeholders;
      hfst_ol::SymbolTable symbol_table;
      SymbolNumber seen_input_symbols = 1; // We always have epsilon
      std::set<SymbolNumber> flag_symbols;
      get_states_and_symbols(t,
                             state_placeholders,
                             symbol_table,
                             seen_input_symbols,
                             flag_symbols,
                             harmonizer_ol);

      // For determining the index table we first sort the states (excepting
    // the starting state) by number of different input symbols.
    // if (state_placeholders.begin() != state_placeholders.end()) {
    // std::sort(state_placeholders.begin() + 1, state_placeholders.end(),
    //       hfst_ol::compare_states_by_input_size);
    // }

    hfst_ol::IndexPlaceholders * used_indices =
        new hfst_ol::IndexPlaceholders();

    // Now we assign starting indices (or alternatively determine a state
    // doesn't need an entry in the TIA ("is simple"). The starting state has
    // index 0. Used indices are stored in a map (at the beginning, every
    // index below or equal to the alphabet size is available except index 0).
    // For every state (in the TIA) thereafter, we check each available
    // starting index to see if it fits.

    // The starting state is special because it will have a TIA entry even if
    // it's simple, so we deal with it every time.

    unsigned int first_available_index = 0;
    unsigned int previous_first_index = 0;
    unsigned int previous_successful_index = 0;
    int floor_stuck_counter = 0;
    for (std::vector<hfst_ol::StatePlaceholder>::iterator it =
             state_placeholders.begin();
         it != state_placeholders.end(); ++it) {
        if (it->is_simple()) {
            continue;
        }
        unsigned int i = first_available_index;

        // While this index is not suitable for a starting index, keep looking
        while (!used_indices->fits(*it, flag_symbols, i)) {
            ++i;
        }
        it->start_index = i;
        previous_successful_index = i;
        // Once we've found a starting index, insert a finality marker and
        // mark all the used indices
        used_indices->assign(i, it->state_number, NO_SYMBOL_NUMBER);
        for (std::vector<std::vector<hfst_ol::TransitionPlaceholder> >
                 ::const_iterator tr_it = it->transition_placeholders.begin();
             tr_it != it->transition_placeholders.end(); ++tr_it) {
            SymbolNumber index_offset = tr_it->at(0).input;
            if (flag_symbols.count(index_offset) != 0) {
                index_offset = 0;
            }
            used_indices->assign(i + index_offset + 1, it->state_number, index_offset);
        }

        while (used_indices->unsuitable(first_available_index, seen_input_symbols, packing_aggression)) {
            ++first_available_index;
        }
        if (first_available_index == previous_first_index) {
            if (floor_stuck_counter > floor_jump_threshold) {
                first_available_index = previous_successful_index + 1;
                floor_stuck_counter = 0;
                previous_first_index = first_available_index;
            } else {
                ++floor_stuck_counter;
            }
        } else {
            previous_first_index = first_available_index;
            floor_stuck_counter = 0;
        }
    }

    // Now resort by state number for the rest
    // (this could definitely be neater...)
   // if (state_placeholders.begin() != state_placeholders.end()) {
   // std::sort(state_placeholders.begin() + 1, state_placeholders.end(),
   //       hfst_ol::compare_states_by_state_number);
   // }

    // Now for each index entry we write its input symbol and target

    hfst_ol::TransducerTable<hfst_ol::TransitionWIndex> windex_table;
    
    unsigned int greatest_index = 0;
    if (used_indices->indices.size() != 0) {
        greatest_index = used_indices->indices.size() - 1;
    }

    for(unsigned int i = 0; i <= greatest_index; ++i) {
        if (!used_indices->used(i)) { // blank entries
            windex_table.append(hfst_ol::TransitionWIndex());
        } else if (used_indices->get_target(i).second ==
                   NO_SYMBOL_NUMBER) { // finality markers
            if (state_placeholders[used_indices->get_target(i).first].final) {
                windex_table.append(
                    hfst_ol::TransitionWIndex::create_final(
                        state_placeholders[
                            used_indices->get_target(i).first].final_weight));
            } else {
                windex_table.append(hfst_ol::TransitionWIndex());
            }
        } else { // actual entries
            unsigned int idx = used_indices->get_target(i).first;
            SymbolNumber sym = used_indices->get_target(i).second;
            windex_table.append(
                hfst_ol::TransitionWIndex(
                    sym,
                    state_placeholders[idx].first_transition +
                    state_placeholders[idx].symbol_offset(
                        sym, flag_symbols) + TA_OFFSET));
        }
    }

    delete used_indices;

    for (unsigned int i = 0; i < seen_input_symbols; ++i) {
        windex_table.append(hfst_ol::TransitionWIndex()); // padding
    }

    // Now write the transition table

    hfst_ol::TransducerTable<hfst_ol::TransitionW> wtransition_table;

    hfst_ol::write_transitions_from_state_placeholders(
        wtransition_table,
        state_placeholders,
        flag_symbols);

    hfst_ol::TransducerAlphabet alphabet(symbol_table);
    hfst_ol::TransducerHeader header(seen_input_symbols,
                                     symbol_table.size(),
                                     windex_table.size(),
                                     wtransition_table.size(),
                                     weighted);
    return new hfst_ol::Transducer(header,
                                   alphabet,
                                   windex_table,
                                   wtransition_table);
  }

HfstTransducer * ConversionFunctions::hfst_ol_to_hfst_transducer(
    hfst_ol::Transducer * t)
{
    hfst::ImplementationType type = t->is_weighted() ? HFST_OLW_TYPE : HFST_OL_TYPE;
    HfstTransducer * retval = new HfstTransducer(type);
    delete retval->implementation.hfst_ol;
    retval->implementation.hfst_ol = new hfst_ol::Transducer(*t);
    return retval;
}

hfst_ol::Transducer * ConversionFunctions::hfst_transducer_to_hfst_ol(
    HfstTransducer * t)
{
    if (t->type != HFST_OL_TYPE && t->type != HFST_OLW_TYPE) {
        t->convert(HFST_OLW_TYPE);
    }
    return t->implementation.hfst_ol;
}

}}
#else // MAIN_TEST was defined

#include <iostream>
#include <cassert>

int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":" << std::endl;

    if (not hfst::HfstTransducer::
        is_implementation_type_available(hfst::TROPICAL_OPENFST_TYPE)) {
        std::cout << "No tests run in absence of OpenFst library" << std::endl;
        return 0;
    }

    hfst::HfstBasicTransducer basic;
    basic.add_state(1);
    basic.add_state(2);
    basic.add_transition(0, hfst::HfstBasicTransition(1, "a", "a", 0));
    basic.add_transition(1, hfst::HfstBasicTransition(2, "a", "a", 0));
    basic.add_transition(1, hfst::HfstBasicTransition(2, "a", "b", 0));
    basic.add_transition(0, hfst::HfstBasicTransition(
                             2, "a", hfst::internal_epsilon, 0));
    basic.set_final_weight(2, 0);
    hfst::HfstBasicTransducer basic_w(basic);
    basic_w.set_final_weight(2, 1.0);
    hfst_ol::Transducer * basic_ol =
        hfst::implementations::ConversionFunctions::
        hfst_basic_transducer_to_hfst_ol(& basic, false);
    hfst_ol::Transducer * basic_olw =
        hfst::implementations::ConversionFunctions::
        hfst_basic_transducer_to_hfst_ol(& basic_w, true);
    hfst::HfstBasicTransducer * basic_converted =
        hfst::implementations::ConversionFunctions::
        hfst_ol_to_hfst_basic_transducer(basic_ol);
    hfst::HfstBasicTransducer * basic_converted_w =
        hfst::implementations::ConversionFunctions::
        hfst_ol_to_hfst_basic_transducer(basic_olw);

    hfst::HfstTransducer cmp(basic, hfst::TROPICAL_OPENFST_TYPE);
    hfst::HfstTransducer cmp_w(basic_w, hfst::TROPICAL_OPENFST_TYPE);
    assert(cmp.compare(hfst::HfstTransducer(*basic_converted, hfst::TROPICAL_OPENFST_TYPE)));
    assert(cmp_w.compare(hfst::HfstTransducer(*basic_converted_w, hfst::TROPICAL_OPENFST_TYPE)));
    
    std::cout << "ok" << std::endl;
    return 0;
}

#endif // MAIN_TEST

