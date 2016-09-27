%option 8Bit batch noyylineno noyywrap reentrant bison-bridge prefix="xre"

%{

// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include <string.h>

#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstXeroxRules.h"

#ifdef YACC_USE_PARSER_H_EXTENSION
  #include "xre_parse.h"
#else
  #include "xre_parse.hh"
#endif
#include "xre_utils.h"

#undef YY_INPUT
#define YY_INPUT(buf, retval, maxlen)   (retval = hfst::xre::getinput(buf, maxlen))

// These variablese are used when scanning a regex for a given SYMBOL
// when performing variable substitution in function definition.
namespace hfst {
  namespace xre {
    extern unsigned int cr; // number of characters read
    extern std::set<unsigned int> positions; // positions of a given SYMBOL
    extern char * position_symbol;  // the given SYMBOL
} }

// a macro that increments the number of characters read
#define CR hfst::xre::cr += (unsigned int)strlen(yytext);

extern int xrelex ( YYSTYPE * lvalp, yyscan_t scanner );

%}

/* c.f. Unicode Standard 5.1 D92 Table 3-7 */
U1 [\x20-\x7e]
U2 [\xc2-\xdf][\x80-\xbf]
U31 \xe0[\xa0-\xbf][\x80-\xbf]
U32 [\xe1-\xec][\x80-\xbf][\x80-\xbf]
U33 \xed[\x80-\x9f][\x80-\xbf]
U34 [\xee-\xef][\x80-\xbf][\x80-\xbf]
U41 \xf0[\x90-\xbf][\x80-\xbf][\x80-\xbf]
U42 [\xf1-\xf3][\x80-\xbf][\x80-\xbf][\x80-\xbf]
U43 \xf4[\x80-\x8f][\x80-\xbf][\x80-\xbf]
/* non US-ASCII */
U8H {U43}|{U42}|{U41}|{U34}|{U33}|{U32}|{U31}|{U2}
/* any UTF-8 */
U8C {U8H}|{U1}
/* Escaped */
EC "%"{U8C}

