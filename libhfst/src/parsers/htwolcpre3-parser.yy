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
#include <unistd.h>
#include "io_src/InputReader.h"
#include "grammar_defs.h"
#include "HfstTwolcDefs.h"
#include "rule_src/TwolCGrammar.h"
#include "rule_src/OtherSymbolTransducer.h"
#include "alphabet_src/Alphabet.h"
#include "string_src/string_manipulation.h"
#include <cstdio>

#ifdef DEBUG_TWOLC_3_GRAMMAR
#define YYDEBUG 1
#endif

#define COLOUR_BOLD "\033[01m"
#define COLOUR_RED "\033[31m"
#define COLOUR_GREEN "\033[32m"
#define COLOUR_YELLOW "\033[33m"
#define COLOUR_BLUE "\033[34m"
#define COLOUR_MAGENTA "\033[35m"
#define COLOUR_CYAN "\033[36m"
#define COLOUR_RESET "\033[0m"

static bool
should_colourise()
{
    if (isatty(1))
    {
        return true;
    }
    else
    {
        return false;
    }
    return false;
}

  extern int htwolcpre3lineno;
  extern char * htwolcpre3text;
  extern int htwolcpre3lineno;
  extern char * htwolcpre3text;
  void htwolcpre3error(const char * text );
  void htwolcpre3_semantic_error(const char * text);
  void htwolcpre3warn(const char * warning );
  void htwolcpre3message(const std::string &m);
  int htwolcpre3lex();
  int htwolcpre3parse();

  bool silent_ = false;
  bool verbose_ = false;

namespace hfst {
  namespace twolcpre3 {
  void set_silent(bool val)
  {
    silent_ = val;
  }
  void set_verbose(bool val)
  {
    verbose_ = val;
  }
  int parse()
  {
    return htwolcpre3parse();
  }
  void message(const std::string &m)
  {
    return htwolcpre3message(m);
  }
}}

#define YYERROR_VERBOSE 1

  // For reading input one byte at a time.
  size_t htwolcpre3_line_number = 1;
  InputReader htwolcpre3_input_reader(htwolcpre3_line_number);

namespace hfst { namespace twolcpre3 {

  void set_input(std::istream & istr, const std::string& filename)
  {
    htwolcpre3_input_reader.set_input(istr);
  }
  void set_warning_stream(std::ostream & ostr)
  {
    htwolcpre3_input_reader.set_warning_stream(ostr);
  }
  void set_error_stream(std::ostream & ostr)
  {
    htwolcpre3_input_reader.set_error_stream(ostr);
  }

}}

#ifdef HAVE_XFSM
  #define Alphabet TwolCAlphabet
#endif

  // The Alphabet of the grammar
  Alphabet alphabet;

  // A map storing named regular expressions (i.e. definitions).
  HandyMap<std::string,OtherSymbolTransducer> definition_map;

  // The grammar, which compiles rules, resolves conflicts and stores
  // rule transducers.
  // TwolCGrammar grammar(true,true);
  TwolCGrammar * grammar;

namespace hfst {
  namespace twolcpre3 {
  void set_grammar(TwolCGrammar * grammar_)
  {
    grammar = grammar_;
  }
  TwolCGrammar * get_grammar()
  {
    return grammar;
  }

  void reset_parser()
  {
    htwolcpre3_line_number = 1;
    htwolcpre3_input_reader.reset();
    alphabet = Alphabet();
    definition_map = HandyMap<std::string,OtherSymbolTransducer>();
  }

}}

  unsigned int get_number(const std::string &);
  unsigned int get_second_number(const std::string &s);
  std::string get_name(const std::string &s);

%}

%name-prefix "htwolcpre3"

%union
{
  int symbol_number;
  OtherSymbolTransducer * regular_expression;
  char * value;
  SymbolRange * symbol_range;
  SymbolPairVector * symbol_pair_range;
  op::OPERATOR rule_operator;
};


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

 /* "[", "]", "(", ")". */
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
%token <symbol_number>  RULES_DECLARATION COLON SEMI_COLON
%token <symbol_number>  EQUALS CENTER_MARKER QUESTION_MARK EXCEPT
%token <value>          RULE_NAME SYMBOL DEFINITION_NAME NUMBER NUMBER_RANGE

%type<regular_expression> PAIR REGULAR_EXPRESSION RE_LIST RE RULE_CONTEXT
%type<regular_expression> RE_RULE_CENTER
%type<regular_expression> RULE_CONTEXTS NEGATIVE_RULE_CONTEXTS
%type<symbol_range>       SYMBOL_LIST
%type<symbol_pair_range>  CENTER_PAIR CENTER_LIST RULE_CENTER
%type<rule_operator>      RULE_OPERATOR RE_RULE_OPERATOR
%type<value>              CENTER_SYMBOL PAIR_SYMBOL

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

