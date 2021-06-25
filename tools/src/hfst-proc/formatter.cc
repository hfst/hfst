//       This program is free software: you can redistribute it and/or modify
//       it under the terms of the GNU General Public License as published by
//       the Free Software Foundation, version 3 of the License.
//
//       This program is distributed in the hope that it will be useful,
//       but WITHOUT ANY WARRANTY; without even the implied warranty of
//       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//       GNU General Public License for more details.
//
//       You should have received a copy of the GNU General Public License
//       along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <algorithm>
#include <sstream>
#include "formatter.h"

//////////Function definitions for OutputFormatter

TokenVector
OutputFormatter::clear_superblanks(const TokenVector& tokens) const
{
  TokenVector output_tokens;
  for(auto token : tokens)
  {
    if(token.type == Superblank)
      output_tokens.push_back(Token::as_symbol(token_stream.get_alphabet().get_blank_symbol()));
    else
      output_tokens.push_back(token);
  }
  return output_tokens;
}

void
OutputFormatter::filter_compound_analyses(LookupPathSet& finals) const
{
  int fewest_boundaries = std::numeric_limits<int>::max();
  std::vector<int> boundary_counts;

  // first look to find the analysis with the fewest compound boundaries
  for(auto final : finals)
  {
    int num = token_stream.get_alphabet().num_compound_boundaries(final->get_output_symbols());
    boundary_counts.push_back(num);
    if(num < fewest_boundaries)
      fewest_boundaries = num;
  }

  // filter all analyses with more boundaries than the minimum
  int i=0;
  for(LookupPathSet::iterator it=finals.begin(); it!=finals.end();i++)
  {
    if(boundary_counts[i] > fewest_boundaries)
    {
      LookupPathSet::iterator to_delete = it;
      it++;
      finals.erase(to_delete);
      if(printDebuggingInformationFlag)
        std::cout << "Filtering compound analysis with " << boundary_counts[i] << " boundary(s)" << std::endl;
    }
    else
      it++;
  }
}

LookupPathSet
OutputFormatter::preprocess_finals(const LookupPathSet& finals) const
{
  LookupPathSet goodcmp_finals(LookupPathW::compare_weights);
  // insertion sort :)
  for (auto final : finals)
  {
      goodcmp_finals.insert(final);
  }
  if(do_compound_filtering)
  {
    filter_compound_analyses(goodcmp_finals);
    if(printDebuggingInformationFlag)
    {
            if(goodcmp_finals.size() < finals.size()) {
        std::cout << "Filtered " << finals.size()-goodcmp_finals.size() << " compound analyses" << std::endl;
            }
    }
  }

  // Keep only the N best weight classes
  int classes_found = -1;
  Weight last_weight_class = 0.0;
  LookupPathSet goodweight_finals(LookupPathW::compare_weights);
  for(auto goodcmp_final : goodcmp_finals)
  {
          LookupPathW* pw = dynamic_cast<LookupPathW*>(goodcmp_final);
    if(pw != NULL) {
      Weight current_weight = pw->get_weight();
      if (classes_found == -1) // we're just starting
      {
        classes_found = 1;
        last_weight_class = current_weight;
      }
      else if (last_weight_class != current_weight) // we might want to ignore the rest due to weight classes
      {
        last_weight_class = current_weight;
        ++classes_found;
      }
      if (classes_found > maxWeightClasses)
      {
        break;
      }
    }
    goodweight_finals.insert(goodcmp_final);
  }
  // Keep no more than maxAnalyses
  LookupPathSet clipped_finals(LookupPathW::compare_weights);
  LookupPathSet::const_iterator it = goodweight_finals.begin();
  for(int i=0; i < maxAnalyses && it != goodweight_finals.end(); i++, it++)
  {
          clipped_finals.insert(*it);
  }
  return clipped_finals;
}

//////////Function definitions for TransliterateOutputFormatter

ProcResult
TransliterateOutputFormatter::process_finals(const LookupPathSet& finals, CapitalizationState caps) const
{
  ProcResult results;
  LookupPathSet new_finals = preprocess_finals(finals);

  for(auto new_final : new_finals)
  {
    std::ostringstream res;
    res << token_stream.get_alphabet().symbols_to_string(new_final->get_output_symbols(), caps);
    if(dynamic_cast<const LookupPathW*>(new_final) != NULL && displayWeightsFlag)
      res << '~' << dynamic_cast<const LookupPathW*>(new_final)->get_weight() << '~';

    results.push_back(res.str());
  }
  return results;
}

