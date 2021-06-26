//! @file SymbolTransducer.cc
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

#include "OtherSymbolTransducer.h"

#ifndef TEST_OTHER_SYMBOL_TRANSDUCER

HandySet<std::string> OtherSymbolTransducer::input_symbols;
HandySet<std::string> OtherSymbolTransducer::output_symbols;
HandySet<std::string> OtherSymbolTransducer::diacritics;
HandySet<SymbolPair> OtherSymbolTransducer::symbol_pairs;
ImplementationType OtherSymbolTransducer::transducer_type;

void OtherSymbolTransducer::set_symbol_pairs
(const HandySet<SymbolPair> &symbol_pairs)
{
  OtherSymbolTransducer::input_symbols.clear();
  OtherSymbolTransducer::output_symbols.clear();
  OtherSymbolTransducer::symbol_pairs.clear();
  OtherSymbolTransducer::symbol_pairs.insert(symbol_pairs.begin(),
                         symbol_pairs.end());
  for (const auto & symbol_pair : symbol_pairs)
    {
      OtherSymbolTransducer::input_symbols.insert(symbol_pair.first);
      OtherSymbolTransducer::output_symbols.insert(symbol_pair.second);
    }
  OtherSymbolTransducer::symbol_pairs.insert
    (SymbolPair(TWOLC_DIAMOND,TWOLC_DIAMOND));
}

void OtherSymbolTransducer::define_diacritics
(const std::vector<std::string> &diacritics)
{
  OtherSymbolTransducer::diacritics.clear();
  OtherSymbolTransducer::diacritics.insert
    (diacritics.begin(),diacritics.end());
  for (const auto & diacritic : OtherSymbolTransducer::diacritics)
    {
      symbol_pairs.erase(SymbolPair(diacritic,diacritic));
      symbol_pairs.erase(SymbolPair(diacritic,TWOLC_EPSILON));
      input_symbols.erase(diacritic);
      output_symbols.erase(diacritic);
    }
}

OtherSymbolTransducer OtherSymbolTransducer::get_universal(void)
{
  OtherSymbolTransducer universal;
  HfstBasicTransducer fst(universal.transducer);
  HfstState target = fst.add_state();
  fst.set_final_weight(target,0.0);
  fst.add_transition(0,HfstBasicTransition
             (target,TWOLC_IDENTITY,TWOLC_IDENTITY,0.0));
  for (const auto & symbol_pair : symbol_pairs)
    {
      if (symbol_pair.first == TWOLC_DIAMOND)
    { continue; }
      fst.add_transition(0,HfstBasicTransition
             (target,symbol_pair.first,symbol_pair.second,0.0));
    }
  universal.transducer = HfstTransducer(fst,transducer_type);
  return universal;
}

void OtherSymbolTransducer::set_transducer_type
(ImplementationType transducer_type)
{ OtherSymbolTransducer::transducer_type = transducer_type; }

OtherSymbolTransducer::OtherSymbolTransducer(void):
  is_broken(false),
  transducer(transducer_type)
{ }

