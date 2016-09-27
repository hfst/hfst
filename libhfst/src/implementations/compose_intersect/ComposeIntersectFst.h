// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
#ifndef COMPOSE_INTERSECT_FST_H
#define COMPOSE_INTERSECT_FST_H

#include <set>
#include <map>

#include "ComposeIntersectUtilities.h"
#include "../../HfstExceptionDefs.h"
#include "../HfstBasicTransducer.h"

HFST_EXCEPTION_CHILD_DECLARATION(StateNotDefined);

namespace hfst
{
  namespace implementations
  {
    class ComposeIntersectFst
    {
    public:
      struct Transition
      {
        size_t ilabel;
        size_t olabel;
        float weight;
        HfstState target;
        Transition(const HfstBasicTransition &);
        Transition(HfstState,size_t,size_t,float);
        bool operator==(const Transition&) const;
      };
      
      struct CompareTransitions
      {
    bool operator() (const Transition &transition1,
             const Transition &transition2) const;
      };

      typedef compose_intersect_utilities::SpaceSavingSet
    <Transition,CompareTransitions>
    TransitionSet;
      typedef std::set<size_t> SymbolSet;
      static const HfstState START; // = 0;
      ComposeIntersectFst(const HfstBasicTransducer &, bool input_keys);
      ComposeIntersectFst(void);
      virtual ~ComposeIntersectFst(void);
      virtual const TransitionSet &
    get_transitions(HfstState,size_t);
      virtual float get_final_weight(HfstState) const;
      const SymbolSet &get_symbols(void) const;
#ifdef MAIN_TEST
      std::ostream &print(std::ostream &) const;
#endif
    protected:
      typedef std::map<size_t,TransitionSet> SymbolTransitionMap;
      typedef std::vector<SymbolTransitionMap> TransitionMapVector;
      typedef std::vector<Transition> TransitionVector;
      typedef std::vector<float> FloatVector;
      Transition get_identity_transition(HfstState);
      bool has_identity_transition(HfstState);
      bool is_known_symbol(size_t) const;
      size_t get_symbol_number(const std::string &symbol);
      HfstBasicTransducer t;
      SymbolSet symbol_set;
      TransitionMapVector transition_map_vector;
      FloatVector finality_vector;
      TransitionVector identity_transition_vector;
    };
  }
}

#endif