void
TransliterateOutputFormatter::print_word(const TokenVector& surface_form,
                                  ProcResult const &analyzed_forms) const
{
  // any superblanks in the surface form should not be printed as part of the
  // analysis output, but should be output directly afterwards
  TokenVector output_surface_form;
  std::vector<unsigned int> superblanks;
  for(auto it : surface_form)
  {
    if(it.type == Superblank)
    {
      output_surface_form.push_back(Token::as_symbol(token_stream.to_symbol(it)));
      superblanks.push_back(it.superblank_index);
    }
    else 
    {
      output_surface_form.push_back(it);
    }
  }

  if(printDebuggingInformationFlag) 
  {
    std::cout << "surface_form consists of " << output_surface_form.size() << " tokens" << std::endl;
  }

  bool first = true;
  for(const auto & analyzed_form : analyzed_forms)
  {
    if(!first) 
    {
      token_stream.ostream() << "/" ;
    }
    token_stream.ostream() << analyzed_form;
    first = false;
  }

  for(unsigned int superblank : superblanks)
  {
    token_stream.ostream() << token_stream.get_superblank(superblank);
  }
}
void
TransliterateOutputFormatter::print_unknown_word(const TokenVector& surface_form) const
{
  token_stream.write_escaped(surface_form);
}



//////////Function definitions for ApertiumOutputFormatter

ProcResult
ApertiumOutputFormatter::process_finals(const LookupPathSet& finals, CapitalizationState caps) const
{
  ProcResult results;
  LookupPathSet new_finals = preprocess_finals(finals);

  for(auto new_final : new_finals)
  {
    std::ostringstream res;
    res << token_stream.get_alphabet().symbols_to_string(new_final->get_output_symbols(), caps);
    if (dynamic_cast<const LookupPathW *>(new_final) != NULL && displayWeightsFlag) {
      res << '~' << dynamic_cast<const LookupPathW *>(new_final)->get_weight() << '~';
    }
    results.push_back(res.str());
  }
  return results;
}

////////

void
ApertiumOutputFormatter::print_word(const TokenVector& surface_form,
                                  ProcResult const &analyzed_forms) const
{
  // any superblanks in the surface form should not be printed as part of the
  // analysis output, but should be output directly afterwards
  TokenVector output_surface_form;
  std::vector<unsigned int> superblanks;
  for(auto it : surface_form)
  {
    if(it.type == Superblank)
    {
      output_surface_form.push_back(Token::as_symbol(token_stream.to_symbol(it)));
      superblanks.push_back(it.superblank_index);
    }
    else
      output_surface_form.push_back(it);
  }

  if(printDebuggingInformationFlag) {
    std::cout << "surface_form consists of " << output_surface_form.size() << " tokens" << std::endl;
  }

  token_stream.ostream() << '^';
  token_stream.write_escaped(output_surface_form);
  for(const auto & analyzed_form : analyzed_forms) {
    token_stream.ostream() << "/" << analyzed_form;
  }
  token_stream.ostream() << "$";

  for(unsigned int superblank : superblanks) {
    token_stream.ostream() << token_stream.get_superblank(superblank);
  }
}

void
ApertiumOutputFormatter::print_unknown_word(const TokenVector& surface_form) const
{
  token_stream.ostream() << '^';
  token_stream.write_escaped(surface_form);
  token_stream.ostream() << "/*";
  token_stream.write_escaped(surface_form);
  token_stream.ostream() << '$';
}


//////////Function definitions for CGOutputFormatter

