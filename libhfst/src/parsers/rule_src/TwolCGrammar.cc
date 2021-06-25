//! @file TwolCGrammar.cc
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

#include "TwolCGrammar.h"

std::string TwolCGrammar::get_original_name(const std::string &name)
{ return name.substr(0,name.find("SUBCASE:")); }

TwolCGrammar::TwolCGrammar(bool be_quiet,
                           bool be_verbose,
                           bool resolve_left_conflicts,
                           bool resolve_right_conflicts):
  be_quiet(be_quiet),
  be_verbose(be_verbose)
{
  left_arrow_rule_container.set_report_left_arrow_conflicts(! be_quiet);
  left_arrow_rule_container.set_resolve_left_arrow_conflicts
    (resolve_left_conflicts);
  right_arrow_rule_container.set_report_right_arrow_conflicts
    (be_verbose);
  right_arrow_rule_container.set_resolve_right_arrow_conflicts
    (resolve_right_conflicts);
}

void TwolCGrammar::define_diacritics(const SymbolRange &diacritics)
{
  this->diacritics = diacritics;
  OtherSymbolTransducer::define_diacritics(diacritics);
}

void TwolCGrammar::add_rule(const std::string &name,
                            const SymbolPair &center,
                            op::OPERATOR oper,
                            const OtherSymbolTransducerVector contexts)
{
  Rule * rule;
  switch (oper)
    {
    case op::RIGHT:
      rule = new ConflictResolvingRightArrowRule(name,center,contexts);
      right_arrow_rule_container.add_rule_and_display_and_resolve_conflicts
    (static_cast<ConflictResolvingRightArrowRule*>(rule),std::cerr);
      break;
    case op::LEFT:
      rule = new ConflictResolvingLeftArrowRule(name,center,contexts);
      left_arrow_rule_container.add_rule_and_display_and_resolve_conflicts
    (static_cast<ConflictResolvingLeftArrowRule*>(rule),std::cerr);
      break;
    case op::LEFT_RIGHT:
      rule = new ConflictResolvingRightArrowRule(name,center,contexts);
      right_arrow_rule_container.add_rule_and_display_and_resolve_conflicts
    (static_cast<ConflictResolvingRightArrowRule*>(rule),std::cerr);
      name_to_rule_subcases[get_original_name(name)].insert(rule);
      rule = new ConflictResolvingLeftArrowRule(name,center,contexts);
      left_arrow_rule_container.add_rule_and_display_and_resolve_conflicts
    (static_cast<ConflictResolvingLeftArrowRule*>(rule),std::cerr);
      break;
    case op::NOT_LEFT:
      rule = new LeftRestrictionArrowRule(name,center,contexts);
      other_rule_container.add_rule
    (static_cast<LeftRestrictionArrowRule*>(rule));
      break;
    default:
      assert(false);
    }
  name_to_rule_subcases[get_original_name(name)].insert(rule);
}

void TwolCGrammar::add_rule(const std::string &name,
                const OtherSymbolTransducer &center,
                op::OPERATOR oper,
                const OtherSymbolTransducerVector contexts)
{
  OtherSymbolTransducer center_fst = Rule::get_center(center);

  Rule * rule;
  switch (oper)
    {
    case op::RE_RIGHT:
      rule = new RightArrowRule(name,center_fst,contexts);
      other_rule_container.add_rule
    (static_cast<RightArrowRule*>(rule));
      break;
    case op::RE_LEFT:
      rule = new LeftArrowRule(name,center_fst,contexts);
      other_rule_container.add_rule
    (static_cast<LeftArrowRule*>(rule));
      break;
    case op::RE_LEFT_RIGHT:
      rule = new RightArrowRule(name,center_fst,contexts);
      other_rule_container.add_rule
    (static_cast<RightArrowRule*>(rule));
      name_to_rule_subcases[get_original_name(name)].insert(rule);
      rule = new LeftArrowRule(name,center_fst,contexts);
      other_rule_container.add_rule
    (static_cast<LeftArrowRule*>(rule));
      break;
    case op::RE_NOT_LEFT:
      rule = new LeftRestrictionArrowRule(name,center_fst,contexts);
      other_rule_container.add_rule
    (static_cast<LeftRestrictionArrowRule*>(rule));
      break;
    default:
      assert(false);
    }
  name_to_rule_subcases[get_original_name(name)].insert(rule);
}

