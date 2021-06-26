// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include "HfstLookupFlagDiacritics.h"

using hfst::StringVector;

DiacriticOperators FlagDiacriticTable::diacritic_operators;
DiacriticFeatures FlagDiacriticTable::diacritic_features;
DiacriticValues FlagDiacriticTable::diacritic_values;
DiacriticSettingMap FlagDiacriticTable::diacritic_has_value;

bool FlagDiacriticTable::is_genuine_diacritic
(const std::string &diacritic_string)
{
  // All diacritics have form @[A-Z][.][A-Z]+([.][A-Z]+)?@
  if (diacritic_string.size() < 5)
    { return false; }
  if (diacritic_string.at(2) != '.')
    { return false; }
  // These two checks probably always succeed...
  if (diacritic_string.at(0) != '@')
    { return false; }
  if (diacritic_string.at(diacritic_string.size()-1) != '@')
    { return false; }
  switch (diacritic_string.at(1))
    {
    case 'P':
      break;
    case 'N':
      break;
    case 'D':
      break;
    case 'R':
      break;
    case 'C':
      break;
    case 'U':
      break;
    default:
      return false;
    }
  if (diacritic_string.find_last_of('.') == 2)
    {
      if ((diacritic_string.at(1) != 'R') &&
      (diacritic_string.at(1) != 'D') &&
      (diacritic_string.at(1) != 'C'))
    { return false; }
    }
  return true;
}

// Precondition: diacritic_string matches @[A-Z][.][A-Z]+([.][A-Z]+)?@
void FlagDiacriticTable::split_diacritic(const std::string &diacritic_string)
{
  switch (diacritic_string.at(1))
    {
    case 'P':
      diacritic_operators[diacritic_string] = Pop;
      break;
    case 'N':
      diacritic_operators[diacritic_string] = Nop;
      break;
    case 'D':
      diacritic_operators[diacritic_string] = Dop;
      break;
    case 'R':
      diacritic_operators[diacritic_string] = Rop;
      break;
    case 'C':
      diacritic_operators[diacritic_string] = Cop;
      break;
    case 'U':
      diacritic_operators[diacritic_string] = Uop;
      break;
    default:
      assert(false);
    }

  // Third character is always the first fullstop.
  size_t first_full_stop_pos = 2;
  // Find the second full stop, if there is one.
  size_t second_full_stop_pos =
    diacritic_string.find('.',first_full_stop_pos+1);
  size_t last_char_pos = diacritic_string.size() - 1;
  if (second_full_stop_pos == std::string::npos)
    {
      assert((diacritic_operators[diacritic_string] == Cop) ||
         (diacritic_operators[diacritic_string] == Dop) ||
         (diacritic_operators[diacritic_string] == Rop));
      diacritic_has_value[diacritic_string] = false;
      diacritic_features[diacritic_string] =
    diacritic_string.substr(first_full_stop_pos+1,
                last_char_pos - first_full_stop_pos - 1);
    }
  else
    {
      diacritic_has_value[diacritic_string] = true;
      diacritic_features[diacritic_string] =
    diacritic_string.substr(first_full_stop_pos+1,
                second_full_stop_pos-first_full_stop_pos - 1);
      diacritic_values[diacritic_string] =
    diacritic_string.substr(second_full_stop_pos+1,
                last_char_pos - second_full_stop_pos - 1);
    }
}

FlagDiacriticTable::FlagDiacriticTable(void):
  error_flag(false)
{}

bool FlagDiacriticTable::is_diacritic(const std::string &symbol)
{ //return diacritic_operators.find(symbol) != diacritic_operators.end(); }
  bool res = is_genuine_diacritic(symbol);
  if (res)
    split_diacritic(symbol);
  return res;
}

void FlagDiacriticTable::set_positive_value(std::string &feature,
                        std::string &value)
{
  feature_values[feature] = value;
  feature_polarities[feature] = true;
}
void FlagDiacriticTable::set_negative_value(std::string &feature,
                        std::string &value)
{
  feature_values[feature] = value;
  feature_polarities[feature] = false;
}
void FlagDiacriticTable::disallow(std::string &feature,
                  std::string &value)
{
  if (feature_values.find(feature) == feature_values.end())
    { return; }
  if (feature_values[feature] == value)
    { error_flag = error_flag || feature_polarities[feature]; }
}
void FlagDiacriticTable::disallow(std::string &feature)
{
  if (feature_values.find(feature) != feature_values.end())
    { error_flag = true; }
}
void FlagDiacriticTable::require(std::string &feature,
                 std::string &value)
{
  if (feature_values.find(feature) == feature_values.end())
    {
      error_flag = true;
      return;
    }
  else if (feature_values[feature] != value)
    { error_flag = true; }
  else
    { error_flag = error_flag || (! feature_polarities[feature]); }
}
void FlagDiacriticTable::require(std::string &feature)
{
  if (feature_values.find(feature) == feature_values.end())
    { error_flag = true; }
}
void FlagDiacriticTable::unify(std::string &feature,
                   std::string &value)
{
  // If feature not set, set it to value.
  if (feature_values.find(feature) == feature_values.end())
    { set_positive_value(feature,value); }
  // If feature set to something else negatively, set it to value.
  else if (feature_values[feature] != value)
    {
      if (! feature_polarities[feature])
    { set_positive_value(feature,value); }
    }
  require(feature,value);
}
void FlagDiacriticTable::clear(std::string &feature)
{
  feature_values.erase(feature);
  feature_polarities.erase(feature);
}

