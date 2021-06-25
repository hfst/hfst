#include "SequenceTagger.h"
#include "AcyclicAutomaton.h"

#ifndef MAIN_TEST

using hfst::implementations::HfstBasicTransition;
using hfst::HfstTwoLevelPaths;
using hfst::HfstTwoLevelPath;
using hfst::HfstTransducer;
using hfst::TROPICAL_OPENFST_TYPE;

State sentence_state = 0;

SequenceTagger::SequenceTagger
(SequenceModelComponent &sequence_model_component):
  sequence_model_component(sequence_model_component),
  //  states(0),
  sentences(0)
{}

SequenceTagger::~SequenceTagger(void)
{}

WeightedStringPairVector SequenceTagger::operator[]
(const SentenceTransducer &sentence_transducer)
{
  // Clear old info.
  state_pair_to_state_map.clear();
  state_to_state_pair_map.clear();
  result.clear();
  sequence_model_component.clear();

  // The first state pair configuration is (0,0).
  StatePair start_pair = StatePair(SequenceModelComponent::START_STATE,
				   SequenceModelComponent::START_STATE);
  
  // Initialize the agenda.
  agenda.push(start_pair);

  // Initialize the result.
  //HfstBasicTransducer result;

  // The start configuration (0,0) correspond to the start state of the result
  // which is 0.
  state_pair_to_state_map[start_pair] = 0;
  state_to_state_pair_map.push_back(start_pair);

  // Process all configurations.
  while (not agenda.empty())
    {
      StatePair configuration = agenda.front();
      agenda.pop();

      State result_state = state_pair_to_state_map[configuration];

      process_configuration
	(sentence_transducer,configuration,result_state,result);
    }
  
  ++sentences;

  return get_weighted_analysis(result);
}

inline void SequenceTagger::process_configuration
(const SentenceTransducer &sentence_transducer,
 const StatePair &configuration,
 State result_state,
 AcyclicAutomaton &result)
{
  // Extract the sentence and model state from configuration.
  State sentence_state = configuration.first;
  State model_state    = configuration.second;

  // Get the sentence transducer transitions for state sentence_state.

  const Symbol2TransitionDataMap &sentence_transitions =
    sentence_transducer[sentence_state];

  // Iterate throught the sentence transitions, get corresponding model
  // transitions and construct the composition of the transitions in result.
  for (auto sentence_transition : sentence_transitions)
    {
      Symbol symbol             = sentence_transition.first;

      Weight sentence_weight = sentence_transition.second.weight;
      State  sentence_target = sentence_transition.second.target;

      TransitionData model_transition =
	sequence_model_component.get_transition(model_state,symbol);

      State  model_target = model_transition.target;
      Weight model_weight = model_transition.weight;

      StatePair new_configuration(sentence_target,model_target);
      State result_target =
	get_state(new_configuration,sentence_transducer,result);

      if (result_target == -1)
	{ continue; }

      result.add_transition
	(result_state,symbol,sentence_weight+model_weight,result_target);
    }
}

inline State SequenceTagger::get_state
(const StatePair &configuration,
 const SentenceTransducer &sentence_transducer,
 AcyclicAutomaton &result)
{
  if (sentence_state != configuration.first)
    {
      result.finalize_position();
      sentence_state = configuration.first;
    }

  StatePair2StateMap::iterator configuration_it =
    state_pair_to_state_map.find(configuration);

  if (configuration_it == state_pair_to_state_map.end())
    {
      State result_state = result.add_state();

      agenda.push(configuration);

      state_pair_to_state_map[configuration] = result_state;
      state_to_state_pair_map.push_back(configuration);

      Weight sentence_final_weight =
	sentence_transducer.get_final_weight(configuration.first);

      Weight model_final_weight =
	sequence_model_component.get_final_weight(configuration.second);

      result.set_final_weight(result_state,
			      sentence_final_weight + model_final_weight);

      return result_state;
    }
  
  return configuration_it->second;
}

WeightedStringPairVector
SequenceTagger::get_weighted_analysis(AcyclicAutomaton &result) const
{
  WeightedStringPairVector tagging;

  WeightedSymbolVector path = result.get_best_path();

  // Set tagging weight.
  tagging.first = path.first;

  // We know that each word should have exactly one tag, so there should
  // be an even number or entries.

  // Go from format
  // word1:word1 tag1:tag1 word2:word2 tag2:tag2 ...
  // to format
  // word1:tag1 word2:tag2 ...
  for (size_t i = 0; i < path.second.size(); ++i)
    {
      if (path.second[i] == -1)
	{ continue; }

      tagging.second.push_back
	(StringPair
	 (SequenceModelComponent::get_string_symbol(path.second[i]),
	  SequenceModelComponent::get_string_symbol(path.second[i+1])));
      ++i;
      }
  
  return tagging;
}

