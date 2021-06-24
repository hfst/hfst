#ifndef HEADER_NewLexicalModel_h
#define HEADER_NewLexicalModel_h

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "HfstTransducer.h"

#include <unordered_map>
#include <unordered_set>

using std::unordered_map;
using std::unordered_set;

#include <iostream>

#include "DataTypes.h"

using hfst::HfstTransducer;
using hfst::HfstInputStream;
using hfst::HfstOneLevelPaths;
using hfst::HfstTokenizer;
using hfst::StringVector;

#define GUESSES_PRESERVED 10

class NewLexicalModel
{
 public:
  NewLexicalModel(const std::string &filename,
                  std::istream * paradigm_guess_stream = 0);
  const WeightedStringVector &operator[] (const std::string &word);
  const WeightedStringVector &get_first_word_analysis(const std::string &word);
  bool is_oov(const std::string &word);
  bool is_lexicon_oov(const std::string &word);
 private:

  typedef unordered_map<std::string,WeightedStringVector>
    AnalysisCache;

  AnalysisCache   analysis_cache;
  AnalysisCache   upper_case_suffix_cache;
  AnalysisCache   lower_case_suffix_cache;
  HfstTokenizer   tokenizer;
  StringWeightMap tag_probability_hash;
  ProbabilityMap  upper_case_suffix_penalties;
  ProbabilityMap  lower_case_suffix_penalties;
  ProbabilityMap  upper_case_tag_penalties;
  ProbabilityMap  lower_case_tag_penalties;
  StringWeightMap word_probability_hash;
  StringWeightMap suffix_tag_probability_hash;
  WeightedStringVector upper_case_empty_suffix_guesses;
  WeightedStringVector upper_case_empty_suffix_null;
  WeightedStringVector lower_case_empty_suffix_guesses;
  WeightedStringVector lower_case_empty_suffix_null;
  HfstInputStream in;
  HfstTransducer  lexical_model;
  float upper_case_interpolator;
  float lower_case_interpolator;
  size_t id;
  bool lexical_model_is_broken;
  std::istream * paradigm_guess_stream;
  unordered_set<std::string> o_o_v_words;
  unordered_set<std::string> lexicon_o_o_v_words;

  void initialize_tag_probabilities(void);

  const WeightedStringVector &cache_word_analyses(const std::string &word);
  const WeightedStringVector &guess(const std::string &word,
                                    bool upper_case,
                                    const StringVector &rev_tokenized_word);
  const WeightedStringVector &cache_analyses
    (const std::string &word,
     HfstOneLevelPaths * paths,
     AnalysisCache &cache,
     bool convert_to_probabilities=false);

  const WeightedStringVector &get_suffix_analyses
    (const std::string &word,
     StringVector::const_iterator begin,
     StringVector::const_iterator end,
     bool upper_case);

  void merge_analyses(const WeightedStringVector &suffix_analyses,
                      WeightedStringVector &word_analyses,
                      bool  upper_case);
  
  void bayesian_invert(WeightedStringVector &word_analyses,
                       const std::string &suffix,
                       bool upper_case);
  size_t get_suffix_length(StringVector::const_iterator begin,
                           StringVector::const_iterator end);
  const WeightedStringVector &cache_suffix_analyses
    (const std::string &word,
     const std::string &suffix,
     StringVector::const_iterator begin,
     StringVector::const_iterator end);

  float get_suffix_penalty(const std::string &suffix, bool upper_case);
  float get_tag_penalty(const std::string &tag, bool upper_case);
  
  std::string to_string(StringVector::const_iterator begin,
                        StringVector::const_iterator end);
  float get_prob(float weight);
  float get_penalty(float prob);
  std::string to_lower_case(const std::string &word) const;

  const WeightedStringVector &paradigm_guess(const std::string &word,
                                             const StringVector &guesses);
  StringVector split(const std::string &guesses);
  float get_weight(const std::string &weight_string);
};


#endif // HEADER_NewLexicalModel_h