void TwolCGrammar::add_rule(const std::string &name,
                            const SymbolPairVector &center,
                            op::OPERATOR oper,
                            const OtherSymbolTransducerVector contexts)
{
  /*
  OtherSymbolTransducer center_fst = Rule::get_center(center);
  */

  for (const auto & it : center)
    {
      Rule * rule;

      std::string center_name =
        name + " CENTER=" + it.first + ":" + it.second;
      
      switch (oper)
        {
        case op::RIGHT:
          rule = new ConflictResolvingRightArrowRule(center_name,it,contexts);
          right_arrow_rule_container.add_rule_and_display_and_resolve_conflicts
            (static_cast<ConflictResolvingRightArrowRule*>(rule),std::cerr);
          break;
        case op::LEFT:
          rule = new ConflictResolvingLeftArrowRule(center_name,it,contexts);
          left_arrow_rule_container.add_rule_and_display_and_resolve_conflicts
            (static_cast<ConflictResolvingLeftArrowRule*>(rule),std::cerr);
          break;
        case op::LEFT_RIGHT:
          rule = new ConflictResolvingRightArrowRule(center_name,it,contexts);
          right_arrow_rule_container.add_rule_and_display_and_resolve_conflicts
            (static_cast<ConflictResolvingRightArrowRule*>(rule),std::cerr);
          name_to_rule_subcases[get_original_name(center_name)].insert(rule);

          rule = new ConflictResolvingLeftArrowRule(center_name,it,contexts);
          left_arrow_rule_container.add_rule_and_display_and_resolve_conflicts
            (static_cast<ConflictResolvingLeftArrowRule*>(rule),std::cerr);
          break;
        case op::NOT_LEFT:
          rule = new LeftRestrictionArrowRule(center_name,it,contexts);
          other_rule_container.add_rule
            (static_cast<LeftRestrictionArrowRule*>(rule));
          break;
        default:
          assert(false);
        }
      
      name_to_rule_subcases[get_original_name(center_name)].insert(rule);
    }

}

void TwolCGrammar::compile_and_store(HfstOutputStream &out)
{
  if (! be_quiet)
    { std::cerr << "Compiling rules." << std::endl; }

  left_arrow_rule_container.compile(std::cerr,(! be_quiet) && be_verbose);
  right_arrow_rule_container.compile(std::cerr,(! be_quiet) && be_verbose);
  other_rule_container.compile(std::cerr,(! be_quiet) && be_verbose);

  for (StringRuleSetMap::const_iterator it = name_to_rule_subcases.begin();
       it != name_to_rule_subcases.end();
       ++it)
    { compiled_rule_container.add_rule
    (new Rule(it->first,Rule::RuleVector(it->second.begin(),
                         it->second.end()))); }
  compiled_rule_container.add_missing_symbols_freely(diacritics);

  if (! be_quiet)
    { std::cerr << "Storing rules." << std::endl; }
  compiled_rule_container.store(out,std::cerr,(! be_quiet) && be_verbose);
}

#ifdef TEST_TWOL_C_GRAMMAR
#include <cassert>
#include "../alphabet_src/Alphabet.h"
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
= have_openfst ? hfst::TROPICAL_OPENFST_TYPE :
  have_sfst ? hfst::SFST_TYPE :
  have_foma ? hfst::FOMA_TYPE :
  hfst::ERROR_TYPE;

 OtherSymbolTransducer::set_transducer_type(transducer_type);

 TwolCGrammar g(true,false,true,true);

  /*HandySet<SymbolPair> symbols;
  symbols.insert(SymbolPair("a","b"));
  symbols.insert(SymbolPair("a","d"));
  symbols.insert(SymbolPair("b","c"));*/
  //g.set_alphabet(symbols);

#ifdef HAVE_XFSM
  #define Alphabet TwolCAlphabet
#endif

  Alphabet alphabet;
  alphabet.define_alphabet_pair(SymbolPair("a","b"));
  alphabet.define_alphabet_pair(SymbolPair("a","d"));
  alphabet.define_alphabet_pair(SymbolPair("b","c"));
  alphabet.alphabet_done();

  OtherSymbolTransducer unknown("__HFST_TWOLC_?","__HFST_TWOLC_?");
  OtherSymbolTransducer diamond("__HFST_TWOLC_DIAMOND");
  OtherSymbolTransducer b_c_pair("b","c");

  OtherSymbolTransducer context = unknown;
  context.
    apply(&HfstTransducer::concatenate,b_c_pair).
    apply(&HfstTransducer::concatenate,diamond).
    apply(&HfstTransducer::concatenate,unknown).
    apply(&HfstTransducer::concatenate,diamond).
    apply(&HfstTransducer::concatenate,unknown);
  OtherSymbolTransducerVector contexts(1,context);
  g.add_rule("\"test1\"",SymbolPair("a","b"),
         op::LEFT_RIGHT,contexts);
  /*
  g.add_rule("\"test2\"",SymbolPair("a","b"),
         op::LEFT_RIGHT,contexts);

  OtherSymbolTransducer a_d_pair("a","d");
  OtherSymbolTransducer context1 = unknown;
  context1.
    //apply(&HfstTransducer::concatenate,a_d_pair).
    apply(&HfstTransducer::concatenate,b_c_pair).
    apply(&HfstTransducer::concatenate,diamond).
    apply(&HfstTransducer::concatenate,unknown).
    apply(&HfstTransducer::concatenate,diamond).
    apply(&HfstTransducer::concatenate,unknown);
  OtherSymbolTransducerVector contexts1(1,context1);
  g.add_rule("\"test3\"",SymbolPair("a","b"),
  op::LEFT_RIGHT,contexts1);*/
}
#endif // TEST_TWOL_C_GRAMMAR
