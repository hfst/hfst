#include "ModelBuilder.h"

#ifndef MAIN_TEST

using hfst::internal_epsilon;

using hfst::TROPICAL_OPENFST_TYPE;

#define DEFAULT_SYMBOL "<NONE>"

bool ModelBuilder::verbose=false;

void ModelBuilder::verbose_printf(const std::string &message)
{
  if (ModelBuilder::verbose)
    { std::cerr << message; }
}

ModelBuilder::ModelBuilder
(const WeightedStringVectorCollection &model_weights):
  FstBuilder(TROPICAL_OPENFST_TYPE,
	     std::numeric_limits<float>::infinity(),
	     model_weights.get_name()),
  depth(0)
{
  for (WeightedStringVectorCollection::const_iterator it =
	 model_weights.begin();
       it != model_weights.end();
       ++it)
    {
      add_sequence(*it,
		   (model_weights.is_lexical_model ? LEXICAL : SEQUENCE));
    }

  if (not model_weights.is_lexical_model)
    { complete_model(model_weights.get_penalty_weight()); }
}

void ModelBuilder::add_sequence(const WeightedStringVector &v,
				weighted_string_type string_type)
{
  StringVector symbol_sequence = v.string_vector;

  this->depth = symbol_sequence.size();

  float weight = v.weight;

  // This should only happen once with the prob of the empty suffix.
  if (symbol_sequence.empty())
    {
      model_fst.set_final_weight(START_STATE,weight);
      return;
    }

  HfstState target_state =
    FstBuilder::add_sequence(symbol_sequence.begin(),
			     symbol_sequence.end() - 1);
  
  std::string last_output_symbol = symbol_sequence.back();
  std::string last_input_symbol =
    ((string_type == LEXICAL) ? internal_epsilon : symbol_sequence.back());
  
  HfstState final_state =
    ((string_type == LEXICAL) ? model_fst.add_state() : START_STATE);

  FstBuilder::add_transition(target_state,
			     final_state,
			     last_input_symbol,
			     last_output_symbol,
			     weight);

  model_fst.set_final_weight(final_state,0.0);
}

void ModelBuilder::complete_model
(HfstState s,
 StateVector::const_iterator default_state_vector_it,
 float penalty_weight)
{
  const hfst::implementations::HfstBasicTransitions &transitions = model_fst[s];
  
  for (const auto & transition : transitions)
    {
      if (transition.get_target_state() == START_STATE)
	{ continue; }

      complete_model(transition.get_target_state(),
		     default_state_vector_it + 1,
		     penalty_weight);
    }

  if (not FstBuilder::has_target(s,DEFAULT_SYMBOL))
    {
      FstBuilder::add_transition(s,
				 *default_state_vector_it,
				 DEFAULT_SYMBOL,
                                 DEFAULT_SYMBOL,
				 (*default_state_vector_it == START_STATE ?
				  penalty_weight :
				  0.0));
    }
}

void ModelBuilder::complete_model(float penalty_weight)
{
  StateVector default_states;

  for (size_t i = 1; i < depth; ++i)
    {
      default_states.push_back(model_fst.add_state());
    }

  default_states.push_back(START_STATE);

  for (size_t i = 0; i < depth - 1; ++i)
    {
      FstBuilder::add_transition(default_states[i],
				 default_states[i+1],
				 DEFAULT_SYMBOL,
				 DEFAULT_SYMBOL,
				 (default_states[i+1] == START_STATE ?
				  penalty_weight :
				  0.0));
    }



  complete_model(START_STATE,
		 default_states.begin(),
  		 penalty_weight);

}

#else // MAIN_TEST

#include <iostream>
#include <cassert>

using hfst::HfstTwoLevelPaths;
using hfst::HfstTwoLevelPath;
using hfst::StringPairVector;
using hfst::StringPair;
using hfst::internal_epsilon;

int main(void)
{

  std::string line1 = "START LEX MODEL\n";
  std::string line2 = "a\tDT\t1.0\n";
  std::string line3 = "oof\tNN\t2.0\n";
  std::string line4 = "STOP LEX MODEL\n";

  std::string file1 = line1 + line2 + line3 + line4;

  std::istringstream in1(file1);

  WeightedStringVectorCollection c1(in1,LEXICAL);

  ModelBuilder model_builder1(c1);

  HfstTransducer model = model_builder1.get_model();
  assert(model.get_name() == "LEX MODEL");

  HfstTwoLevelPaths paths;
  model.extract_paths(paths);

  assert(paths.size() == 2);

  StringPairVector a_vector;
  a_vector.push_back(StringPair("a","a"));
  a_vector.push_back(StringPair(internal_epsilon,"DT"));

  StringPairVector foo_vector;
  foo_vector.push_back(StringPair("o","o"));
  foo_vector.push_back(StringPair("o","o"));
  foo_vector.push_back(StringPair("f","f"));
  foo_vector.push_back(StringPair(internal_epsilon,"NN"));

  assert(paths.count(HfstTwoLevelPath(1,a_vector)) == 1);
  assert(paths.count(HfstTwoLevelPath(2,foo_vector)) == 1);
}

#endif // MAIN_TEST
