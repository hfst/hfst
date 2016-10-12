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

#include <sstream>
#include "applicators.h"
#include "lookup-state.h"
#include "lookup-path.h"
#include "formatter.h"

//////////Function definitions for TokenizationApplicator

std::string
TokenizationApplicator::process_token(const Token& t) const
{
  std::ostringstream s;
  if(t.type == Symbol)
    s << "Symbol:        #" << t.symbol << "(" << transducer.get_alphabet().symbol_to_string(t.symbol) << ")";
  else if(t.type == Character)
    s << "Character:     '" << t.character << "'" << (token_stream.is_space(t)?" (space)":"");
  else if(t.type == Superblank)
    s << "Superblank:    " << token_stream.get_superblank(t.superblank_index);
  else if(t.type == ReservedCharacter)
    s << "Reserved char: '" << t.character << "'";
  else if(t.type == None)
    s << "None/EOF";

  s << " (to_symbol: " << token_stream.to_symbol(t) << ")"
    << " (alphabetic: " << token_stream.is_alphabetic(t) << ")";
  if(t.type == Symbol)
  {
    SymbolNumber symb=t.symbol;
    s << " (";
    if(transducer.get_alphabet().is_lower(symb))
    {
      SymbolNumber s2 = transducer.get_alphabet().to_upper(symb);
      s << "lowercase, upper: " << s2 << "/" << transducer.get_alphabet().symbol_to_string(s2);
    }
    else if(transducer.get_alphabet().is_upper(symb))
    {
      SymbolNumber s2 = transducer.get_alphabet().to_lower(symb);
      s << "uppercase, lower: " << s2 << "/" << transducer.get_alphabet().symbol_to_string(s2);
    }
    else
      s << "no case";
    s << ")";
  }
  return s.str();
}

void
TokenizationApplicator::apply()
{
  Token next_token;
  std::set<Token> alltokens;
  while((next_token=token_stream.get_token()).type != None)
  {
    std::cout << process_token(next_token) << std::endl;
    if(printDebuggingInformationFlag)
      alltokens.insert(next_token);
  }
  if(verboseFlag)
  {
    std::cout << "Set of tokens:" << std::endl;
    for(std::set<Token>::const_iterator it=alltokens.begin(); it!=alltokens.end(); it++)
      std::cout << process_token(*it) << std::endl;
  }
}


//////////Function definitions for AnalysisApplicator

