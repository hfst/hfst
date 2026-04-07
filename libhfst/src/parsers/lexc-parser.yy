%{
// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

//! @file lexc-parser.yy
//!
//! @brief A parser for lexc
//!
//! @author Tommi A. Pirinen

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cstdlib>
#include <cstdio>

#include <string>
using std::string;

#include "LexcCompiler.h"
#include "lexc-utils.h"

// obligatory yacc stuff
extern int hlexclineno;
void hlexcerror(const char *text);
int hlexclex(void);

// Actual functions to handle parsed stuff
static
void
upgrade_alphabets()
{
    hfst::lexc::lexc_->setStrictAlphabets(true);
}

static
void
handle_multichar(const string& multichar)
{
    //in multichars, both @ZERO@ ("%0") and literal 0 ("0") are allowed
    hfst::lexc::lexc_->addAlphabet(multichar);

    string str = std::string(multichar);
    string zero = "@ZERO@";
    size_t start_pos = str.find(zero);
    if (start_pos != std::string::npos) {
        str.replace(start_pos, zero.length(), "0");
    }
    hfst::lexc::lexc_->addAlphabet(str);

    str = std::string(multichar);
    zero = "0";
    start_pos = str.find(zero);
    if (start_pos != std::string::npos) {
        str.replace(start_pos, zero.length(), "@ZERO@");
    }
    hfst::lexc::lexc_->addAlphabet(str);
}

static
void
handle_noflag(const string& lexname)
{
    //fprintf(stderr, "DEBUG: Adding %s to noflags\n", lexname.c_str());
    std::ostream * err = hfst::lexc::lexc_->get_stream((hfst::lexc::lexc_->get_error_stream()));
    *err << "DEBUG: Adding " << lexname << " to noflags" << std::endl;
    hfst::lexc::lexc_->flush(err);
    hfst::lexc::lexc_->addNoFlag(lexname);
}
static
void
handle_definition(const string& variable_name, const string& reg_exp)
{
    hfst::lexc::lexc_->addXreDefinition(variable_name, reg_exp);
}

static
bool
handle_lexicon_name(const string& lexiconName)
{
  try
  {
    hfst::lexc::lexc_->setCurrentLexiconName(lexiconName);
  }
  catch(const char *)
  {
    return false;
  }
  return true;
}

static
void
handle_string_entry_common(const string&,
                           const string& gloss,
                           double* weight,
                           bool* is_glossed, bool* is_heavy)
{
    string gloss_handled;
    *is_glossed = false;
    *is_heavy = false;
    if (gloss != "")
    {
        gloss_handled = string(gloss);
        *is_glossed = true;
        size_t wstart = gloss_handled.find("weight:");
        if (wstart != string::npos)
        {
            *is_heavy = true;
            wstart = gloss_handled.find_first_of("-0.123456789", wstart);
            size_t wend = gloss_handled.find_first_not_of("-0.123456789",
                                                          wstart);
            *weight = strtod(gloss_handled.substr(wstart, wend).c_str(), NULL);
        }
        else
        {
            *is_heavy = false;
        }
    }
    else
    {
        *is_glossed = false;
    }
}

static
void
handle_string_entry(const string& data, const string& cont, const string& gloss)
{
    double weight = 0;
    bool is_glossed = false;
    bool is_heavy = false;
    handle_string_entry_common(cont, gloss, &weight, &is_glossed, &is_heavy);
    hfst::lexc::lexc_->addStringEntry(data, cont, weight);
}

static
bool
handle_string_pair_entry(const string& upper, const string& lower,
                                const string& cont, const string& gloss)
{
    double weight = 0;
    bool is_glossed = false;
    bool is_heavy = false;
    handle_string_entry_common(cont, gloss, &weight, &is_glossed, &is_heavy);
    // handle epsilon "0"
    if (upper != "0" && lower != "0")
    {
        try 
        {
            hfst::lexc::lexc_->addStringPairEntry(upper, lower, cont, weight);
        }
        catch(const char * ) 
        {
            return false;
        }
    }
    else
    {
        std::string upper_(upper);
        std::string lower_(lower);
        if (upper == "0")
        {
            upper_ = std::string("");
        }
        if (lower == "0")
        {
            lower_ = std::string("");
        }
        try 
        {
            hfst::lexc::lexc_->addStringPairEntry(upper_, lower_, cont, weight);
        }
        catch(const char * )
        {
            return false;
        }
    }
    return true;
}

static
void
handle_regexp_entry(const string& reg_exp, const string& cont,
                           const string& gloss)
{
    double weight = 0;
    bool is_glossed = false;
    bool is_heavy = false;
    handle_string_entry_common(cont, gloss, &weight, &is_glossed, &is_heavy);
    hfst::lexc::lexc_->addXreEntry(reg_exp, cont, weight);
}

static
void
hlexcwarn(const char* text)
{
    if (!hfst::lexc::lexc_->isQuiet()) 
    {
        hfst::lexc::error_at_current_token(0, 0, text);
    }
}


static
void
handle_eof()
{
}

static
void
handle_end()
{
}

%}

