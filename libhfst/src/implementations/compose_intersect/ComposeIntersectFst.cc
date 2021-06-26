// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.
#include "ComposeIntersectFst.h"

HFST_EXCEPTION_CHILD_DEFINITION(StateNotDefined);

#ifndef MAIN_TEST

namespace hfst
{
  namespace implementations
  {

    const HfstState ComposeIntersectFst::START = 0;

    template<> ComposeIntersectFst::CompareTransitions
    ComposeIntersectFst::TransitionSet::comparator =
     ComposeIntersectFst::CompareTransitions();
    
    ComposeIntersectFst::Transition::Transition(const HfstBasicTransition &t):
      ilabel(HfstTropicalTransducerTransitionData::get_number
         (t.transition_data.get_input_symbol())),
      olabel(HfstTropicalTransducerTransitionData::get_number
         (t.transition_data.get_output_symbol())),
      weight(t.get_weight()),
      target(t.get_target_state())
    {
      assert(t.get_input_symbol() != "");
      assert(t.get_output_symbol() != "");
    }

    ComposeIntersectFst::Transition::Transition
    (HfstState target,size_t ilabel,size_t olabel, float weight):
    ilabel(ilabel),
    olabel(olabel),
    weight(weight),
    target(target)
    {}

    bool ComposeIntersectFst::Transition::operator==
    (const ComposeIntersectFst::Transition &another) const
    {
      return
    ilabel == another.ilabel &&
    olabel == another.olabel &&
    weight == another.weight &&
    target == another.target;
    }
    bool ComposeIntersectFst::CompareTransitions::operator()
    (const Transition &tr1,
     const Transition &tr2) const
    {
      if (tr1.ilabel == tr2.ilabel)
    {
      if (tr1.olabel == tr2.olabel)
        {
          if (tr1.weight == tr2.weight)
        { return tr1.target < tr2.target; }
          else
        { return tr1.weight < tr2.weight; }
        }
      else
        { return tr1.olabel < tr2.olabel; }
    }
      else
    { return tr1.ilabel < tr2.ilabel; }
    }

    ComposeIntersectFst::ComposeIntersectFst(void):
      t(HfstBasicTransducer())
    {}

    ComposeIntersectFst::ComposeIntersectFst
    (const HfstBasicTransducer &t,bool input_keys):
      t(t)
    {
      this->t.sort_arcs();
      std::set<std::string> alphabet = this->t.get_alphabet();

      for (const auto& it : alphabet)
        {
          symbol_set.insert(HfstTropicalTransducerTransitionData::get_number(it));
        }

      unsigned int source_state=0;
      for (const auto & state : this->t)
    {
      transition_map_vector.push_back(SymbolTransitionMap());
      if (this->t.is_final_state(source_state))
        { finality_vector.push_back(this->t.get_final_weight(source_state)); }
      else
        { finality_vector.push_back
        (std::numeric_limits<float>::infinity()); }
      ++source_state;
      SymbolTransitionMap &symbol_transition_map =
        transition_map_vector.back();
      bool identity_found = false;
      for (const auto & arc : state)
        {
          if (arc.get_input_symbol() == "@_IDENTITY_SYMBOL_@")
        {
          identity_found = true;
          identity_transition_vector.push_back(arc);
        }
          else
        { symbol_transition_map
            [input_keys ?
             HfstTropicalTransducerTransitionData::get_number
             (arc.get_input_symbol()) :
             HfstTropicalTransducerTransitionData::get_number
             (arc.get_output_symbol())].
            insert(arc); }
        }
      if (! identity_found)
        { identity_transition_vector.push_back
        (Transition
         (0,
          HfstTropicalTransducerTransitionData::get_number
          ("@_EPSILON_SYMBOL_@"),
          HfstTropicalTransducerTransitionData::get_number
          ("@_EPSILON_SYMBOL_@"),
          0)); }
    }
    }

    ComposeIntersectFst::~ComposeIntersectFst(void)
    {}

    float ComposeIntersectFst::get_final_weight(HfstState s) const
    {
      if (s >= transition_map_vector.size())
    { HFST_THROW(StateNotDefined); }
      return finality_vector.at(s);
    }