void
AnalysisApplicator::apply()
{
  LookupState state(transducer);
  size_t last_stream_location = 0;
  TokenVector surface_form;
  ProcResult analyzed_forms;

  Token next_token;
  while((next_token = token_stream.get_token()).type != None)
  {
    if(printDebuggingInformationFlag)
    {
      if(next_token.type == Symbol)
        std::cout << "Got symbol #" << next_token.symbol << "(" << transducer.get_alphabet().symbol_to_string(next_token.symbol) << ")" << std::endl;
      else if(next_token.type == Character)
        std::cout << "Got non-symbolic character '" << next_token.character << "'" << (token_stream.is_space(next_token)?" (space)":"") << std::endl;
      else if(next_token.type == Superblank)
        std::cout << "Got superblank " << token_stream.get_superblank(next_token.superblank_index) << std::endl;
      else if(next_token.type == ReservedCharacter)
        std::cout << "Got reserved character '" << next_token.character << "'" << std::endl;
    }
    if(next_token.type == ReservedCharacter)
      stream_error(std::string("Found unexpected character ")+next_token.character+" unescaped in stream");

        if(surface_form.size() > 0 && state.is_final())
        {
          LookupPathSet finals = state.get_finals_set();
      if (caps_mode == DictionaryCase || caps_mode == CaseSensitiveDictionaryCase)
        {
              analyzed_forms = formatter.process_finals(finals,
                                                        Unknown);
        }
      else
        {
          analyzed_forms = formatter.process_finals(finals,
                                                                                token_stream.get_capitalization_state(surface_form));
        }
          last_stream_location = token_stream.get_pos()-1;

          if(printDebuggingInformationFlag)
            std::cout << "Final paths (" << finals.size() << ") found and saved, stream location is " << last_stream_location << std::endl;
        }

        state.step(token_stream.to_symbol(next_token), caps_mode);

    if(printDebuggingInformationFlag)
      std::cout << "After stepping, there are " << state.num_active() << " active paths" << std::endl;

    if(state.is_active())
    {
      surface_form.push_back(next_token);
    }
    else
    {
      if(surface_form.empty() && !token_stream.is_alphabetic(next_token))
      {
        if(formatter.preserve_nonalphabetic())
          token_stream << next_token;
      }
      else if(analyzed_forms.size() == 0 ||
              (token_stream.is_alphabetic(token_stream.at(last_stream_location)) &&
               token_stream.is_alphabetic(surface_form[surface_form.size()-1])))
      {
        // if this is false, then we need to move the token stream back far
        // enough that next_token will be read again next iteration
        bool next_token_is_part_of_word = false;
        if(token_stream.is_alphabetic(next_token))
        {
          next_token_is_part_of_word = true;
          do
          {
            surface_form.push_back(next_token);
          }
          while((next_token = token_stream.get_token()).type != None && token_stream.is_alphabetic(next_token));
          // we overstepped the word by one token
          token_stream.move_back(1);
        }

        if(!token_stream.is_alphabetic(surface_form[0]))
        {
          if(formatter.preserve_nonalphabetic())
            token_stream << surface_form[0];
          token_stream.move_back(surface_form.size()-1);
        }
        else
        {
          //size_t word_length = token_stream.first_nonalphabetic(surface_form);
          //if(word_length == string::npos)
          size_t word_length = surface_form.size();

          int revert_count = surface_form.size()-word_length+
                         next_token_is_part_of_word ? 0 : 1;

          if(printDebuggingInformationFlag)
            std::cout << "word_length=" << word_length << ", surface_form.size()=" << surface_form.size()
                      << ", moving back " << revert_count << " characters" << std::endl;

          formatter.print_unknown_word(TokenVector(surface_form.begin(),
                                                surface_form.begin()+word_length));
          token_stream.move_back(revert_count);
        }
      }
      else // there are one or more valid tranductions
      {
        // the number of symbols on the end of surface_form that aren't a part
        // of the transduction(s) found
        int revert_count = token_stream.diff_prev(last_stream_location+1);
        formatter.print_word(TokenVector(surface_form.begin(),
                                                surface_form.end()-revert_count),
                                   analyzed_forms);
        token_stream.move_back(revert_count+1);
      }

      state.reset();
      surface_form.clear();
      analyzed_forms.clear();
    }
  }

  if(verboseFlag)
    std::cout << std::endl << "Got None/EOF symbol; done." << std::endl;

  // print any valid transductions stored
  if(analyzed_forms.size() != 0)// && token_stream.get_pos() == last_stream_location)
    formatter.print_word(surface_form, analyzed_forms);
}


//////////Function definitions for GenerationApplicator

void
GenerationApplicator::apply()
{
  Token next_token;
  while((next_token = token_stream.get_token()).type != None)
  {
    if(next_token.type == ReservedCharacter)
    {
      if(next_token.character[0] == '^') // start of a word form to generate
      {
        char prefix_char = '\0';
        TokenVector form;
        while(true)
        {
          next_token = token_stream.get_token();
          if(next_token.type == ReservedCharacter && next_token.character[0] == '$')
            break;
          else if(token_stream.token_to_string(next_token) == "*" ||
                  token_stream.token_to_string(next_token) == "@" ||
                  token_stream.token_to_string(next_token) == "#")
          {
            prefix_char = token_stream.token_to_string(next_token)[0];
            break;
          }
          else if(next_token.type == None)
            stream_error("Stream ended before end-of-word marker $ was found");
          else
            form.push_back(next_token);
        }

        //Figure out how to output the word

        if(prefix_char == '*' || prefix_char == '@') // the word is unanalyzed (*) or untranslated (@)
        {
          if(mode != gm_clean)
            token_stream.write_escaped(std::string(1,prefix_char));

          std::string word = token_stream.read_delimited('$');

          if(prefix_char == '*' || (prefix_char == '@' && mode != gm_all))
          {
            size_t loc = word.find('<');
            if(loc != std::string::npos)
              word = word.substr(0,loc);
            else
              word = word.substr(0, word.length()-1); // no tags, but still remove the $
          }
          else
            word = word.substr(0, word.length()-1); // remove the $
          token_stream.write_escaped(word);
        }
        else
        {
          // first try unsplit

          if(!lookup(form, false))
            {
              std::vector<TokenVector> parts = split(form);
              for (std::vector<TokenVector>::const_iterator it=parts.begin();
                   it!=parts.end(); it++)
                {
                  lookup(*it, true);
                }
            }

          if(prefix_char == '#') // if there is an invariant part remaining
          {
            std::string invariant = token_stream.read_delimited('$');
            invariant = invariant.substr(0, invariant.length()-1);
            token_stream.write_escaped(invariant);
            if(printDebuggingInformationFlag)
              std::cout << std::endl << "Invariant: '" << invariant << "'" << std::endl;
          }
        }
      }
      else
        stream_error(std::string("Found unexpected character ")+next_token.character+" unescaped in stream");
    }
    else
      token_stream << next_token;
  }
}