OtherSymbolTransducer::OtherSymbolTransducer
(const std::string &i_symbol,const std::string &o_symbol):
  is_broken(false),
  transducer(transducer_type)
{
  std::string input_symbol = i_symbol;
  std::string output_symbol = o_symbol;

  input_symbol = (input_symbol == TWOLC_UNKNOWN ? HFST_UNKNOWN : input_symbol);
  output_symbol = (output_symbol == TWOLC_UNKNOWN ? HFST_UNKNOWN : output_symbol);

  check_pair(input_symbol,output_symbol);
  if (is_broken)
    { return; }
  if (input_symbol == HFST_UNKNOWN && output_symbol == HFST_UNKNOWN)
    { transducer = get_universal().transducer; }
  else
    {
      HfstBasicTransducer fst(transducer);
      HfstState target = fst.add_state();
      fst.set_final_weight(target,0.0);
      
      if (input_symbol == HFST_UNKNOWN)
    {
      for (const auto & input_symbol : input_symbols)
        {
          if (symbol_pairs.has_element(SymbolPair(input_symbol,output_symbol)))
        { fst.add_transition
            (0,HfstBasicTransition(target,input_symbol,output_symbol,0.0)); }
        }
    }
      else if (output_symbol == HFST_UNKNOWN)
    {
      for (const auto & output_symbol : output_symbols)
        {
          if (symbol_pairs.has_element(SymbolPair(input_symbol,output_symbol)))
        { fst.add_transition
            (0,HfstBasicTransition(target,input_symbol,output_symbol,0.0)); }
        }
    }
      else
    { fst.add_transition
        (0,HfstBasicTransition(target,input_symbol,output_symbol,0.0)); }
      transducer = HfstTransducer(fst,transducer_type);
    }
}

OtherSymbolTransducer::OtherSymbolTransducer(const std::string &sym):
  is_broken(false),
  transducer(transducer_type)
{
  std::string symbol = sym;

  symbol = (symbol == TWOLC_UNKNOWN ? HFST_UNKNOWN : symbol);

  if (diacritics.has_element(symbol))
    { check_pair(symbol,TWOLC_EPSILON); }
  else
    { check_pair(symbol,symbol); }

  if (is_broken)
    { return; }

  if (symbol == HFST_UNKNOWN)
    { transducer = get_universal().transducer; }
  else if (diacritics.has_element(symbol))
    { transducer = HfstTransducer(symbol,TWOLC_EPSILON,transducer_type); }
  else
    { transducer = HfstTransducer(symbol,transducer_type); }

}

OtherSymbolTransducer::OtherSymbolTransducer
(const OtherSymbolTransducer &another):
  is_broken(another.is_broken),
  transducer(another.transducer)
{ /*add_diamond_transition();*/ }

OtherSymbolTransducer &OtherSymbolTransducer::harmonize_diacritics
(OtherSymbolTransducer &t)
{
  HfstBasicTransducer basic(transducer);
  std::set<std::string> alphabet =
    basic.get_alphabet();

  HfstBasicTransducer basic_t(t.transducer);
  std::set<std::string> t_alphabet =
    basic_t.get_alphabet();

  HandySet<std::string> missing_diacritics;

  for (const auto & diacritic : diacritics)
    {
      if (t_alphabet.find(diacritic) != t_alphabet.end() &&
      alphabet.find(diacritic) == alphabet.end())
    { missing_diacritics.insert(diacritic); }
    }
  if (missing_diacritics.empty())
    { return *this; }

  HfstState s = 0;
  for (HfstBasicTransducer::const_iterator it = basic.begin();
       it != basic.end();
       ++it)
    {
      for (const auto & jt : *it)
       {
      if (jt.get_input_symbol() == TWOLC_IDENTITY)
        {
          HfstState target = jt.get_target_state();
          
          for (const auto & missing_diacritic : missing_diacritics)
        {
          basic.add_transition(s,
                       HfstBasicTransition
                       (target,missing_diacritic,missing_diacritic,0.0));
        }
          break;
        }
    }
     ++s;
    }
  transducer = HfstTransducer(basic,transducer_type);
  return *this;
}

OtherSymbolTransducer OtherSymbolTransducer::get_context
(OtherSymbolTransducer &left,OtherSymbolTransducer &right)
{
  OtherSymbolTransducer universal = get_universal();
  universal.apply(&HfstTransducer::repeat_star);
  OtherSymbolTransducer result(universal);
  OtherSymbolTransducer diamond(TWOLC_DIAMOND);
  universal.apply(&HfstTransducer::repeat_star);

  return result.
    apply(&HfstTransducer::concatenate,left).
    apply(&HfstTransducer::concatenate,diamond).
    apply(&HfstTransducer::concatenate,universal).
    apply(&HfstTransducer::concatenate,diamond).
    apply(&HfstTransducer::concatenate,right).
    apply(&HfstTransducer::concatenate,universal);
}

