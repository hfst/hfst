//! @file guessify_fst.cc
//!
//! @brief Functions for making an affix guesser from a morphological analyzer.
//!
//! @author Miikka Silfverberg (HFST Team)


//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, version 3 of the License.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "guessify_fst.h"

#ifndef MAIN_TEST

#include "HfstLookupFlagDiacritics.h"

using hfst::TROPICAL_OPENFST_TYPE;
using hfst::HFST_OLW_TYPE;

using hfst::implementations::HfstBasicTransducer;
using hfst::implementations::HfstState;
using hfst::implementations::HfstBasicTransition;

typedef hfst::implementations::HfstBasicTransitions HfstBasicTransitions;

using hfst::internal_default;
using hfst::internal_epsilon;
using hfst::internal_identity;
using hfst::internal_unknown;

using hfst::StringPair;

using hfst::HfstSymbolSubstitutions;
using hfst::HfstSymbolPairSubstitutions;

using hfst::HfstInputStream;
using hfst::HfstOutputStream;

// Marker for removed symbols.
#define REMOVED_SYMBOL "<removed_symbol>"

std::string my_default = "$_DEFAULT_SYMBOL_$";

void remove_flag_diacritics(HfstTransducer &morphological_analyzer,
                            const StringSet &alphabet)
{
  HfstSymbolSubstitutions flag_diacritic_epsilon_pairs;

  for (const auto & it : alphabet)
    {
      if (FlagDiacriticTable::is_diacritic(it))
        { flag_diacritic_epsilon_pairs[it] = internal_epsilon; }
    }

  morphological_analyzer.substitute(flag_diacritic_epsilon_pairs);
}

bool is_cathegory_symbol(const std::string &symbol)
{ return symbol.find(CATEGORY_SYMBOL_PREFIX) == 0; }

StringSet get_cathegory_symbols(const StringSet &alphabet)
{
  StringSet cathegory_symbols;
  
  for (const auto & it : alphabet)
    {
      if (is_cathegory_symbol(it))
        { cathegory_symbols.insert(it); }
    }

  return cathegory_symbols;
}

HfstTransducer get_prefix_remover(const StringSet &alphabet)
{
  StringSet cathegory_symbols = get_cathegory_symbols(alphabet);

  HfstTransducer cathegory_symbols_fst(TROPICAL_OPENFST_TYPE);

  HfstTransducer identity_except_cathegory(internal_identity,
                                           TROPICAL_OPENFST_TYPE);
  HfstBasicTransducer basic_identity(identity_except_cathegory);

  // Add cathegory symbols as paths in cathegory_symbols_fst and add
  // them to the alphabet of basic_identity so that the identity
  // transitions won't cover cathegory symbols.
  for (const auto & cathegory_symbol : cathegory_symbols)
    {
      HfstTransducer cathegory_symbol_fst(cathegory_symbol,TROPICAL_OPENFST_TYPE);
      cathegory_symbols_fst.disjunct(cathegory_symbol_fst);
      basic_identity.add_symbol_to_alphabet(cathegory_symbol);
    }
  
  cathegory_symbols_fst.minimize();
  
  // Preserve one symbol after the cathegory marker.
  HfstTransducer identity(internal_identity,TROPICAL_OPENFST_TYPE);
  cathegory_symbols_fst.concatenate(identity).minimize();
  identity_except_cathegory = HfstTransducer(basic_identity,
                                             TROPICAL_OPENFST_TYPE);
  identity_except_cathegory.repeat_star().minimize();

  HfstTransducer remove_symbol(internal_unknown,
                               REMOVED_SYMBOL,
                               TROPICAL_OPENFST_TYPE);
  remove_symbol.repeat_star().minimize();

  HfstTransducer remove_suffix(cathegory_symbols_fst);
  remove_suffix.concatenate(remove_symbol);
  remove_suffix.optionalize().minimize();

  identity_except_cathegory.concatenate(remove_suffix).minimize();

  return identity_except_cathegory;
}

HfstTransducer get_invalid_form_filterer(const StringSet &alphabet)
{
  StringSet cathegory_symbols = get_cathegory_symbols(alphabet);

  HfstTransducer cathegory_symbols_fst(TROPICAL_OPENFST_TYPE);
  for (const auto & cathegory_symbol : cathegory_symbols)
    {
      HfstTransducer cathegory_symbol_fst(cathegory_symbol,TROPICAL_OPENFST_TYPE);
      cathegory_symbols_fst.disjunct(cathegory_symbol_fst);
    }

  cathegory_symbols_fst.minimize();

  HfstTransducer identity(internal_identity,TROPICAL_OPENFST_TYPE);

  HfstTransducer identity_star(identity);

  identity_star.repeat_star().minimize();

  HfstTransducer remover(identity_star);

  remover.
    concatenate(cathegory_symbols_fst).
    concatenate(identity).
    concatenate(identity_star).
    minimize();

  return remover;
}