RULES:RULES_HEADER RULE_LIST

RULES_HEADER:RULES_DECLARATION
{ htwolcpre3message("Reading rules and compiling their contexts and centers."); }

RULE_LIST: /* empty */
| RULE_LIST RULE

RULE: RULE_NAME RULE_CENTER RULE_OPERATOR RULE_CONTEXTS NEGATIVE_RULE_CONTEXTS
{
  // Subtract negative contexts from positive contexts.
  $4->apply(&HfstTransducer::subtract,*$5);

  if (verbose_)
    { htwolcpre3message(std::string("Processing: ")+ get_name($1)); }

  if ($2->size() == 1)
    { grammar->add_rule($1,(*$2)[0],$3,OtherSymbolTransducerVector(1,*$4)); }
  else
    { grammar->add_rule($1,*$2,$3,OtherSymbolTransducerVector(1,*$4)); }
  free($1);
  delete $2;
  delete $4;
  delete $5;
}
| RULE_NAME RE_RULE_CENTER RE_RULE_OPERATOR RULE_CONTEXTS
NEGATIVE_RULE_CONTEXTS
{
  // Subtract negative contexts from positive contexts.
  $4->apply(&HfstTransducer::subtract,*$5);

  grammar->add_rule($1,*$2,$3,OtherSymbolTransducerVector(1,*$4));
  free($1);
  delete $2;
  delete $4;
  delete $5;
}

RULE_CENTER: CENTER_PAIR
{ $$ = $1; }
| RULE_CENTER UNION CENTER_PAIR
{
  $$ = $1;
  $$->push_back(*$3->begin());
  delete $3;
}
| LEFT_SQUARE_BRACKET CENTER_LIST RIGHT_SQUARE_BRACKET
{ $$ = $2; }

RE_RULE_CENTER: RE_LEFT_SQUARE_BRACKET REGULAR_EXPRESSION RE_RIGHT_SQUARE_BRACKET
{ $$ = $2; }

CENTER_LIST: CENTER_PAIR
{ $$ = $1; }
| CENTER_LIST UNION CENTER_PAIR
{
  $$ = $1;
  $$->push_back(*$3->begin());
  delete $3;
}

CENTER_PAIR: CENTER_SYMBOL COLON CENTER_SYMBOL
{
  $$ = alphabet.get_symbol_pair_vector(SymbolPair($1,$3));
  free($1); free($3);
}

CENTER_SYMBOL: SYMBOL
{ $$ = $1; }
| QUESTION_MARK
{ $$ = string_copy("__HFST_TWOLC_?"); }

RULE_OPERATOR:LEFT_ARROW
{ $$ = op::LEFT; }
| RIGHT_ARROW
{ $$ = op::RIGHT; }
| LEFT_RESTRICTION_ARROW
{ $$ = op::NOT_LEFT; }
| LEFT_RIGHT_ARROW
{ $$ = op::LEFT_RIGHT; }

RE_RULE_OPERATOR: RE_LEFT_ARROW
{ $$ = op::RE_LEFT; }
| RE_RIGHT_ARROW
{ $$ = op::RE_RIGHT; }
| RE_LEFT_RESTRICTION_ARROW
{ $$ = op::RE_NOT_LEFT; }
| RE_LEFT_RIGHT_ARROW
{ $$ = op::RE_LEFT_RIGHT; }

RULE_CONTEXTS: /* empty */
{ $$ = new OtherSymbolTransducer(); }
| RULE_CONTEXTS RULE_CONTEXT
{
  $$ = &$1->apply(&HfstTransducer::disjunct,*$2);
  delete $2;
}

NEGATIVE_RULE_CONTEXTS: /* empty */
{ $$ = new OtherSymbolTransducer(); }
| EXCEPT RULE_CONTEXTS
{ $$ = $2; }

RULE_CONTEXT: REGULAR_EXPRESSION CENTER_MARKER REGULAR_EXPRESSION
SEMI_COLON_LIST
{
  $$ = new OtherSymbolTransducer(OtherSymbolTransducer::get_context(*$1,*$3));
  delete $1;
  delete $3;
}

ALPHABET: ALPHABET_HEADER ALPHABET_PAIR_LIST SEMI_COLON_LIST
{ alphabet.alphabet_done(); }

ALPHABET_HEADER:ALPHABET_DECLARATION
{ htwolcpre3message("Reading alphabet."); }

