//! @file Rule.cc
//!
//! @author Miikka Silfverberg

//  This library is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, version 3 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "Rule.h"

Rule::Rule(const std::string &name,
       const OtherSymbolTransducer &center,
       const OtherSymbolTransducerVector &contexts):
  is_empty(false),
  name(unescape_name(name)),
  center(center)
{
  OtherSymbolTransducerVector contexts_copy = contexts;
  for (auto & it : contexts_copy)
    { context.apply(&HfstTransducer::disjunct,it); }
  this->center.harmonize_diacritics(context);
}

Rule::Rule(const std::string &name,
       const RuleVector &v):
  is_empty(true),
  name(unescape_name(name)),
  rule_transducer(TWOLC_UNKNOWN)
{
  rule_transducer.apply(&HfstTransducer::repeat_star);
  for (auto it : v)
    {
      if (! it->empty())
    {
      rule_transducer.apply
        (&HfstTransducer::intersect,it->rule_transducer);
      is_empty = false;
    }
    }
}

Rule::~Rule(void)
{}

std::string Rule::get_print_name(const std::string &s)
{
  std::string ss(s);
  while (ss.find("__HFST_TWOLC_SPACE") != std::string::npos)
    { ss.replace(ss.find("__HFST_TWOLC_SPACE"),
         std::string("__HFST_TWOLC_SPACE").size()," "); }

  while (ss.find("__HFST_TWOLC_RULE_NAME=") != std::string::npos)
    { ss.replace(ss.find("__HFST_TWOLC_RULE_NAME="),
         std::string("__HFST_TWOLC_RULE_NAME=").size()," "); }

  while (ss.find("__HFST_TWOLC_SET_NAME=") != std::string::npos)
    { ss.replace(ss.find("__HFST_TWOLC_SET_NAME="),
         std::string("__HFST_TWOLC_SET_NAME=").size(),""); }

  while (ss.find("__HFST_TWOLC_") != std::string::npos)
    { ss.replace(ss.find("__HFST_TWOLC_"),
         std::string("__HFST_TWOLC_").size(),""); }

  return ss;
}

bool Rule::empty(void) const
{ return is_empty; }

OtherSymbolTransducer Rule::compile(void)
{ return OtherSymbolTransducer(); }

void Rule::store(HfstOutputStream &out)
{
  if (is_empty)
    { return; }
  add_name();
  rule_transducer.remove_diacritics_from_output();
  rule_transducer.apply(&HfstTransducer::substitute,TWOLC_EPSILON,HFST_EPSILON,
            true,true);
  rule_transducer.apply(&HfstTransducer::substitute,"__HFST_TWOLC_.#.","@#@",
            true,true);
  rule_transducer.apply(&HfstTransducer::substitute,"__HFST_TWOLC_SPACE",
            " ",
            true,true);
  rule_transducer.apply(&HfstTransducer::substitute,SymbolPair("@#@","@#@"),
            SymbolPair("@#@",HFST_EPSILON));
  rule_transducer.apply(&HfstTransducer::substitute,TWOLC_IDENTITY,
            HFST_IDENTITY,
            true,true);
  HfstTransducer &t = rule_transducer.transducer;
  out << t;
}

std::string Rule::get_name(void)
{ return name; }

void Rule::add_name(void)
{ rule_transducer.add_info_symbol(name); }

OtherSymbolTransducer Rule::get_universal_language_with_diamonds(void)
{
  OtherSymbolTransducer universal(TWOLC_UNKNOWN);
  universal.apply(&HfstTransducer::repeat_star);
  OtherSymbolTransducer diamond(TWOLC_DIAMOND);
  OtherSymbolTransducer universal_with_diamonds(universal);
  universal_with_diamonds.
    apply(&HfstTransducer::concatenate,diamond).
    apply(&HfstTransducer::concatenate,universal).
    apply(&HfstTransducer::concatenate,diamond).
    apply(&HfstTransducer::concatenate,universal);
  return universal_with_diamonds;
}

OtherSymbolTransducer Rule::get_center(const std::string &input,
                       const std::string &output)
{
  OtherSymbolTransducer unknown(TWOLC_UNKNOWN);
  unknown.apply(&HfstTransducer::repeat_star);
  OtherSymbolTransducer diamond(TWOLC_DIAMOND);
  OtherSymbolTransducer center(unknown);
  OtherSymbolTransducer center_pair(input,output);
  center.
    apply(&HfstTransducer::concatenate,diamond).
    apply(&HfstTransducer::concatenate,center_pair).
    apply(&HfstTransducer::concatenate,diamond).
    apply(&HfstTransducer::concatenate,unknown);
  return center;
}

OtherSymbolTransducer Rule::get_center(const SymbolPairVector &v)
{
  OtherSymbolTransducer unknown(TWOLC_UNKNOWN);
  unknown.apply(&HfstTransducer::repeat_star);
  OtherSymbolTransducer diamond(TWOLC_DIAMOND);
  OtherSymbolTransducer center_pair_transducer;
  for (const auto & it : v)
    {
      OtherSymbolTransducer pair(it.first,it.second);
      center_pair_transducer.
    apply(&HfstTransducer::disjunct,pair);
    }
  OtherSymbolTransducer center(unknown);
   center.
     apply(&HfstTransducer::concatenate,diamond).
     apply(&HfstTransducer::concatenate,center_pair_transducer).
     apply(&HfstTransducer::concatenate,diamond).
     apply(&HfstTransducer::concatenate,unknown);
  return center;
}

OtherSymbolTransducer Rule::get_center
(const OtherSymbolTransducer &restricted_center)
{
  OtherSymbolTransducer unknown(TWOLC_UNKNOWN);
  unknown.apply(&HfstTransducer::repeat_star);
  OtherSymbolTransducer diamond(TWOLC_DIAMOND);
  OtherSymbolTransducer center(unknown);
   center.
     apply(&HfstTransducer::concatenate,diamond).
     apply(&HfstTransducer::concatenate,restricted_center).
     apply(&HfstTransducer::concatenate,diamond).
     apply(&HfstTransducer::concatenate,unknown);
   return center;
}

void Rule::add_missing_symbols_freely(const SymbolRange &diacritics)
{
  std::set<std::string> symbol_set =
    HfstBasicTransducer(rule_transducer.get_transducer()).get_alphabet();
  for (const auto & diacritic : diacritics)
    {
      if (symbol_set.find(diacritic) == symbol_set.end())
    {
      rule_transducer.add_symbol_to_alphabet(diacritic);
      rule_transducer.apply(&HfstTransducer::insert_freely,
                SymbolPair(diacritic,diacritic), true); }
    }
  
}

#ifdef TEST_RULE
int main(void)
{ /* TEST */ }
#endif