#else // MAIN_TEST

#include "SequenceModelComponentPair.h"
#include "DelayedSequenceModelComponent.h"
#include <cassert>
#include <iostream>

using hfst::implementations::HfstState;
using hfst::HfstTransducer;
using hfst::TROPICAL_OPENFST_TYPE;
using hfst::implementations::HfstBasicTransducer;
using hfst::implementations::HfstBasicTransition;

int main(void)
{
  HfstBasicTransducer b_a;
  b_a.add_state();
  b_a.add_state();
  b_a.add_state();

  b_a.add_transition(0,HfstBasicTransition(1,DEFAULT_SYMBOL,DEFAULT_SYMBOL,0.0));

  b_a.add_transition(1,HfstBasicTransition(2,"A","A",1.0));
  b_a.add_transition(1,HfstBasicTransition(2,"B","B",10.0));
  b_a.add_transition(1,HfstBasicTransition(2,DEFAULT_SYMBOL,DEFAULT_SYMBOL,10.0));

  b_a.add_transition(2,HfstBasicTransition(3,DEFAULT_SYMBOL,DEFAULT_SYMBOL,0.0));

  b_a.add_transition(3,HfstBasicTransition(0,"A","A",10.0));
  b_a.add_transition(3,HfstBasicTransition(0,"B","B",2.0));
  b_a.add_transition(3,HfstBasicTransition(0,DEFAULT_SYMBOL,DEFAULT_SYMBOL,10.0));

  for (HfstState s = 0; s <= b_a.get_max_state(); ++s)
    { b_a.set_final_weight(s,0.0); }

  HfstTransducer a(b_a,TROPICAL_OPENFST_TYPE);

  SequenceModelComponent m(a);

  SequenceModelComponentPair mp(m,m);

  SequenceTagger sequence_tagger(mp);

  SentenceTransducer sentence_transducer;

  WeightedStringVector a_tags;
  a_tags.push_back(WeightedString(0.5,"A"));
  a_tags.push_back(WeightedString(1.5,"B"));
  a_tags.push_back(WeightedString(2.0,"C"));

  WeightedStringVector b_tags;
  b_tags.push_back(WeightedString(0.5,"B"));
  b_tags.push_back(WeightedString(1.5,"A"));
  b_tags.push_back(WeightedString(2.0,"C"));

  WeightedStringVector c_tags;
  c_tags.push_back(WeightedString(0.5,"E"));
  c_tags.push_back(WeightedString(1.5,"F"));
  c_tags.push_back(WeightedString(2.0,"G"));

  sentence_transducer.add_word("a",a_tags);
  sentence_transducer.add_word("b",b_tags);
  sentence_transducer.add_word("a",a_tags);
  sentence_transducer.add_word("b",b_tags);
  sentence_transducer.add_word("c",c_tags);
  sentence_transducer.add_word("c",c_tags);
  sentence_transducer.finalize();

  WeightedStringPairVector result = sequence_tagger[sentence_transducer];
  
  assert(result.second.size() == 6);
  assert(result.second.at(0).first == "a");
  assert(result.second.at(0).second == "A"); // 0.5 + 2*1.0
  assert(result.second.at(1).first == "b");
  assert(result.second.at(1).second == "B"); // 0.5 + 2*2.0
  assert(result.second.at(2).first == "a");
  assert(result.second.at(2).second == "A"); // 0.5 + 2*1.0
  assert(result.second.at(3).first == "b");
  assert(result.second.at(3).second == "B"); // 0.5 + 2*2.0
  assert(result.second.at(4).first == "c");
  assert(result.second.at(4).second == "E"); // 0.5 + 2*10.0
  assert(result.second.at(5).first == "c");
  assert(result.second.at(5).second == "E"); // 0.5 + 2*10.0
                                             //
                                             // SUM: 55.0

  assert(result.first == static_cast<float>(55.0));

  DelayedSequenceModelComponent d(m,2*1);
  
  SequenceModelComponentPair mp2(m,d);

  SequenceTagger sequence_tagger2(mp2);

  result = sequence_tagger2[sentence_transducer];

  assert(result.second.size() == 6);

  // There are multiple paths with the best weight, but the best
  // possible weight is 79.
  assert(result.first == static_cast<float>(79.0));
}
#endif //MAIN_TEST