// ouch
%name-prefix="hlexc"
// yup, nice messages
%error-verbose
// just cos I use the llloc struct
%locations

%union
{
    char* name;
    int number;
}

%token <number> ERROR MULTICHARS_START ALPHABETS_START
    DEFINITIONS_START END_START NOFLAGS_START
%token <name>   LEXICON_START LEXICON_START_WRONG_CASE
    LEXICON_NAME ULSTRING ENTRY_GLOSS
    MULTICHAR_SYMBOL
    DEFINITION_NAME DEFINITION_EXPRESSION
    XEROX_REGEXP

%%

LEXC_FILE: NOFLAGS_PART MULTICHAR_PART DEFINITIONS_PART LEXICON_PART END_PART {
                handle_eof();
            }
             ;

MULTICHAR_PART: MULTICHAR_SYMBOLS2 MULTICHAR_SYMBOL_LIST
                 |
                 ;

MULTICHAR_SYMBOLS2: MULTICHARS_START
                | ALPHABETS_START {
                    upgrade_alphabets();
                }
                ;

MULTICHAR_SYMBOL_LIST: MULTICHAR_SYMBOL_LIST MULTICHAR_SYMBOL2
                 | MULTICHAR_SYMBOL2
                 ;

MULTICHAR_SYMBOL2: MULTICHAR_SYMBOL {
            handle_multichar($1);
            free($1);
        }
        ;

NOFLAGS_PART: NOFLAG_LEXICONS2 NOFLAG_LEXICON_LIST
                 |
                 ;

NOFLAG_LEXICONS2: NOFLAGS_START
                ;

NOFLAG_LEXICON_LIST: NOFLAG_LEXICON_LIST NOFLAG_LEXICON2
                 | NOFLAG_LEXICON2
                 ;

NOFLAG_LEXICON2: LEXICON_NAME {
            handle_noflag($1);
            free($1);
        }
        ;

DEFINITIONS_PART: DEFINITIONS_START2 DEFINITION_LIST
                  |
                  ;

DEFINITIONS_START2: DEFINITIONS_START
                  ;

DEFINITION_LIST: DEFINITION_LIST DEFINITION_LINE
                  | DEFINITION_LINE
                  ;

DEFINITION_LINE: DEFINITION_NAME DEFINITION_EXPRESSION {
                    handle_definition($1, $2);
                    free( $1);
                    free( $2);
                }
                ;

LEXICON_PART: LEXICONS
          ;

LEXICONS: LEXICONS LEXICON2 LEXICON_LINES
          | LEXICON2 LEXICON_LINES
          ;

