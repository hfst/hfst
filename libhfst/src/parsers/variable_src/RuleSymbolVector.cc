//! @file RuleSymbolVector.cc
//!
//! @author Miikka Silfverberg

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

#include "RuleSymbolVector.h"

RuleSymbolVector::RuleSymbolVector(const VariableValueMap &vvm):
  vvm(vvm)
{}

std::string RuleSymbolVector::replace_variables(void)
{
  std::string result;
  for (auto symbol : *this)
    {
      if (symbol.find("__HFST_TWOLC_RULE_NAME") != std::string::npos)
    {
      if (! vvm.empty())
        {
          symbol.insert
        (symbol.size()-1,
         "__HFST_TWOLC_SPACE" "SUBCASE:");
        }
      for (const auto & it : vvm)
        { symbol.insert
        (symbol.size()-1,
         "__HFST_TWOLC_SPACE"+ it.first + "=" + it.second); }
    }
      result +=
    (vvm.has_key(symbol) ? vvm.get_value(symbol) : symbol) + " ";
    }
  return result;
}

std::string RuleSymbolVector::replace_variables(const RuleCenter &center)
{
  std::string result;
  for (auto symbol : *this)
    {
      if (symbol.find("__HFST_TWOLC_RULE_NAME") != std::string::npos)
    {
      if (! vvm.empty())
        {
          symbol.insert
        (symbol.size()-1,
         "__HFST_TWOLC_SPACE" "SUBCASE:");
        }
      for (const auto & it : vvm)
        { symbol.insert
        (symbol.size()-1,
         "__HFST_TWOLC_SPACE"+ it.first + "=" + it.second); }
    }
      else if (symbol == "__HFST_TWOLC_RULE_CENTER")
        { symbol = center.first + " __HFST_TWOLC_: " + center.second; }

      result +=
    (vvm.has_key(symbol) ? vvm.get_value(symbol) : symbol) + " ";
    }
  return result;
}

RuleSymbolVector &RuleSymbolVector::push_back(const std::string &s)
{
  std::vector<std::string>::push_back(s);
  return *this;
}

RuleSymbolVector &RuleSymbolVector::push_back
(const std::vector<std::string> &v)
{
  std::vector<std::string>::insert(std::vector<std::string>::end(),
                   v.begin(),v.end());
  return *this;
}
