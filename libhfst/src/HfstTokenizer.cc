// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include "HfstTokenizer.h"
#include "HfstFlagDiacritics.h"
#include <string>
#include <cassert>

#ifndef MAIN_TEST

using std::string;
namespace hfst
{
  bool MultiCharSymbolTrie::is_end_of_string(const char * p) const
  { return *(p+1) == 0; }

  void MultiCharSymbolTrie::set_symbol_end(const char * p)
  { is_leaf[(unsigned char)(*p)] = true; }

  bool MultiCharSymbolTrie::is_symbol_end(const char * p) const
  { return is_leaf[(unsigned char)(*p)]; }
  
  void MultiCharSymbolTrie::init_symbol_rests(const char * p)
  {
    if (symbol_rests[(unsigned char)(*p)] == NULL)
      { symbol_rests[(unsigned char)(*p)] = new MultiCharSymbolTrie(); }
  }
  
  void MultiCharSymbolTrie::add_symbol_rest(const char * p)
  { symbol_rests[(unsigned char)(*p)]->add(p+1); }

  MultiCharSymbolTrie * MultiCharSymbolTrie::get_symbol_rest_trie
  (const char * p) const
  { return symbol_rests[(unsigned char)(*p)]; }
  
  MultiCharSymbolTrie::MultiCharSymbolTrie(void):
    symbol_rests(UCHAR_MAX, (MultiCharSymbolTrie*) NULL),
    is_leaf(UCHAR_MAX, false)
  {}

  MultiCharSymbolTrie::~MultiCharSymbolTrie(void)
  {
    for (MultiCharSymbolTrieVector::iterator it = symbol_rests.begin();
         it != symbol_rests.end();
         ++it)
      { delete *it; }
  }

  void MultiCharSymbolTrie::add(const char * p)
  {
    if (is_end_of_string(p))
      { set_symbol_end(p); }
    else
      { init_symbol_rests(p);
        add_symbol_rest(p); }
  }
  
  const char * MultiCharSymbolTrie::find(const char * p) const
  {
    MultiCharSymbolTrie * symbol_rest_trie = get_symbol_rest_trie(p);
    if (symbol_rest_trie == NULL)
      {
        if (is_symbol_end(p))
          { return p+1; }
        return NULL;
      }
    const char * symbol_end = symbol_rest_trie->find(p+1);
    if (symbol_end == NULL)
      {
        if (is_symbol_end(p))
          { return p+1; }
      }
    return symbol_end;
  }
  
  HfstTokenizer::HfstTokenizer() {}


int HfstTokenizer::get_next_symbol_size(const char * symbol)
const
{
  if (! *symbol)
    { return 0; }

  const char * multi_char_symbol_end = multi_char_symbols.find(symbol);

  /* The string begins with a multi character symbol */
  if (multi_char_symbol_end != NULL)
    { return multi_char_symbol_end - symbol; }
  if ((128 & *symbol) == 0)
    { return 1; }
  else if ((32 & *symbol) == 0)
    { return 2; }
  else if ((16 & *symbol) == 0)
    { return 3; }
  else
    {
      return 4;
    }
}