void OtherSymbolTransducer::add_diamond_transition(void)
{ add_symbol_to_alphabet(TWOLC_DIAMOND); }


void OtherSymbolTransducer::add_symbol_to_alphabet(const std::string &symbol)
{
  HfstBasicTransducer mutable_transducer(transducer);
  mutable_transducer.add_symbol_to_alphabet(symbol);
  transducer = HfstTransducer(mutable_transducer,transducer_type);
}

void OtherSymbolTransducer::remove_diacritics_from_output(void)
{
  for (const auto & diacritic : diacritics)
    {
      apply(&HfstTransducer::substitute,SymbolPair(diacritic,diacritic),
        SymbolPair(diacritic,TWOLC_EPSILON));
    }
}

void OtherSymbolTransducer::check_pair(const std::string &input_symbol,
                       const std::string &output_symbol)
{
  // id:id is a valid pair.
  if (input_symbol == TWOLC_IDENTITY)
    { is_broken = false; }
  // other:other is a valid pair.
  else if (input_symbol == HFST_UNKNOWN && output_symbol == HFST_UNKNOWN)
    { is_broken = false; }
  // eps:eps is valid
  else if (input_symbol == TWOLC_EPSILON && output_symbol == TWOLC_EPSILON)
    { is_broken = false; }
  // 0:0 is a valid pair.
  else if (input_symbol == HFST_EPSILON && output_symbol == HFST_EPSILON)
    { is_broken = false; }
  // diamond:diamond is a valid pair.
  else if (input_symbol == TWOLC_DIAMOND)
    { is_broken = false; }
  // other:X is valid, iff X is an output symbol or 0.
  else if (input_symbol == HFST_UNKNOWN)
    { is_broken = ! (output_symbol == TWOLC_EPSILON ||
               output_symbols.has_element(output_symbol)); }
  // X:other is valid, iff X is an input symbol or 0.
  else if (output_symbol == HFST_UNKNOWN)
    { is_broken = ! (input_symbol == TWOLC_EPSILON ||
               input_symbols.has_element(input_symbol)); }
  // 0:X is valid, iff X is an output symbol.
  else if (input_symbol == TWOLC_EPSILON)
    { is_broken = ! output_symbols.has_element(output_symbol); }
  // X:0 is valid, iff X is an input symbol or a diacritic.
  else if (output_symbol == TWOLC_EPSILON)
    { is_broken = ! input_symbols.has_element(input_symbol); }
  // X:X is valid if X is a diacritic.
  else if (diacritics.has_element(input_symbol) /*and
                          input_symbol == output_symbol*/)
    { is_broken = false; }
  // X:Y is valid iff it has been declared in the alphabet.
  else
    { is_broken =
    ! symbol_pairs.has_element(SymbolPair(input_symbol,output_symbol)); }
  if (is_broken)
    { std::cerr << "Unknown pair: "
        << input_symbol << " " << output_symbol << std::endl; }
}

OtherSymbolTransducer &OtherSymbolTransducer::operator=
(const OtherSymbolTransducer &another)
{
  if (&another == this)
    { return *this; }
  is_broken = another.is_broken;
  transducer = another.transducer;
  return *this;
}

OtherSymbolTransducer &OtherSymbolTransducer::apply
(HfstTransducerZeroArgMember p)
{
  if (symbol_pairs.empty())
    { throw EmptySymbolPairSet(); }
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }
  transducer = CALL_MEMBER_FN(transducer,p)();
  transducer = CALL_MEMBER_FN(transducer,&HfstTransducer::minimize)();
  return *this;
}

