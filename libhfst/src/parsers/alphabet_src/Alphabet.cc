 //! @file Alphabet.cc
//!
//! @author Miikka Silfverberg
//!
//! @brief The alphabet of a twolc-grammar.

//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation, version 3 of the Licence.
//
//   This library is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "Alphabet.h"

#ifdef HAVE_XFSM
  #define Alphabet TwolCAlphabet
#endif

void Alphabet::define_set(const std::string &name,const SymbolRange &elements)
{ sets[name] = elements; }

SymbolPairVector * Alphabet::get_symbol_pair_vector(const SymbolPair &pair)
{
  const OtherSymbolTransducer result_fst = get_transducer(pair);
  SymbolPairVector * result = new SymbolPairVector();
  result_fst.get_initial_transition_pairs(*result);
  return result;
}

const OtherSymbolTransducer &Alphabet::get_transducer(const SymbolPair &pair)
{ return alphabet.has_key(pair) ? alphabet[pair] : compute(pair); }

bool Alphabet::is_pair(const std::string &input,const std::string &output)
{
  if (input == "__HFST_TWOLC_?" && output == "__HFST_TWOLC_?")
    { return true; }
  if (diacritics.has_element(input) && input == output)
    { return true; }
  if (diacritics.has_element(input) && output == "__HFST_TWOLC_?")
    { return true; }
  if (input == "__HFST_TWOLC_?")
    { return output_symbols.has_element(output); }
  if (output == "__HFST_TWOLC_?")
    { return input_symbols.has_element(input); }

  return alphabet_set.has_element(SymbolPair(input,output));
}

bool Alphabet::is_empty_pair(const SymbolPair &pair)
{
  assert(is_pair(pair.first,pair.second));
  return alphabet[pair].is_empty();
}

void Alphabet::define_singleton_set(const std::string &name)
{ sets[name] = SymbolRange(1,name); }

bool Alphabet::is_set_pair(const SymbolPair &pair) const
{
  return
    pair.first.find("__HFST_TWOLC_SET_NAME=") != std::string::npos ||
    pair.second.find("__HFST_TWOLC_SET_NAME=") != std::string::npos;
}

const OtherSymbolTransducer &Alphabet::compute(const SymbolPair &pair)
{
  if (! sets.has_key(pair.first))
    { define_singleton_set(pair.first); }
  if (! sets.has_key(pair.second))
    { define_singleton_set(pair.second); }

  const std::string &input = pair.first;
  const std::string &output = pair.second;

  OtherSymbolTransducer pair_transducer;

  if (diacritics.has_element(input))
    {
      pair_transducer.apply(&HfstTransducer::disjunct,
                OtherSymbolTransducer(input,input));
      if (input != output && output != TWOLC_EPSILON &&
      output != TWOLC_UNKNOWN)
    { std::cerr << "Warning: Diacritic " << input << " in pair "
            << input << ":" << output << " will correspond 0."
            << std::endl; }
    }
  else if (input == TWOLC_UNKNOWN && output == TWOLC_UNKNOWN)
    {
      for (const auto & it : alphabet_set)
        {
          if (is_set_pair(it))
            { continue; }

          pair_transducer.apply(&HfstTransducer::disjunct,
                                OtherSymbolTransducer(it.first,it.second));
        }
      pair_transducer.apply(&HfstTransducer::disjunct,
                            OtherSymbolTransducer(TWOLC_UNKNOWN));
    }
  else if (input == TWOLC_UNKNOWN)
    {
      output_symbols.insert(pair.second);
      const SymbolRange &output_set = sets[output];
      for (const auto & it : output_set)
        {
          for (const auto& jt : alphabet_set)
            {
              if (is_set_pair(jt))
                {
                  continue;
                }

              if (it == jt.second)
                {
                  pair_transducer.apply(&HfstTransducer::disjunct,
                                        OtherSymbolTransducer(jt.first, jt.second));
                }
            }
        }
    }
  else if (output == TWOLC_UNKNOWN)
    {
      input_symbols.insert(pair.first);
      const SymbolRange &input_set = sets[input];
      for (const auto& it : input_set)
        {
          for (const auto& jt : alphabet_set)
            {
              if (is_set_pair(jt))
                {
                  continue;
                }

              if (it == jt.first)
                {
                  pair_transducer.apply(&HfstTransducer::disjunct,
                                        OtherSymbolTransducer(jt.first, jt.second));
                }
            }
        }
    }
  else
    {
      const SymbolRange &input_set = sets[input];
      const SymbolRange &output_set = sets[output];

      for (const auto& it : input_set)
        {
          for (const auto& jt : output_set)
            {
              if (is_pair(it, jt))
                {
                  pair_transducer.apply(&HfstTransducer::disjunct, OtherSymbolTransducer(it, jt));
                }
            }
        }
    }
  alphabet[pair] = pair_transducer;
  alphabet_set.insert(pair);
  return alphabet[pair];
}

void Alphabet::define_alphabet_pair(const SymbolPair &pair)
{
  alphabet_set.insert(pair);
  input_symbols.insert(pair.first);
  output_symbols.insert(pair.second);
}

void Alphabet::define_diacritics(const SymbolRange &diacs)
{
  diacritics.insert(diacs.begin(),diacs.end());
  for (const auto & diacritic : diacritics)
    {
      alphabet_set.erase(SymbolPair(diacritic,diacritic));
      alphabet_set.erase(SymbolPair(diacritic,TWOLC_EPSILON));
      input_symbols.erase(diacritic);
      output_symbols.erase(diacritic);
    }
}

void Alphabet::alphabet_done(void)
{ OtherSymbolTransducer::set_symbol_pairs(alphabet_set); }

#ifdef TEST_ALPHABET
const char * arr1[2] = { "a","b" };
const char * arr2[3] = { "a","b","c" };
int main(void)
{
  bool have_openfst = false;
#if HAVE_OPENFST
  have_openfst = true;
#endif //HAVE_OPENFST

  bool have_sfst = false;
#if HAVE_SFST
  have_sfst = true;
#endif //HAVE_SFST

  bool have_foma = false;
#if HAVE_FOMA
  have_foma = true;
#endif // HAVE_FOMA

  OtherSymbolTransducer::set_transducer_type
    (have_openfst ? hfst::TROPICAL_OPENFST_TYPE :
     have_sfst ? hfst::SFST_TYPE :
     have_foma ? hfst::FOMA_TYPE :
     hfst::ERROR_TYPE);

  Alphabet alphabet;
  std::string name1 = "X";
  SymbolRange sr1(arr1,arr1+2);
  std::string name2 = "Y";
  SymbolRange sr2(arr2,arr2+3);
  std::string name3 = "Z";
  SymbolRange sr3(arr1,arr1+1);

  alphabet.define_alphabet_pair(SymbolPair("a","b"));
  alphabet.define_alphabet_pair(SymbolPair("b","c"));
  alphabet.define_alphabet_pair(SymbolPair("b","b"));
  alphabet.alphabet_done();

  alphabet.define_set(name1,sr1);
  alphabet.define_set(name2,sr2);
  alphabet.define_set(name3,sr3);


}
#endif
