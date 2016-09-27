// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

//! @file lexc-utils.cc
//!
//! @brief Implementation of some string handling in HFST lexc.

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <string.h>
#include <vector>

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>

// for internal epsilon
#include "HfstSymbolDefs.h"

#include "LexcCompiler.h"
#include "lexc-utils.h"
#include "string-utils.h"

#ifdef YACC_USE_PARSER_H_EXTENSION
  #include "lexc-parser.h"
#else
  #include "lexc-parser.hh"
#endif

using std::string;

// flex stuffa
extern int hlexclineno;
static char* hlexcfilename = 0;
extern char* hlexctext;
extern YYLTYPE hlexclloc;

namespace hfst { namespace lexc {

extern hfst::lexc::LexcCompiler * lexc_;


string&
stripPercents(string& s)
{
    string& stripped = s;
    stripped = replace_all(stripped, "%%", "@PERCENT@");
    stripped = replace_all(stripped, "%", "");
    stripped = replace_all(stripped, "@PERCENT@", "%");
    return stripped;
}

string&
addPercents(string& s)
{
    string& added = s;
    added = replace_all(added, "%", "%%");
    added = replace_all(added, "<", "%<");
    added = replace_all(added, ">", "%>");
    return added;
}

string&
flagJoinerEncode(string& s, bool left)
{
    if (left)
      {
        string& lxs = s;
        lxs = lxs.insert(0, LEXC_FLAG_LEFT_START);
        lxs = lxs.append(LEXC_FLAG_END);
        return lxs;
      }
    else
      {
        string& lxs = s;
        lxs = lxs.insert(0, LEXC_FLAG_RIGHT_START);
        lxs = lxs.append(LEXC_FLAG_END);
        return lxs;
      }
}

string&
joinerEncode(string& s)
{
    string& lxs = s;
    lxs = lxs.insert(0, LEXC_JOINER_START);
    lxs = lxs.append(LEXC_JOINER_END);
    return lxs;
}
string&
joinerDecode(string& s)
{
    assert(s.length() >= 4);
    string& decoded = s;
    size_t jStart = strlen(LEXC_JOINER_START);
    size_t jEnd = strlen(LEXC_JOINER_END);
    decoded = decoded.substr(jStart, (s.length() - (jStart + jEnd)));
    return decoded;
}

string&
regExpresionEncode(string& s)
{
    string& lxs = s;
    lxs = lxs.insert(0, REG_EX_START);
    lxs = lxs.append(LEXC_JOINER_END);
    return lxs;
}

string&
regExpresionDecode(string& s)
{
    assert(s.length() >= 4);
    string& decoded = s;
    size_t jStart = strlen(REG_EX_START);
    size_t jEnd = strlen(LEXC_JOINER_END);
    decoded = decoded.substr(jStart, (s.length() - (jStart + jEnd)));
    return decoded;
}

string&
xreDefinitionEncode(string& s)
{
    string& lxs = s;
    lxs = lxs.insert(0, LEXC_DFN_START);
    lxs = lxs.append(LEXC_DFN_END);
    return lxs;
}


//replaces @ZERO@ with "0" in a string
string replace_zero(const string s) {
    string str = s;
    string zero = "@ZERO@";
    size_t start_pos = str.find(zero);
    if(start_pos != std::string::npos)
    {
        str.replace(start_pos, zero.length(), "0");
    }
    return str;
}


// FLEX stuff



static
size_t
count_newlines(const char *linestring)
{
    size_t linecount = 0;
    char *linep = (char*)linestring;
    while(*linep != '\0')
    {
        if(*linep == '\n')
        {
            linecount++;
        }
        linep++;
    }
    return linecount;
}

void
set_infile_name(const char* s)
{
    free(hlexcfilename);
    hlexcfilename = strdup(s);
}

void
token_reset_positions()
{
    hlexclloc.first_line = hlexclloc.last_line = 1;
    hlexclloc.first_column = hlexclloc.last_column = 1;
    hlexclineno = 1;
    if (hlexcfilename != 0)
    {
        free(hlexcfilename);
    }
    hlexcfilename = 0;
}

void
token_update_positions(const char *token)
{
    size_t token_length = strlen(token);
    int newlines = count_newlines(token);
    hlexclloc.first_line = hlexclloc.last_line;
    hlexclloc.last_line = (hlexclloc.first_line + newlines);
    // FIXME: columns equal bytes not characters
    hlexclloc.first_column = hlexclloc.last_column + 1;
    if (0 == newlines)
    {
        hlexclloc.last_column = (hlexclloc.first_column + token_length);
    }
    else
    {
      char *token_last_line_start = strrchr((char*) token, '\n');
      char *token_end = strrchr((char*) token, '\0');
        hlexclloc.last_column = (token_end - token_last_line_start) - 1;
    }
}

char*
strdup_token_positions()
{
    // N.B. reason for this error format is automagic support by vim/emacs/jedit
    // must be â€œfilename:lineno:colno-lineno:colno: stuffâ€�
    // c.f. http://www.gnu.org/prep/standards/standards.html#Errors
    char* filenames_lines_cols = (char*)malloc(sizeof(char) *
            (strlen(hlexcfilename) + 100));
    // linenumbers and columns
    if( (hlexclloc.first_line == hlexclloc.last_line) &&
        (hlexclloc.first_column == (hlexclloc.last_column - 1)) )
    {
        // TRANSLATORS: filename, line and column
        (void)sprintf(filenames_lines_cols, "%s:%d.%d",
            hlexcfilename, hlexclloc.first_line, hlexclloc.first_column);
    }
    else if ( hlexclloc.first_line == hlexclloc.last_line )
    {
        // TRANSLATORS: filename, line, column to column
        (void)sprintf(filenames_lines_cols, "%s:%d.%d-%d",
            hlexcfilename,
            hlexclloc.first_line, hlexclloc.first_column,
            hlexclloc.last_column);
    }
    else
    {
        // TRANSLATORS: filename, line-column to line-column
        (void)sprintf(filenames_lines_cols, "%s:%d.%d-%d.%d",
            hlexcfilename,
            hlexclloc.first_line, hlexclloc.first_column,
            hlexclloc.last_line, hlexclloc.last_column);
    }
    return filenames_lines_cols;
}

char*
strdup_token_part()
{
    char *error_token = (char*)malloc(sizeof(char)*strlen(hlexctext)+100);
    char* maybelbr = strchr(hlexctext, '\n');
    if (maybelbr != NULL)
    {
        char* beforelbr = (char*)malloc(sizeof(char)*strlen(hlexctext)+1);
        (void)memcpy(beforelbr, hlexctext, maybelbr-hlexctext);
        beforelbr[maybelbr-hlexctext] = '\0';
        (void)sprintf(error_token, "[near: `%s\\n']", beforelbr);
        free(beforelbr);
    }
    else if (strlen(hlexctext) < 80)
    {
        (void)sprintf(error_token, "[near: `%s']", hlexctext);
    }
    else
    {
        (void)sprintf(error_token, "[near: `%30s...' (truncated)]",
                      hlexctext);
    }
    return error_token;
}

char*
strip_percents(const char* s, bool do_zeros)
{
    char* rv = 0;
    if (do_zeros)
      {
        // worst case scenario is string of 0's...
        rv = (char*)malloc(sizeof(char) *
                             strlen(s) * strlen("@0@")
                             + 1);
      }
    else
      {
        // worst case scenario is string of %0's...
        rv = (char*)malloc(sizeof(char) *
                           ((strlen(s) / 2) + 1) * strlen("@ZERO@") + 1);
      }
    char* p = rv;
    const char* c = s;
    bool escaping = false;
    bool in_at = false;
    while (*c != '\0')
      {
        if (in_at)
          {
            if (*c == '@')
              {
                in_at = false;
              }
            *p = *c;
            p++;
            c++;
          }
        else if (escaping)
          {
            if (*c != '0')
              {
                *p = *c;
                p++;
              }
            else
              {
                const char* escaped = "@ZERO@";
                while (*escaped != '\0')
                  {
                    *p = *escaped;
                    p++;
                    escaped++;
                  }
              }
            escaping = false;
            ++c;
          }
        else if (*c == '%')
          {
            escaping = true;
            ++c;
          }
        else if (*c == '@')
          {
            in_at = true;
            *p = *c;
            p++;
            c++;
          }
        else if (do_zeros && (*c == '0'))
          {
            const char* escaped = "@0@";
            while (*escaped != '\0')
              {
                *p = *escaped;
                p++;
                escaped++;
              }
            c++;
          }
        else
          {
            *p = *c;
            ++p;
            ++c;
          }
      }
    *p = '\0';
    if (escaping)
    {
      //fprintf(stderr, "Stray escape char %% in %s\n", s);
      std::ostream * err = hfst::lexc::lexc_->get_stream((hfst::lexc::lexc_->get_error_stream()));
      *err << "Stray escape char %% in " << s << std::endl;
      hfst::lexc::lexc_->flush(err);
      return NULL;
    }
    return rv;
}

char*
strstrip(const char* s)
{
    char* rv = (char*)malloc(sizeof(char)*strlen(s)+1);

    if (*s == '\0') // empty string is a special case
    {
      *rv = '\0';
      return rv;
    }

    char* p = rv;
    while (isspace(*s))
    {
        ++s;
    }
    while (*s != '\0')
    {
        *p = *s;
        ++p;
        ++s;
    }
    *p = '\0';
    --p;
    while (isspace(*p))
    {
        *p = '\0';
        --p;
    }
    return rv;
}

char*
strdup_nonconst_part(const char* token, const char* prefix,
                          const char* suffix, bool strip)
{
    size_t prefix_len = 0;
    size_t suffix_len = 0;
    size_t varpart_len = 0;
    size_t token_len = strlen(token);
    char* token_part = (char*)malloc(sizeof(char) * token_len + 1);
    if (prefix)
    {
        prefix_len = strlen(prefix);
    }
    if (suffix)
    {
        suffix_len = strlen(suffix);
    }
    varpart_len = strlen(token) - prefix_len - suffix_len;
    assert(varpart_len <= token_len);

    if (prefix == NULL)
      { assert(strncmp(token, "", prefix_len) == 0); }
    else
      { assert(strncmp(token, prefix, prefix_len) == 0); }

    if (suffix == NULL)
      { assert(strncmp(token + prefix_len + varpart_len, "", suffix_len) == 0); }
    else
      { assert(strncmp(token + prefix_len + varpart_len, suffix, suffix_len) == 0); }

    (void)memcpy(token_part, token + prefix_len, varpart_len);
    token_part[varpart_len] = '\0';
    if (strip)
    {
        char* tmp = strstrip(token_part);
        free(token_part);
        token_part = tmp;
    }
    return token_part;
}

void
error_at_current_token(int, int, const char* format)
{
    char* leader = strdup_token_positions();
    char* token = strdup_token_part();
    //fprintf(stderr, "%s: %s %s\n", leader, format, token);
      std::ostream * err = hfst::lexc::lexc_->get_stream((hfst::lexc::lexc_->get_error_stream()));
      *err << leader << ": " << format << ": " << token << std::endl;
      hfst::lexc::lexc_->flush(err);
    free(leader);
}



pair<vector<string>, vector<string> > find_med_alingment(const vector<string> &s1, const vector<string> &s2)
{
    const size_t substitution = 100;
    const size_t deletion = 1;
    const size_t insertion = 1;

    const size_t len1 = s1.size(), len2 = s2.size();
    vector<vector<unsigned int> > d(len1 + 1, vector<unsigned int>(len2 + 1));
    vector<vector<unsigned int> > dir(len1 + 1, vector<unsigned int>(len2 + 1));
    d[0][0] = 0;
    dir[0][0] = 0;
    for(unsigned int i = 1; i <= len1; ++i)
    {
        d[i][0] = deletion * i;
        dir[i][0] = DELETE;
        
    }
    for(unsigned int i = 1; i <= len2; ++i)
    {
        d[0][i] = insertion * i;
        dir[0][i] = INSERT;
    }
    
    for(unsigned int i = 1; i <= len1; ++i)
    {
        for(unsigned int j = 1; j <= len2; ++j)
        {
           
            int sub = d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : substitution);
            int ins = d[i][j - 1] + insertion ;
            int del = d[i - 1][j] + deletion ;
            
           
            if ( sub <= ins &&  sub <= del )
            {
                d[i][j] = sub;
                dir[i][j] = SUBSTITUTE;
            }
             // It's important to prioritize "del" when it has the same value as "ins"" because we want the first
             // string to have zeroes before the second one.
             // For example, if we have two strings: abc and xyz, we would prefer the output 000abc:xyz000
             // over abc000:000xyz, because we need to invert the transducer to use it for the analysis and in lookup
             // we want zeroes as late as possible on the upper side.
             // Anyway, the order of this two following blocks determines the order of zeroes on upper/lower side
            else if (del <= sub && del <= ins)
            {
                d[i][j] = del;
                dir[i][j] = DELETE;
            }
            else
            {
                d[i][j] = ins;
                dir[i][j] = INSERT;
            }
        }
    }
    
    vector<string> medcwordin;
    vector<string> medcwordout;
    
    int x, y, i ;
    for ( x = s1.size(), y = s2.size(), i = 0; (x > 0) || (y > 0); i++)
    {
        int dirValue = dir[x][y];
         
        if (dirValue == SUBSTITUTE)
        {
            medcwordin.push_back(s1[x-1]) ;
            medcwordout.push_back(s2[y-1]);
            x--;
            y--;
        }
        else if (dirValue == INSERT)
        {
            medcwordin.push_back(EPSILON_);
            medcwordout.push_back(s2[y-1]);
            y--;
        }
        else
        {
            medcwordin.push_back(s1[x-1]);
            medcwordout.push_back(EPSILON_);
            x--;
        }
    }

    std::reverse(medcwordin.begin(), medcwordin.end());
    std::reverse(medcwordout.begin(), medcwordout.end());
    
    pair<vector<string>, vector<string> > returnValue (medcwordin,medcwordout);
    return returnValue;
}
        
    
} }

