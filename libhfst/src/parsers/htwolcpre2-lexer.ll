%option 8Bit batch yylineno noyywrap nounput prefix="htwolcpre2"

%{
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
  
  // Autotools stuff
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


// The parser. Supplies flex with its symbol tables.
#include "htwolcpre2-parser.hh"

#include "../HfstExceptionDefs.h"

extern void htwolcpre2error(const char*);

#undef YY_FATAL_ERROR
#define YY_FATAL_ERROR(msg) htwolcpre2error(msg);

  // input_defs.h declares the inputHandler, which is
  // an object that overrides flex' default input reading.
  //#include "io_src/input_defs.h"
#include "io_src/InputReader.h"
extern InputReader htwolcpre2_input_reader;
#undef YY_INPUT
#define YY_INPUT( buf, result, max_size ) { \
char c = htwolcpre2_input_reader.input(); \
if (c == 0) { \
result = YY_NULL; \
} \
 else { \
buf[0] = c; \
result = 1; \
} \
}

  // non_alphabet_symbol_queue is used to store the grammar symbols which are
  // not located in the Alphabet section of the grammar.
  #include "HfstTwolcDefs.h"
  extern HandyDeque<std::string> htwolcpre2_non_alphabet_symbol_queue;

  // alphabet_symbol_queue is used to store the symbols in the Alphabet section
  // of the grammar.
  extern HandyDeque<std::string> htwolcpre2_alphabet_symbol_queue;

  // Tells whether the Alphabet section ended. It ends when the first
  // __HFST_TWOLC_; is seen.
  bool alphabet_ended = false;

namespace hfst {
namespace twolcpre2 {
void reset_lexer()
{
  alphabet_ended = false;
}
}}

%}

%%

__HFST_TWOLC_Alphabet {
  // Alphabet declaration.
  htwolcpre2_alphabet_symbol_queue.push_back("__HFST_TWOLC_Alphabet");
  return ALPHABET_DECLARATION;
}

__HFST_TWOLC_Diacritics {
  // Diacritics declaration.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_Diacritics");
  return DIACRITICS_DECLARATION;
}
__HFST_TWOLC_Definitions {
  // Definitions declaration.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_Definitions");
  return DEFINITION_DECLARATION;
}
__HFST_TWOLC_Sets {
  // Sets declaration.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_Sets");
  return SETS_DECLARATION;
}
__HFST_TWOLC_Rules {
  // Rules declaration.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_Rules");
  return RULES_DECLARATION;
}
__HFST_TWOLC_except {
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_except");
  return EXCEPT;
}