void rewrite_removed_symbols(HfstTransducer &morphological_analyzer,
                             const StringSet &alphabet)
{
  HfstSymbolPairSubstitutions substitution_pairs;

  substitution_pairs[ StringPair(internal_epsilon,REMOVED_SYMBOL) ] =
    StringPair(internal_epsilon,internal_epsilon);

  for (const auto & it : alphabet)
    {
      if (it != internal_epsilon)
        {
          substitution_pairs[ StringPair(it,REMOVED_SYMBOL) ] =
            StringPair(it, it);
        }
    }
  
  morphological_analyzer.substitute(substitution_pairs);
  
}

HfstTransducer guessify_analyzer(HfstTransducer morphological_analyzer,
                                 float penalty)
{
  // Convert to tropical openfst type so that all operations can be
  // performed.
  morphological_analyzer.convert(TROPICAL_OPENFST_TYPE);

  std::string morphological_analyzer_name = morphological_analyzer.get_name();

  // Start be reversing the morphological analyzer, since guessing is
  // based on suffixes of words.
  morphological_analyzer.reverse().minimize();

  // Get rid of flag diacritics. They're a nuissance and the
  // combinatorics would any way be screwed up because we modify the
  // behavior of the transducer using default-transitions.
  StringSet alphabet = morphological_analyzer.get_alphabet();
  remove_flag_diacritics(morphological_analyzer, alphabet);

  morphological_analyzer.minimize();

  // Remove the parts of analyses that precede the last cathegory
  // tag. After the last cathegory tag all input should be echoed as is to
  // the output.
  /*
  HfstTransducer analysis_prefix_remover = get_prefix_remover(alphabet);

  morphological_analyzer.compose(analysis_prefix_remover);
  rewrite_removed_symbols(morphological_analyzer, alphabet);

  morphological_analyzer.minimize();
  */

  // Add a sink state and default transitions from every state
  // (including the sink state) to the sink state. The default
  // transitions all have the same weight @a penalty.
  HfstBasicTransducer basic_guesser(morphological_analyzer);

  HfstState sink_state = basic_guesser.add_state();

  for (HfstState s = 0; s <= basic_guesser.get_max_state(); ++s)
    {
      basic_guesser.set_final_weight(s,0.0);
    }


  for (HfstState s = 0; s <= basic_guesser.get_max_state(); ++s)
    {
      basic_guesser.add_transition
        (s,HfstBasicTransition(sink_state,
                               my_default,
                               my_default,
                               penalty));
    }

  // Add an a-transition to the sink state to all states where there
  // is only one transitions which is a default-transition, since some
  // version of hfst didn't suporrt default transitions in states
  // without any additional transitions.
  for (HfstState s = 0; s <= basic_guesser.get_max_state(); ++s)
    {
      if (basic_guesser[s].size() == 1 and
          basic_guesser[s][0].get_input_symbol() == my_default)
        {
          basic_guesser.add_transition
            (s, HfstBasicTransition(sink_state, "a",  "a", penalty));
        }
    }
  
  HfstTransducer guesser(basic_guesser,TROPICAL_OPENFST_TYPE);

  HfstTransducer invalid_form_filterer = get_invalid_form_filterer(alphabet);

  guesser.compose(invalid_form_filterer).minimize();
  
  guesser.set_name(std::string("guessified(") +
                   morphological_analyzer_name +
                   ")");

  guesser.set_property("reverse input","true");

  return guesser;
}

void store_guesser(HfstTransducer &guesser,
                   HfstOutputStream &out,
                   bool compile_generator)
{
  HfstTransducer generator(TROPICAL_OPENFST_TYPE);
  if (compile_generator)
    { generator = guesser; }
  
  guesser.substitute(my_default, internal_default);
  guesser.convert(HFST_OLW_TYPE);
  out << guesser;

  if (compile_generator)
    {
      generator.invert();
      generator.set_name(std::string("inverted(") + guesser.get_name() + ")");
      generator.substitute(my_default, internal_default);
      generator.convert(HFST_OLW_TYPE);
      out << generator;
    }

}

#else // MAIN_TEST

using hfst::HfstInputStream;
using hfst::HfstOutputStream;

int main(void)
{
  HfstInputStream in;
  
  HfstTransducer analyzer(in);

  float default_penalty = DEFAULT_PENALTY;

  HfstTransducer guesser = guessify_analyzer(analyzer,
                                             default_penalty);

  bool compile_generator = false;

  HfstOutputStream out;
  store_guesser(guesser,
                out,
                compile_generator);
}

#endif // MAIN_TEST
