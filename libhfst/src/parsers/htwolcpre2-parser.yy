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

%{
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef WINDOWS
#include <io.h>
#endif


#include <iostream>
#include <fstream>
#include <cstdlib>
#include "io_src/InputReader.h"
#include "HfstTwolcDefs.h"
#include "grammar_defs.h"
#include "../HfstExceptionDefs.h"

  extern int htwolcpre2lineno;
  extern char * htwolcpre2text;
  extern int htwolcpre2lineno;
  extern char * htwolcpre2text;
  void htwolcpre2error(const char * text );
  int htwolcpre2lex();
  int htwolcpre2parse();

#define YYERROR_VERBOSE 1

  size_t htwolcpre2_line_number = 1;

  // For reading input one byte at a time.
  InputReader htwolcpre2_input_reader(htwolcpre2_line_number);

namespace hfst { namespace twolcpre2 {

  void set_input(std::istream & istr, const std::string& filename)
  {
    htwolcpre2_input_reader.set_input(istr, filename);
  }
  int parse()
  {
    return htwolcpre2parse();
  }
  void set_warning_stream(std::ostream & ostr)
  {
    htwolcpre2_input_reader.set_warning_stream(ostr);
  }
  void set_error_stream(std::ostream & ostr)
  {
    htwolcpre2_input_reader.set_error_stream(ostr);
  }

}}

%}

%name-prefix "htwolcpre2"

%union
{ int symbol_number; };


 /*
    All unary operators have stronger precedence than binary ones.
 */

 /* Binary operators ordered by precedence from lowest to highest. */
%left  <symbol_number> FREELY_INSERT
%left  <symbol_number> DIFFERENCE
%left  <symbol_number> INTERSECTION
%left  <symbol_number> UNION

 /* Unary operators ordered by precedence from lowest to highest. */
%right <symbol_number> STAR PLUS
%left  <symbol_number> CONTAINMENT CONTAINMENT_ONCE TERM_COMPLEMENT COMPLEMENT
%right <symbol_number> POWER

 /* "[", "]", "(" and ")". */
%right <symbol_number> RIGHT_SQUARE_BRACKET RIGHT_PARENTHESIS
%left  <symbol_number> LEFT_SQUARE_BRACKET LEFT_PARENTHESIS

 /* Twolc rule operators */
%token <symbol_number> LEFT_RESTRICTION_ARROW LEFT_ARROW RIGHT_ARROW
%token <symbol_number> LEFT_RIGHT_ARROW

 /* Twolc regular expression rule operators */
%token <symbol_number> RE_LEFT_RESTRICTION_ARROW RE_LEFT_ARROW RE_RIGHT_ARROW
%token <symbol_number> RE_LEFT_RIGHT_ARROW

 /* Twolc regular expression rule center brackets. */
%right <symbol_number> RE_RIGHT_SQUARE_BRACKET
%left  <symbol_number> RE_LEFT_SQUARE_BRACKET

 /* Basic tokens. */
%token <symbol_number>  ALPHABET_DECLARATION DIACRITICS_DECLARATION
%token <symbol_number>  SETS_DECLARATION DEFINITION_DECLARATION
%token <symbol_number>  RULES_DECLARATION PAIR_SEPARATOR SYMBOL SEMI_COLON
%token <symbol_number>  SET_NAME DEFINITION_NAME EQUALS CENTER_MARKER
%token <symbol_number>  RULE_NAME NUMBER QUESTION_MARK EXCEPT
%%

ALL: GRAMMAR {}
;

GRAMMAR: ALPHABET GRAMMAR1
| GRAMMAR1

GRAMMAR1: DIACRITICS GRAMMAR2
| GRAMMAR2

GRAMMAR2: SETS GRAMMAR3
| GRAMMAR3

GRAMMAR3: DEFINITIONS GRAMMAR4
| GRAMMAR4

GRAMMAR4: RULES

RULES:RULES_DECLARATION RULE_LIST

RULE_LIST: /* empty */
| RULE_LIST RULE

RULE: RULE_NAME_DECL RULE_CENTER RULE_OPERATOR RULE_CONTEXTS
NEGATIVE_RULE_CONTEXTS
| RULE_NAME_DECL RE_RULE_CENTER RE_RULE_OPERATOR RULE_CONTEXTS
NEGATIVE_RULE_CONTEXTS