std::vector<TokenVector>
GenerationApplicator::split(const TokenVector& tokens) const
{
  std::vector<TokenVector> res;
  TokenVector::const_iterator start = tokens.begin();
  for(TokenVector::const_iterator it=tokens.begin(); it!=tokens.end(); it++)
  {
    if(token_stream.token_to_string(*it) == "+" && it!=tokens.begin() &&
       transducer.get_alphabet().is_tag(token_stream.to_symbol(*(it-1))))
    {
      res.push_back(TokenVector(start, it));
      start = it+1;
    }
  }
  if(start != tokens.end())
    res.push_back(TokenVector(start, tokens.end()));

  return res;
}


LookupPathSet
GenerationApplicator::preprocess_finals(const LookupPathSet& finals) const
{
  LookupPathSet goodcmp_finals(LookupPathW::compare_weights);
  // insertion sort :)
  for (LookupPathSet::const_iterator it = finals.begin(); it != finals.end(); ++it)
  {
      goodcmp_finals.insert(*it);
  }
  
  // Keep only the N best weight classes
  int classes_found = -1;
  Weight last_weight_class = 0.0;
  LookupPathSet goodweight_finals(LookupPathW::compare_weights); 
  for(LookupPathSet::const_iterator it = goodcmp_finals.begin(); it != goodcmp_finals.end(); it++)
  {
          LookupPathW* pw = dynamic_cast<LookupPathW*>(*it);
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
    goodweight_finals.insert(*it);
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


bool
GenerationApplicator::lookup(const TokenVector& tokens, bool generate_on_fail)
{
  LookupState state(transducer);
  state.lookup(token_stream.to_symbols(tokens), caps_mode);
  if(state.is_final()) // generation succeeded
  {
    LookupPathSet finals = state.get_finals_set();
    CapitalizationState caps_change = Unknown;
    if(caps_mode!=DictionaryCase) {
        CapitalizationState lm_caps_state = token_stream.get_capitalization_state((*finals.begin())->get_output_symbols());
        CapitalizationState sf_caps_state = token_stream.get_capitalization_state(TokenVector(tokens.begin(),tokens.size()>1?tokens.begin()+2:tokens.end()));
        if(sf_caps_state != lm_caps_state) {
            // We *only* change capitalization if lm and sf caps state differs
            caps_change = sf_caps_state;
        }
    }

    if(printDebuggingInformationFlag)
            std::cout << "Generated " << finals.size() << " forms" << std::endl;

    LookupPathSet new_finals = preprocess_finals(finals);

    token_stream.put_symbols((*new_finals.begin())->get_output_symbols(),caps_change);
    if(new_finals.size() > 1)
    {
      LookupPathSet::const_iterator it=new_finals.begin();
      it++; // start from begin+1
      for(;it!=new_finals.end();it++)
      {
        token_stream.ostream() << '/';
        token_stream.put_symbols((*it)->get_output_symbols(),caps_change);
      }
    }
    return true;
  }
  else if (generate_on_fail) // no generations found
  {
    if(mode != gm_clean)
      token_stream.put_token(Token::as_reservedcharacter('#'));
    std::string word = token_stream.tokens_to_string(tokens, true); // get an unescaped string
    if(mode != gm_all) // strip apertium-style tags
    {
      size_t pos1;
      while((pos1 = word.find('<')) != std::string::npos)
      {
        size_t pos2 = word.find('>', pos1);
        if(pos2 != std::string::npos)
          word = word.substr(0,pos1)+word.substr(pos2+1);
        else
          stream_error("Found tag without closing '>'");
      }
    }
    token_stream.write_escaped(word);
    return true;
  }
  else
    {
      return false;
    }
  return false;
}
