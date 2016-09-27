// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include "HarmonizeUnknownAndIdentitySymbols.h"
#include "HfstTransducer.h"
#include "implementations/HfstBasicTransducer.h"
#include "HfstFlagDiacritics.h"
#include "implementations/optimized-lookup/pmatch.h"

#ifndef MAIN_TEST

namespace hfst
{

const char * HarmonizeUnknownAndIdentitySymbols::identity =
  "@_IDENTITY_SYMBOL_@"; // --- internal_identity.c_str();
const char * HarmonizeUnknownAndIdentitySymbols::unknown =
  "@_UNKNOWN_SYMBOL_@";  // --- internal_unknown.c_str();

  // --- these functions could be useful elsewhere, too
  //     maybe they should be in separate h- and cc-files?
size_t max_(size_t t1,size_t t2)
{ return t1 < t2 ? t2 : t1; }

static bool is_subset(const StringSet &subset,const StringSet &superset)
{
  for (StringSet::const_iterator it = subset.begin();
       it != subset.end();
       ++it)
    {
      if (superset.find(*it) == superset.end())
    { return false; }
    }
  return true;
}

static StringSet remove_flags(const StringSet & alpha)
{
  StringSet retval;
  for (StringSet::const_iterator it = alpha.begin();
      it != alpha.end(); it++)
    {
        if (!FdOperation::is_diacritic(*it) && !hfst_ol::PmatchAlphabet::is_special(*it))
        {
          retval.insert(*it);
        }
    }
  return retval;
}

HarmonizeUnknownAndIdentitySymbols::HarmonizeUnknownAndIdentitySymbols
(HfstBasicTransducer &t1,HfstBasicTransducer &t2) :
  t1(t1),
  t2(t2)
{
  t1_symbol_set = remove_flags(t1.get_alphabet());
  t2_symbol_set = remove_flags(t2.get_alphabet());
 
  if (debug_harmonize) // --- good
    {
      StringSet t1_symbols_in_transitions;
      StringSet t2_symbols_in_transitions;
      populate_symbol_set(t1,t1_symbols_in_transitions);
      populate_symbol_set(t2,t2_symbols_in_transitions);
      assert(is_subset(t1_symbols_in_transitions,t1_symbol_set));
      assert(is_subset(t2_symbols_in_transitions,t2_symbol_set));
    }

  // --- typedef StringVector (not in HfstSymbolDefs.h, maybe it should be?)
  std::vector<std::string> diff_vector
    (max_(t1_symbol_set.size(),t2_symbol_set.size()),"");
  
  if (debug_harmonize)
    { debug_harmonize_print("Computing t1 symbols - t2 symbols."); }

  // --- documentation would be useful
  std::vector<std::string>::iterator diff_end =
    std::set_difference(t1_symbol_set.begin(),t1_symbol_set.end(),
            t2_symbol_set.begin(),t2_symbol_set.end(),
            diff_vector.begin());

  StringSet t1_symbols_minus_t2_symbols(diff_vector.begin(),diff_end);

  t1_symbols_minus_t2_symbols.erase(identity);
  t1_symbols_minus_t2_symbols.erase(unknown);

  if (debug_harmonize)
    {
      debug_harmonize_print("Symbols:");
      debug_harmonize_print(t1_symbols_minus_t2_symbols);
    }

  if (debug_harmonize)
    { debug_harmonize_print("Computing t2 symbols - t1 symbols."); }

  diff_end = std::set_difference(t2_symbol_set.begin(),t2_symbol_set.end(),
                 t1_symbol_set.begin(),t1_symbol_set.end(),
                 diff_vector.begin());

  StringSet t2_symbols_minus_t1_symbols(diff_vector.begin(),diff_end);

  t1_symbols_minus_t2_symbols.erase(unknown);
  t1_symbols_minus_t2_symbols.erase(identity);

  if (debug_harmonize)
    {
      debug_harmonize_print("Symbols:");
      debug_harmonize_print(t2_symbols_minus_t1_symbols);
    }
  
  if (debug_harmonize)
    { debug_harmonize_print("Harmonizing identity symbols."); }

  harmonize_identity_symbols(t1,t2_symbols_minus_t1_symbols);
  harmonize_identity_symbols(t2,t1_symbols_minus_t2_symbols);

  if (debug_harmonize)
    { debug_harmonize_print("Harmonizing unknown symbols."); }

  harmonize_unknown_symbols(t1,t2_symbols_minus_t1_symbols);
  harmonize_unknown_symbols(t2,t1_symbols_minus_t2_symbols);

  // Add new symbols to the alphabets of the transducers.
  // We used to only add the difference of the alphabets,
  // but that way we miss the things removed by remove_flags()
  // which we do want. Since we'd be iterating over them again
  // to collect them, might as well just add everything.
  add_symbols_to_alphabet(t1,t2.get_alphabet());
  add_symbols_to_alphabet(t2,t1.get_alphabet());


  if (debug_harmonize)
    {
      if (t2_symbols_minus_t1_symbols.empty())
    { debug_harmonize_print("t1 includes no symbols not found in t2."); }
      else
    {
      HfstTransducer fst1(t1,TROPICAL_OPENFST_TYPE);
      std::cerr << fst1 << std::endl;
    }

      if (t1_symbols_minus_t2_symbols.empty())
    { debug_harmonize_print("t2 includes no symbols not found in t1."); }
      else
    {
      HfstTransducer fst2(t2,TROPICAL_OPENFST_TYPE);
      std::cerr << fst2 << std::endl;
    }
    }
}

void HarmonizeUnknownAndIdentitySymbols::populate_symbol_set
(const HfstBasicTransducer &t,StringSet &s)
{
  for (HfstBasicTransducer::const_iterator it = t.begin(); it != t.end(); ++it)
    {
      for (HfstBasicTransducer::HfstTransitions::const_iterator jt =
         it->begin();
       jt != it->end();
       ++jt)
    {
      s.insert(jt->get_input_symbol());
      s.insert(jt->get_output_symbol());
    }
    }
  if (debug_harmonize)
    {
      debug_harmonize_print("Symbols:");
      debug_harmonize_print(s);
    }
}

void HarmonizeUnknownAndIdentitySymbols::add_symbols_to_alphabet
(HfstBasicTransducer &t, const StringSet &s)
{
  for (StringSet::const_iterator it = s.begin(); it != s.end(); it++)
    {
      t.add_symbol_to_alphabet(*it);
    }
}

// --- documentation would make this function easier to follow
void HarmonizeUnknownAndIdentitySymbols::harmonize_identity_symbols
(HfstBasicTransducer &t,const StringSet &missing_symbols)
{
  if (missing_symbols.empty())
    { return; }

  for (HfstBasicTransducer::iterator it = t.begin(); it != t.end(); ++it)
    {

      HfstBasicTransducer::HfstTransitions added_transitions;

      for (HfstBasicTransducer::HfstTransitions::const_iterator jt =
         it->begin();
       jt != it->end();
       ++jt)
    {
      if (jt->get_input_symbol() == identity)
        {
          // --- an exception instead, this could also be checked
          //     at an earlier stage
          assert(jt->get_output_symbol() == identity);
          for (StringSet::const_iterator kt = missing_symbols.begin();
           kt != missing_symbols.end();
           ++kt)
        { added_transitions.push_back
            (HfstBasicTransition(jt->get_target_state(),
                     *kt,*kt,
                     jt->get_weight())); }
        }
    }
      it->insert(it->end(),
         added_transitions.begin(),added_transitions.end());
    }
}

// --- documentation would make this function easier to follow
void HarmonizeUnknownAndIdentitySymbols::harmonize_unknown_symbols
(HfstBasicTransducer &t,const StringSet &missing_symbols)
{
  if (missing_symbols.empty())
    { return; }

  for (HfstBasicTransducer::iterator it = t.begin(); it != t.end(); ++it)
    {
      HfstBasicTransducer::HfstTransitions added_transitions;

      for (HfstBasicTransducer::HfstTransitions::const_iterator jt =
         it->begin();
       jt != it->end();
       ++jt)
    {
      if (jt->get_input_symbol() == unknown)
        {
          // --- an exception instead, this could also be checked
          //     at an earlier stage
          assert(jt->get_output_symbol() != identity);
        
          for (StringSet::const_iterator kt = missing_symbols.begin();
           kt != missing_symbols.end();
           ++kt)
        {
          added_transitions.push_back
            (HfstBasicTransition(jt->get_target_state(),
                     *kt,jt->get_output_symbol(),
                     jt->get_weight()));
        }
        }
      if (jt->get_output_symbol() == unknown)
        {
          // --- an exception instead, this could also be checked
          //     at an earlier stage
          assert(jt->get_input_symbol() != identity);

          for (StringSet::const_iterator kt = missing_symbols.begin();
           kt != missing_symbols.end();
           ++kt)
        { added_transitions.push_back
            (HfstBasicTransition(jt->get_target_state(),
                     jt->get_input_symbol(),*kt,
                     jt->get_weight())); }

        }
      if (jt->get_input_symbol() == unknown &&
          jt->get_output_symbol() == unknown)
        {
          for (StringSet::const_iterator kt = missing_symbols.begin();
           kt != missing_symbols.end();
           ++kt)
        {
          for (StringSet::const_iterator lt = missing_symbols.begin();
               lt != missing_symbols.end();
               ++lt)
            {
              if (kt == lt)
            { continue; }

              added_transitions.push_back
            (HfstBasicTransition(jt->get_target_state(),
                         *lt,*kt,
                         jt->get_weight()));
            }
        }
        }
    }
      it->insert(it->end(),
         added_transitions.begin(),added_transitions.end());
    }
}

// --- this could be useful elsewhere, too (print_string_set?)
void debug_harmonize_print(const StringSet &s)
{
  for (StringSet::const_iterator it = s.begin();
       it != s.end();
       ++it)
    { std::cerr << *it << std::endl; }
}

void debug_harmonize_print(const std::string &s)
{ std::cerr << s << std::endl; }

}