RULE_NAME_DECL: RULE_NAME

RULE_CENTER: CENTER_PAIR
| RULE_CENTER UNION CENTER_PAIR
| LEFT_SQUARE_BRACKET CENTER_LIST RIGHT_SQUARE_BRACKET

RE_RULE_CENTER: RE_LEFT_SQUARE_BRACKET REGULAR_EXPRESSION RE_RIGHT_SQUARE_BRACKET

CENTER_LIST: CENTER_PAIR
| CENTER_LIST UNION CENTER_PAIR

CENTER_PAIR: CENTER_SYMBOL PAIR_SEPARATOR CENTER_SYMBOL

CENTER_SYMBOL: SYMBOL
| SET_NAME
| QUESTION_MARK

RULE_OPERATOR:LEFT_ARROW
| RIGHT_ARROW
| LEFT_RESTRICTION_ARROW
| LEFT_RIGHT_ARROW

RE_RULE_OPERATOR: RE_LEFT_ARROW
| RE_RIGHT_ARROW
| RE_LEFT_RESTRICTION_ARROW
| RE_LEFT_RIGHT_ARROW

RULE_CONTEXTS: /* empty */
| RULE_CONTEXTS RULE_CONTEXT

NEGATIVE_RULE_CONTEXTS: /* empty */
| EXCEPT RULE_CONTEXTS

RULE_CONTEXT: REGULAR_EXPRESSION CENTER_MARKER REGULAR_EXPRESSION
SEMI_COLON_LIST

ALPHABET: ALPHABET_DECLARATION ALPHABET_PAIR_LIST SEMI_COLON_LIST

DIACRITICS: DIACRITICS_DECLARATION SYMBOL_LIST SEMI_COLON_LIST

SETS: SETS_DECLARATION SET_LIST

DEFINITIONS: DEFINITION_DECLARATION DEFINITION_LIST

DEFINITION_LIST: /* empty */
| DEFINITION_LIST DEFINITION

DEFINITION: DEFINITION_NAME EQUALS REGULAR_EXPRESSION SEMI_COLON_LIST

REGULAR_EXPRESSION: RE_LIST
| REGULAR_EXPRESSION UNION RE_LIST
| REGULAR_EXPRESSION INTERSECTION RE_LIST
| REGULAR_EXPRESSION DIFFERENCE RE_LIST
| REGULAR_EXPRESSION FREELY_INSERT RE_LIST

RE_LIST: /* empty */
| RE_LIST RE

RE: PAIR
| RE POWER NUMBER
| RE STAR
| RE PLUS
| CONTAINMENT RE
| CONTAINMENT_ONCE RE
| COMPLEMENT RE
| TERM_COMPLEMENT RE
| LEFT_SQUARE_BRACKET REGULAR_EXPRESSION RIGHT_SQUARE_BRACKET
| LEFT_PARENTHESIS REGULAR_EXPRESSION RIGHT_PARENTHESIS

SET_LIST: /* empty */
| SET_LIST SET_DEFINITION

SYMBOL_LIST: /* empty */
| SYMBOL_LIST SYMBOL
| SYMBOL_LIST SET_NAME

SET_DEFINITION: SET_NAME EQUALS SYMBOL_LIST SEMI_COLON_LIST

ALPHABET_PAIR_LIST: /* empty */
| ALPHABET_PAIR_LIST ALPHABET_PAIR

PAIR: PAIR_SYMBOL PAIR_SEPARATOR PAIR_SYMBOL

PAIR_SYMBOL: SYMBOL
| SET_NAME
| DEFINITION_NAME
| QUESTION_MARK

ALPHABET_PAIR: SYMBOL PAIR_SEPARATOR SYMBOL

SEMI_COLON_LIST: SEMI_COLON
| SEMI_COLON_LIST SEMI_COLON

%%

// Print warning.
void htwolcpre2warn(const char * warning)
{ htwolcpre2_input_reader.warn(warning); }

// Print error messge and throw an exception.
void htwolcpre2error(const char * text)
{
  //std::cerr << text << std::endl;
  htwolcpre2_input_reader.error(text);
  HFST_THROW(HfstException);
}

