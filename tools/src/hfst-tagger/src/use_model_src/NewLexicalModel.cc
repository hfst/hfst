#include "NewLexicalModel.h"

#ifndef MAIN_TEST

#include <algorithm>
#include <queue>
#include <math.h>

using hfst::StringVector;
using hfst::HfstOneLevelPaths;
using hfst::HfstOneLevelPath;

NewLexicalModel::NewLexicalModel(const std::string &filename, std::istream * paradigm_guess_stream):
  in(filename),
  lexical_model(in),
  upper_case_interpolator(8.79), // <----- FIX ME!!!
  lower_case_interpolator(31.0), // <----- FIX ME!!!
  id(0),
  lexical_model_is_broken(false),
  paradigm_guess_stream(paradigm_guess_stream)
{ initialize_tag_probabilities(); }

void NewLexicalModel::initialize_tag_probabilities(void)
{
  HfstOneLevelPaths * upper_case_tag_paths =
    lexical_model.lookup("<upper_suffix_and_tag>");

  HfstOneLevelPaths * lower_case_tag_paths =
    lexical_model.lookup("<lower_suffix_and_tag>");

  if (upper_case_tag_paths->empty())
    { lexical_model_is_broken = true; }

  if (lower_case_tag_paths->empty())
    { lexical_model_is_broken = true; }

  for (HfstOneLevelPaths::const_iterator it = upper_case_tag_paths->begin();
       it != upper_case_tag_paths->end();
       ++it)
    {
      float penalty = it->first;

      if (it->second.empty())
	{
	  lexical_model_is_broken = true;
	  break;
	}

      const std::string &tag = *(it->second.rbegin());

      upper_case_tag_penalties[tag] = penalty;
      upper_case_empty_suffix_guesses.push_back
	(WeightedString(get_prob(penalty),tag));
      upper_case_empty_suffix_null.push_back(WeightedString(0,tag));
    }

  for (HfstOneLevelPaths::const_iterator it = lower_case_tag_paths->begin();
       it != lower_case_tag_paths->end();
       ++it)
    {
      float penalty = it->first;

      if (it->second.empty())
	{
	  lexical_model_is_broken = true;
	  break;
	}

      const std::string &tag = *(it->second.rbegin());

      lower_case_tag_penalties[tag] = penalty;
      lower_case_empty_suffix_guesses.push_back
	(WeightedString(get_prob(penalty),tag));
      lower_case_empty_suffix_null.push_back(WeightedString(0,tag));
    }

  delete upper_case_tag_paths;
  delete lower_case_tag_paths;
}

std::string NewLexicalModel::to_lower_case(const std::string &word) const
{
  std::string ret = word;
  std::transform(ret.begin(), ret.end(),ret.begin(),::tolower);
  return ret;
}

const WeightedStringVector &NewLexicalModel::get_first_word_analysis
(const std::string &word)
{
  StringVector rev_tokenized_word = tokenizer.tokenize_one_level(word);
  std::reverse(rev_tokenized_word.begin(),rev_tokenized_word.end());

  HfstOneLevelPaths * paths = lexical_model.lookup(rev_tokenized_word);
  bool word_not_in_lexicon = paths->empty();
  delete paths;

  if (word_not_in_lexicon)
    {
      std::string lower_case_word = to_lower_case(word);

      lexicon_o_o_v_words.insert(word);
      if (not is_lexicon_oov(lower_case_word))
	{
	  o_o_v_words.insert(word);
	  return operator[](lower_case_word);
	}
      else
	{ return operator[](word); }
    }
  else
    { return operator[](word); }
}

const WeightedStringVector &NewLexicalModel::operator[]
(const std::string &word)
{
  if (lexical_model_is_broken)
    { throw BrokenLexicalModel(); }

  AnalysisCache::const_iterator word_it = analysis_cache.find(word);

  if (word_it == analysis_cache.end())
    {
      const WeightedStringVector & v = cache_word_analyses(word);
      return v;
    }

  return word_it->second;
}

