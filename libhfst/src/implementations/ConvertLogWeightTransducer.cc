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

#ifdef _MSC_VER
#include "back-ends/openfstwin/src/include/fst/fstlib.h"
#else
#include "back-ends/openfst/src/include/fst/fstlib.h"
#endif // _MSC_VER

namespace fst
{
  extern template class LogWeightTpl<float>;
  extern template class ArcTpl<LogWeight>;
  extern template class VectorFst<LogArc>;
  extern template class ArcIterator<LogFst>;
  extern template class StateIterator<LogFst>;
}


#ifndef MAIN_TEST
namespace hfst { namespace implementations
{

#if HAVE_OPENFST
#if HAVE_OPENFST_LOG

  /* --- Conversion between log OpenFst and HfstBasicTransducer --- */
  
  /* Create an HfstBasicTransducer equivalent to an OpenFst log weight
     transducer \a t. */
  HfstBasicTransducer * ConversionFunctions::
  log_ofst_to_hfst_basic_transducer
  (LogFst * t, bool has_hfst_header) {

    const fst::SymbolTable *inputsym = t->InputSymbols();
    const fst::SymbolTable *outputsym = t->OutputSymbols();

    /* An HFST log transducer always has an input symbol table. */
    if (has_hfst_header && inputsym == NULL) {
      HFST_THROW(MissingOpenFstInputSymbolTableException);
    }

  HfstBasicTransducer * net = new HfstBasicTransducer();

  // An empty transducer
  if (t->Start() == fst::kNoStateId)
    {
      /* An empty OpenFst transducer does not necessarily have to have
         an input or output symbol table. */
      if (inputsym != NULL) {
        for ( fst::SymbolTableIterator it =
                fst::SymbolTableIterator(*(inputsym));
              not it.Done(); it.Next() ) {
          if (it.Value() != 0) // epsilon is not inserted
            net->alphabet.insert( it.Symbol() );
        }
      }
      /* If the transducer is an OpenFst transducer, it might have an output
         symbol table. If the transducer is an HFST log transducer, it
         can have an output symbol table, but it is equivalent to the
         input symbol table. */
      if (not has_hfst_header && outputsym != NULL) {
        for ( fst::SymbolTableIterator it =
                fst::SymbolTableIterator(*(outputsym));
              not it.Done(); it.Next() ) {
          if (it.Value() != 0) // epsilon is not inserted
            net->alphabet.insert( it.Symbol() );
        }
      }
      return net;
    }

  /* A non-empty OpenFst transducer must have at least an input symbol table.
     If the output symbol table is missing, we assume that it would be
     equivalent to the input symbol table. */
  if (inputsym == NULL) {
    HFST_THROW(MissingOpenFstInputSymbolTableException);
  }
  if (outputsym == NULL)
    outputsym = inputsym;

  /* This takes care that initial state is always number zero
     and state number zero (if it is not initial) is some other number
     (basically as the number of the initial state in that case, i.e.
     the numbers of initial state and state number zero are swapped) */
  StateId zero_print=0;
  StateId initial_state = t->Start();
  if (initial_state != 0) {
    zero_print = initial_state;
  }

  /* Go through all states */
  for (fst::StateIterator<LogFst> siter(*t);
       not siter.Done(); siter.Next())
    {
      StateId s = siter.Value();
      if (s == initial_state) {
        int origin;  // how origin state is printed, see the first comment
        if (s == 0)
          origin = zero_print;
        else if (s == initial_state)
          origin = 0;
        else
          origin = (int)s;

        /* Go through all transitions in a state */
        for (fst::ArcIterator<LogFst> aiter(*t,s);
             !aiter.Done(); aiter.Next())
          {
            const fst::LogArc &arc = aiter.Value();
            int target;  // how target state is printed, see the first comment
            if (arc.nextstate == 0)
              target = zero_print;
            else if (arc.nextstate == initial_state)
              target = 0;
            else
              target = (int)arc.nextstate;

            // Copy the transition
            std::string istring = inputsym->Find(arc.ilabel);
            std::string ostring = outputsym->Find(arc.olabel);
            if (arc.ilabel == 0)
              istring = std::string(internal_epsilon);
            if (arc.olabel == 0)
              ostring = std::string(internal_epsilon);
            net->add_transition(origin,
                                HfstBasicTransition
                                (target,
                                 istring,
                                 ostring,
                                 arc.weight.Value()
                                 ));
          }
        if (t->Final(s) != fst::LogWeight::Zero()) {
          // Set the state as final
          net->set_final_weight(origin, t->Final(s).Value());
        }
        break;
      }
    }

    for (fst::StateIterator<LogFst> siter(*t);
         not siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if (s != initial_state) {
          int origin;  // how origin state is printed, see the first comment
          if (s == 0)
            origin = zero_print;
          else if (s == initial_state)
            origin = 0;
          else
            origin = (int)s;
          for (fst::ArcIterator<LogFst> aiter(*t,s);
               !aiter.Done(); aiter.Next())
            {
              const fst::LogArc &arc = aiter.Value();
              int target;  // how target state is printed, see the first comment
              if (arc.nextstate == 0)
                target = zero_print;
              else if (arc.nextstate == initial_state)
                target = 0;
              else
                target = (int)arc.nextstate;

              std::string istring = inputsym->Find(arc.ilabel);
              std::string ostring = outputsym->Find(arc.olabel);
              if (arc.ilabel == 0)
                istring = std::string(internal_epsilon);
              if (arc.olabel == 0)
                ostring = std::string(internal_epsilon);
              net->add_transition(origin,
                                  HfstBasicTransition
                                  (target,
                                   istring,
                                   ostring,
                                   arc.weight.Value()
                                   ));
            }
          if (t->Final(s) != fst::LogWeight::Zero())
            net->set_final_weight(origin, t->Final(s).Value());
        }
      }

    /* Make sure that also the symbols that occur only in the alphabet
       but not in transitions are copied. */
    for ( fst::SymbolTableIterator it =
            fst::SymbolTableIterator(*(inputsym));
          not it.Done(); it.Next() ) {
      if (it.Value() != 0) // epsilon is not inserted
        net->alphabet.insert( it.Symbol() );
    }
    for ( fst::SymbolTableIterator it =
            fst::SymbolTableIterator(*(outputsym));
          not it.Done(); it.Next() ) {
      if (it.Value() != 0) // epsilon is not inserted
        net->alphabet.insert( it.Symbol() );
    }

    return net;
}



