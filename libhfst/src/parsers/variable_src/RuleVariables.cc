//! @file RuleVariables.cc
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

#include "RuleVariables.h"
#include "RuleVariablesConstIterator.h"

void RuleVariables::set_variable(const std::string &var)
{
  VariableValues vv;
  vv.set_variable(var);
  current_variable_block.push_back(vv);
}

void RuleVariables::add_value(const std::string &value)
{
  if (current_variable_block.empty())
    { throw EmptyContainer(); }
  current_variable_block.back().push_back(value);
}

void RuleVariables::add_values(const std::vector<std::string> &values)
{
  if (current_variable_block.empty())
    { throw EmptyContainer(); }
  for (const auto & value : values)
    { add_value(value); }
}

void RuleVariables::set_matcher(Matcher matcher)
{
  switch (matcher)
    {
    case FREELY:
      freely_blocks.add_object(current_variable_block);
      break;
    case MATCHED:
      matched_blocks.add_object(current_variable_block);
      break;
    case MIXED:
      mixed_blocks.add_object(current_variable_block);
      break;
    }
  current_variable_block.clear();
}

RuleVariables::const_iterator RuleVariables::begin(void) const
{ return RuleVariables::const_iterator::begin(*this); }

RuleVariables::const_iterator RuleVariables::end(void) const
{ return RuleVariables::const_iterator::end(*this); }

void RuleVariables::clear(void)
{
  freely_blocks.clear();
  matched_blocks.clear();
  mixed_blocks.clear();
}

bool RuleVariables::empty(void) const
{
  return
    freely_blocks.begin() == freely_blocks.end() &&
    matched_blocks.begin() == matched_blocks.end() &&
    mixed_blocks.begin() == mixed_blocks.end();
}