StringVector NewLexicalModel::split(const std::string &line)
{
  StringVector result;

  size_t tab_pos = std::string::npos;
  size_t old_tab_pos = std::string::npos;

  do
    {
      tab_pos = line.find('\t',old_tab_pos + 1);
      result.push_back(line.substr(old_tab_pos + 1,tab_pos == std::string::npos ? std::string::npos : tab_pos - old_tab_pos - 1));
      old_tab_pos = tab_pos;
    }
  while (old_tab_pos != std::string::npos && old_tab_pos != line.size() - 1);

  return result;
}

float NewLexicalModel::get_weight(const std::string &weight_string)
{
  std::istringstream in(weight_string);
  float weight;
  in >> weight;
  return weight;
}

const WeightedStringVector &NewLexicalModel::paradigm_guess
(const std::string &word,
 const StringVector &guess_vector)
{
  WeightedStringVector &word_guesses = analysis_cache[word];
  
  assert(guess_vector[0] == word);
  
  for (size_t i = 1; i < guess_vector.size(); ++(++i))
    {
      float weight = get_weight(guess_vector[i]);
      const std::string &tag = guess_vector[i+1];
 
      word_guesses.push_back(WeightedString(1.6*weight,tag));
    }

  return word_guesses;
}

const WeightedStringVector &NewLexicalModel::cache_word_analyses
(const std::string &word)
{
  // Tokenize word and reverse the sequence of tokens.
  StringVector rev_tokenized_word = tokenizer.tokenize_one_level(word);
  std::reverse(rev_tokenized_word.begin(),rev_tokenized_word.end());

  bool upper_case = false;

  if (word.size() > 0)
    {
      if (word[0] >= 65 and word[0] <= 90)
        { upper_case = true; }
      else if (word.find("Å") == 0 or word.find("Ä") == 0 or
	       word.find("Ö") == 0)
        { upper_case = true; }
    }

  HfstOneLevelPaths * paths = lexical_model.lookup(rev_tokenized_word);

  if (paths->empty())
    {
      delete paths;

      lexicon_o_o_v_words.insert(word);

      if (paradigm_guess_stream != 0)
	{
	  std::string line;
	  StringVector guesses = split(line);
	  while (guesses[0] != word and paradigm_guess_stream->peek() != EOF)
	    {
	      std::getline(*paradigm_guess_stream,line);
	      guesses = split(line);
	    }
	  if (line.find("+?") == std::string::npos)
	    {
	      assert((guesses.size() + 1) % 2 == 0);
	      return paradigm_guess(word,guesses);
	    }
	}
      o_o_v_words.insert(word);
      return guess(word,upper_case,rev_tokenized_word);
    }
  else
    {
      return cache_analyses(word,paths,analysis_cache);
    }
}

float NewLexicalModel::get_prob(float weight)
{ return exp(-1 * weight); }

float NewLexicalModel::get_penalty(float prob)
{ return -1 * log(prob); }

std::string NewLexicalModel::to_string(StringVector::const_iterator begin,
				       StringVector::const_iterator end)
{
  std::string result;
  result.reserve(end - begin);
  for (StringVector::const_iterator it = begin;
       it != end;
       ++it)
    { result += *it; }
  return result;
}

const WeightedStringVector &NewLexicalModel::guess
(const std::string &word,bool upper_case,
 const StringVector &rev_tokenized_word)
{
  WeightedStringVector &word_analyses = analysis_cache[word];
 
  word_analyses = upper_case ? upper_case_empty_suffix_null : lower_case_empty_suffix_null;

  size_t suffix_length = std::min(rev_tokenized_word.size(),
				  static_cast<size_t>(100));

  size_t last_found_suffix_length = suffix_length;

  for (size_t i = suffix_length; i > 0; --i)
    {
      const WeightedStringVector &suffix_analyses =
	get_suffix_analyses(word,
			    rev_tokenized_word.begin(),
			    rev_tokenized_word.begin() + i,
			    upper_case);
      
      if (suffix_analyses.empty())
	{
	  --last_found_suffix_length;
	  continue;
	}

      merge_analyses(suffix_analyses,word_analyses,upper_case);
    }

  merge_analyses(upper_case ? upper_case_empty_suffix_guesses : lower_case_empty_suffix_guesses,word_analyses,upper_case);
 
  
  // For Finnish
  word_analyses = get_suffix_analyses(word,
				      rev_tokenized_word.begin(),
				      rev_tokenized_word.begin() + last_found_suffix_length,
				      upper_case);

  bayesian_invert
    (word_analyses,
     to_string(rev_tokenized_word.begin(),
	       rev_tokenized_word.begin() + last_found_suffix_length),
     upper_case);
    
  return word_analyses;
}

