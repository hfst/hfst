#include "FstBuilder.h"

#ifndef MAIN_TEST

using hfst::implementations::HfstBasicTransition;

FstBuilder::FstBuilder(ImplementationType type,
		       float default_final_weight,
		       const std::string &name):
  type(type),
  default_final_weight(default_final_weight),
  name(name)
{}

HfstTransducer FstBuilder::get_model(void)
{
  HfstTransducer model(model_fst,type);
  model.set_name(name);
  return model;
}

void FstBuilder::add_transition
(HfstState initial_state,
 HfstState target_state,
 const std::string &symbol,
 float weight)
{
  model_fst.add_transition(initial_state,
			   HfstBasicTransition
			   (target_state,symbol,symbol,weight));
}

void FstBuilder::add_transition
(HfstState initial_state,
 HfstState target_state,
 const std::string &isymbol,
 const std::string &osymbol,
 float weight)
{
  model_fst.add_transition(initial_state,
			   HfstBasicTransition
			   (target_state,isymbol,osymbol,weight));
}

bool FstBuilder::has_target(HfstState s, const std::string &symbol)
{
  if (s == START_STATE)
    { return start_state_targets.count(symbol) != 0; }

  const hfst::implementations::HfstBasicTransitions &transitions = model_fst[s];

  for (const auto & transition : transitions)
    {
      if (transition.get_input_symbol() == symbol)
	{ return true; }
    }

  return false;
}

HfstState FstBuilder::get_target(HfstState s,
				 const std::string &symbol,
				 bool &new_transition_required)
{
  HfstState target = NO_STATE;

  if (s == START_STATE)
    {
      if (start_state_targets.count(symbol) == 0)
	{
	  target = model_fst.add_state();
	  start_state_targets[symbol] = target;
	  new_transition_required = true;
	}
      else
	{
	  target = start_state_targets[symbol];
	  new_transition_required = false;
	}
    }
  else
    {
      const hfst::implementations::HfstBasicTransitions &transitions = model_fst[s];

      for (const auto & transition : transitions)
	{
	  if (transition.get_input_symbol() == symbol)
	    {
	      target = transition.get_target_state();
	      break;
	    }
	}

      if (target == NO_STATE)
	{
	  target = model_fst.add_state();
	  new_transition_required = true;
	}
      else
	{ new_transition_required = false; }
    }

  model_fst.set_final_weight(target,default_final_weight);
  return target;
}

FstBuilder::StringVector
FstBuilder::split_at_tabs(const std::string &str)
{
  StringVector result;
  
  size_t beg_pos = 0;
  size_t end_pos = std::string::npos;
  
  do
    {
      beg_pos = end_pos + 1;
      end_pos = str.find('\t',beg_pos);
      std::string token = str.substr(beg_pos,end_pos - beg_pos);
      result.push_back(token);
    }
  while (end_pos != std::string::npos);
  
  return result;
}

HfstState FstBuilder::add_sequence
(FstBuilder::StringVector::const_iterator it,
 FstBuilder::StringVector::const_iterator end)
{
  HfstState initial_state = START_STATE;
  HfstState target_state  = START_STATE;

  bool new_transition_required = false;

  for ( ; it != end; ++it)
    {
      target_state = get_target(initial_state,
				*it,
				new_transition_required);
      
      if (new_transition_required)
	{ add_transition(initial_state,target_state,*it,0.0); }

      new_transition_required = false;

      initial_state = target_state;
    }

    return target_state;
}

#else // MAIN_TEST

#include <cassert>
#include <iostream>
int main(void)
{
  // Unit test begins.

  

  // Unit test ends.

}
#endif // MAIN_TEST
