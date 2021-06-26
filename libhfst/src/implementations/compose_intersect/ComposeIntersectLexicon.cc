// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.
#include "ComposeIntersectLexicon.h"

#ifndef MAIN_TEST

namespace hfst
{
  namespace implementations
  {
    ComposeIntersectLexicon::ComposeIntersectLexicon
    (const HfstBasicTransducer &t):
      ComposeIntersectFst(t,false)
    {}

    ComposeIntersectLexicon::ComposeIntersectLexicon(void):
      ComposeIntersectFst()
    {}

    bool ComposeIntersectLexicon::is_flag_diacritic(size_t symbol)
    { return FdOperation::is_diacritic
        (HfstTropicalTransducerTransitionData::get_symbol(hfst::size_t_to_uint(symbol))); }

    void ComposeIntersectLexicon::clear_all_info(void)
    {
      state_pair_map.clear();
      pair_vector.clear();

      while (! agenda.empty())
    { agenda.pop(); }

      result = HfstBasicTransducer();
    }

    HfstState ComposeIntersectLexicon::map_state_and_add_to_agenda
    (const StatePair &p, bool allow_lexicon_epsilons)
    {
      HfstState s;

      if (p.first == START && p.second == ComposeIntersectRule::START)
    { s = 0; }
      else
    { s = result.add_state(); }

      // Sanity check...
      assert(s == state_pair_map.size());

      state_pair_map[p] = s;
      pair_vector.push_back(p);
      agenda.push(s);
      lexicon_non_epsilon_states.insert(s);

      return s;
    }

    bool ComposeIntersectLexicon::can_have_lexicon_epsilons(HfstState s)
    { return lexicon_non_epsilon_states.count(s) > 0; }

    HfstBasicTransducer ComposeIntersectLexicon::compose_with_rules
    (ComposeIntersectRule * rules)
    {
      clear_all_info();
      StatePair start_pair = StatePair(START,ComposeIntersectRule::START);

      // This will return 0.
      (void)map_state_and_add_to_agenda(start_pair, true);

      return compute_composition_result(rules);
    }

    HfstState ComposeIntersectLexicon::get_state(const StatePair &p,
                                                 bool allow_lexicon_epsilons)
    {
      if (state_pair_map.find(p) == state_pair_map.end())
    {
      return map_state_and_add_to_agenda(p, allow_lexicon_epsilons);
    }

      return state_pair_map[p];
    }

    void ComposeIntersectLexicon::set_final_state_weights
    (ComposeIntersectRule * rules)
    {
      for (size_t s = 0; s < pair_vector.size(); ++s)
    {
      float lexicon_weight = get_final_weight(pair_vector[s].first);
      float rules_weight = rules->get_final_weight(pair_vector[s].second);
      if (lexicon_weight != std::numeric_limits<float>::infinity() &&
          rules_weight != std::numeric_limits<float>::infinity())
        { result.set_final_weight(hfst::size_t_to_uint(s),lexicon_weight+rules_weight); }
    }
    }

    HfstBasicTransducer &ComposeIntersectLexicon::compute_composition_result
    (ComposeIntersectRule * rules)
    {
      while (! agenda.empty())
        {
          HfstState s = agenda.front();
          agenda.pop();
          
          compute_state(s, rules, can_have_lexicon_epsilons(s));
        }
      
      set_final_state_weights(rules);
      return result;
    }

    ComposeIntersectLexicon::StatePair ComposeIntersectLexicon::get_pair
    (HfstState s)
    {
      if (s >= pair_vector.size())
    { HFST_THROW(StateNotDefined); }

      return pair_vector[s];
    }

    void ComposeIntersectLexicon::compute_state
    (HfstState state,ComposeIntersectRule * rules, bool allow_lexicon_epsilons)
    {
      StatePair p = get_pair(state);

      //bool lexicon_eps_transition_found = false;
      
      for (SymbolTransitionMap::const_iterator it =
         transition_map_vector[p.first].begin();
       it != transition_map_vector[p.first].end();
       ++it)
    {
      if (it->first == HfstTropicalTransducerTransitionData::get_number
          ("@_EPSILON_SYMBOL_@"))
        {
          if (allow_lexicon_epsilons)
            {
              lexicon_skip_symbol_compose(it->second,p.second,state);
              //lexicon_eps_transition_found = true;
            }
        }
      else if (is_flag_diacritic(it->first) &&
           (! rules->known_symbol(it->first)))
        {
          lexicon_skip_symbol_compose(it->second,p.second,state);
          //lexicon_eps_transition_found = true;
        }
      else
        {
          compose(it->second,rules->get_transitions
                  (p.second,it->first),state);
        }
    }
      
      rule_skip_symbol_compose
        (rules->get_transitions
         (p.second,HfstTropicalTransducerTransitionData::get_number
          ("@_EPSILON_SYMBOL_@")),p.first,state);
    }


    void ComposeIntersectLexicon::lexicon_skip_symbol_compose
    (const TransitionSet &transitions,HfstState rule_state,HfstState origin)
    {
      for (const auto & transition : transitions)
        {
          add_transition
            (origin,transition.ilabel,transition.olabel,transition.weight,
             get_state(StatePair(transition.target,rule_state)));
        }
    }

    void ComposeIntersectLexicon::rule_skip_symbol_compose
    (const TransitionSet &transitions,HfstState lex_state,HfstState origin)
    {
      for (const auto & transition : transitions)
    {
      add_transition
        (origin,transition.ilabel,transition.olabel,transition.weight,
         get_state(StatePair(lex_state,transition.target), false));
    }

    }

    void ComposeIntersectLexicon::compose
    (const TransitionSet &lex_transitions,
     const TransitionSet &rule_transitions,HfstState origin)
    {
      StatePair p = get_pair(origin);
      (void)p;
      for (const auto & lex_transition : lex_transitions)
    {
      for (const auto & rule_transition : rule_transitions)
        { add_transition
        (origin,lex_transition.ilabel,rule_transition.olabel,lex_transition.weight + rule_transition.weight,
         get_state(StatePair(lex_transition.target,rule_transition.target))); }
    }
    }

    void ComposeIntersectLexicon::add_transition
    (HfstState origin, size_t input,size_t output,
     float weight,HfstState target)
    { result.add_transition
    (origin,
     HfstBasicTransition
     (target,
      HfstTropicalTransducerTransitionData::get_symbol(hfst::size_t_to_uint(input)),
      HfstTropicalTransducerTransitionData::get_symbol(hfst::size_t_to_uint(output)),
      weight)); }

  }
}

#else // MAIN_TEST was defined

#include "HfstTokenizer.h"
#include "HfstTransducer.h"

using namespace hfst;
using namespace implementations;
#include <cassert>
int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":" << std::endl;
    
  HfstTokenizer tokenizer;
  tokenizer.add_multichar_symbol("@D.SomeVar.1@");
  tokenizer.add_multichar_symbol("@R.SomeVar.1@");
  tokenizer.add_multichar_symbol("@_IDENTITY_SYMBOL_@");
  HfstTransducer lexicon("x@D.SomeVar.1@y@R.SomeVar.1@zxrfoo",
             "x@D.SomeVar.1@y@R.SomeVar.1@zxr",
             tokenizer,TROPICAL_OPENFST_TYPE);
  ComposeIntersectLexicon l(lexicon);

  HfstTransducer x("x",tokenizer,TROPICAL_OPENFST_TYPE);
  HfstTransducer x_a("x","a",tokenizer,TROPICAL_OPENFST_TYPE);
  HfstTransducer y("y",tokenizer,TROPICAL_OPENFST_TYPE);
  HfstTransducer z("z",tokenizer,TROPICAL_OPENFST_TYPE);
  HfstTransducer diamond("D",tokenizer,TROPICAL_OPENFST_TYPE);

  HfstTransducer universal
    ("@_IDENTITY_SYMBOL_@",tokenizer,TROPICAL_OPENFST_TYPE);
  universal.disjunct(x).disjunct(x_a).disjunct(y).disjunct(z).repeat_star().
    minimize();
  
  HfstBasicTransducer b(universal);
  b.add_symbol_to_alphabet("D");
  universal = HfstTransducer(b,TROPICAL_OPENFST_TYPE);
  HfstTransducer right_rule_context(universal);
  right_rule_context.concatenate(diamond).concatenate(universal).concatenate
    (diamond).concatenate(y).concatenate(z).concatenate(universal).minimize();

  HfstTransducer right_rule_center(universal);
  right_rule_center.concatenate(diamond).concatenate(x_a).concatenate(diamond).
    concatenate(universal).minimize();
  
  HfstTransducer inv_right_rule(right_rule_center);
  inv_right_rule.subtract(right_rule_context);
  inv_right_rule.substitute("D","@_EPSILON_SYMBOL_@");
  
  HfstTransducer right_rule(universal);
  right_rule.subtract(inv_right_rule).minimize();
  ComposeIntersectRule * ci_right_rule =
    new ComposeIntersectRule(right_rule);

  HfstTransducer left_rule_abstract_center_pair(x);
  left_rule_abstract_center_pair.disjunct(x_a).minimize();

  HfstTransducer left_rule_center(universal);
  left_rule_center.concatenate(diamond).concatenate
    (x_a).concatenate(diamond).concatenate(universal).minimize();

  HfstTransducer left_rule_context(universal);
  left_rule_context.concatenate(diamond).concatenate
    (left_rule_abstract_center_pair).concatenate(diamond).concatenate(y).
    concatenate(z).concatenate(universal).minimize();
  
  HfstTransducer inv_left_rule(left_rule_context);
  inv_left_rule.subtract(left_rule_center).
    substitute("D","@_EPSILON_SYMBOL_@").minimize();

  HfstTransducer left_rule(universal);
  left_rule.subtract(inv_left_rule).minimize();
  ComposeIntersectRule * ci_left_rule =
    new ComposeIntersectRule(left_rule);

  ComposeIntersectRulePair * rules =
    new ComposeIntersectRulePair(ci_left_rule,ci_right_rule);
  
  ComposeIntersectRule * some_rule =
    new ComposeIntersectRule(universal);
  
  ComposeIntersectRulePair three_rules(some_rule,rules);
  HfstBasicTransducer lex = l.compose_with_rules(&three_rules);
  HfstTransducer lex_fst(lex,TROPICAL_OPENFST_TYPE);
  lex_fst.minimize();
  std::cerr << lex_fst << std::endl;

  std::cout << "ok" << std::endl;
  return 0;
}
#endif // MAIN_TEST