std::string
CGOutputFormatter::process_final(const SymbolNumberVector& symbols, CapitalizationState caps) const
{
  std::ostringstream res;
  size_t start_pos = 0;

  res << '"'; // before start of lexical form

  while(start_pos < symbols.size())
  {
    size_t tag_start = symbols.size();
    size_t compound_split = symbols.size();
    for(size_t i=start_pos; i<symbols.size(); i++)
    {
      if(tag_start == symbols.size() &&
         token_stream.get_alphabet().is_tag(symbols[i]))
        tag_start = i;

      if(compound_split == symbols.size())
      {
        std::string s = token_stream.get_alphabet().symbol_to_string(symbols[i]);
        if(s == "#" || s == "+" || s[s.length()-1] == '+')
          compound_split = i;
      }

      if(tag_start != symbols.size() && compound_split != symbols.size())
        break;
    }

    // grab the base form without tags
    size_t end = (compound_split < tag_start ? compound_split : tag_start);
    res << token_stream.get_alphabet().symbols_to_string(
      SymbolNumberVector(symbols.begin()+start_pos,symbols.begin()+end), caps);

    // look for compounding. Don't output the tags for non-final segments
    if(compound_split != symbols.size())
    {
      res << "#";
      start_pos = compound_split+1;
    }
    else
    {
      res << '"'; // after end of lexical form, now come any tags
      if(tag_start != symbols.size())
      {
        for(size_t i=tag_start; i<symbols.size(); i++)
        {
          std::string tag = token_stream.get_alphabet().symbol_to_string(symbols[i]);
          // remove the < and > around Apertium tags
          if(tag.size() > 0 && tag[0] == '<')
          {
            tag = tag.substr(1);
            if(tag.size() > 0 && tag[tag.length()-1] == '>')
              tag = tag.substr(0,tag.length()-1);
          }
          // remove the + in front of GT/Divvun tags:
          if(tag.size() > 0 && tag[0] == '+')
          {
            tag = tag.substr(1);
          }

          res << (i==tag_start?"\t":" ") << tag;
        }
      }
      // When -r option is given, print the full analysis string as well
      // (as a specially prefixed tag):
      if(displayRawAnalysisInCG)
      {
        res << " " << "∏" << '"';
        res << token_stream.get_alphabet().symbols_to_string(
          SymbolNumberVector(symbols.begin()+0,symbols.begin()+symbols.size()), caps);
        res << '"';
      }

      break;
    }
  }

  return res.str();
}

ProcResult
CGOutputFormatter::process_finals(const LookupPathSet& finals, CapitalizationState caps) const
{
  ProcResult results;
  LookupPathSet new_finals = preprocess_finals(finals);

  for(auto new_final : new_finals)
    results.push_back(process_final(new_final->get_output_symbols(), caps));

  return results;
}

void
CGOutputFormatter::print_word(const TokenVector& surface_form,
                                ProcResult const &analyzed_forms) const
{
  token_stream.ostream() << "\"<"
                         << token_stream.tokens_to_string(clear_superblanks(surface_form))
                         << ">\"" << std::endl;

  for(const auto & analyzed_form : analyzed_forms)
    token_stream.ostream() << "\t" << analyzed_form << std::endl;
}

void
CGOutputFormatter::print_unknown_word(const TokenVector& surface_form) const
{
  std::string form = token_stream.tokens_to_string(clear_superblanks(surface_form));
  token_stream.ostream() << "\"<" << form << ">\"" << std::endl
                         << "\t\"*" << form << "\"" << std::endl;
}


//////////Function definitions for XeroxOutputFormatter

ProcResult
XeroxOutputFormatter::process_finals(const LookupPathSet& finals, CapitalizationState caps) const
{
  ProcResult results;
  LookupPathSet new_finals = preprocess_finals(finals);

  for(auto new_final : new_finals)
  {
    std::ostringstream res;
    res << token_stream.get_alphabet().symbols_to_string(new_final->get_output_symbols(), caps);
    if(dynamic_cast<const LookupPathW*>(new_final) != NULL && displayWeightsFlag)
      res << "\t" << dynamic_cast<const LookupPathW*>(new_final)->get_weight();

    results.push_back(res.str());
  }
  return results;
}

void
XeroxOutputFormatter::print_word(const TokenVector& surface_form,
                                ProcResult const &analyzed_forms) const
{
  std::string surface = token_stream.tokens_to_string(clear_superblanks(surface_form));

  for(const auto & analyzed_form : analyzed_forms)
    token_stream.ostream() << surface << "\t" << analyzed_form << std::endl;
  token_stream.ostream() << std::endl;
}

void
XeroxOutputFormatter::print_unknown_word(const TokenVector& surface_form) const
{
  token_stream.ostream() << token_stream.tokens_to_string(clear_superblanks(surface_form))
                         << "\t+?" << std::endl << std::endl;
}
