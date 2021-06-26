#include "generate_model_forms.h"

#ifndef MAIN_TEST

#include "guessify_fst.h"

#include <algorithm>
#include <fstream>

using hfst::HfstOneLevelPaths;
using hfst::HfstOneLevelPath;

using hfst::TROPICAL_OPENFST_TYPE;

InvalidModelLine::InvalidModelLine(const std::string &line):
  line(line)
{}

std::ostream &operator<<(std::ostream &out,const StringVector &v)
{
  for (const auto & it : v)
    {
      out << it;
    }

  return out;
}

HfstTokenizer get_alphabet_string_tokenizer(HfstTransducer &fst)
{
  // FIXME: temporary until optimized lookup transducers implement
  // get_alphabet.
  HfstTransducer temp(fst);
  temp.convert(TROPICAL_OPENFST_TYPE);

  StringSet alphabet = temp.get_alphabet();
  
  HfstTokenizer tokenizer;

  for (const auto & it : alphabet)
    { tokenizer.add_multichar_symbol(it); }

  return tokenizer;
}

StringVector get_analysis_prefix(const StringVector &reversed_analysis)
{
  StringVector prefix;

  // We want a prefix of a reversed string, so we iterate from end to
  // beginning.
  for (StringVector::const_reverse_iterator it = reversed_analysis.rbegin();
       it != reversed_analysis.rend();
       ++it)
    {
      prefix.push_back(*it);
      
      if (is_cathegory_symbol(*it))
    {
      // When we return, we have to reverse the result.
      return StringVector(prefix.rbegin(), prefix.rend());
    }
    }
  
  // It should actually be impossible to get here, since valid
  // analyses contain at least one cathegory symbol.
  return prefix;
}

StringVector join(StringVector sv1, const StringVector &sv2)
{
  sv1.insert(sv1.end(), sv2.begin(), sv2.end());
  return sv1;
}

bool contains_analysis_symbols(const StringVector &word_form)
{
  for (const auto & symbol : word_form)
    {
      if (symbol.size() > 1 and
      symbol[0] == '[' and
      symbol[symbol.size() - 1] == ']')
    { return true; }
    }
  
  return false;
}

StringVector generate_word_forms(const StringVector &analysis,
                                 HfstTransducer &form_generator,
                                 size_t max_generated_forms,
                                 float generate_threshold)
{
  HfstOneLevelPaths * word_forms = form_generator.lookup(analysis);

  StringVectorSet result_set;

  size_t num = 1;
  
  float best_weight = -1;

  for (const auto & it : *word_forms)
    {
      if (num > max_generated_forms)
        { break; }

      if (best_weight == -1)
        { best_weight = it.first; }

      if (it.first - best_weight >= generate_threshold)
        { break; }

      const StringVector &word_form = it.second;
      
      if (contains_analysis_symbols(word_form))
        { continue; }
      
      // The word form is reversed, so we start from the end and
      // iterate to the beginning.
      result_set.insert(StringVector(word_form.rbegin(),
                                     word_form.rend()));
      
      ++num;
    }

  delete word_forms;

  StringVector results;

  bool first_form = true;

  for (const auto & it : result_set)
    {
      if (not first_form)
        { results.push_back(", "); }

      results.insert(results.end(), it.begin(), it.end());

      first_form = false;
    }

  if (results.empty())
    { results.push_back("<no word forms>"); }

  return results;
}

StringVectorVector get_model_forms(const StringVector &reversed_analysis,
                                   const StringVectorVector &model_forms,
                                   HfstTransducer &form_generator,
                                   size_t max_generated_forms,
                                   float generate_threshold)
{
  StringVector reversed_analysis_prefix =
    get_analysis_prefix(reversed_analysis);

  StringVectorVector results;

  for (const auto & model_form : model_forms)
    {
      StringVector model_analysis = join(model_form, reversed_analysis_prefix);

      results.push_back(generate_word_forms(model_analysis,
                                            form_generator,
                                            max_generated_forms,
                                            generate_threshold));
    }

  return results;
}

StringPair split(const std::string &line,const std::string &separator)
{
  size_t separator_pos = line.find(separator);
  
  return StringPair(line.substr(0,separator_pos),
            line.substr(separator_pos + 1));
}