void htwolcpre2_semantic_error(const char * text)
{
  htwolcpre2_input_reader.error(text); 
}

// non_alphabet_symbol_queue is used to store the grammar symbols which are
// not located in the Alphabet section of the grammar.
HandyDeque<std::string> htwolcpre2_non_alphabet_symbol_queue;

// alphabet_symbol_queue is used to store the symbols in the Alphabet section
// of the grammar.
HandyDeque<std::string> htwolcpre2_alphabet_symbol_queue;

// alphabet_symbol_queue is used to store the symbols in the Alphabet section
// of the grammar after it has been completed with all symbol pairs in the
// grammar.
HandyDeque<std::string> total_alphabet_symbol_queue;

namespace hfst 
{ 
namespace twolcpre2 
{

const HandyDeque<std::string> & get_total_alphabet_symbol_queue()
{
  return total_alphabet_symbol_queue;
}

const HandyDeque<std::string> & get_non_alphabet_symbol_queue()
{
  return htwolcpre2_non_alphabet_symbol_queue;
}

}
}

namespace hfst 
{ 
namespace twolcpre2 
{

  void reset_parser()
  {
    htwolcpre2_line_number = 1;
    htwolcpre2_input_reader.reset();
    htwolcpre2_non_alphabet_symbol_queue = HandyDeque<std::string>();
    htwolcpre2_alphabet_symbol_queue = HandyDeque<std::string>();
    total_alphabet_symbol_queue = HandyDeque<std::string>();
  }
}
}

void insert_alphabet_pairs(const HandyDeque<std::string> &symbol_queue,
			   HandySet<SymbolPair> &symbol_pair_set)
{
  for (HandyDeque<std::string>::const_iterator it = symbol_queue.begin();
       it != symbol_queue.end();
       ++it)
    {
      //If we found a symbol pair, we insert it into symbol_pair_set.
      if ((*it == "__HFST_TWOLC_0" ||
	   *it == "__HFST_TWOLC_.#." ||
	   *it == "__HFST_TWOLC_#" ||
	   *it == "__HFST_TWOLC_SPACE" ||
	   *it == "__HFST_TWOLC_TAB" ||
           it->find("__HFST_TWOLC_") == std::string::npos)
	  &&
	  *(it+1) == "__HFST_TWOLC_:"
	  &&
	  (*(it+2) == "__HFST_TWOLC_0" ||
	   *(it+2) == "__HFST_TWOLC_.#." ||
	   *(it+2) == "__HFST_TWOLC_#" ||
	   *(it+2) == "__HFST_TWOLC_SPACE" ||
	   *(it+2) == "__HFST_TWOLC_TAB" ||
           (it+2)->find("__HFST_TWOLC_") == std::string::npos))
	{
	  std::string input_symbol = *it == "__HFST_TWOLC_#" ? "#" : *it;
	  ++(++it);
	  std::string output_symbol = *it == "__HFST_TWOLC_#" ? "#" : *it;
	  symbol_pair_set.insert(SymbolPair(input_symbol,output_symbol));
	}
    }
  symbol_pair_set.insert(SymbolPair("__HFST_TWOLC_.#.","__HFST_TWOLC_.#."));
}

// Add all pairs in the grammar, which are missing from the Alphabet section,
// into the Alphabet section.
namespace hfst 
{
namespace twolcpre2
{

void complete_alphabet(void)
{
  HandySet<SymbolPair> symbol_pair_set;
  insert_alphabet_pairs(htwolcpre2_alphabet_symbol_queue,symbol_pair_set);
  insert_alphabet_pairs(htwolcpre2_non_alphabet_symbol_queue,symbol_pair_set);

  total_alphabet_symbol_queue.push_back("__HFST_TWOLC_Alphabet");
  for(HandySet<SymbolPair>::const_iterator it = symbol_pair_set.begin();
      it != symbol_pair_set.end();
      ++it)
    {
      total_alphabet_symbol_queue.push_back(it->first);
      total_alphabet_symbol_queue.push_back("__HFST_TWOLC_:");
      total_alphabet_symbol_queue.push_back(it->second);
    }
}

}
}