  bool HfstTokenizer::is_skip_symbol(hfst::String &s) const
{ return (s == "") || (skip_symbol_set.find(s) != skip_symbol_set.end()); }

void
HfstTokenizer::add_multichar_symbol(const std::string& symbol)
{  if (symbol == "")
    { return; }
  multi_char_symbols.add(symbol.c_str()); }

void
HfstTokenizer::add_skip_symbol(const std::string &symbol)
{ if (symbol == "")
    { return; }
  multi_char_symbols.add(symbol.c_str());
  skip_symbol_set.insert(symbol.c_str()); }

StringPairVector HfstTokenizer::tokenize
(const std::string& input_string) const
{
  check_utf8_correctness(input_string);
  StringPairVector spv;
  const char* s = input_string.c_str();
  while (*s)
    {
      int symbol_size = get_next_symbol_size(s);
      std::string symbol(s,0,symbol_size);
      s += symbol_size;
      if (is_skip_symbol(symbol))
        { continue; }
      spv.push_back(StringPair(symbol,symbol));
    }
  return spv;
}

StringVector HfstTokenizer::tokenize_one_level
(const std::string& input_string) const
{
  check_utf8_correctness(input_string);

  StringVector sv;
  const char* s = input_string.c_str();
  while (*s)
    {
      int symbol_size = get_next_symbol_size(s);
      std::string symbol(s,0,symbol_size);
      s += symbol_size;
      if (is_skip_symbol(symbol))
        { continue; }
      sv.push_back(symbol);
    }
  return sv;
}

StringPairVector HfstTokenizer::tokenize_space_separated(const std::string & str)
{
  check_utf8_correctness(str);

  StringPairVector retval;
  size_t pos = 0;
  // position where a symbol begins, not yet defined
  size_t symbol_pos = std::string::npos;

  while (pos < str.size())
    {
      // end of symbol reached
      if (str[pos] == ' ' && symbol_pos != std::string::npos)
        {
          std::string symbol(str, symbol_pos, pos - symbol_pos);
          retval.push_back(StringPair(symbol, symbol));
          symbol_pos = std::string::npos; // next symbol not yet found
        }
      // next symbol found
      else if (str[pos] != ' ' && symbol_pos == std::string::npos)
        {
          symbol_pos = pos;
        }
      else
        {}
      ++pos;
    }
  
  // last symbol
  if (symbol_pos != std::string::npos)
    {
      std::string symbol(str, symbol_pos, std::string::npos);
      retval.push_back(StringPair(symbol, symbol));
    }

  return retval;
}

StringPairVector HfstTokenizer::tokenize
(const std::string& input_string,const std::string& output_string) const
{
  check_utf8_correctness(input_string);
  check_utf8_correctness(output_string);

  StringPairVector spv;
  
  StringPairVector input_spv = tokenize(input_string.c_str());
  StringPairVector output_spv = tokenize(output_string.c_str());

  if (input_spv.size() < output_spv.size())
    {
      StringPairVector::iterator jt = output_spv.begin();
      for (StringPairVector::iterator it = input_spv.begin();
           it != input_spv.end();
           ++it)
        { spv.push_back(StringPair(it->first,
                                    jt->first));
          ++jt; }
      for ( ; jt != output_spv.end(); ++jt)
        { spv.push_back(StringPair(internal_epsilon,jt->first)); }
    }
  else
    {
      StringPairVector::iterator it = input_spv.begin();
      for (StringPairVector::iterator jt = output_spv.begin();
           jt != output_spv.end();
           ++jt)
        { spv.push_back(StringPair(it->first,
                                   jt->first));
          ++it; }
      for ( ; it != input_spv.end(); ++it)
        { spv.push_back(StringPair(it->first,internal_epsilon)); }
    }
  return spv;
}

StringPairVector HfstTokenizer::tokenize
(const std::string& input_string,const std::string& output_string,
 void (*warn_about_pair)(const std::pair<std::string, std::string> &symbol_pair)) const
{
  check_utf8_correctness(input_string);
  check_utf8_correctness(output_string);

  StringPairVector spv;
  
  StringPairVector input_spv = tokenize(input_string.c_str());
  StringPairVector output_spv = tokenize(output_string.c_str());

  if (input_spv.size() < output_spv.size())
    {
      StringPairVector::iterator jt = output_spv.begin();
      for (StringPairVector::iterator it = input_spv.begin();
           it != input_spv.end();
           ++it)
        {
          StringPair sp(it->first, jt->first);
          warn_about_pair(sp);
          spv.push_back(sp);
          ++jt; }
      for ( ; jt != output_spv.end(); ++jt)
        { StringPair sp(internal_epsilon,jt->first);
          warn_about_pair(sp);
          spv.push_back(sp); }
    }
  else
    {
      StringPairVector::iterator it = input_spv.begin();
      for (StringPairVector::iterator jt = output_spv.begin();
           jt != output_spv.end();
           ++jt)
        { StringPair sp(it->first, jt->first);
          warn_about_pair(sp);
          spv.push_back(sp);
          ++it; }
      for ( ; it != input_spv.end(); ++it)
        { StringPair sp(it->first,internal_epsilon);
          warn_about_pair(sp);
          spv.push_back(sp); }
    }
  return spv;
}

StringPairVector HfstTokenizer::tokenize_and_align_flag_diacritics
(const std::string& input_string,const std::string& output_string,
 void (*warn_about_pair)(const std::pair<std::string, std::string> &symbol_pair)) const
{
  check_utf8_correctness(input_string);
  check_utf8_correctness(output_string);

  StringPairVector spv;
  
  StringPairVector input_spv = tokenize(input_string.c_str());
  StringPairVector output_spv = tokenize(output_string.c_str());
  
  assert(input_spv.size() > 0 && output_spv.size() > 0);
  StringPairVector::const_iterator it = input_spv.begin();
  StringPairVector::const_iterator jt = output_spv.begin();

  // proceed until both token vectors are exhausted
  while(it != input_spv.end() || jt != output_spv.end())
    {
      StringPair sp("", "");  // string pair to push back to the result
      StringPair sp_cont("", "");  // possible continuation in case of missaligned flags

      if (it == input_spv.end())
        {
          if (FdOperation::is_diacritic(jt->first)) // copy diacritic to other side
            {
              sp = StringPair(jt->first, jt->first);
            }
          else // pad input with epsilons
            {
              sp = StringPair(internal_epsilon, jt->first);
            }
          jt++;
        }
      else if (jt == output_spv.end())
        {
          if (FdOperation::is_diacritic(it->first)) // copy diacritic to other side
            {
              sp = StringPair(it->first, it->first);
            }
          else // pad output with epsilons
            {
              sp = StringPair(it->first, internal_epsilon);
            }
          it++;
        }
      else
        {
          // take from both vectors (cases foo:bar, foo:foo, flag1:flag1)
          if ((!FdOperation::is_diacritic(it->first) && !FdOperation::is_diacritic(jt->first)) ||
              *it == *jt)
            {
              sp = StringPair(it->first, jt->first);
            }
          // take first from first vector and then from second
          // (cases flag1:flag2, flag1::bar, foo:flag2)
          else
            {
              StringPair wrong_pair(it->first, jt->first);
              warn_about_pair(wrong_pair);
              sp = StringPair(it->first, it->first);
              sp_cont = StringPair(jt->first, jt->first);
            }
          it++;
          jt++;
        }
      
      spv.push_back(sp);
      if (sp_cont.first.size() != 0 && sp_cont.second.size() != 0)
        {
          spv.push_back(sp_cont);
        }
    }

  return spv;
}


void 
HfstTokenizer::check_utf8_correctness(const std::string &input_string)
{
  (void)check_utf8_correctness_and_calculate_length(input_string);
}
  