StringVector read_model_form(std::istream &in, HfstTokenizer &tokenizer)
{
  std::string line;
  std::getline(in,line);

  if (line.find(MODEL_FORM_PREFIX) != 0)
    { throw InvalidModelLine(line); }

  std::string model_form = line.substr(std::string(MODEL_FORM_PREFIX).size());

  StringVector tokenized_model_form = tokenizer.tokenize_one_level(model_form);
  std::reverse(tokenized_model_form.begin(),tokenized_model_form.end());

  return tokenized_model_form;
}

StringVectorVector read_model_forms(const std::string &model_form_filename,
                    HfstTokenizer &tokenizer)
{
  std::ifstream in(model_form_filename.c_str());

  if (in.peek() == EOF)
    { throw InvalidModelFile(); }

  StringVectorVector results;
  
  while (in.peek() != EOF)
    {
      results.push_back(read_model_form(in, tokenizer));
    }
  
  return results;
}


StringVectorVector get_guesses(const std::string &word_form,
                   HfstTransducer &guesser,
                   size_t number_of_guesses,
                   HfstTokenizer &tokenizer)
{
  StringVector tokenized_line = tokenizer.tokenize_one_level(word_form);
  std::reverse(tokenized_line.begin(),tokenized_line.end());

  HfstOneLevelPaths * paths = guesser.lookup_fd(tokenized_line);

  size_t num = 1;

  StringVectorVector results;

  for (const auto & path : *paths)
    {
      if (num > number_of_guesses)
    { break; }
      results.push_back(path.second);
      ++num;
    }

  delete paths;

  return results;
}

StringVectorVector get_paradigms(const std::string &word_form,
                                 const StringVectorVector &guesses,
                                 HfstTransducer &generator,
                                 const StringVectorVector &model_forms,
                                 size_t number_of_generated_forms,
                                 float generate_threshold)
{
  StringVectorVector paradigm_guesses;

  for (auto analysis_guess : guesses)
    {
      StringVectorVector results = get_model_forms(analysis_guess,
                                                   model_forms,
                                                   generator,
                                                   number_of_generated_forms,
                                                   generate_threshold);
      
      StringVector paradigm;
      paradigm.push_back(word_form);
      paradigm.push_back("\t");

      StringVector rev_analysis_guess(analysis_guess);
      std::reverse(rev_analysis_guess.begin(), rev_analysis_guess.end());
      paradigm.insert(paradigm.end(),
              rev_analysis_guess.begin(),
              rev_analysis_guess.end());

      for (const auto & model_form : results)
        {
          paradigm.push_back("\t");
          paradigm.insert(paradigm.end(),
                  model_form.begin(),
                  model_form.end());
        }
      
      paradigm_guesses.push_back(paradigm);
    }

  return paradigm_guesses;
}

#else // MAIN_TEST

#include <cstdio>
#include <iostream>
#include <exception>

using hfst::HfstInputStream;

int main(int argc, char * argv[])
{
  // Open input fst-file for reading the guesser and generator.
  std::cerr << "Reading guesser and generator from file "
        << argv[1] << std::endl;

  HfstInputStream in(argv[1]);

  HfstTransducer guesser(in);
  HfstTransducer generator(in);

  // Use the generator alphabet (same as the guesser alphabet) to
  // create a tokenizer.
  HfstTokenizer tokenizer = get_alphabet_string_tokenizer(generator);

  // Read the model analyses, that are used to generate word forms.
  std::cerr << "Reading model analyses from file " << argv[2] << std::endl;

  StringVectorVector model_forms = read_model_forms(argv[2],tokenizer);

  // Read input words and generate model forms. Input words are
  // tokenized and reversed for guessing. The MAX_ANALYSES first
  // analysis guesses for each word are split into a prefix consisting
  // of the lemma, pos-tag and categroy symbol. -each of the prefixes
  // is combined with each of the model analyses and the generator is
  // used to generate the corresponding word-forms, which are
  // displayed for the user.
  std::cerr << "Reading unknown word forms from STDIN." << std::endl;
  while (std::cin.peek() != EOF)
    {
      std::string line;
      std::getline(std::cin,line);
      
      StringVectorVector guesses = get_guesses(line,
                           guesser,
                           MAX_ANALYSES,
                           tokenizer);

      StringVectorVector guess_model_forms = get_paradigms(line,
                               guesses,
                               generator,
                               model_forms,
                               MAX_GENERATED_FORMS)
    ;
      

      for (StringVectorVector::const_iterator it = guess_model_forms.begin();
       it != guess_model_forms.end();
       ++it)
    {
      std::cout << *it << std::endl;
    }
    }
}
#endif // MAIN_TEST

