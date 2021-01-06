%option 8Bit batch noyywrap prefix="hxfst"

%{
// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

//! @file xfst-lexer.ll
//!
//! @brief a lexer for xfst
//!
//! @author Tommi A. Pirinen

#if HAVE_CONFIG_H
#  include <config.h>
#endif

namespace hfst {
  class HfstTransducer;
}

#include "xfst-parser.hh"
#include "xfst-utils.h"
#include "XfstCompiler.h"

#include <assert.h>

#include "HfstDataTypes.h"

extern void hxfsterror(const char *text);

#undef YY_FATAL_ERROR
#define YY_FATAL_ERROR(msg) hxfsterror(msg)

int source_stack_size = 0;

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
/* special meaning in xfst */
A7RESTRICTED [ <>()\[\]%!;:""]
/* non-restricted ASCII */
A7UNRESTRICTED [\x21-\x7e]{-}[ <>%()\[\]!;:""]
/* special meaning in xre */
XRERESTRICTED [- <>%!:;@0~\\&?$+*/_{}\]\[-]
/* non-restricted ASCII in XRE */
XREUNRESTRICTED [\x21-\x7e]{-}[- <>%!:;@0~\\&?$+*/_{}\]\[-]
/* keywords */
KEYWORDS "read"|"regex"|"save"|"write"|"stack"

/* RegExp. stuff */
XREALPHA {XREUNRESTRICTED}|{U8H}|{EC}
XREOPERATOR [~\\&-/ |+*$_\]\[{}()0?:""]|"@\""|"$."|"$?"|"./."|"<>"|".#."|"."[riul]
XRECHAR {XREALPHA}+|{XREOPERATOR}
XRETOKEN {XRECHAR}+

/* String entry part:
 * - any unrestricted ASCII-7,
 * - any high unicode codepoint (past U+007F) UTF-8-encoded
 * - any %-escaped UTF-8
 */
NAMECHAR {A7UNRESTRICTED}|{U8H}|{EC}
NAMETOKEN {NAMECHAR}{NAMECHAR}*
PROTOTYPE {NAMETOKEN}"("[a-zA-Z_0-9 ,]*")"
RANGE {NAMECHAR}"-"{NAMECHAR}

/* White space */
WSP [\t ]
LWSP [\t ]*

%x REGEX_STATE
%x SOURCE_STATE
%x APPLY_STATE
%%

^{LWSP}("add properties"|"add") {
    return ADD_PROPS;
}

^{LWSP}("apply up"|"up"){LWSP}(""|"\r")$ {
    if (hfst::xfst::xfst_->getReadInteractiveTextFromStdin())
    {
        // let XfstCompiler take care of the input to apply up command
        return APPLY_UP;
    }
    else
    {
        // read input to apply up command
        BEGIN(APPLY_STATE);
        return APPLY_UP;
    }
}

^{LWSP}("apply up"){WSP}.* {
    hxfstlval.text = hfst::xfst::strstrip(hxfsttext + strlen("apply up "));
    return APPLY_UP_SINGLE;
}

^{LWSP}("up"){WSP}.* {
    hxfstlval.text = hfst::xfst::strstrip(hxfsttext + strlen("up "));
    return APPLY_UP_SINGLE;
}

^{LWSP}("apply down"|"down"){LWSP}(""|"\r")$ {
    if (hfst::xfst::xfst_->getReadInteractiveTextFromStdin())
    {
        // let XfstCompiler take care of the input to apply down command
        return APPLY_DOWN;
    }
    else
    {
        // read input to apply down command
        BEGIN(APPLY_STATE);
        return APPLY_DOWN;
    }
}

^{LWSP}("apply down"){WSP}.* {
    hxfstlval.text = hfst::xfst::strstrip(hxfsttext + strlen("apply down "));
    return APPLY_DOWN_SINGLE;
}

^{LWSP}("down"){WSP}.* {
    hxfstlval.text = hfst::xfst::strstrip(hxfsttext + strlen("down "));
    return APPLY_DOWN_SINGLE;
}

^{LWSP}("apply med"|"med") {
    return APPLY_MED;
}

^{LWSP}("ambiguous upper"|"ambiguous") {
    return AMBIGUOUS;
}

^{LWSP}"alias" {
    return DEFINE_ALIAS;
}

^{LWSP}"apropos"{WSP}+.* {
    hxfstlval.text = hfst::xfst::strstrip(hxfsttext + strlen("apropos "));
    return APROPOS;
}

^{LWSP}"apropos"{WSP}* {
    hxfstlval.text = strdup("");
    return APROPOS;
}

^{LWSP}"assert" {
    return ASSERT;
}

^{LWSP}("cleanup net"|"cleanup") {
    return CLEANUP;
}

^{LWSP}("clear"|"clear stack") {
    return CLEAR;
}

^{LWSP}("collect epsilon-loops"|"epsilon-loops") {
    return COLLECT_EPSILON_LOOPS;
}

^{LWSP}"compact sigma" {
    return COMPACT_SIGMA;
}

^{LWSP}("compile-replace lower"|"com-rep lower") {
    return COMPILE_REPLACE_LOWER;
}

^{LWSP}("compile-replace upper"|"com-rep upper") {
    return COMPILE_REPLACE_UPPER;
}

^{LWSP}("complete net"|"complete") {
    return COMPLETE;
}

^{LWSP}("compose net"|"compose") {
    return COMPOSE;
}

^{LWSP}("concatenate net"|"concatenate") {
    return CONCATENATE;
}

^{LWSP}("lookup-optimize"|"lookup-optimise") {
    return LOOKUP_OPTIMIZE;
}

^{LWSP}("remove-optimization"|"remove-optimisation") {
    return REMOVE_OPTIMIZATION;
}


^{LWSP}("crossproduct net"|"crossproduct") {
    return CROSSPRODUCT;
}

"define"{WSP}+{NAMETOKEN}{LWSP}";" {
    char * dup = strdup(yytext);
    dup[strlen(dup)-1] = ' '; // get rid of the ';'
    hxfstlval.name = hfst::xfst::strstrip(dup+strlen("define "));
    free(dup);
    return DEFINE_NAME;
}

"define"{WSP}+{NAMETOKEN}{LWSP}(""|"\r")$ {
    hxfstlval.name = hfst::xfst::strstrip(yytext+strlen("define "));
    return DEFINE_NAME;
}

"define"{WSP}+{NAMETOKEN}{PROTOTYPE} {
    BEGIN(REGEX_STATE);
    hxfstlval.name = hfst::xfst::strstrip(yytext+strlen("define "));
    return DEFINE_FUNCTION;
}

"define"{WSP}+{NAMETOKEN} {
    BEGIN(REGEX_STATE);
    hxfstlval.name = hfst::xfst::strstrip(yytext+strlen("define "));
    return DEFINE_NAME;
}

^{LWSP}("determinize net"|"determinize"|"determinise net"|"determinise") {
    return DETERMINIZE;
}

"echo"{WSP}+.* {
    hxfstlval.text = hfst::xfst::strstrip(hxfsttext + strlen("echo "));
    return ECHO_;
}

^{LWSP}"echo"{WSP}* {
    hxfstlval.text = strdup("");
    return ECHO_;
}

^{LWSP}("edit properties"|"edit") {
    return EDIT_PROPS;
}

^{LWSP}"eliminate flag" {
    return ELIMINATE_FLAG;
}

^{LWSP}"eliminate flags" {
    return ELIMINATE_ALL;
}

^{LWSP}("epsilon-remove net"|"epsilon-remove") {
    return EPSILON_REMOVE;
}

^{LWSP}"extract ambiguous" {
    return EXTRACT_AMBIGUOUS;
}

^{LWSP}"extract unambiguous" {
    return EXTRACT_UNAMBIGUOUS;
}

"help"{WSP}+.* {
    hxfstlval.text = hfst::xfst::strstrip(hxfsttext + strlen("help "));
    return DESCRIBE;
}

^{LWSP}("help"|"apropos"){WSP}* {
    hxfstlval.text = strdup("");
    return DESCRIBE;
}

"hfst"{WSP}*.*$ {
    hxfstlval.text = strdup(hxfsttext);
    return HFST;
}

^{LWSP}("ignore net"|"ignore") {
    return XFST_IGNORE;
}

^{LWSP}("intersect net"|"intersect"|"conjunct") {
    return INTERSECT;
}

^{LWSP}("inspect"|"inspect net") {
    return INSPECT;
}

^{LWSP}("invert net"|"invert") {
    return INVERT;
}

^{LWSP}("label net") {
    return LABEL_NET;
}

^{LWSP}("list") {
    return LIST;
}

^{LWSP}("load defined"|"loadd") {
    return LOADD;
}

^{LWSP}("load"|"load stack") {
    return LOADS;
}

^{LWSP}("lower-side net"|"lower-side") {
    return LOWER_SIDE;
}

^{LWSP}("minimize net"|"minimize"|"minimise") {
    return MINIMIZE;
}

^{LWSP}("minus net"|"minus"|"subtract") {
    return MINUS;
}

^{LWSP}("name net"|"name") {
    return NAME;
}

^{LWSP}("negate net"|"negate") {
    return NEGATE;
}

^{LWSP}("one-plus net"|"one-plus") {
    return ONE_PLUS;
}

^{LWSP}("pop"|"pop stack") {
    return POP;
}

^{LWSP}("print aliases"|"aliases") {
    return PRINT_ALIASES;
}
^{LWSP}("print arc-tally"|"arc-tally") {
    return PRINT_ARCCOUNT;
}
^{LWSP}("print defined"|"pdefined") {
    return PRINT_DEFINED;
}
^{LWSP}("write definition"|"wdef") {
    return SAVE_DEFINITION;
}

^{LWSP}("write definitions"|"wdefs") {
    return SAVE_DEFINITIONS;
}

^{LWSP}("print directory"|"directory") {
    return PRINT_DIR;
}

^{LWSP}("write dot"|"wdot"|"dot") {
    return SAVE_DOT;
}
^{LWSP}("write att"|"att") {
    return WRITE_ATT;
}

^{LWSP}("print file-info"|"file-info") {
    return PRINT_FILE_INFO;
}
^{LWSP}("print flags"|"flags") {
    return PRINT_FLAGS;
}
^{LWSP}("print labels"|"labels") {
    return PRINT_LABELS;
}
^{LWSP}("print label-maps"|"label-maps") {
    return PRINT_LABELMAPS;
}
^{LWSP}("print label-tally"|"label-tally") {
    return PRINT_LABEL_COUNT;
}
^{LWSP}"print list" {
    return PRINT_LIST;
}
^{LWSP}"print lists" {
    return PRINT_LISTS;
}
^{LWSP}("print longest-string"|"longest-string"|"pls") {
    return PRINT_LONGEST_STRING;
}
^{LWSP}("print longest-string-size"|"longest-string-size"|"plz") {
    return PRINT_LONGEST_STRING_SIZE;
}
^{LWSP}("print lower-words"|"lower-words") {
    return PRINT_LOWER_WORDS;
}
^{LWSP}("print name"|"pname") {
    return PRINT_NAME;
}
^{LWSP}("print net") {
    return PRINT;
}
^{LWSP}("print"|"write")" "("properties"|"props") {
    return PRINT_PROPS;
}
^{LWSP}("print random-lower"|"random-lower") {
    return PRINT_RANDOM_LOWER;
}
^{LWSP}("print random-upper"|"random-upper") {
    return PRINT_RANDOM_UPPER;
}
^{LWSP}("print random-words"|"random-words") {
    return PRINT_RANDOM_WORDS;
}
^{LWSP}("print shortest-string-"("size"|"length")|"shortest-string-size"|"psz") {
    return PRINT_SHORTEST_STRING_SIZE;
}
^{LWSP}("print shortest-string"|"shortest-string"|"pss") {
    return PRINT_SHORTEST_STRING;
}
^{LWSP}("print sigma"|"sigma") {
    return PRINT_SIGMA;
}
^{LWSP}("print sigma-tally"|"sigma-tally"|"sitally") {
    return PRINT_SIGMA_COUNT;
}
^{LWSP}"print sigma-word-tally" {
    return PRINT_SIGMA_WORD_COUNT;
}
^{LWSP}("print size"|"size") {
    return PRINT_SIZE;
}
^{LWSP}("print stack"|"stack") {
    return PRINT_STACK;
}
^{LWSP}("print upper-words"|"upper-words") {
    return PRINT_UPPER_WORDS;
}

^{LWSP}("print words"|"words") {
    return PRINT_WORDS;
}
^{LWSP}("prune net"|"prune") {
    return PRUNE_NET;
}


^{LWSP}("push"|"push defined") {
    return PUSH_DEFINED;
}

^{LWSP}("quit"|"exit"|"bye"|"stop"|"hyvästi"|"au revoir"|"näkemiin"|"viszlát"|"auf wiedersehen"|"has") {
    hxfstlval.name = strdup("");
    return QUIT;
}

^{LWSP}("lexc"|"read lexc") {
    return READ_LEXC;
}

^{LWSP}("att"|"read att") {
    return READ_ATT;
}

^{LWSP}("read properties"|"rprops") {
    return READ_PROPS;
}
^{LWSP}("read prolog"|"rpl") {
    return READ_PROLOG;
}
^{LWSP}("regex"|"read regex") {
    BEGIN(REGEX_STATE);
    return READ_REGEX;
}
^{LWSP}("rs"|"read spaced-text") {
    return READ_SPACED;
}
^{LWSP}("rt"|"read text") {
    return READ_TEXT;
}

^{LWSP}("reverse net"|"reverse") {
    return REVERSE;
}

^{LWSP}("rotate"|"rotate stack") {
    return ROTATE;
}

^{LWSP}("save defined"|"saved") {
    return SAVE_DEFINITIONS;
}

^{LWSP}("save stack"|"save"|"ss") {
    return SAVE_STACK;
}

^{LWSP}"set" {
    return SET;
}

^{LWSP}"show variables" {
    return SHOW_ALL;
}

^{LWSP}("show variable"|"show") {
    return SHOW;
}

^{LWSP}("shuffle net"|"shuffle") {
    return SHUFFLE;
}

^{LWSP}"sigma net" {
    return SIGMA;
}

^{LWSP}("sort net"|"sort") {
    return SORT;
}

^{LWSP}"source" {
    BEGIN(SOURCE_STATE);
}

^{LWSP}"substitute defined" {
    return SUBSTITUTE_NAMED;
}

"for" {
    return FOR;
}

^{LWSP}"substitute label" {
    return SUBSTITUTE_LABEL;
}

^{LWSP}"substitute symbol" {
    return SUBSTITUTE_SYMBOL;
}

^{LWSP}("substring net"|"substring") {
    return SUBSTRING;
}

"system"{WSP}+.*$ {
    hxfstlval.text = strdup(yytext+7);
    return SYSTEM;
}

"test equivalent"|"equivalent"|"te" {
    return TEST_EQ;
}
"test functional"|"functional"|"tf" {
    return TEST_FUNCT;
}
"test identity"|"identity"|"ti" {
    return TEST_ID;
}
"test infinitely ambiguous"|"infinitely ambiguous"|"test infinitely-ambiguous"|"infinitely-ambiguous"|"tia" {
    return TEST_INFINITELY_AMBIGUOUS;
}
"test lower-bounded"|"lower-bounded"|"tlb" {
    return TEST_LOWER_BOUNDED;
}
"test lower-universal"|"lower-universal"|"tlu" {
    return TEST_LOWER_UNI;
}
"test non-null"|"tnn" {
    return TEST_NONNULL;
}
"test null"|"tnu" {
    return TEST_NULL;
}
"test overlap"|"overlap"|"to" {
    return TEST_OVERLAP;
}
"test sublanguage"|"sublanguage"|"ts" {
    return TEST_SUBLANGUAGE;
}
"test upper-bounded"|"upper-bounded"|"tub" {
    return TEST_UPPER_BOUNDED;
}
"test upper-universal"|"upper-universal"|"tuu" {
    return TEST_UPPER_UNI;
}
"test unambiguous"|"tu" {
    return TEST_UNAMBIGUOUS;
}

^{LWSP}("turn"|"turn stack") {
    return TURN;
}

^{LWSP}("twosided flag-diacritics"|"tfd") {
    return TWOSIDED_FLAGS;
}

^{LWSP}"undefine" {
    return UNDEFINE;
}

^{LWSP}"unlist" {
    return UNLIST;
}

^{LWSP}("union net"|"union"|"disjunct") {
    return UNION;
}

^{LWSP}("upper-side net"|"upper-side") {
    return UPPER_SIDE;
}

^{LWSP}"view net" {
    return VIEW;
}

^{LWSP}("wpl"|"write prolog") {
    return SAVE_PROLOG;
}
^{LWSP}("wspaced-text"|"write spaced-text") {
    return SAVE_SPACED;
}
^{LWSP}("wt"|"write text") {
    return SAVE_TEXT;
}

^{LWSP}("zero-plus net"|"zero-plus") {
    return ZERO_PLUS;
}

<APPLY_STATE>(.|"\n"|"\r")* {
    // ^ to return input to apply up/down command, first read everything

    BEGIN(0);

    // search for a special end string
    std::string str(hxfsttext);
    std::size_t end_found = str.find("<ctrl-d>");

    // CASE 1: no end string found: the rest is input to apply
    if (end_found == std::string::npos) {
        hxfstlval.text = strdup(hxfsttext);
        return APPLY_INPUT;
    }

    // CASE 2: there are other commands after the input to apply
    unsigned int total_length = (unsigned int)strlen(hxfsttext);
    unsigned int endpos = (unsigned int)end_found;

    // copy the input to apply and set is as return value
    char * buf = (char*) malloc(endpos + 1);
    for (unsigned int i=0; i < endpos; i++)
    {
      buf[i] = hxfsttext[i];
    }
    buf[endpos] = '\0';
    hxfstlval.text = strdup(buf);
    free(buf);

    // put back the rest of the input text, excluding the "<ctrl-d>"
    if (total_length > 0)
    {
      // unput modifies hxfsttext, so it must be copied before unputting
      char * text_read = strdup(hxfsttext);
      // unputting must be done in reverse order
      for(unsigned int i=total_length-1;
          i >= (endpos + (unsigned int)strlen("<ctrl-d>"));
          i--)
      {
        unput(*(text_read+i));
      }
      free(text_read);
    }

    return APPLY_INPUT;
}


<REGEX_STATE>(.|"\n"|"\r")* {
    // ^ to return input to regex command, first read everything

    BEGIN(0);

    unsigned int chars_read = 0;
    unsigned int total_length = (unsigned int)strlen(hxfsttext);

    // compile regex to find out where it ends
    // as a positive side effect, the regex is also conveniently compiled
    // into a transducer which is stored in XfstCompiler::latest_regex_compiled
    (void) hfst::xfst::xfst_->compile_regex(hxfsttext, chars_read);

    // copy the input to regex and set is as return value
    char * buf = (char*) malloc(chars_read+1);
    for (unsigned int i=0; i < chars_read; i++)
    {
      buf[i] = hxfsttext[i];
    }
    buf[chars_read] = '\0';
    hxfstlval.text = strdup(buf);
    free(buf);

    // put back the rest of the input text
    if (total_length > 0)
    {
      // unput modifies hxfsttext, so it must be copied before unputting
      char * text_read = strdup(hxfsttext);
      // unputting must be done in reverse order
      for(unsigned int i=total_length-1; i >= chars_read; i--)
      {
        unput(*(text_read+i));
      }
      free(text_read);
    }
   
    return REGEX;
}

<SOURCE_STATE>[A-Za-z\/]{NAMECHAR}* {
  // ^ include directive

  FILE * tmp = NULL;
  if ((tmp = hfst::hfst_fopen(hfst::xfst::strstrip(hxfsttext), "r" )) != NULL)
  {
    //printf("Opening file '%s'.\n", hfst::xfst::strstrip(hxfsttext));
    // push the included text onto the lexer stack
    hxfstpush_buffer_state(hxfst_create_buffer(tmp, 32000));
    ++source_stack_size;
  }
  else
  {
    char buffer [1024];
    sprintf(buffer, "Error opening file '%s'\n",hfst::xfst::strstrip(hxfsttext));
    hxfsterror(buffer);
  }
  BEGIN(INITIAL);
}

">"{WSP}*{NAMETOKEN} {
    hxfstlval.file = hfst::xfst::strstrip(hxfsttext+1);
    return REDIRECT_OUT;
}
"<"{WSP}*{NAMETOKEN} {
    hxfstlval.file = hfst::xfst::strstrip(hxfsttext+1);
    return REDIRECT_IN;
}

{RANGE} {
    char* range = hfst::xfst::strstrip(hxfsttext);
    char* s = range;
    hxfstlval.list = static_cast<char**>(malloc(sizeof(char*)*2));
    char* p = hxfstlval.list[0];
    while (*s != '-')
    {
        *p = *s;
        p++;
        s++;
    }
    *p = '\0';
    p = hxfstlval.list[1];
    s++;
    while (*s != '\0')
    {
        *p = *s;
        p++;
        s++;
    }
    *p = '\0';
    free(range);
    return RANGE;
}

"("[a-zA-Z_0-9 ,]*")" {
    hxfstlval.name = strdup(hxfsttext);
    return PROTOTYPE;
}

"(" {
    return LEFT_PAREN;
}
";" {
    return SEMICOLON;
}
":" {
    return COLON;
}
"END;"|"END" {
    return END_SUB;
}
{NAMETOKEN} {
    hxfstlval.name = strdup(hxfsttext);
    return NAMETOKEN;
}

[\x04] {
    return CTRLD;
}

[\n\r]     { /* skip newline */ }
[\t ]*     { /* skip whitespace */ }
"!"[^\n]*  { /* skip comments */ }
"#"[^\n]*  { /* skip comments */ }
"#"\n      { /* skip an empty comment */ }
"#"+\n     { /* skip an empty comment */ }
"!"\n      { /* skip an empty comment */ }
"!"+\n     { /* skip an empty comment */ }

<<EOF>> {
    --source_stack_size;
    // end of input
    if (source_stack_size < 0) {
      yyterminate();
    }
    // EOF encountered because reaching end of included input
    else {
      hxfstpop_buffer_state();
    }
}

[\x80-\xff] {
    hxfsterror("Illegal 8-bit sequence (cannot form valid UTF-8)");
    return XFST_ERROR;
}

. {
    hxfsterror("Syntax error in lexer (no valid token found at the point)");
    return XFST_ERROR;
}


