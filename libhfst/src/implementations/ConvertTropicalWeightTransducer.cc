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
  extern template class TropicalWeightTpl<float>;
  extern template class ArcTpl<TropicalWeight>;
  extern template class VectorFst<StdArc>;
  extern template class ArcIterator<StdVectorFst>;
  extern template class StateIterator<StdVectorFst>;
}


#ifndef MAIN_TEST
namespace hfst { namespace implementations
{

#if HAVE_OPENFST


  /* Handle symbol tables when converting \a t to \a net. \a hfst_hfst_header
     defines whether \a t is an HFST transducer. */
  static void handle_symbol_tables
  (fst::StdVectorFst * t, HfstBasicTransducer * net, bool has_hfst_header)
  {
    const fst::SymbolTable *inputsym = t->InputSymbols();
    const fst::SymbolTable *outputsym = t->OutputSymbols();
    
    /* An HFST tropical transducer always has an input symbol table. */
    if (has_hfst_header && inputsym == NULL) {
      HFST_THROW(MissingOpenFstInputSymbolTableException);
    }
  
    // An empty transducer
    if (t->Start() == fst::kNoStateId)
      {
        /* An empty OpenFst transducer does not necessarily have to have
           an input or output symbol table. */
        if (inputsym != NULL) {
          for ( fst::SymbolTableIterator it =
                  fst::SymbolTableIterator(*(inputsym));
              ! it.Done(); it.Next() ) {
          assert(it.Symbol() != "");

          if (it.Value() != 0) // epsilon is not inserted
            net->add_symbol_to_alphabet( it.Symbol() );
        }
      }
      /* If the transducer is an OpenFst transducer, it might have an output
         symbol table. If the transducer is an HFST tropical transducer, it
         can have an output symbol table, but it is equivalent to the
         input symbol table. */
        if (! has_hfst_header && outputsym != NULL) {
          for ( fst::SymbolTableIterator it =
                  fst::SymbolTableIterator(*(outputsym));
                ! it.Done(); it.Next() ) {
            assert(it.Symbol() != "");
            if (it.Value() != 0) // epsilon is not inserted
              net->add_symbol_to_alphabet( it.Symbol() );
          }
        }
        return;
      }

    /* A non-empty OpenFst transducer must have at least an input symbol table.
     If the output symbol table is missing, we assume that it would be
     equivalent to the input symbol table. */
    if (inputsym == NULL) {
      HFST_THROW(MissingOpenFstInputSymbolTableException);
    }
    
  }


  /* Copy alphabet of \a t to \a net. */
  static void copy_alphabet(fst::StdVectorFst * t, HfstBasicTransducer * net)
  {
    const fst::SymbolTable *inputsym = t->InputSymbols();
    const fst::SymbolTable *outputsym = t->OutputSymbols();

    if (inputsym != NULL)
      {
        for ( fst::SymbolTableIterator it =
                fst::SymbolTableIterator(*(inputsym));
              ! it.Done(); it.Next() )
          {
            assert(it.Symbol() != "");
            if (it.Value() != 0) // epsilon is not inserted
              net->add_symbol_to_alphabet( it.Symbol() );
          }
      }
    if (outputsym != NULL)
      {
        for ( fst::SymbolTableIterator it =
                fst::SymbolTableIterator(*(outputsym));
              ! it.Done(); it.Next() )
          {
            assert(it.Symbol() != "");
            if (it.Value() != 0) // epsilon is not inserted
              net->add_symbol_to_alphabet( it.Symbol() );
          }
      }
  }



  /* ------------------------------------------------------------------------

     Create an HfstBasicTransducer equivalent to an OpenFst tropical weight
     transducer \a t.

     ------------------------------------------------------------------------ */