DIACRITICS: DIACRITICS_HEADER SYMBOL_LIST SEMI_COLON_LIST
{
  grammar->define_diacritics(*$2);
  alphabet.define_diacritics(*$2);
  delete $2;
}

DIACRITICS_HEADER:DIACRITICS_DECLARATION
{ htwolcpre3message("Reading diacritics."); }

SETS: SETS_HEADER SET_LIST

SETS_HEADER:SETS_DECLARATION
{ htwolcpre3message("Reading sets."); }

DEFINITIONS: DEFINITION_HEADER DEFINITION_LIST

DEFINITION_HEADER:DEFINITION_DECLARATION
{ htwolcpre3message("Reading and compiling definitions."); }

DEFINITION_LIST: /* empty */
| DEFINITION_LIST DEFINITION

DEFINITION: DEFINITION_NAME EQUALS REGULAR_EXPRESSION SEMI_COLON_LIST
{
  definition_map[$1] = *$3;
  free($1);
  delete $3;
}

REGULAR_EXPRESSION: RE_LIST
{ $$ = $1; }
| REGULAR_EXPRESSION UNION RE_LIST
{
  $$ = &$1->apply(&HfstTransducer::disjunct,*$3);
  delete $3;
}
| REGULAR_EXPRESSION INTERSECTION RE_LIST
{
  $$ = &$1->apply(&HfstTransducer::intersect,*$3);
  delete $3;
}
| REGULAR_EXPRESSION DIFFERENCE RE_LIST
{
  $$ = &$1->apply(&HfstTransducer::subtract,*$3);
  delete $3;
}

RE_LIST: /* empty */
{ $$ = new OtherSymbolTransducer(HFST_EPSILON); }
| RE_LIST RE
{
  $$ = &$1->apply(&HfstTransducer::concatenate,*$2);
  delete $2;
}

RE: PAIR
{ $$ = $1; }
| RE POWER NUMBER
{
  $$ = &$1->apply(&HfstTransducer::repeat_n,get_number($3));
  free($3);
}
| RE POWER NUMBER_RANGE
{
  $$ = &$1->apply(&HfstTransducer::repeat_n_to_k,
          get_number($3),get_second_number($3));
  free($3);
}
| RE STAR
{ $$ = &$1->apply(&HfstTransducer::repeat_star); }
| RE PLUS
{ $$ = &$1->apply(&HfstTransducer::repeat_plus); }
| CONTAINMENT RE
{ $$ = &$2->contained(); }
| CONTAINMENT_ONCE RE
{ $$ = &$2->contained_once(); }
| COMPLEMENT RE
{ $$ = &$2->negated(); }
| TERM_COMPLEMENT RE
{ $$ = &$2->term_complemented(); }
| LEFT_SQUARE_BRACKET REGULAR_EXPRESSION RIGHT_SQUARE_BRACKET
{ $$ = $2; }
| LEFT_PARENTHESIS REGULAR_EXPRESSION RIGHT_PARENTHESIS
{ $$ = &$2->apply(&HfstTransducer::optionalize); }
| RE FREELY_INSERT RE
{
  $1->apply
    (&HfstTransducer::insert_freely,
     SymbolPair(TWOLC_FREELY_INSERT,TWOLC_FREELY_INSERT),
     true);
  $1->apply
    (&HfstTransducer::substitute,
     SymbolPair(TWOLC_FREELY_INSERT,TWOLC_FREELY_INSERT),
     *$3,
     true);
  $$ = $1;

  delete $3;
}

SET_LIST: /* empty */
| SET_LIST SET_DEFINITION

SYMBOL_LIST: /* empty */
{ $$ = new SymbolRange; }
| SYMBOL_LIST SYMBOL
{
  $1->push_back($2);
  $$ = $1;
  free($2);
}

SET_DEFINITION: SYMBOL EQUALS SYMBOL_LIST SEMI_COLON_LIST
{
  alphabet.define_set($1,*$3);
  free($1);
  delete $3;
}

ALPHABET_PAIR_LIST: /* empty */
| ALPHABET_PAIR_LIST ALPHABET_PAIR