    size_t ComposeIntersectFst::get_symbol_number(const std::string &symbol)
    { return HfstTropicalTransducerTransitionData::get_number(symbol); }

    const ComposeIntersectFst::TransitionSet
    &ComposeIntersectFst::get_transitions
    (HfstState s,size_t symbol)
    {
      if (s >= transition_map_vector.size())
    { HFST_THROW(StateNotDefined); }
      if (transition_map_vector.at(s).find(symbol) ==
      transition_map_vector.at(s).end())
    {
      if (is_known_symbol(symbol) || ! has_identity_transition(s))
        { return transition_map_vector.at(s)[symbol] = TransitionSet(); }
      else
        {
          Transition identity_transition =
        get_identity_transition(s);
          transition_map_vector.at(s)[symbol] = TransitionSet();
          transition_map_vector.at(s)[symbol].insert
        (Transition(identity_transition.target,symbol,symbol,
                identity_transition.weight));
          return transition_map_vector.at(s)[symbol];
        }
    }
      return transition_map_vector.at(s)[symbol];
    }

    bool ComposeIntersectFst::is_known_symbol(size_t symbol) const
    { return symbol_set.find(symbol) != symbol_set.end(); }

    ComposeIntersectFst::Transition
    ComposeIntersectFst::get_identity_transition
    (HfstState s)
    {
      if (s >= transition_map_vector.size())
    { HFST_THROW(StateNotDefined); }
      return identity_transition_vector.at(s);
    }

    bool ComposeIntersectFst::has_identity_transition(HfstState s)
    {
      if (s >= transition_map_vector.size())
    { HFST_THROW(StateNotDefined); }
      return identity_transition_vector.at(s).ilabel
    == HfstTropicalTransducerTransitionData::get_number
    ("@_IDENTITY_SYMBOL_@");
    }

    const ComposeIntersectFst::SymbolSet
    &ComposeIntersectFst::get_symbols(void) const
    { return symbol_set; }
  }
}

#else // MAIN_TEST was defined

#include "HfstTransducer.h"
#include <iostream>
using namespace hfst::implementations;
using namespace hfst;
std::ostream &ComposeIntersectFst::print(std::ostream &out) const
{
/*  for (size_t i = 0; i < transition_map_vector.size(); ++i)
    {
      std::cout << "State " << i << ":" << std::endl;
      std::cout << " Final weight: " << get_final_weight(i) << std::endl;
      if (identity_transition_vector.at(i).get_input_symbol() ==
      "@_IDENTITY_SYMBOL_@")
    { std::cout << " Identity target:"
            << identity_transition_vector.at(i).get_target_state()
            << std::endl; }
      else
    { std::cout << " No identity transition" << std::endl; }
      for (SymbolTransitionMap::const_iterator it =
         transition_map_vector.at(i).begin();
       it != transition_map_vector.at(i).end();
       ++it)
    {
      std::cout << it->first << " transitions: " << std::endl;
      for (TransitionSet::const_iterator jt = it->second.begin();
           jt != it->second.end();
           ++jt)
        {
          std::cout << it->first << "\t"
            << jt->get_target_state() << "\t"
            << jt->get_input_symbol() << "\t"
            << jt->get_output_symbol() << "\t"
            << jt->get_weight() << std::endl;
        }
    }
    }
*/ // FIXME: getter function names have changed..
  return out;
}
std::ostream &operator<<(std::ostream &out,const ComposeIntersectFst &fst)
{ return fst.print(out); }

int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":" << std::endl;
  HfstTokenizer tokenizer;
  tokenizer.add_multichar_symbol("@_IDENTITY_SYMBOL_@");
  HfstTransducer t("abc@_IDENTITY_SYMBOL_@",tokenizer,TROPICAL_OPENFST_TYPE);
  HfstTransducer s("bcd@_IDENTITY_SYMBOL_@",tokenizer,TROPICAL_OPENFST_TYPE);
  t.disjunct(s);
  t.repeat_star();
  t.minimize();
//  ComposeIntersectFst cif(t); // FIXME: there is no appropriate constructor
//  std::cout << cif << std::endl;
  
  std::cout << "ok" << std::endl;
  return 0;
}
#endif // MAIN_TEST