#else // MAIN_TEST --- good
using hfst::HfstInputStream;
using hfst::HfstTransducer;
using hfst::HfstOutputStream;
using hfst::HfstBasicTransducer;

int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":" << std::endl;

    // the following tests require stdio, not appropriate for unit testing..
    // TODO: implement whatever we want to do in functionality tests?
    /*
  HfstInputStream in;
  
  std::cerr << "Reading first transducer" << std::endl;
  HfstTransducer t1(in);

  std::cerr << "Converting first transducer to basic transducer" << std::endl;
  HfstBasicTransducer b1(t1);

  std::cerr << "Reading second transducer" << std::endl;
  HfstTransducer t2(in);

  std::cerr << "Converting second transducer to basic transducer" << std::endl;
  HfstBasicTransducer b2(t2);
  
  std::cerr << "Harmonizing" << std::endl;
  hfst::HarmonizeUnknownAndIdentitySymbols(b1,b2);

  t1 = HfstTransducer(b1,hfst::TROPICAL_OPENFST_TYPE);
  t2 = HfstTransducer(b2,hfst::TROPICAL_OPENFST_TYPE);

  HfstOutputStream out(hfst::TROPICAL_OPENFST_TYPE);

  std::cerr << "composing" << std::endl;
  out << t1.compose(t2);
    */
  std::cout << "ok" << std::endl;
  return 0;
}


#endif // MAIN_TEST