  /* Get a state id for a state in transducer \a t that corresponds
     to HfstState s as defined in \a state_map.
     Used by function hfst_basic_transducer_to_log_ofst. */
  StateId ConversionFunctions::hfst_state_to_state_id
  (HfstState s, std::map<HfstState, StateId> &state_map,
   LogFst * t)
  {
    std::map<HfstState, StateId>::iterator it = state_map.find(s);
    if (it == state_map.end())
      {
        // If not found, add a state
        StateId retval = t->AddState();
        state_map[s] = retval;
        return retval;
      }
    return it->second;
  }

  /* Create an OpenFst transducer equivalent to HfstBasicTransducer \a net. */
  LogFst * ConversionFunctions::
  hfst_basic_transducer_to_log_ofst
  (const HfstBasicTransducer * net) {
    
    LogFst * t = new LogFst();
    StateId start_state = t->AddState();
    t->SetStart(start_state);
    
    // The mapping between states in HfstBasicTransducer and StdVectorFst
    std::map<HfstState, StateId> state_map;
    state_map[0] = start_state;
    
    fst::SymbolTable st("");
    st.AddSymbol(internal_epsilon, 0);
    st.AddSymbol(internal_unknown, 1);
    st.AddSymbol(internal_identity, 2);
    
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
            // Copy the transition
            t->AddArc( hfst_state_to_state_id(source_state, state_map, t),
                       fst::LogArc
                       ( st.AddSymbol(tr_it->get_input_symbol()),
                         st.AddSymbol(tr_it->get_output_symbol()),
                         tr_it->get_weight(),
                         hfst_state_to_state_id
                          (tr_it->get_target_state(), state_map, t)) );
          }
      }
    
    // Go through the final states
    for (HfstBasicTransducer::FinalWeightMap::const_iterator it
           = net->final_weight_map.begin();
         it != net->final_weight_map.end(); it++)
      {
        t->SetFinal(hfst_state_to_state_id(it->first, state_map, t),
                    it->second);
      }
    
    // Add also symbols that do not occur in transitions
    for (HfstBasicTransducer::HfstAlphabet::iterator it
           = net->alphabet.begin();
         it != net->alphabet.end(); it++) {
        st.AddSymbol(*it);
      }
    
    t->SetInputSymbols(&st);
    return t;
  }

#endif
#endif // HAVE_OPENFST


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