OtherSymbolTransducer &OtherSymbolTransducer::apply
(const HfstTransducerOneArgMember p,const OtherSymbolTransducer &another)
{
  if (symbol_pairs.empty())
    { throw EmptySymbolPairSet(); }
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }
  if (another.is_broken)
    { throw UndefinedSymbolPairsFound(); }
  OtherSymbolTransducer another_copy(another);
  if (! diacritics.empty())
    {
      harmonize_diacritics(another_copy);
      another_copy.harmonize_diacritics(*this);
    }
  transducer = CALL_MEMBER_FN(transducer,p)(another_copy.transducer);
  transducer = CALL_MEMBER_FN(transducer,&HfstTransducer::minimize)();
  return *this;
}

OtherSymbolTransducer &OtherSymbolTransducer::apply
(const HfstTransducerBoolArgMember p,const OtherSymbolTransducer &another)
{
  if (symbol_pairs.empty())
    { throw EmptySymbolPairSet(); }
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }
  if (another.is_broken)
    { throw UndefinedSymbolPairsFound(); }
  OtherSymbolTransducer another_copy(another);
  if (! diacritics.empty())
    {
      harmonize_diacritics(another_copy);
      another_copy.harmonize_diacritics(*this);
    }
  transducer = CALL_MEMBER_FN(transducer,p)(another_copy.transducer, true);
  transducer = CALL_MEMBER_FN(transducer,&HfstTransducer::minimize)();
  return *this;
}

bool OtherSymbolTransducer::apply
(const HfstTransducerOneArgMemberBool p,const OtherSymbolTransducer &another)
const
{
  if (symbol_pairs.empty())
    { throw EmptySymbolPairSet(); }
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }
  if (another.is_broken)
    { throw UndefinedSymbolPairsFound(); }
  OtherSymbolTransducer copy(*this);
  OtherSymbolTransducer another_copy(another);
  return CALL_MEMBER_FN(copy.transducer,p)(another_copy.transducer);
}

OtherSymbolTransducer &OtherSymbolTransducer::apply
(const HfstTransducerOneNumArgMember p,unsigned int num)
{
  if (symbol_pairs.empty())
    { throw EmptySymbolPairSet(); }
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }
  transducer = CALL_MEMBER_FN(transducer,p)(num);
  transducer = CALL_MEMBER_FN(transducer,&HfstTransducer::minimize)();
  return *this;
}

OtherSymbolTransducer &OtherSymbolTransducer::apply
(const HfstTransducerTwoNumArgMember p,unsigned int num1,unsigned int num2)
{
  if (symbol_pairs.empty())
    { throw EmptySymbolPairSet(); }
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }
  transducer = CALL_MEMBER_FN(transducer,p)(num1,num2);
  transducer = CALL_MEMBER_FN(transducer,&HfstTransducer::minimize)();
  return *this;
}

OtherSymbolTransducer &OtherSymbolTransducer::apply
(const HfstTransducerOneSymbolPairArgMember p,const SymbolPair &pair)
{
  if (symbol_pairs.empty())
    { throw EmptySymbolPairSet(); }
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }
  transducer = CALL_MEMBER_FN(transducer,p)(pair);
  transducer = CALL_MEMBER_FN(transducer,&HfstTransducer::minimize)();
  return *this;
}

OtherSymbolTransducer &OtherSymbolTransducer::apply
(const HfstTransducerOneSymbolPairBoolArgMember p,const SymbolPair &pair, bool b)
{
  if (symbol_pairs.empty())
    { throw EmptySymbolPairSet(); }
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }
  transducer = CALL_MEMBER_FN(transducer,p)(pair,b);
  transducer = CALL_MEMBER_FN(transducer,&HfstTransducer::minimize)();
  return *this;
}

OtherSymbolTransducer &OtherSymbolTransducer::apply
(const HfstTransducerSubstMember p,const std::string &str1,
 const std::string &str2, bool b1, bool b2)
{
  if (symbol_pairs.empty())
    { throw EmptySymbolPairSet(); }
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }
  transducer = CALL_MEMBER_FN(transducer,p)(str1,str2,b1,b2);
  transducer = CALL_MEMBER_FN(transducer,&HfstTransducer::minimize)();
  return *this;
}