  HfstBasicTransducer * ConversionFunctions::
  tropical_ofst_to_hfst_basic_transducer
  (fst::StdVectorFst * t, bool has_hfst_header) {
    
    HfstBasicTransducer * net = new HfstBasicTransducer();

    handle_symbol_tables(t, net, has_hfst_header);
    
    StringVector symbol_vector = TropicalWeightTransducer::get_symbol_vector(t);

    std::vector<unsigned int> harmonization_vector
      = HfstTropicalTransducerTransitionData::get_harmonization_vector(symbol_vector);
    
    /* This takes care that initial state is always number zero
       and state number zero (if it is not initial) is some other number
       (basically as the number of the initial state in that case, i.e.
       the numbers of initial state and state number zero are swapped) */
    StateId initial_state = t->Start();
    
    /* Go through all states */
    for (fst::StateIterator<fst::StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        
        HfstState origin = s;
        if (origin == (unsigned int)initial_state)
          origin = 0;
        else if (origin == 0)
          origin = (unsigned int)initial_state;
        
        unsigned int number_of_arcs = t->NumArcs(s);
        net->initialize_transition_vector(s, number_of_arcs);
        
        /* Go through all transitions in a state */
        for (fst::ArcIterator<fst::StdVectorFst> aiter(*t,s);
             !aiter.Done(); aiter.Next())
          {
            const fst::StdArc &arc = aiter.Value();
            
            HfstState target = arc.nextstate;
            if (target == (unsigned int)initial_state)
              target = 0;
            else if (target == 0)
              target = (unsigned int)initial_state;

            if (arc.ilabel >= symbol_vector.size())
              {
                std::ostringstream oss;
                oss << "FATAL ERROR: input number " << arc.ilabel << " not in symbol_vector" << std::endl;
                HFST_THROW_MESSAGE(HfstFatalException, oss.str());
                //exit(1);
              }
            if (arc.olabel >= symbol_vector.size())
              {
                std::ostringstream oss;
                oss << "FATAL ERROR: output number " << arc.olabel << " not in symbol_vector" << std::endl;
                HFST_THROW_MESSAGE(HfstFatalException, oss.str());
                //exit(1);
              }

            net->add_transition(origin,
                                HfstBasicTransition
                                (target,
                                 harmonization_vector[arc.ilabel],
                                 harmonization_vector[arc.olabel],
                                 arc.weight.Value(),
                                 false), // dummy parameter needed because numbers are used
                                false); // do not insert symbols to alphabet
          }
        
        if (t->Final(s) != fst::TropicalWeight::Zero()) {
          // Set the state as final
          net->set_final_weight(origin, t->Final(s).Value());
        }
        
      }
    
    // Copy the alphabet
    copy_alphabet(t, net);

    assert(net != NULL);
    return net;
}


  /* ---------------------------------------------------------------------------

     Create an OpenFst transducer equivalent to HfstBasicTransducer \a net.

     --------------------------------------------------------------------------- */

  fst::StdVectorFst * ConversionFunctions::
  hfst_basic_transducer_to_tropical_ofst
  (const HfstBasicTransducer * net) {
    
    fst::StdVectorFst * t = new fst::StdVectorFst();
    StateId start_state = t->AddState(); // always zero
    t->SetStart(start_state);
    
    // How state numbers are recoded
    std::vector<StateId> state_vector;
    state_vector.push_back(start_state);
    for (unsigned int i = 1; i <= (net->get_max_state()); i++) {
      state_vector.push_back(t->AddState());
    }

    fst::SymbolTable st("");
    st.AddSymbol(internal_epsilon, 0);
    st.AddSymbol(internal_unknown, 1);
    st.AddSymbol(internal_identity, 2);
    
    // Copy the alphabet
    for (HfstBasicTransducer::HfstAlphabet::iterator it
           = net->alphabet.begin();
         it != net->alphabet.end(); it++) {
      assert(! it->empty());
      st.AddSymbol(*it, net->get_symbol_number(*it));
    }

    // Go through all states...
    unsigned int source_state=0;
    for (HfstBasicTransducer::const_iterator it = net->begin();
         it != net->end(); it++)
      {
        // Go through the set of transitions in each state...
        for (HfstBasicTransducer::HfstTransitions::const_iterator tr_it
               = it->begin();
             tr_it != it->end(); tr_it++)
          {
            // Copy the transition

            unsigned int in = tr_it->get_input_number();
            unsigned int out = tr_it->get_output_number();

            t->AddArc
              (state_vector[source_state],
               fst::StdArc
               ( in,
                 out,
                 tr_it->get_weight(),
                 state_vector[tr_it->get_target_state()]));
          } // ... set of transitions gone through
        source_state++;
      } // ... all states gone through
    
    // Go through the final states...
    for (HfstBasicTransducer::FinalWeightMap::const_iterator it
           = net->final_weight_map.begin();
         it != net->final_weight_map.end(); it++)
      {
        t->SetFinal
          (state_vector[it->first],
           it->second);
      }
    // ... final states gone through
    
    t->SetInputSymbols(&st);
    return t;
  }

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
