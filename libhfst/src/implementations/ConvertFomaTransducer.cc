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

#include "ConvertTransducerFormat.h"
#include "HfstBasicTransducer.h"
#include "HfstTransducer.h"

#ifndef MAIN_TEST
namespace hfst { namespace implementations
{

  /* -----------------------------------------------------------

      Conversion functions between HfstBasicTransducer and foma transducer.

      ---------------------------------------------------------- */


#if HAVE_FOMA


  /* -----------------------------------------------------------------
     
     Internal functions used by the actual conversion functions.
     
     ----------------------------------------------------------------- */

  /* Get the number of transitions in state \a fsm */
  static unsigned int get_number_of_transitions(struct fsm_state * fsm)
  {
    unsigned int number_of_transitions=0;
    for (unsigned int j=0;
         (fsm+j)->target != -1 &&
           (fsm+j)->state_no == (fsm)->state_no;
         j++)
      {
        number_of_transitions++;
      }
    return number_of_transitions;
  }

  /*
     Handle a start state in a foma transducer.
     
     @param fsm            The start state.
     @param start_state_id The number of the start state, the value -1 means
                           not defined.
     @param start_state_found Whether the start state has been encountered
                              at some point.
    */
  static void handle_start_state
      (struct fsm_state * fsm,
       int &start_state_id,
       bool &start_state_found)
    {
        // If the start state has not yet been encountered.
      if (! start_state_found) {
        start_state_id = (fsm)->state_no; // define the start state
        start_state_found=true;           // define that it is found
      }
      // If the start state is encountered again,
      else if ((fsm)->state_no == start_state_id) {
        // do nothing.
      }
      // If there are several initial states in foma transducer,
      else {
        // throw an exception.
        HFST_THROW_MESSAGE
          (HfstFatalException,
           "Foma transducer has more than one start state");
      }
    }

  /* Copy the alphabet of foma transducer \a t to HFST basic transducer
     \a net. */
  static void copy_alphabet(const struct fsm * t, HfstBasicTransducer * net)
  {
    struct sigma * p = t->sigma;
    while (p != NULL) {
      if (p->symbol == NULL)
        break;
      net->add_symbol_to_alphabet(std::string(p->symbol));
      p = p->next;
    }
  }

  /* Copy the alphabet of hfst basic transducer \a hfst_fsm
     to fsm construct handle \a h. */
  static void copy_alphabet(const HfstBasicTransducer * hfst_fsm,
                            struct fsm_construct_handle * h)
  {
    const HfstBasicTransducer::HfstAlphabet & alpha
      = hfst_fsm->get_alphabet();
    for (HfstBasicTransducer::HfstAlphabet::iterator it
           = alpha.begin();
         it != alpha.end(); it++)
      {
        const char * symbol = it->c_str();
        if ( fsm_construct_check_symbol(h,const_cast<char*>(symbol)) == -1 ) {
          fsm_construct_add_symbol(h,const_cast<char*>(symbol));
        }
        //free(symbol);
      }
  }


  
  /* ----------------------------------------------------------------------

     Create an HfstBasicTransducer equivalent to foma transducer \a t.
     
     ---------------------------------------------------------------------- */