OtherSymbolTransducer &OtherSymbolTransducer::apply
(const HfstTransducerSubstPairMember p,const SymbolPair &p1,
 const SymbolPair &p2)
{
  if (symbol_pairs.empty())
    { throw EmptySymbolPairSet(); }
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }
  transducer = CALL_MEMBER_FN(transducer,p)(p1,p2);
  transducer = CALL_MEMBER_FN(transducer,&HfstTransducer::minimize)();
  return *this;
}

OtherSymbolTransducer &OtherSymbolTransducer::apply
(const HfstTransducerSubstPairFstMember p,const SymbolPair &p1,
 const OtherSymbolTransducer &t, bool b)
{
  if (symbol_pairs.empty())
    { throw EmptySymbolPairSet(); }
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }

  OtherSymbolTransducer t_copy(t);
  transducer = CALL_MEMBER_FN(transducer,p)(p1,t_copy.transducer,b);
  transducer = CALL_MEMBER_FN(transducer,&HfstTransducer::minimize)();
  return *this;
}

void OtherSymbolTransducer::add_transition
(HfstBasicTransducer &center_t, size_t source_state, size_t target_state,
 const std::string &input, const std::string &output)
{ center_t.add_transition
    (source_state,HfstBasicTransition(target_state,input,output,0.0)); }

bool OtherSymbolTransducer::has_symbol
(const HfstBasicTransducer &t,const std::string &sym)
{
  const std::set<std::string> &s = t.get_alphabet();
  return s.find(sym) != s.end();
}

void OtherSymbolTransducer::set_final
(HfstBasicTransducer &center_t,size_t state)
{ center_t.set_final_weight(state,0.0); }

OtherSymbolTransducer &OtherSymbolTransducer::add_info_symbol
(const std::string &info_symbol)
{
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }
  std::string name = transducer.get_name();
  if (name.size() != 0)
    { name += " & "; }
  name += info_symbol;
  transducer.set_name(name);
  return *this;
}

OtherSymbolTransducer OtherSymbolTransducer::get_inverse_of_upper_projection
(void)
{
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }
  HfstBasicTransducer fst(this->transducer);
  HfstBasicTransducer new_fst;

  HfstState state=0;
  for (HfstBasicTransducer::const_iterator it = fst.begin();
       it != fst.end();
       ++it)
    {
      new_fst.add_state(state);
      if (fst.is_final_state(state))
    { new_fst.set_final_weight(state,fst.get_final_weight(state)); }
      for (const auto & jt : *it)
    {
      HfstBasicTransition arc = jt;
      std::string input = arc.get_transition_data().get_input_symbol();
      std::string output = arc.get_transition_data().get_output_symbol();
      HfstState target = arc.get_target_state();
      if (input == HFST_UNKNOWN)
        {
          add_transition(new_fst,state,target,HFST_UNKNOWN,HFST_UNKNOWN);
          for (const auto & output_symbol : output_symbols)
        {
          if (has_symbol(fst,output_symbol))
            { add_transition(new_fst,state,target,HFST_UNKNOWN,output_symbol); }
        }
        }
      else
        {
          HfstBasicTransition arc = jt;
          add_transition
        (new_fst,state,arc.get_target_state(),input,output);
          for (const auto & symbol_pair : symbol_pairs)
        {
          if (symbol_pair.first == input && has_symbol(fst,symbol_pair.second))
            { add_transition(new_fst,state,target,input,symbol_pair.second); }
        }
          if (input == TWOLC_EPSILON)
        {
          add_transition(new_fst,state,target,HFST_EPSILON,HFST_EPSILON);
          add_transition(new_fst,state,target,TWOLC_EPSILON,HFST_UNKNOWN);
        }
          else if (input != TWOLC_DIAMOND)
        { add_transition(new_fst,state,target,input,HFST_UNKNOWN); }
        }
    }
      state++;
    }
  OtherSymbolTransducer copy(*this);
  copy.transducer = HfstTransducer(new_fst,transducer_type);
  return copy.apply(&HfstTransducer::minimize);
}