LEXICON2: LEXICON_START {
            bool retval = handle_lexicon_name($1);
            free($1);
            if (!retval)
              { hlexcerror("Sublexicon defined more than once."); YYABORT; }
          }
          | LEXICON_START_WRONG_CASE {
            if (hfst::lexc::lexc_->areWarningsTreatedAsErrors())
              { hlexcerror("Keyword 'Lexicon' used instead of 'LEXICON'. [--Werror]"); YYABORT; }
            else
              { hlexcwarn("Titlecase Lexicon parsed as LEXICON"); }
            bool retval = handle_lexicon_name($1);
            free($1);
            if (!retval)
              { hlexcerror("Sublexicon defined more than once."); YYABORT; }
          }
          ;

LEXICON_LINES: LEXICON_LINES LEXICON_LINE
          | LEXICON_LINE
          ;

LEXICON_LINE: ULSTRING LEXICON_NAME ';' {
                handle_string_entry($1, $2, "");
                free( $1);
                free( $2);
              }
              | ULSTRING ':' ULSTRING
                LEXICON_NAME ';' {
                bool retval = handle_string_pair_entry($1, $3, $4, "");
                free( $1);
                free( $3);
                free( $4);
                if (!retval)
                  { hlexcerror("Erroneous string pair entry."); YYABORT; }
              }
              | LEXICON_NAME ';' {
                handle_string_entry("", $1, "");
                free( $1);
              }
              | ULSTRING ':' LEXICON_NAME ';' {
                bool retval = handle_string_pair_entry($1, "", $3, "");
                free( $1);
                free( $3);
                if (!retval)
                  { hlexcerror("Erroneous string pair entry."); YYABORT; }
              }
              | ':' ULSTRING LEXICON_NAME ';' {
                bool retval = handle_string_pair_entry("", $2, $3, "");
                free( $2);
                free( $3);
                if (!retval)
                  { hlexcerror("Erroneous string pair entry."); YYABORT; }
              }
              | ':' LEXICON_NAME ';' {
                handle_string_entry("", $2, "");
                free( $2);
              }
              | ULSTRING LEXICON_NAME ENTRY_GLOSS ';' {
                handle_string_entry($1, $2, $3);
                free( $1);
                free( $2);
                free( $3);
              }
              | ULSTRING ':' ULSTRING
                LEXICON_NAME ENTRY_GLOSS ';' {
                bool retval = handle_string_pair_entry($1, $3, $4, $5);
                free( $1);
                free( $3);
                free( $4);
                free( $5);
                if (!retval)
                  { hlexcerror("Erroneous string pair entry."); YYABORT; }
              }
              | LEXICON_NAME ENTRY_GLOSS ';' {
                handle_string_entry("", $1, $2);
                free( $1);
                free( $2);
              }
              | ULSTRING ':' LEXICON_NAME ENTRY_GLOSS ';' {
                bool retval = handle_string_pair_entry($1, "", $3, $4);
                free( $1);
                free( $3);
                free( $4);
                if (!retval)
                  { hlexcerror("Erroneous string pair entry."); YYABORT; }
              }
              | ':' ULSTRING LEXICON_NAME ENTRY_GLOSS ';' {
                bool retval = handle_string_pair_entry("", $2, $3, $4);
                free( $2);
                free( $3);
                free( $4);
                if (!retval)
                  { hlexcerror("Erroneous string pair entry."); YYABORT; }
              }
              | ':' LEXICON_NAME ENTRY_GLOSS ';' {
                handle_string_entry("", $2, $3);
                free( $2);
                free( $3);
              }
              | XEROX_REGEXP LEXICON_NAME ';' {
                handle_regexp_entry($1, $2, "");
                free( $1);
                free( $2);
              }
              | XEROX_REGEXP LEXICON_NAME ENTRY_GLOSS ';' {
                handle_regexp_entry($1, $2, $3);
                free( $1);
                free( $2);
                free( $3);
              }
              ;

END_PART: END_START {
            handle_end();
        }
        |
        ;
%%

// oblig. declarations
extern FILE* hlexcin;
int hlexcparse(void);

// gah, bison/flex error mechanism here
void
hlexcerror(const char* text)
{
    hfst::lexc::error_at_current_token(0, 0, text);
    hlexcnerrs++;
}

// vim: set ft=yacc:
