#include <limits>

#include "SequenceModelComponent.h"


#ifndef MAIN_TEST

const State SequenceModelComponent::START_STATE = 0;

typedef HfstBasicTransducer::HfstAlphabet
HfstTransitionGraphAlphabet;

using hfst::internal_epsilon;

Symbol2NumberMap SequenceModelComponent::symbol_to_number_map;
Number2SymbolMap SequenceModelComponent::number_to_symbol_map;

SequenceModelComponent::SequenceModelComponent(void):
  state_final_weight_map(1),
  transition_map(1)
{ state_final_weight_map[0] = std::numeric_limits<float>::infinity(); }

SequenceModelComponent::SequenceModelComponent(const HfstBasicTransducer &fst):
  state_final_weight_map(fst.get_max_state() + 1),
  transition_map(fst.get_max_state() + 1)
{
  add_symbols_from(fst);
  add_state_final_weights_from(fst);
  add_transitions_from(fst);
}

SequenceModelComponent::~SequenceModelComponent(void)
{ /* For inheritance */ }

void SequenceModelComponent::clear(void)
{ /* none */ }

void SequenceModelComponent::add_symbols_from(const HfstBasicTransducer &fst)
{
  add_symbol(internal_epsilon);
  add_symbol(DEFAULT_SYMBOL);

  const HfstTransitionGraphAlphabet &alphabet = fst.get_alphabet();

  for (HfstTransitionGraphAlphabet::const_iterator it = alphabet.begin();
       it != alphabet.end();
       ++it)
    { add_symbol(*it); }
}

void SequenceModelComponent::add_symbol(const std::string &string_symbol)
{
  if (symbol_to_number_map.count(string_symbol) == 0)
    {
      symbol_to_number_map[string_symbol] = number_to_symbol_map.size();
      number_to_symbol_map.push_back(string_symbol);
    }
}

Symbol SequenceModelComponent::get_symbol(const std::string &string_symbol)
{
  add_symbol(string_symbol);
  return symbol_to_number_map[string_symbol];
}

std::string SequenceModelComponent::get_string_symbol(Symbol symbol)
{
#ifndef OPTIMIZE_DANGEROUSLY
  assert(symbol <= static_cast<int>(number_to_symbol_map.size()));
#endif // OPTIMIZE_DANGEROUSLY
  return number_to_symbol_map[symbol];
}

void SequenceModelComponent::add_state_final_weights_from
(const HfstBasicTransducer &fst)
{
  Weight infinity = std::numeric_limits<float>::infinity();

  for (State s = 0; s <= static_cast<int>(fst.get_max_state()); ++s)
    {
      if (fst.is_final_state(s))
	{ state_final_weight_map[s] = fst.get_final_weight(s); }
      else
	{ state_final_weight_map[s] = infinity; }
    }
}

void SequenceModelComponent::add_transitions_from
(const HfstBasicTransducer &fst)
{
  for (State s = 0; s <= static_cast<int>(fst.get_max_state()); ++s)
    {
      const HfstBasicTransducer::HfstTransitions transitions = fst[s];

      Symbol2TransitionDataMap &symbol_to_transition = transition_map[s];

      for (HfstBasicTransducer::HfstTransitions::const_iterator it =
	     transitions.begin();
	   it != transitions.end();
	   ++it)
	{
	  Symbol symbol = symbol_to_number_map[it->get_input_symbol()];
	  
	  add_transition_to_map(symbol_to_transition,
				symbol,
				it->get_weight(),
				it->get_target_state());
	}
    }
}

State SequenceModelComponent::add_state(Weight final_weight)
{
  State state = transition_map.size();
  transition_map.push_back(Symbol2TransitionDataMap());
  state_final_weight_map.push_back(final_weight);
  return state;
}

void SequenceModelComponent::add_transition_to_map
(Symbol2TransitionDataMap &symbol_to_transition,
 Symbol symbol,
 Weight weight,
 State target)
{
  TransitionData transition_data;
  transition_data.weight = weight;
  transition_data.target = target;
  symbol_to_transition[symbol] = transition_data;
}

Weight SequenceModelComponent::get_final_weight(State state) const
{
#ifndef OPTIMIZE_DANGEROUSLY
  if (static_cast<int>(state_final_weight_map.size()) <= state)
    { throw InvalidState(); }
#endif // OPTIMIZE_DANGEROUSLY

  return state_final_weight_map[state];
}

TransitionData SequenceModelComponent::get_transition(State state,
						      Symbol symbol)
{
#ifndef OPTIMIZE_DANGEROUSLY
  if (static_cast<int>(transition_map.size()) <= state)
    { throw InvalidState(); }
#endif // OPTIMIZE_DANGEROUSLY

  const Symbol2TransitionDataMap &symbol_to_transition =
    transition_map.at(state);
  if (symbol_to_transition.count(symbol) == 0)
    {
      if (symbol_to_transition.count(DEFAULT) == 0)
	{
	  std::cerr << state << " " << symbol << std::endl;
	  throw InvalidKey();
	}
      
      // Can't use the regular []-operator, since symbol_to_transition
      // is const.
      return symbol_to_transition.find(DEFAULT)->second;
    }

  // Can't use the regular []-operator, since symbol_to_transition
  // is const.
  return symbol_to_transition.find(symbol)->second;
}

#else // MAIN_TEST

#include <cassert>
#include <iostream>

using hfst::implementations::HfstBasicTransition;

int main(void)
{
  // fst initially has one state.
  HfstBasicTransducer fst;

  // Add three states in order to get four state totally.
  fst.add_state();
  fst.add_state();
  fst.add_state();

  // Set states 1 and 3 final.
  fst.set_final_weight(1,1.0);
  fst.set_final_weight(3,2.0);

  // Add clockwise a-transitions.
  fst.add_transition(0,HfstBasicTransition(1,"a","a",0.1));
  fst.add_transition(1,HfstBasicTransition(2,"a","a",0.2));
  fst.add_transition(2,HfstBasicTransition(3,"a","a",0.3));
  fst.add_transition(3,HfstBasicTransition(0,"a","a",0.4));

  // Add counter clockwise b-transitions.
  fst.add_transition(0,HfstBasicTransition(3,"b","b",1.1));
  fst.add_transition(3,HfstBasicTransition(2,"b","b",1.2));
  fst.add_transition(2,HfstBasicTransition(1,"b","b",1.3));
  fst.add_transition(1,HfstBasicTransition(0,"b","b",1.4));

  // Add an extra <NONE>-transition from state 1 to state 2.
  fst.add_transition(1,HfstBasicTransition(2,
					   DEFAULT_SYMBOL,
					   DEFAULT_SYMBOL,
					   10.0));

  SequenceModelComponent sequence_model_component(fst);

  Weight infinity = std::numeric_limits<float>::infinity();

  // Check state final weights.
  assert(sequence_model_component.get_final_weight(0) == infinity);
  assert(sequence_model_component.get_final_weight(1) == 1.0);
  assert(sequence_model_component.get_final_weight(2) == infinity);
  assert(sequence_model_component.get_final_weight(3) == 2.0);

  // Check clockwise transitions.
  assert(sequence_model_component.get_transition
	 (0, SequenceModelComponent::get_symbol("a")).weight ==
	 static_cast<float>(0.1));

  assert(sequence_model_component.get_transition
	 (0, SequenceModelComponent::get_symbol("a")).target ==
	 static_cast<float>(1));

  assert(sequence_model_component.get_transition
	 (1, SequenceModelComponent::get_symbol("a")).weight ==
	 static_cast<float>(0.2));

  assert(sequence_model_component.get_transition
	 (1, SequenceModelComponent::get_symbol("a")).target ==
	 static_cast<float>(2));

  assert(sequence_model_component.get_transition
	 (2, SequenceModelComponent::get_symbol("a")).weight ==
	 static_cast<float>(0.3));

  assert(sequence_model_component.get_transition
	 (2, SequenceModelComponent::get_symbol("a")).target ==
	 static_cast<float>(3));

  assert(sequence_model_component.get_transition
	 (3, SequenceModelComponent::get_symbol("a")).weight ==
	 static_cast<float>(0.4));

  assert(sequence_model_component.get_transition
	 (3, SequenceModelComponent::get_symbol("a")).target ==
	 static_cast<float>(0));

  // Check counter-clockwise transitions.
  assert(sequence_model_component.get_transition
	 (0, SequenceModelComponent::get_symbol("b")).weight ==
	 static_cast<float>(1.1));

  assert(sequence_model_component.get_transition
	 (0, SequenceModelComponent::get_symbol("b")).target ==
	 static_cast<float>(3));

  assert(sequence_model_component.get_transition
	 (1, SequenceModelComponent::get_symbol("b")).weight ==
	 static_cast<float>(1.4));

  assert(sequence_model_component.get_transition
	 (1, SequenceModelComponent::get_symbol("b")).target ==
	 static_cast<float>(0));

  assert(sequence_model_component.get_transition
	 (2, SequenceModelComponent::get_symbol("b")).weight ==
	 static_cast<float>(1.3));

  assert(sequence_model_component.get_transition
	 (2, SequenceModelComponent::get_symbol("b")).target ==
	 static_cast<float>(1));

  assert(sequence_model_component.get_transition
	 (3, SequenceModelComponent::get_symbol("b")).weight ==
	 static_cast<float>(1.2));

  assert(sequence_model_component.get_transition
	 (3, SequenceModelComponent::get_symbol("b")).target ==
	 static_cast<float>(2));

  // Check default transition
  assert(sequence_model_component.get_transition
	 (1, SequenceModelComponent::get_symbol("c")).target ==
	 static_cast<float>(2));

  assert(sequence_model_component.get_transition
	 (1, SequenceModelComponent::get_symbol("c")).weight ==
	 static_cast<float>(10.0));
}
#endif // MAIN_TEST