OtherSymbolTransducer &OtherSymbolTransducer::contained(void)
{
  OtherSymbolTransducer universal(TWOLC_UNKNOWN);
  universal.apply(&HfstTransducer::repeat_star);
  OtherSymbolTransducer result = universal;
  result.
    apply(&HfstTransducer::concatenate,*this).
    apply(&HfstTransducer::concatenate,universal);
  return *this = result;
}

OtherSymbolTransducer &OtherSymbolTransducer::contained_once(void)
{
  OtherSymbolTransducer universal(TWOLC_UNKNOWN);
  universal.apply(&HfstTransducer::repeat_star);
  OtherSymbolTransducer result1 = universal;
  result1.
    apply(&HfstTransducer::concatenate,*this).
    apply(&HfstTransducer::concatenate,universal);
  OtherSymbolTransducer result2 = universal;
  result2.
    apply(&HfstTransducer::concatenate,*this).
    apply(&HfstTransducer::concatenate,universal).
    apply(&HfstTransducer::concatenate,*this).
    apply(&HfstTransducer::concatenate,universal);
  result1.apply(&HfstTransducer::subtract,result2);
  return *this = result1;
}

OtherSymbolTransducer &OtherSymbolTransducer::negated(void)
{
  OtherSymbolTransducer universal(TWOLC_UNKNOWN);
  universal.apply(&HfstTransducer::repeat_star);
  universal.apply(&HfstTransducer::subtract,*this);
  return *this = universal;
}

OtherSymbolTransducer &OtherSymbolTransducer::term_complemented(void)
{
  OtherSymbolTransducer universal(TWOLC_UNKNOWN);
  universal.apply(&HfstTransducer::subtract,*this);
  return *this = universal;
}

HfstTransducer OtherSymbolTransducer::get_transducer(void) const
{
#ifndef TEST_OTHER_SYMBOL_TRANSDUCER
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }
#endif
  return transducer;
}

void OtherSymbolTransducer::get_initial_transition_pairs
(SymbolPairVector &pair_container) const
{
  if (is_broken)
    { throw UndefinedSymbolPairsFound(); }

  HfstBasicTransducer fst(this->transducer);
  HfstBasicTransducer::const_iterator start_state_it = fst.begin();
  for (const auto & jt : *start_state_it)
    {
      std::string input = jt.get_transition_data().get_input_symbol();
      std::string output = jt.get_transition_data().get_output_symbol();
      pair_container.push_back(SymbolPair(input,output));
    }
}

bool have_common_string(HfstState state1,HfstState state2,
            const HfstBasicTransducer &fst1,
            const HfstBasicTransducer &fst2,
            HandySet<StatePair> &visited_pairs,
            StringVector &v)
{
  if (fst1.is_final_state(state1) && fst2.is_final_state(state2))
    { return true; }

  const auto & fst1_transitions = fst1[state1];
  const auto & fst2_transitions = fst2[state2];

  HandyMap<SymbolPair,HfstState> fst1_transition_map;
  for (const auto& it : fst1_transitions)
    {
      fst1_transition_map[{it.get_input_symbol(), it.get_output_symbol()}] = it.get_target_state();
    }

  for (const auto & it : fst2_transitions)
    {
      SymbolPair symbol_pair = SymbolPair(it.get_input_symbol(),
                      it.get_output_symbol());
      if (fst1_transition_map.has_key(symbol_pair))
    {
      StatePair state_pair(fst1_transition_map[symbol_pair],
                   it.get_target_state());
      if (! visited_pairs.has_element(state_pair))
        {
          v.push_back(symbol_pair.first + ":" + symbol_pair.second);
          visited_pairs.insert(state_pair);
          if (have_common_string(state_pair.first,state_pair.second,
                     fst1,fst2,visited_pairs,v))
        { return true; }
          else
        { v.pop_back(); }
        }
    }
    }
  return false;
}