/*void
FlagDiacriticTable::define_diacritic(short diacritic_number,
                     const std::string &diacritic_string)
{ if (is_genuine_diacritic(diacritic_string))
{ split_diacritic(diacritic_number,diacritic_string); } }*/

void FlagDiacriticTable::insert_symbol(const std::string &symbol)
{
  if (is_diacritic(symbol))
    {
      switch (diacritic_operators[symbol])
    {
    case Pop:
      set_positive_value(diacritic_features[symbol],
                 diacritic_values[symbol]);
      break;
    case Nop:
      set_negative_value(diacritic_features[symbol],
                 diacritic_values[symbol]);
      break;
    case Dop:
      if (! diacritic_has_value[symbol])
        {
          disallow(diacritic_features[symbol]);
        }
      else
        {
          disallow(diacritic_features[symbol],
               diacritic_values[symbol]);
        }
      break;
    case Rop:
      if (! diacritic_has_value[symbol])
        {
          require(diacritic_features[symbol]);
        }
      else
        {
          require(diacritic_features[symbol],
              diacritic_values[symbol]);
        }
      break;
    case Cop:
      clear(diacritic_features[symbol]);
      break;
    case Uop:
      unify(diacritic_features[symbol],
        diacritic_values[symbol]);
      break;
    default:
      assert(false);
    }
    }
}

bool FlagDiacriticTable::fails(void)
{ return error_flag; }

void FlagDiacriticTable::reset(void)
{
  error_flag = false;
  feature_values.clear();
  feature_polarities.clear();
}

bool FlagDiacriticTable::is_valid_string(const StringVector &input_string)
{
  reset();
  for (const auto& it : input_string)
    {
      insert_symbol(it);
      if (fails())
        {
          return false;
        }
    }
  return true;
}

StringVector FlagDiacriticTable::filter_diacritics
(const StringVector &input_string)
{
  StringVector filtered;
  for (const auto & it : input_string)
    {
      if (! is_diacritic(it))
    { filtered.push_back(it); }
    }
  return filtered;
}

#ifdef DEBUG
void FlagDiacriticTable::display(short diacritic)
{
  if (diacritic_operators.find(diacritic) == diacritic_operators.end())
    {
      std::cout << diacritic << " not defined." << std::endl;
    }
  else
    {
      std::cout << diacritic_operators[diacritic]
        << " "
        << diacritic_features[diacritic]
        << " "
        << diacritic_values[diacritic]
        << std::endl;
    }
}
/*************************************************

                  TEST MAIN

 *************************************************/
int main(void)
{
  std::string set_dc("@P.NeedNoun.ON@");
  FlagDiacriticTable::define_diacritic(1,set_dc);
  std::string negative_set_dc("@N.NeedNoun.ON@");
  FlagDiacriticTable::define_diacritic(2,negative_set_dc);
  std::string require_dc("@R.NeedNoun.ON@");
  FlagDiacriticTable::define_diacritic(3,require_dc);
  std::string disallow_dc("@D.NeedNoun.ON@");
  FlagDiacriticTable::define_diacritic(4,disallow_dc);
  std::string unify_dc("@U.NeedNoun.ON@");
  FlagDiacriticTable::define_diacritic(5,unify_dc);
  std::string clear_dc("@C.NeedNoun@");
  FlagDiacriticTable::define_diacritic(6,clear_dc);
  std::string set_dc1("@P.BlaBla.ON@");
  FlagDiacriticTable::define_diacritic(7,set_dc1);
  std::string require_dc1("@R.BlaBla.ON@");
  FlagDiacriticTable::define_diacritic(8,require_dc1);
  std::string negative_set_dc_alt("@N.NeedNoun.foo@");
  FlagDiacriticTable::define_diacritic(9,negative_set_dc_alt);

  FlagDiacriticTable fdt;
  std::cout << "\"\" should pass:";
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == false);
  fdt.reset();
  std::cout << "\"a\" should pass:";
  fdt.insert_number(100);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == false);
  fdt.reset();
  std::cout << "\"@P.NeedNoun.ON@ @R.NeedNoun.ON@\" should pass:";
  fdt.insert_number(1);
  fdt.insert_number(3);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == false);
  fdt.reset();
  std::cout << "\"@P.NeedNoun.ON@ @D.NeedNoun.ON@\" should fail:";
  fdt.insert_number(1);
  fdt.insert_number(4);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == true);
  fdt.reset();
  std::cout << "\"@P.NeedNoun.ON@ a @R.NeedNoun.ON@\" should pass:";
  fdt.insert_number(1);
  fdt.insert_number(100);
  fdt.insert_number(3);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == false);
  fdt.reset();
  std::cout << "\"@N.NeedNoun.ON@ @D.NeedNoun.ON@\" should pass:";
  fdt.insert_number(2);
  fdt.insert_number(4);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == false);
  fdt.reset();
  std::cout << "\"@D.NeedNoun.ON@\" should pass:";
  fdt.insert_number(4);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == false);
  fdt.reset();
  std::cout << "\"@R.NeedNoun.ON@\" should fail:";
  fdt.insert_number(3);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == true);
  fdt.reset();
  std::cout << "\"@P.NeedNoun.ON@ @C.NeedNoun@ @D.NeedNoun.ON@\" should pass:";
  fdt.insert_number(1);
  fdt.insert_number(6);
  fdt.insert_number(4);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == false);
  fdt.reset();
  std::cout
    << "\"@P.NeedNoun.ON@ @P.BlaBla.ON@ @C.NeedNoun@ @D.NeedNoun.ON@\""
    << " should pass:";
  fdt.insert_number(1);
  fdt.insert_number(7);
  fdt.insert_number(6);
  fdt.insert_number(4);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == false);
  fdt.reset();
  std::cout
    << "\"@P.NeedNoun.ON@ @P.BlaBla.ON@ @C.NeedNoun@ @D.NeedNoun.ON@ "
    << "@R.BlaBla.ON@\" should pass:";
  fdt.insert_number(1);
  fdt.insert_number(7);
  fdt.insert_number(6);
  fdt.insert_number(4);
  fdt.insert_number(8);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == false);
  fdt.reset();
  std::cout
    << "\"@P.NeedNoun.ON@ @C.NeedNoun@ @D.NeedNoun.ON@ @R.BlaBla.ON@\""
    << " should fail:";
  fdt.insert_number(1);
  fdt.insert_number(6);
  fdt.insert_number(4);
  fdt.insert_number(8);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == true);
  fdt.reset();
  std::cout << "\"@U.NeedNoun.ON@\" should pass:";
  fdt.insert_number(5);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == false);
  fdt.reset();
  std::cout << "\"@P.NeedNoun.ON@ @U.NeedNoun.ON@\" should pass:";
  fdt.insert_number(1);
  fdt.insert_number(5);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == false);
  fdt.reset();
  std::cout << "\"@N.NeedNoun.ON@ @U.NeedNoun.ON@\" should fail:";
  fdt.insert_number(2);
  fdt.insert_number(5);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == true);
  fdt.reset();
  std::cout << "\"@N.NeedNoun.foo@ @U.NeedNoun.ON@\" should pass:";
  fdt.insert_number(9);
  fdt.insert_number(5);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == false);
  fdt.reset();
  std::cout << "\"@N.NeedNoun.ON@ @C.NeedNoun@ @U.NeedNoun.ON@\""
        << " should pass:";
  fdt.insert_number(2);
  fdt.insert_number(6);
  fdt.insert_number(5);
  std::cout << " " << ! fdt.fails() << std::endl;
  assert(fdt.fails() == false);
  fdt.reset();
  short kv[3] = {2,6,5};
  KeyVector * key_vector = new KeyVector(kv,kv+3);
  assert(fdt.is_valid_string(key_vector));
  key_vector = fdt.filter_diacritics(key_vector);
  assert(key_vector->empty());
  delete key_vector;
  short kv1[2] = {2,5};
  KeyVector * key_vector1 = new KeyVector(kv1,kv1+2);
  key_vector1 = fdt.filter_diacritics(key_vector1);
  assert(key_vector1 == NULL);
  short kv2[4] = {2,6,100,5};
  KeyVector * key_vector2 = new KeyVector(kv2,kv2+4);
  assert(fdt.is_valid_string(key_vector2));
  key_vector2 = fdt.filter_diacritics(key_vector2);
  assert(key_vector2->size() == 1);
  assert(key_vector2->at(0) == 100);
  delete key_vector2;
}
#endif


