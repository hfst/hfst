//! @file LeftArrowRuleContainer.h
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

#include "LeftArrowRuleContainer.h"

bool LeftArrowRuleContainer::resolve_left_arrow_conflicts = false;
bool LeftArrowRuleContainer::report_left_arrow_conflicts = false;

void LeftArrowRuleContainer::set_resolve_left_arrow_conflicts(bool option)
{ resolve_left_arrow_conflicts = option; }

void LeftArrowRuleContainer::set_report_left_arrow_conflicts(bool option)
{ report_left_arrow_conflicts = option; }

void LeftArrowRuleContainer::add_rule_and_display_and_resolve_conflicts
(ConflictResolvingLeftArrowRule * rule,std::ostream &out)
{
  std::string input = rule->input_symbol;
  if (input_to_rule_map.has_key(input))
    {
      for (auto & it : input_to_rule_map[input])
    {
      StringVector conflicting_context;
      if (it->conflicts_this(*rule,conflicting_context))
        {
          if (report_left_arrow_conflicts)
        {
          out << "There is a <=-rule conflict between "
              << Rule::get_print_name(it->name) << " and "
              << Rule::get_print_name(rule->name) << "."
              << std::endl
              << "E.g. in context ";
          bool diamond_seen = false;
          for (auto symbol_pair : conflicting_context)
            {
              symbol_pair = replace_substr
            (symbol_pair,TWOLC_EPSILON,"");
              if (symbol_pair ==
              "__HFST_TWOLC_DIAMOND:__HFST_TWOLC_DIAMOND")
            {
              if (diamond_seen)
                { continue; }
              symbol_pair = "_";
              diamond_seen = true;
            }
              else if
            (symbol_pair ==
             "@_TWOLC_IDENTITY_SYMBOL_@:@_TWOLC_IDENTITY_SYMBOL_@")
            { symbol_pair = "?"; }
              
              out << symbol_pair << " ";
            }
          out << std::endl;
        }
          if (resolve_left_arrow_conflicts)
        {
          if (it->resolvable_conflict(*rule))
            {
              if (report_left_arrow_conflicts)
            {
              out << "Resolving the conflict by restricting the "
                  << "context of "
                  << Rule::get_print_name(it->name) << "."
                  << std::endl;
            }
              it->resolve_conflict(*rule);
            }
          else if (rule->resolvable_conflict(*it))
            {
              if (report_left_arrow_conflicts)
            {
              out << "Resolving the conflict by restricting the "
                  << "context of " << rule->name << "."
                  << std::endl;
            }
              rule->resolve_conflict(*it);
            }
          else
            {
              if (report_left_arrow_conflicts)
            {
              out << "WARNING! The conflict is unresolvable."
                  << std::endl;
            }
            }
        }
          if (report_left_arrow_conflicts)
        {
          out << std::endl;
        }
        }
    }
    }
  input_to_rule_map[input].push_back(rule);
  rule_vector.push_back(rule);
}

#ifdef TEST_LEFT_ARROW_RULE_CONTAINER
#include <cassert>
int main(void)
{

}
#endif // TEST_LEFT_ARROW_RULE_CONTAINER