  unsigned int
  HfstTokenizer::check_utf8_correctness_and_calculate_length(const std::string &input_string)
{
  unsigned int retval = 0;
  // Check that input_string is made up from utf-8 sequences.
  for (std::string::const_iterator it = input_string.begin();
       it != input_string.end();
       ++it)
    {
      // The initial byte of the sequence.
      unsigned char initial_char = *it;
      size_t additional_chars = 0;
  
      // The bytes 192, 193, 245, 246 and 247 are invalid in utf8.
      if (initial_char == 192 || initial_char == 193 ||
      initial_char == 245 || initial_char == 246 || initial_char == 247)
    { HFST_THROW_MESSAGE(IncorrectUtf8CodingException,
                         "leading octet in [192, 193, 245, 246, 247]"); }
      // Case 0xxxxxxx, i.e. ASCII byte.
      else if ((128 & initial_char) == 0)
    { additional_chars = 0; }
      // Case 10xxxxxx cannot be an initial byte.
      else if ((64 & initial_char) == 0)
    { HFST_THROW_MESSAGE(IncorrectUtf8CodingException,
                         "leading octet & 10000000b"); }
      // Case 110xxxxx, i.e. read one more byte.
      else if ((32 & initial_char) == 0)
    { additional_chars = 1; }
      // Case 1110xxxx, i.e. read two more bytes.
      else if ((16 & initial_char) == 0)
    { additional_chars = 2; }
      // Case 11110xxx, i.e. read three more bytes.
      else if ((8 & initial_char) == 0)
    { additional_chars = 3; }
      // Case 11111xxx is not allowed in utf8.
      else
    { HFST_THROW_MESSAGE(IncorrectUtf8CodingException,
                         "leading octet & 11111000b"); }

      // Read the continuation bytes.
      for (size_t i = 0; i < additional_chars; ++i)
    {
      ++it;
      // String ends too early.
      if (it == input_string.end())
        { HFST_THROW_MESSAGE(IncorrectUtf8CodingException,
                             "eos in multioctet sequence"); }
      unsigned char byte = *it;
      // All continuation bytes look like 10xxxxxx.
      if (! (128 & byte && 64 ^ byte))
        { HFST_THROW_MESSAGE(IncorrectUtf8CodingException,
                             "not continuation octet & 100000000b"); }
    }
      retval = retval + 1;
    }
  return retval;
}
  
}

#else // MAIN_TEST was defined

#include <iostream>

int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":" << std::endl;
    
    std::cout << "ok" << std::endl;
    return 0;
}

#endif // MAIN_TEST