__HFST_TWOLC_KILL_SYMBOL {
  // Signifies a syntax error in the first compilation phase.
  // Just die quietly, since syntax error msgs have been issued by
  // the first compilation phase.
  HFST_THROW(HfstException);
}
[ ] { /* space: ignore */ }
__HFST_TWOLC_RULE_NAME=\"[^\"]+\" {
  // Rule name.
  htwolcpre2_non_alphabet_symbol_queue.push_back(htwolcpre2text);
  return RULE_NAME;
}
__HFST_TWOLC_[*] {
  // Kleene star operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_*");
  return STAR;
}
__HFST_TWOLC_[+] {
  // Kleene plus operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_+");
  return PLUS;
}
__HFST_TWOLC_[/] {
  // Freely insert operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_/");
  return FREELY_INSERT;
}
__HFST_TWOLC_[~] {
  // Complement operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_~");
  return COMPLEMENT;
}
__HFST_TWOLC_[\\] {
  // Term complement operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_\\");
  return TERM_COMPLEMENT;
}
__HFST_TWOLC_[$][.] {
  // Containment once operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_$.");
  return CONTAINMENT_ONCE;
}
__HFST_TWOLC_[$] {
  // Containment at least once operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_$");
  return CONTAINMENT;
}
__HFST_TWOLC_[?] {
  // Any symbol.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_?");
  return QUESTION_MARK;
}
__HFST_TWOLC_[0] {
  if (alphabet_ended)
    { htwolcpre2_non_alphabet_symbol_queue.push_back(htwolcpre2text); }
  else
    { htwolcpre2_alphabet_symbol_queue.push_back(htwolcpre2text); }
  return SYMBOL;
}
__HFST_TWOLC_[|] {
  // Or.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_|");
  return UNION;
}
__HFST_TWOLC_[&] {
  // And.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_&");
  return INTERSECTION;
}
__HFST_TWOLC_[\^] {
  // Power operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_^");
  return POWER;
}
__HFST_TWOLC_[\-] {
  // Difference operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_-");
  return DIFFERENCE;
}
__HFST_TWOLC_NUMBER=[0-9]+,[0-9]+ {
  // Number.
  htwolcpre2_non_alphabet_symbol_queue.push_back(htwolcpre2text);
  return NUMBER;
}
__HFST_TWOLC_NUMBER=[0-9]+ {
  // Number.
  htwolcpre2_non_alphabet_symbol_queue.push_back(htwolcpre2text);
  return NUMBER;
}
__HFST_TWOLC_[.][#][.] {
  // Word boundary.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_.#.");
  return SYMBOL;
}
__HFST_TWOLC_\[ {
  // Beginning of a bracketed regex.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_[");
  return LEFT_SQUARE_BRACKET;
}
__HFST_TWOLC_\] {
  // End of a bracketed regex.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_]");
  return RIGHT_SQUARE_BRACKET;
}
__HFST_TWOLC_\[\[ {
  // Beginning of a bracketed regex.
  //
  // For some bizarre reason [[ can't occur in the string-literal here...
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_[" "[");
  return RE_LEFT_SQUARE_BRACKET;
}
__HFST_TWOLC_\]\] {
  // End of a bracketed regex.
  //
  // For some bizarre reason ]] can't occur in the string-literal here...
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_]" "]");
  return RE_RIGHT_SQUARE_BRACKET;
}
__HFST_TWOLC_\( {
  // Beginning of an optional bracketed regex.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_(");
  return LEFT_PARENTHESIS;
}
__HFST_TWOLC_\) {
  // End of an optional bracketed regex.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_)");
  return RIGHT_PARENTHESIS;
}
__HFST_TWOLC_[/][<][=] {
  // Restriction rule operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_/<=");
  return LEFT_RESTRICTION_ARROW;
}
__HFST_TWOLC_[<][=] {
  // Left rule operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_<=");
  return LEFT_ARROW;
}
__HFST_TWOLC_[=][>] {
  // Right rule operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_=>");
  return RIGHT_ARROW;
}
__HFST_TWOLC_[<][=][>] {
  // Equivalence rule operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_<=>");
  return LEFT_RIGHT_ARROW;
}
__HFST_TWOLC_[/][<][=][=] {
  // Restriction rule operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_/<==");
  return RE_LEFT_RESTRICTION_ARROW;
}
__HFST_TWOLC_[<][=][=] {
  // Left rule operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_<==");
  return RE_LEFT_ARROW;
}
__HFST_TWOLC_[=][=][>] {
  // Right rule operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_==>");
  return RE_RIGHT_ARROW;
}
__HFST_TWOLC_[<][=][=][>] {
  // Equivalence rule operator.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_<==>");
  return RE_LEFT_RIGHT_ARROW;
}
__HFST_TWOLC_[:] {
  if (alphabet_ended)
    { htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_:"); }
  else
    { htwolcpre2_alphabet_symbol_queue.push_back("__HFST_TWOLC_:"); }
  // Pair separator in expressions like "[a:]".
  return PAIR_SEPARATOR;
}
__HFST_TWOLC_[;] {
  // End-of-line symbol.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_;");
  // The first __HFST_TWOLC_; ends the alphabet.
  alphabet_ended = true;
  return SEMI_COLON;
}
__HFST_TWOLC_[=] {
  // Equals sign used when defining sets and making definitions.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC_=");
  return EQUALS;
}
__HFST_TWOLC__ {
  // Center-marker in a rule context.
  htwolcpre2_non_alphabet_symbol_queue.push_back("__HFST_TWOLC__");
  return CENTER_MARKER;
}
__HFST_TWOLC_DIE {
  // If this symbol is seen, pass it on and exit quietly.
  //std::cout << "__HFST_TWOLC_DIE";
  HFST_THROW(HfstException);
}
__HFST_TWOLC_SET_NAME=[^ ]+ {
  htwolcpre2_non_alphabet_symbol_queue.push_back(htwolcpre2text);
  return SET_NAME;
}
__HFST_TWOLC_DEFINITION_NAME=[^ ]+ {
  htwolcpre2_non_alphabet_symbol_queue.push_back(htwolcpre2text);
  return DEFINITION_NAME;
}
[^ ]+ {
  if (alphabet_ended)
    { 
        htwolcpre2_non_alphabet_symbol_queue.push_back(htwolcpre2text); 
    }
  else
    { htwolcpre2_alphabet_symbol_queue.push_back(htwolcpre2text); }
  return SYMBOL;
}

%%
/* vim: set ft=lex: */