  HfstBasicTransducer * ConversionFunctions::
  foma_to_hfst_basic_transducer(struct fsm * t) {

#ifdef DEBUG_CONVERSION
    StringSet alphabet_before;
    struct sigma * P = t->sigma;
    while (P != NULL) {
      if (P->symbol == NULL)
    break;
      alphabet_before.insert(std::string(P->symbol));
      P = P->next;
    }
    alphabet_before.insert(internal_epsilon);
    alphabet_before.insert(internal_unknown);
    alphabet_before.insert(internal_identity);
#endif // DEBUG_CONVERSION

    StringVector symbol_vector = FomaTransducer::get_symbol_vector(t);
    std::vector<unsigned int> harmonization_vector
      = HfstTropicalTransducerTransitionData::get_harmonization_vector(symbol_vector);
    
    HfstBasicTransducer * net = new HfstBasicTransducer();
    struct fsm_state *fsm;
    fsm = t->states;
    int start_state_id = -1;
    bool start_state_found=false;

  // For every line in foma transducer:
  for (int i=0; (fsm+i)->state_no != -1; i++) {

    // Count the number of transitions in the current state
    // and initialize the transition vector of the net accordingly
    if ((fsm+i)->target != -1 )
      {
        unsigned int number_of_transitions = get_number_of_transitions(fsm+i);
        net->initialize_transition_vector
          ((fsm+i)->state_no,
           number_of_transitions);
      }

    // 1. If the source state is an initial state in foma:
    if ((fsm+i)->start_state == 1) {
      handle_start_state(fsm+i, start_state_id, start_state_found);
    }

    // 2. If there are transitions leaving from the state,
    if ((fsm+i)->target != -1)
      {
        // copy the transition.
        net->add_transition
          ((fsm+i)->state_no,
           HfstBasicTransition
           ((fsm+i)->target,
            harmonization_vector.at((fsm+i)->in),
            harmonization_vector.at((fsm+i)->out),
            0, false), false);
      }
    
    // 3. If the source state is final in foma,
    if ((fsm+i)->final_state == 1) {
      // set the state as final.
      net->set_final_weight((fsm+i)->state_no, 0);
    }
    
  }

  // If there was not an initial state in foma transducer,
  if (! start_state_found) {
    copy_alphabet(t, net);
    return net; // we assume that the transducer is empty.
    // instead of throwing an exception.
    /*HFST_THROW_MESSAGE
      (HfstFatalException,
      "Foma transducer has no start state");*/
  }
  
  /* If start state number (N) is not zero, swap state numbers N and zero
     in internal transducer. TODO */
  if (start_state_id != 0) {
    net->swap_state_numbers(start_state_id,0);
  }

  /* Copy the alphabet */
  copy_alphabet(t, net);

#ifdef DEBUG_CONVERSION
  StringSet alphabet_after = net->get_alphabet();
  if (alphabet_after != alphabet_before) {
    for (StringSet::const_iterator after_it = alphabet_after.begin();
     after_it != alphabet_after.end(); after_it++)
      {
    if (alphabet_before.find(*after_it) == alphabet_before.end())
      {
        std::cerr << "ERROR: "
              << *after_it
              << " was inserted to the alphabet!"
              << std::endl;
      }
      }
    for (StringSet::const_iterator before_it = alphabet_before.begin();
     before_it != alphabet_before.end(); before_it++)
      {
    if (alphabet_after.find(*before_it) == alphabet_after.end())
      {
        std::cerr << "ERROR: "
              << *before_it
              << " was removed from the alphabet!"
              << std::endl;
      }
      }
    assert(false);
  }
#endif // DEBUG_CONVERSION

  return net;
}



  /* ------------------------------------------------------------------------
     
     Create a foma transducer equivalent to HfstBasicTransducer \a hfst_fsm.

     ------------------------------------------------------------------------ */

  struct fsm * ConversionFunctions::
    hfst_basic_transducer_to_foma(const HfstBasicTransducer * hfst_fsm) {

#ifdef DEBUG_CONVERSION
    StringSet alphabet_before = hfst_fsm->get_alphabet();
    alphabet_before.erase(internal_epsilon);
    alphabet_before.erase(internal_unknown);
    alphabet_before.erase(internal_identity);
#endif // DEBUG_CONVERSION

    struct fsm_construct_handle *h;
    struct fsm *net;
    const char * emptystr = "";
    h = fsm_construct_init(const_cast<char*>(emptystr));
    //free(emptystr);
    
    // ----- Go through all states -----
    unsigned int source_state=0;
    for (HfstBasicTransducer::const_iterator it = hfst_fsm->begin();
         it != hfst_fsm->end(); it++)
      {
        // ----- Go through the set of transitions in each state -----
        for (HfstBasicTransducer::HfstTransitions::const_iterator tr_it
               = it->begin();
             tr_it != it->end(); tr_it++)
          {
            // Copy the transition
            const char * input = tr_it->get_transition_data().get_input_symbol().c_str();
            const char * output = tr_it->get_transition_data().get_output_symbol().c_str();
            fsm_construct_add_arc(h,
                                  (int)source_state,
                                  (int)tr_it->get_target_state(),
                                  const_cast<char *>(input),
                                  const_cast<char *>(output) );
            //free(input);
            //free(output);
          }
        // ----- transitions gone through -----
        source_state++;
      }
    // ----- all states gone through -----
    

    // ----- Go through the final states -----
    for (HfstBasicTransducer::FinalWeightMap::const_iterator it
           = hfst_fsm->final_weight_map.begin();
         it != hfst_fsm->final_weight_map.end(); it++)
      {
        // Set the state as final
        fsm_construct_set_final(h, (int)it->first);
      }
    // ----- final states gone through -----

    
    // Copy the alphabet
    copy_alphabet(hfst_fsm, h);
    
    fsm_construct_set_initial(h, 0);
    net = fsm_construct_done(h);
    fsm_count(net);
    net = fsm_topsort(net);

#ifdef DEBUG_CONVERSION
    StringSet alphabet_after;
    struct sigma * p = net->sigma;
    while (p != NULL) {
      if (p->symbol == NULL)
    break;
      if (p->number != 0 && p->number != 1 && p->number != 2)
    alphabet_after.insert(std::string(p->symbol));
      p = p->next;
    }
    assert(alphabet_after == alphabet_before);
#endif // DEBUG_CONVERSION

    return net;
  }

#endif // HAVE_FOMA




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

