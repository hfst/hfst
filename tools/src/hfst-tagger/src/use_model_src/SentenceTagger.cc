#include "SentenceTagger.h"

#ifndef MAIN_TEST

#include "DataTypes.h"
#include "DelayedSequenceModelComponent.h"
#include "SequenceModelComponentPair.h"

SentenceTagger::SentenceTagger(const std::string &lexical_model_filename,
                               const std::string &sequence_model_filename,
                               std::istream * paradigm_guess_stream):
  lexical_model(lexical_model_filename,paradigm_guess_stream)
{
  init_sequence_model(sequence_model_filename);
  buffer_analyses.push_back(WeightedString(0.0,BUFFER));
}

SentenceTagger::~SentenceTagger(void)
{
  for (auto & model : models)
    { delete model; }

  delete sequence_tagger;
}

void SentenceTagger::init_sequence_model
(const std::string &sequence_model_filename)
{
  HfstInputStream in(sequence_model_filename);
  
  SequenceModelComponent * p = NULL;

  if (in.is_good())
    { p = read_model(p,in); }

  while (in.is_good())
    { p = read_model(p,in); }

  top_level_model = p;
  assert(top_level_model != NULL);
  sequence_tagger = new SequenceTagger(*top_level_model);
}

WeightedStringPairVector SentenceTagger::operator[]
(const StringVector &sentence)
{
  // Assert that at least the initial and final buffer symbols are present.
  assert(sentence.size() > 3);

  SentenceTransducer sentence_transducer;

  bool first = true;

  // Add the analyses for initial buffer symbols.
  sentence_transducer.add_word(BUFFER,buffer_analyses);

  for (StringVector::const_iterator it = sentence.begin() + 1;
       it != sentence.end() - 1;
       ++it)
    {
      const std::string &word = *it;

      const WeightedStringVector &unigram_analyses =
        (first ?
         lexical_model.get_first_word_analysis(word) :
         lexical_model[word]);

      sentence_transducer.add_word(word,unigram_analyses);
      
      if (word != "||")
        { first = false; }
    }

  // Add the analyses for final buffer symbols.
  sentence_transducer.add_word(BUFFER,buffer_analyses);

  sentence_transducer.finalize();

  return sequence_tagger->operator[] (sentence_transducer);
}

SequenceModelComponent * SentenceTagger::read_model
(SequenceModelComponent * p,
 HfstInputStream &in)
{
  HfstTransducer model_fst(in);
  SequenceModelComponent * model = new SequenceModelComponent(model_fst);
  models.push_back(model);

  size_t n = get_model_order(model_fst);

  if (p == NULL)
    { p = model; }
  else
    {
      p = new SequenceModelComponentPair(*model,*p);
      models.push_back(p);
    }

  for (size_t i = 1; i < n; ++i)
    {
      DelayedSequenceModelComponent * delayed_model =
        new DelayedSequenceModelComponent(*model,2*i);
      models.push_back(delayed_model);

      p = new SequenceModelComponentPair(*delayed_model,*p);
      models.push_back(p);
    }

  return p;
}

bool SentenceTagger::is_oov(const std::string &word)
{ return lexical_model.is_oov(word); }

bool SentenceTagger::is_lexicon_oov(const std::string &word)
{ return lexical_model.is_lexicon_oov(word); }

size_t SentenceTagger::get_model_order(const HfstTransducer &model_fst)
{
  std::string name = model_fst.get_name();

#ifndef OPTIMIZE_DANGEROUSLY
  assert(name.find("SEQUENCE-MODEL:N=") == 0);
#endif // OPTIMIZE_DANGEROUSLY

  std::string order_string =
    name.substr(std::string("SEQUENCE-MODEL:N=").size());

  std::istringstream in(order_string);

  size_t order;
  in >> order;

  return order;
}

#else // MAIN_TEST

#include <cassert>
#include <iostream>

int main(int argc, char * argv[])
{
  /* FIX ME */
  exit(0);

  if (argc != 3 and argc != 4)
    {
      std::cerr << "Usage: cat input | " << argv[0]
                << " lexical_model sequence_model [ paradigm_guessed_input ]"
                << std::endl;
      exit(1);
    }

  std::istream * paradigm_guesser_in = 0;
  if (argc == 4)
    { paradigm_guesser_in = new std::ifstream(argv[3]); }

  SentenceTagger sentence_tagger(argv[1], argv[2], paradigm_guesser_in);

  StringVector all_data;
  char line[50000];
  while (std::cin.peek() != EOF)
    {
      std::cin.getline(line,50000);
      all_data.push_back(line);
    }

  StringVector::const_iterator start_it        = all_data.begin();
  StringVector::const_iterator end_it          = all_data.begin();
  StringVector::const_iterator absolute_end_it = all_data.end();

  while (end_it != absolute_end_it)
    {
      ++(++end_it);

      while (end_it != absolute_end_it)
        {
          if (*end_it == "||")
            {
              ++end_it;
              ++end_it;
              break;
            }
          ++end_it;
        }

      StringVector sentence(start_it,end_it);

     
      WeightedStringPairVector analysis = sentence_tagger[sentence];

      
      StringPairVector::const_iterator it = analysis.second.begin() + 1;

      for (StringPairVector::const_iterator it = analysis.second.begin();
           it != analysis.second.end() - 2;
           ++it)
        {
          std::cout << /*(sentence_tagger.is_lexicon_oov(it->first) ? (sentence_tagger.is_oov(it->first) ? "*U*" : "*P*" ) + it->first :*/ it->first /*)*/  << "\t" << it->second << std::endl;
        }
     
      start_it = end_it - 2;
    }
  
  std::cout << "||\t||" << std::endl;
  std::cout << "||\t||" << std::endl;

  delete paradigm_guesser_in;
}
#endif // MAIN_TEST
