#include "SentenceTransducer.h"

#ifndef MAIN_TEST

#define LEX_K 1.0

SentenceTransducer::SentenceTransducer(void):
  SequenceModelComponent(),
  current_max_state(SequenceModelComponent::START_STATE)
{}

void SentenceTransducer::add_word
(const std::string &word,const WeightedStringVector &tags)
{
  // Create non-final target states for the word and its analyses.
  State word_target = add_state(std::numeric_limits<float>::infinity());
  State tag_target = add_state(std::numeric_limits<float>::infinity());

  // Add the word transition.
  Symbol2TransitionDataMap &word_symbol_to_transition =
    transition_map[current_max_state];
  Symbol word_symbol = SequenceModelComponent::get_symbol(word);
  add_transition_to_map(word_symbol_to_transition,word_symbol,0,word_target);

  // Add analysis transitions.
  Symbol2TransitionDataMap &tag_symbol_to_transition =
    transition_map[word_target];
  for (const auto & tag : tags)
    {
      Symbol tag_symbol = SequenceModelComponent::get_symbol(tag.second);
      Weight weight = LEX_K*tag.first;

      add_transition_to_map(tag_symbol_to_transition,
			    tag_symbol,
			    weight,
			    tag_target);
    }

  current_max_state = tag_target;
}

const Symbol2TransitionDataMap &SentenceTransducer::operator[](State state)
const
{
#ifndef OPTIMIZE_DANGEROUSLY
  if (state >= static_cast<int>(transition_map.size()))
    { throw InvalidState(); }
#endif // OPTIMIZE_DANGEROUSLY

  return transition_map[state];
}

State SentenceTransducer::get_max_state(void) const
{ return transition_map.size(); }

void SentenceTransducer::finalize(void)
{ state_final_weight_map[current_max_state] = 0; }

#else // MAIN_TEST

#include <cassert>
#include <iostream>

int main(void)
{
  SentenceTransducer sentence_transducer;

  std::string word1 = "a";
  WeightedStringVector word1_tags;
  word1_tags.push_back(WeightedString(1.0,"b"));
  word1_tags.push_back(WeightedString(2.0,"c"));

  sentence_transducer.add_word(word1,word1_tags);

  std::string word2 = "e";
  WeightedStringVector word2_tags;
  word2_tags.push_back(WeightedString(3.0,"f"));
  word2_tags.push_back(WeightedString(4.0,"g"));

  sentence_transducer.add_word(word2,word2_tags);

  sentence_transducer.finalize();

  Weight infinity = std::numeric_limits<float>::infinity();

  assert(sentence_transducer.get_final_weight(0) == infinity);
  assert(sentence_transducer.get_final_weight(1) == infinity);
  assert(sentence_transducer.get_final_weight(2) == infinity);
  assert(sentence_transducer.get_final_weight(3) == infinity);
  assert(sentence_transducer.get_final_weight(4) == static_cast<float>(0));

  assert(sentence_transducer.get_transition
	 (0,SequenceModelComponent::get_symbol("a")).weight ==
	 static_cast<float>(0));

  assert(sentence_transducer.get_transition
	 (0,SequenceModelComponent::get_symbol("a")).target == 1);

  assert(sentence_transducer.get_transition
	 (1,SequenceModelComponent::get_symbol("b")).weight ==
	 static_cast<float>(1.0));

  assert(sentence_transducer.get_transition
	 (1,SequenceModelComponent::get_symbol("b")).target == 2);

  assert(sentence_transducer.get_transition
	 (1,SequenceModelComponent::get_symbol("c")).weight ==
	 static_cast<float>(2.0));

  assert(sentence_transducer.get_transition
	 (1,SequenceModelComponent::get_symbol("c")).target == 2);


  assert(sentence_transducer.get_transition
	 (2,SequenceModelComponent::get_symbol("e")).weight ==
	 static_cast<float>(0));

  assert(sentence_transducer.get_transition
	 (2,SequenceModelComponent::get_symbol("e")).target == 3);

  assert(sentence_transducer.get_transition
	 (3,SequenceModelComponent::get_symbol("f")).weight ==
	 static_cast<float>(3.0));

  assert(sentence_transducer.get_transition
	 (3,SequenceModelComponent::get_symbol("f")).target == 4);

  assert(sentence_transducer.get_transition
	 (3,SequenceModelComponent::get_symbol("g")).weight ==
	 static_cast<float>(4.0));

  assert(sentence_transducer.get_transition
	 (3,SequenceModelComponent::get_symbol("g")).target == 4);
					    

  const Symbol2TransitionDataMap &s = sentence_transducer[3];

  Symbol2TransitionDataMap::const_iterator it = s.begin();
  
  if (it->first == SequenceModelComponent::get_symbol("f"))
    {
      assert(it->second.weight == 3.0);
      assert(it->second.target == 4);

      ++it;

      assert(it->first == SequenceModelComponent::get_symbol("g"));
      assert(it->second.weight == 4.0);
      assert(it->second.target == 4);
    }
  else if (it->first == SequenceModelComponent::get_symbol("g"))
    {
      assert(it->second.weight == 4.0);
      assert(it->second.target == 4);

      ++it;

      assert(it->first == SequenceModelComponent::get_symbol("f"));
      assert(it->second.weight == 3.0);
      assert(it->second.target == 4);
    }
  else
    { assert(false); }

  ++it;
  assert(it == s.end());

  assert(sentence_transducer.get_max_state() == 5);
}

#endif // MAIN_TEST