/* any ASCII */
A7 [\x00-\x7e]
/* special meaning in xre testing " */
A7RESTRICTED [- |<>%!,^:";@0~\\&?$+*/_(){}\]\[-]
/* non-restricted ASCII testing " */
A7UNRESTRICTED [\x21-\x7e]{-}[- |<>%!,^:";@0~\\&?$+*/_(){}\]\[-]

WEIGHT (-|\+)?[0-9]+(\.[0-9]+)?

/* token character */
NAME_CH {A7UNRESTRICTED}|{U8H}|{EC}
UINTEGER [1-9][0-9]*
INTEGER -?[1-9][0-9]*
WSP [\t ]
LWSP [\t\r\n ]

/* curly brackets */
BRACED      [{]([^}]|[\300-\337].|[\340-\357]..|[\360-\367]...)+[}]
%%




{BRACED} {
  CR;
  yylval->label = hfst::xre::strip_curly(yytext);
  return CURLY_BRACKETS;
}



"~"   { CR; return COMPLEMENT; }
"\\"  { CR; return TERM_COMPLEMENT; }
"&"   { CR; return INTERSECTION; }
"-"   { CR; return MINUS; }

"$."  { CR; return CONTAINMENT_ONCE; }
"$?"  { CR; return CONTAINMENT_OPT; }
"$"   { CR; return CONTAINMENT; }

"+"   { CR; return PLUS; }
"*"   { CR; return STAR; }

"./." { CR; return IGNORE_INTERNALLY; }
"/"   { CR; return IGNORING; }

"|"   { CR; return UNION; }

"<>"  { CR; return SHUFFLE; }
"<"   { CR; return BEFORE; }
">"   { CR; return AFTER; }

".o." { CR; return COMPOSITION; }
".O." { CR; return LENIENT_COMPOSITION; }
".m>." { CR; return MERGE_RIGHT_ARROW; }
".<m." { CR; return MERGE_LEFT_ARROW; }
".x." { CR; return CROSS_PRODUCT; }
".P." { CR; return UPPER_PRIORITY_UNION; }
".p." { CR; return LOWER_PRIORITY_UNION; }
".-u." { CR; return UPPER_MINUS; }
".-l." { CR; return LOWER_MINUS; }
"`" {   CR; return SUBSTITUTE_LEFT; }

"\\<=" { CR; return LEFT_RESTRICTION; }
"<=>" { CR; return LEFT_RIGHT_ARROW; }
"<=" { CR; return LEFT_ARROW; }
"=>" { CR; return RIGHT_ARROW; }

"->" { CR; return REPLACE_RIGHT; }
"(->)" { CR; return OPTIONAL_REPLACE_RIGHT; }
"<-" { CR; return REPLACE_LEFT; }
"(<-)" { CR; return OPTIONAL_REPLACE_LEFT; }
"<->" { CR; return REPLACE_LEFT_RIGHT; }
"(<->)" { CR; return OPTIONAL_REPLACE_LEFT_RIGHT; }
"@->" { CR; return LTR_LONGEST_MATCH; }
"@>" { CR; return LTR_SHORTEST_MATCH; }
"->@" { CR; return RTL_LONGEST_MATCH; }
">@" { CR; return RTL_SHORTEST_MATCH; }

"||" { CR; return REPLACE_CONTEXT_UU; }
"//" { CR; return REPLACE_CONTEXT_LU; }
"\\\\" { CR; return REPLACE_CONTEXT_UL; }
"\\/" { CR; return REPLACE_CONTEXT_LL; }
"_"+ { CR; return CENTER_MARKER; }
"..."+ { CR; return MARKUP_MARKER; }

"\\\\\\" { CR; return LEFT_QUOTIENT; }

"^"{WSP}*({UINTEGER}|"0")","({UINTEGER}|"0") {
    CR;
    yylval->values = hfst::xre::get_n_to_k(yytext);
    return CATENATE_N_TO_K;
}

"^{"({UINTEGER}|"0")","({UINTEGER}|"0")"}" {
    CR;
    yylval->values = hfst::xre::get_n_to_k(yytext);
    return CATENATE_N_TO_K;
}

"^>"{WSP}*({UINTEGER}|"0") {
    CR;
    yylval->value = strtol(yytext + 2, 0, 10);
    return CATENATE_N_PLUS;
}

"^<"{WSP}*({UINTEGER}|"0") {
    CR;
    yylval->value = strtol(yytext + 2, 0, 10);
    return CATENATE_N_MINUS;
}

"^"{WSP}*({UINTEGER}|"0")                  {
    CR;
    yylval->value = strtol(yytext + 1, 0, 10);
    return CATENATE_N;
}

".r" { CR; return REVERSE; }
".i" { CR; return INVERT; }
".u" { CR; return XRE_UPPER; }
".l" { CR; return XRE_LOWER; }

"@bin\""[^""]+"\""|"@\""[^""]+"\"" {
    CR;
    yylval->label = hfst::xre::get_quoted(yytext);
    return READ_BIN;
}

"@txt\""[^""]+"\"" {
    CR;
    yylval->label = hfst::xre::get_quoted(yytext);
    return READ_TEXT;
}

"@stxt\""[^""]+"\"" {
    CR;
    yylval->label = hfst::xre::get_quoted(yytext);
    return READ_SPACED;
}

"@pl\""[^""]+"\"" {
    CR;
    yylval->label = hfst::xre::get_quoted(yytext);
    return READ_PROLOG;
}

"@re\""[^""]+"\"" {
    CR;
    yylval->label = hfst::xre::get_quoted(yytext);
    return READ_RE;
}

"[.#.]" { hfst::xre::cr += 5; yylval->label = strdup(".#."); return SYMBOL; }
"[.#." { hfst::xre::cr += 1; unput('.'); unput('#'); unput('.'); return LEFT_BRACKET; }
".#.]" { hfst::xre::cr += 3; unput(']'); yylval->label = strdup(".#."); return SYMBOL; }
"[." { CR; return LEFT_BRACKET_DOTTED; }
".]" { CR; return RIGHT_BRACKET_DOTTED; }
"[" { CR; return LEFT_BRACKET; }
"]" { CR; return RIGHT_BRACKET; }
"(" { CR; return LEFT_PARENTHESIS; }
")" { CR; return RIGHT_PARENTHESIS; }

":" { CR; return PAIR_SEPARATOR; }

"::"{WEIGHT} {
    CR;
    yylval->weight = hfst::xre::get_weight(yytext + 2);
    return WEIGHT;
}

"\""[^""]+"\"" {
    unsigned int length = 0;
    yylval->label = hfst::xre::parse_quoted(yytext, length);
    CR;
    if (length < 2)
      return QUOTED_LITERAL;
    else
      return QUOTED_MULTICHAR_LITERAL;
}


",," { CR; return COMMACOMMA; }
"," { CR; return COMMA; }

"\"\"" { CR; return EPSILON_TOKEN; }
"0" { CR; return EPSILON_TOKEN; }
"[]" { CR; return EPSILON_TOKEN; }
"?" { CR; return ANY_TOKEN; }

"0"({NAME_CH}|"0")+ {
    if (hfst::xre::position_symbol != NULL) {
      if (strcmp(hfst::xre::position_symbol, yytext) == 0) {
        hfst::xre::positions.insert(hfst::xre::cr);
      }
    }
    yylval->label = hfst::xre::strip_percents(yytext);
    CR;
    return MULTICHAR_SYMBOL;
}

{NAME_CH} {
    if (hfst::xre::position_symbol != NULL) {
      if (strcmp(hfst::xre::position_symbol, yytext) == 0) {
        hfst::xre::positions.insert(hfst::xre::cr);
      }
    }
    yylval->label = hfst::xre::strip_percents(yytext);
    CR;
    return SYMBOL;
}

{NAME_CH}({NAME_CH}|"0")+ {
    if (hfst::xre::position_symbol != NULL) {
      if (strcmp(hfst::xre::position_symbol, yytext) == 0) {
        hfst::xre::positions.insert(hfst::xre::cr);
      }
    }
    yylval->label = hfst::xre::strip_percents(yytext);
    CR;
    return MULTICHAR_SYMBOL;
}


{NAME_CH}({NAME_CH}|"0")*"(" {
    CR;
    yylval->label = yytext;
    return FUNCTION_NAME;
}

"0"({NAME_CH}|"0")+"(" {
    CR;
    yylval->label = yytext;
    return FUNCTION_NAME;
}

";" {
    CR;
    return END_OF_EXPRESSION;
}

{LWSP}+ { hfst::xre::count_lines(yytext); /*fprintf(stderr, "ignoring whitespace '%s'..\n", yytext); */ /* ignorable whitespace */ }

("!"|"#")[^\n]*$ { CR; /* fprintf(stderr, "ignoring comment '%s'..\n", yytext); */ /* ignore comments */ }

("!"|"#")[^\n]* { CR; /* fprintf(stderr, "ignoring comment '%s'..\n", yytext); */ /* ignore comments */ }

. {
    CR;
    return LEXER_ERROR;
}

%%