bool OtherSymbolTransducer::is_empty_intersection
(const OtherSymbolTransducer &another,StringVector &v)
{
  HfstBasicTransducer this_fst(transducer);
  HfstBasicTransducer another_fst(another.transducer);
  HandySet<StatePair> visited_pairs;
  visited_pairs.insert(StatePair(0,0));
  return ! have_common_string(0,0,this_fst,another_fst,visited_pairs,v);
}

bool OtherSymbolTransducer::is_subset(const OtherSymbolTransducer &another)
{
  // Do this properly later..
  OtherSymbolTransducer another_fst(another);
  another_fst.apply(&HfstTransducer::subtract,*this);
  HfstBasicTransducer internal(another_fst.get_transducer());
  return empty(internal);
}

bool OtherSymbolTransducer::empty(const HfstBasicTransducer &fsm)
{
  HfstState state=0;
  for (HfstBasicTransducer::const_iterator it = fsm.begin();
       it != fsm.end();
       ++it)
    {
      if (fsm.is_final_state(state))
    { return false; }
      state++;
    }
  return true;
}

bool OtherSymbolTransducer::is_empty(void) const
{ return empty(HfstBasicTransducer(transducer)); }

std::ostream &operator<<(std::ostream &out,const OtherSymbolTransducer &o)
{ return out << "FST:" << std::endl << o.transducer; }

#endif // #ifndef TEST_OTHER_SYMBOL_TRANSDUCER

#ifdef TEST_OTHER_SYMBOL_TRANSDUCER
#include <cassert>
#include <iostream>

int main(void)
{
bool have_openfst = false;
#if HAVE_OPENFST
  have_openfst = true;
#endif // HAVE_OPENFST

bool have_sfst = false;
#if HAVE_SFST
  have_sfst = true;
#endif // HAVE_SFST

bool have_foma = false;
#if HAVE_FOMA
  have_foma = true;
#endif // HAVE_FOMA


ImplementationType transducer_type
= (have_openfst ? hfst::TROPICAL_OPENFST_TYPE :
  have_sfst ? hfst::SFST_TYPE :
  have_foma ? hfst::FOMA_TYPE :
   hfst::ERROR_TYPE);

 OtherSymbolTransducer::set_transducer_type(transducer_type);

  HandySet<SymbolPair> symbol_pairs;
  symbol_pairs.insert(SymbolPair("a","b"));
  symbol_pairs.insert(SymbolPair("c","d"));
  symbol_pairs.insert(SymbolPair("a","d"));
  OtherSymbolTransducer::set_symbol_pairs(symbol_pairs);
  OtherSymbolTransducer ost1(TWOLC_UNKNOWN,TWOLC_UNKNOWN);
  OtherSymbolTransducer ost2("c",TWOLC_UNKNOWN);
  OtherSymbolTransducer ost3(TWOLC_UNKNOWN,"b");
  OtherSymbolTransducer ost4("c","d");
  OtherSymbolTransducer ost5("a","b");
  OtherSymbolTransducer ost6("a","d");
  ost1.apply
    (&HfstTransducer::concatenate,ost2).apply
    (&HfstTransducer::concatenate,ost3).apply
    (&HfstTransducer::concatenate,ost4).apply
    (&HfstTransducer::concatenate,ost5).apply
    (&HfstTransducer::concatenate,ost6);
  HfstTransducer ost1_t = ost1.get_transducer();
  //std::cout << ost1_t << std::endl;

}
#endif // TEST_OTHER_SYMBOL_TRANSDUCER