bool NewLexicalModel::is_oov(const std::string &word)
{ return o_o_v_words.find(word) != o_o_v_words.end(); }

bool NewLexicalModel::is_lexicon_oov(const std::string &word)
{
  StringVector rev_tokenized_word = tokenizer.tokenize_one_level(word);
  std::reverse(rev_tokenized_word.begin(),rev_tokenized_word.end());

  HfstOneLevelPaths * paths = lexical_model.lookup(rev_tokenized_word);
  bool empty = paths->empty();
  delete paths;
  return empty;
}

size_t NewLexicalModel::get_suffix_length(StringVector::const_iterator begin,
					  StringVector::const_iterator end)
{
  size_t length = 0;

  for ( ; begin != end; ++begin)
    { length += begin->size(); }

  return length;
}

const WeightedStringVector &NewLexicalModel::get_suffix_analyses
(const std::string &word,
 StringVector::const_iterator begin,
 StringVector::const_iterator end,
 bool upper_case)
{
  AnalysisCache &suffix_cache = upper_case ? upper_case_suffix_cache : lower_case_suffix_cache;
  std::string suffix = word.substr(word.size() - get_suffix_length(begin,end));

  AnalysisCache::const_iterator suffix_it = suffix_cache.find(suffix);

  
  if (suffix_it == suffix_cache.end())
    {
      return cache_suffix_analyses(word,suffix,begin,end);
    }

  return suffix_it->second;
}

const WeightedStringVector &NewLexicalModel::cache_suffix_analyses
(const std::string &word,
 const std::string &suffix,
 StringVector::const_iterator begin,
 StringVector::const_iterator end)
{
 
  bool upper_case = false;
  
  if (not word.empty())
    {
      if (word[0] >= 65 and word[0] <= 90)
	{ upper_case = true; }
      else if (word.size() > 1)
	{
	  std::string initial_segment = word.substr(0,2);
          if (initial_segment == "Å" or initial_segment == "Ä" or initial_segment == "Ö")
	    { upper_case = true; }
	}
    }
 

  StringVector guess_string_vector(begin,end);
  guess_string_vector.push_back(upper_case ? "<upper_suffix_and_tag>" : "<lower_suffix_and_tag>");
  
  HfstOneLevelPaths * paths = lexical_model.lookup(guess_string_vector);

  return cache_analyses(suffix,paths,
			upper_case ? upper_case_suffix_cache : lower_case_suffix_cache,true);
}

const WeightedStringVector &NewLexicalModel::cache_analyses
(const std::string &word,
 HfstOneLevelPaths * paths,
 AnalysisCache &cache,
 bool convert_to_probabilities)
{
  size_t number_of_tags = paths->size();

  WeightedStringVector &word_analyses = cache[word];
  word_analyses.insert(word_analyses.end(),
		       number_of_tags,
		       WeightedString());

  size_t i = 0;
  for (HfstOneLevelPaths::const_iterator it = paths->begin();
       it != paths->end();
       ++it)
    {
      // Each entry in paths, is a weight analysis pair. The last entry in
      // the analysis is the tag.
      word_analyses[i].first  =
	convert_to_probabilities ? get_prob(it->first) : it->first;
      word_analyses[i].second = *(it->second.rbegin());
      ++i;
    }

  delete paths;
  return word_analyses;
}