PAIR: PAIR_SYMBOL COLON PAIR_SYMBOL
{
  if (std::string("__HFST_TWOLC_0") == $1 &&
      std::string("__HFST_TWOLC_0") == $3)
    { $$ = new OtherSymbolTransducer(HFST_EPSILON); }
  else if (std::string("__HFST_TWOLC_#") == $1)
    {
      // __HFST_TWOLC_# corresponds to the #-symbol in grammars. It should
      // be split into an absolute word boundary "__HFST_TWOLC_.#." and a
      // relative word boundary "#". On the surface the relative word-boundary
      // may correspond to whatever it did correspond to in the rule file.
      // The absolute word boundary surface realization is treated the same as
      // the surface realizations of other absolute word boundaries.
      OtherSymbolTransducer wb =
    alphabet.get_transducer(SymbolPair
                ("__HFST_TWOLC_.#.","__HFST_TWOLC_.#."));
      OtherSymbolTransducer alt_wb =
    alphabet.get_transducer
    (SymbolPair
     ("#",$3 == std::string("__HFST_TWOLC_#") ? "#" : $3));
      wb.apply(&HfstTransducer::disjunct,alt_wb);

      $$ = new OtherSymbolTransducer(wb);
    }
  else
    { $$ = new OtherSymbolTransducer
    (alphabet.get_transducer(SymbolPair($1,$3)));
      if (alphabet.is_empty_pair(SymbolPair($1,$3)))
    {
      std::string error;
      if (std::string($1) == std::string($3))
        {
          std::string symbol = Rule::get_print_name($1);

          error = std::string("The pair set ") + symbol + " is empty.";
        }
      else
        {
          std::string symbol1 = Rule::get_print_name($1);
          std::string symbol2 = Rule::get_print_name($3);

          error = std::string("The pair set ") + symbol1 + ":" + symbol2 +
        " is empty.";
        }
      error +=  std::string("\n\n") +

        "Note that a pair set X:Y can be empty, even if the sets X and\n"+
            "Y are non-empty, since every symbol pair has to be declared in\n"+
        "the alphabet or it has to be the center of a rule, or be in\n"  +
        "the context of a rule or result from substituting values for\n" +
        "variables in a rule with variables.\n\n" +

        "Compilation is terminated because a rule context, definition\n" +
        "or rule center becomes empty.\n\n";

      htwolcpre3_semantic_error(error.c_str());
    }
    }

  free($1);
  free($3);
}
| DEFINITION_NAME COLON DEFINITION_NAME
{
  $$ = new OtherSymbolTransducer(definition_map[$1]);
  free($1);
  free($3);
}

PAIR_SYMBOL: SYMBOL
{ $$ = $1; }
| QUESTION_MARK
{ $$ = string_copy("__HFST_TWOLC_?"); }

ALPHABET_PAIR: SYMBOL COLON SYMBOL
{
  alphabet.define_alphabet_pair(SymbolPair($1,$3));
  free($1);
  free($3);
}

SEMI_COLON_LIST: SEMI_COLON
| SEMI_COLON_LIST SEMI_COLON

%%

// Print warning.
void htwolcpre3warn(const char * warning)
{
#ifdef DEBUG_TWOLC_3_GRAMMAR
  std::cerr << warning << std::endl;
#endif
  htwolcpre3_input_reader.warn(warning);
}

// Print error messge and throw an exception
void htwolcpre3error(const char * text)
{
  htwolcpre3_input_reader.error(text);
  HFST_THROW(HfstException);
}

// Print error messge and exit 1.
void htwolcpre3_semantic_error(const char * text)
{
  htwolcpre3_input_reader.error(text);
  if (should_colourise()) 
  {
    std::cerr << COLOUR_RED << "Error: " << COLOUR_RESET << text << std::endl;
  }
  else
  {
    std::cerr << std::endl << "Error: " << text << std::endl;
  }
  HFST_THROW(HfstException);
}

unsigned int get_number(const std::string &s)
{
  // skip yhe prefix "HFST_TWOLC_NUMBER=".
  std::istringstream i(s.substr(20));
  unsigned int number;
  i >> number;
  return number;
}

unsigned int get_second_number(const std::string &s)
{
  // skip yhe prefix "HFST_TWOLC_NUMBER=".
  std::istringstream i(s.substr(s.find(',')+1));
  unsigned int number;
  i >> number;
  return number;
}

void htwolcpre3message(const std::string &m)
{
  if (! silent_)
    { std::cerr << m << std::endl; }
}

std::string get_name(const std::string &s)
{
  std::string ss = s;
  size_t pos = 0;
  while ((pos = ss.find("__HFST_TWOLC_RULE_NAME=")) != std::string::npos)
    { ss.replace(pos,std::string("__HFST_TWOLC_RULE_NAME=").size(),""); }
  while ((pos = ss.find("__HFST_TWOLC_SPACE")) != std::string::npos)
    { ss.replace(pos,std::string("__HFST_TWOLC_SPACE").size()," "); }
  return ss;
}