void NewLexicalModel::merge_analyses
(const WeightedStringVector &suffix_analyses,
 WeightedStringVector &word_analyses,
 bool upper_case)
{
  ++id;

  for (WeightedStringVector::const_iterator it = suffix_analyses.begin();
       it != suffix_analyses.end();
       ++it)
    { suffix_tag_probability_hash[it->second] = IdWeightPair(id,it->first); }

  for (WeightedStringVector::reverse_iterator it = word_analyses.rbegin();
       it != word_analyses.rend();
       ++it)
    {
      const IdWeightPair &id_weight_pair =
	suffix_tag_probability_hash[it->second];

      // Check this.
      it->first =
	((upper_case ? upper_case_interpolator : lower_case_interpolator) * it->first +
	 (id == id_weight_pair.first ? id_weight_pair.second : 0.0)) /
	(1 + (upper_case ? upper_case_interpolator : lower_case_interpolator));
    }
}

float NewLexicalModel::get_suffix_penalty(const std::string &suffix,
					  bool upper_case)
{
  ProbabilityMap::iterator it = (upper_case ? upper_case_suffix_penalties.find(suffix) : lower_case_suffix_penalties.find(suffix));

  if (it == (upper_case ? upper_case_suffix_penalties.end() : lower_case_suffix_penalties.end()))
    {
      HfstOneLevelPaths * suffix_penalty_paths =
	lexical_model.lookup(suffix + (upper_case ? "<upper_suffix>" : "<lower_suffix>"));
      
      if (suffix_penalty_paths->empty())
	{
	  std::cerr <<  suffix << std::endl;
	  delete suffix_penalty_paths;
	  throw InvalidKey();
	}

      float penalty = suffix_penalty_paths->begin()->first;
      delete suffix_penalty_paths;
      (upper_case ? upper_case_suffix_penalties[suffix] : lower_case_suffix_penalties[suffix] )  = penalty;
      return penalty;
    }
  
  return it->second;
}

float NewLexicalModel::get_tag_penalty(const std::string &tag,bool upper_case)
{
  if (upper_case)
    {
      if (upper_case_tag_penalties.find(tag) == upper_case_tag_penalties.end())
	{
	  throw InvalidKey();
	}
    }
  else if(lower_case_tag_penalties.find(tag) == lower_case_tag_penalties.end())
    { throw InvalidKey(); }

  float penalty = (upper_case ? upper_case_tag_penalties[tag] : lower_case_tag_penalties[tag]);
  
  return penalty;
}

struct OrderWeightedStrings
{
  bool operator() (const WeightedString &w1,const WeightedString &w2) const
  {
    // w1 should be earlier if it's more probable than w2 or
    // alphabetically precedes it.
    return w1.first > w2.first or (w1.first == w2.first and w1.second < w2.second);
  }
};

typedef std::priority_queue<WeightedString,std::vector<WeightedString>,OrderWeightedStrings> WeightedStringQueue;

void preserve_n_best_analyses(WeightedStringVector &word_analyses,size_t number_of_guesses)
{
  WeightedStringQueue analysis_queue(word_analyses.begin(),word_analyses.end());

  word_analyses.clear();

  for (size_t i = 0; i < number_of_guesses; ++i)
    {
      if (analysis_queue.empty())
	{ break; }
      word_analyses.push_back(analysis_queue.top());
      analysis_queue.pop();
    }

}

void NewLexicalModel::bayesian_invert(WeightedStringVector &word_analyses,
				      const std::string &suffix,
				      bool upper_case)
{
  preserve_n_best_analyses(word_analyses,GUESSES_PRESERVED);

  float suffix_penalty = get_suffix_penalty(suffix, upper_case);
  for (WeightedStringVector::iterator it = word_analyses.begin();
       it != word_analyses.end();
       ++it)
    {
      it->first =
	suffix_penalty + get_penalty(it->first) - get_tag_penalty(it->second,upper_case);
      it->first *= 0.2;
    }
}

#else // MAIN_TEST
int main(void)
{
  /*
    Skipped.
   */
}
#endif // MAIN_TEST
