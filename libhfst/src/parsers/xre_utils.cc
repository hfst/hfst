// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

/**
 * @file xre.cc
 *
 * @brief implements xre routines.
 */

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "xre_utils.h"
#include "HfstTransducer.h"
#include "HfstXeroxRules.h"
#include "XreCompiler.h"

using std::string;
using std::map;
using std::ostringstream;

struct yy_buffer_state;
typedef yy_buffer_state * YY_BUFFER_STATE;
typedef void * yyscan_t;
extern int xreparse(yyscan_t);
extern int xrelex_init (yyscan_t*);
extern YY_BUFFER_STATE xre_scan_string (const char *, yyscan_t);
extern void xre_delete_buffer (YY_BUFFER_STATE, yyscan_t);
extern int xrelex_destroy (yyscan_t);
extern char * xreget_text(yyscan_t);

namespace hfst {
  namespace xre {
    extern unsigned int cr; // number of characters read, defined in XreCompiler.cc
    extern unsigned int lr; // number of lines read, defined in XreCompiler.cc
    bool allow_extra_text_at_end = false;
    extern std::ostream * error_;
    extern bool verbose_;
    extern std::set<std::string> * defined_multichar_symbols_;
  }
}

std::ostream * xreerrstr()
{
  return hfst::xre::XreCompiler::get_stream(hfst::xre::error_);
}

void xreflush(std::ostream * os)
{
  hfst::xre::XreCompiler::flush(os);
}

int xreerror(yyscan_t scanner, const char* msg)
{
  if (hfst::xre::verbose_)
    {
      const char * scanner_msg = xreget_text(scanner);

      char * buffer = (char*) malloc(strlen(msg) + strlen(hfst::xre::data) + strlen(scanner_msg) + 100);

      int n = sprintf(buffer, "*** xre parsing failed: %s\n", msg);
      if (strlen(hfst::xre::data) < 60)
        {
          n = sprintf(buffer+n, "***    parsing %s [near %s] on line %u\n%c", hfst::xre::data,
                      xreget_text(scanner), hfst::xre::lr, '\0');
        }
      else
        {
          n = sprintf(buffer+n, "***    parsing %60s [near %s] on line %u...\n%c",
                      hfst::xre::data, xreget_text(scanner), hfst::xre::lr, '\0');
        }

      std::ostream * err = xreerrstr();
      *(err) << std::string(buffer);
      free(buffer);
      xreflush(err);
    }
  return 0;
}

int
xreerror(const char *msg)
{
  char buffer [1024];
  int n = sprintf(buffer, "*** xre parsing failed: %s\n", msg);
  buffer[1023] = '\0';
  std::ostream * err = xreerrstr();
  *err << std::string(buffer);
  xreflush(err);
  return 0;
}

namespace hfst
{
namespace xre
{

  char* data;
  std::map<std::string,hfst::HfstTransducer*> definitions;
  std::map<std::string,std::string>  function_definitions;
  std::map<std::string,unsigned int> function_arguments;
  std::map<std::string,std::set<string> > symbol_lists;
  char* startptr; // changed this to an internal variable in compile functions
hfst::HfstTransducer* last_compiled;
bool contains_only_comments = false;
hfst::ImplementationType format;
size_t len;

  bool expand_definitions=false;
  bool harmonize_=true;
  bool harmonize_flags_=false;
  //bool verbose_=false;

  std::string substitution_function_symbol;

void set_substitution_function_symbol(const std::string &symbol)
{
  substitution_function_symbol=symbol;
}

bool substitution_function(const hfst::StringPair &p, hfst::StringPairSet &sps)
{
  if (p.first == substitution_function_symbol ||
      p.second == substitution_function_symbol)
    {
      sps.insert(hfst::StringPair(substitution_function_symbol,
                                  substitution_function_symbol));
      return true;
    }
  return false;
}


int*
get_n_to_k(const char* s)
{
    int* rv = static_cast<int*>(malloc(sizeof(int)*2));
    char *endptr;
    char *finalptr;
    if (*(s + 1) == '{')
      {
        rv[0] = strtol(s + 2, &endptr, 10);
        rv[1] = strtol(endptr + 1, &finalptr, 10);
        assert(*finalptr == '}');
      }
    else
      {
        rv[0] = strtol(s + 1, &endptr, 10);
        rv[1] = strtol(endptr + 1, &finalptr, 10);
        assert(*finalptr == '\0');
      }
    return rv;
}



int
getinput(char *buf, int maxlen)
{
    int retval = 0;
    if ( maxlen > len ) {
        maxlen = len;
    }
    memcpy(buf, data, maxlen);
    data += maxlen;
    len -= maxlen;
    retval = maxlen;
    return retval;
}

char*
strip_newline(char *s)
{
  for (unsigned int pos = 0; s[pos] != '\0'; pos++)
    {
      if (s[pos] == '\n' || s[pos] == '\r')
        s[pos] = '\0';
    }
  return s;
}

void 
count_lines(const char * s)
{
  const char * c = s;
  while(*c != '\0')
    {
      if (*c == '\n')
        {
          hfst::xre::lr += 1;
        }
      else if (*c == '\r')
        {
          c++;
          if (*c == '\n')
            {
              hfst::xre::cr += 1;
            }
          else
            {
              c--;
            }
          hfst::xre::lr += 1;
        }
      hfst::xre::cr += 1;
      c++;
    }
}

char*
strip_curly(const char *s)
{
    const char *c = s;
    char *stripped = (char*)calloc(sizeof(char),strlen(s)+1);
    int i = 0;
    while (*c != '\0')
    {
        // If first char is { or last one }, then skip it
        if ( ( *c == '{' && i == 0 ) || ( *c == '}' && *(c + 1) == '\0' ) )
                {
                if (*(c + 1) == '\0')
                        {
                                break;
                        }
                        else
                        {
                                stripped[i] = *(c + 1);
                                i++;
                                c += 2;
                        }
                }
        else
        {
                stripped[i] = *c;
            i++;
            c++;
        }

    }
    stripped[i] = '\0';
    return stripped;
}


char*
strip_percents(const char *s)
{
    const char *c = s;
    char *stripped = (char*)calloc(sizeof(char),strlen(s)+1);
    size_t i = 0;
    while (*c != '\0')
    {
        if (*c == '%')
        {
            if (*(c + 1) == '\0')
            {
                break;
            }
            else
            {
                stripped[i] = *(c + 1);
                i++;
                c += 2;
            }
        }
        else
        {
            stripped[i] = *c;
            i++;
            c++;
        }
    }
    stripped[i] = '\0';
    return stripped;
}

char*
add_percents(const char *s)
  {
    char* ns = static_cast<char*>(malloc(sizeof(char)*strlen(s)*2+1));
    char* p = ns;
    while (*s != '\0')
      {
        if ((*s == '@') || (*s == '-') || (*s == ' ') || (*s == '|') ||
            (*s == '!') || (*s == ':') || (*s == ';') || (*s == '0') ||
            (*s == '\\') || (*s == '&') || (*s == '?') || (*s == '$') ||
            (*s == '+') || (*s == '*') || (*s == '/') || (*s == '/') ||
            (*s == '_') || (*s == '(') || (*s == ')') || (*s == '{') ||
            (*s == '}') || (*s == '[') || (*s == ']'))
          {
            *p = '%';
            p++;
          }
        *p = *s;
        p++;
        s++;
      }
    *p = '\0';
    return ns;
  }

char *
get_quoted(const char *s)
{
    const char *qstart = strchr((char*) s, '"') + 1;
    const char *qend = strrchr((char*) s, '"');
    char* qpart = strdup(qstart);
    *(qpart+ (size_t) (qend - qstart)) = '\0';
    return qpart;
}

char*
parse_quoted(const char *s, unsigned int & length)
{
  std::ostream * err = xreerrstr();

    char* quoted = get_quoted(s);

    char* rv = static_cast<char*>(malloc(sizeof(char)*strlen(quoted) + 1)); // added + 1
    char* p = quoted;
    char* r = rv;
    while (*p != '\0')
      {
        if (*p == '\n' || *p == '\r')
          {
            throw "Unescaped newline characters found inside quoted string.";
          }
        else if (*p != '\\')
          {
            *r = *p;
            r++;
            p++;
          }
        else if (*p == '\\')
          {
            switch (*(p + 1))
              {
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
                *err << "*** XRE unimplemented: parse octal escape in " << std::string(p);
                xreflush(err);
                *r = '\0';
                p = p + 5;
                break;
              case 'a':
                *r = '\a';
                r++;
                p = p + 2;
                break;
              case 'b':
                *r = '\b';
                r++;
                p = p + 2;
                break;
              case 'f':
                *r = '\f';
                r++;
                p = p + 2;
                break;
              case 'n':
                *r = '\n';
                r++;
                p = p + 2;
                break;
              case 'r':
                *r = '\r';
                r++;
                p = p + 2;
                break;
              case 't':
                *r = '\t';
                r++;
                p = p + 2;
                break;
              case 'u':
                *err << "Unimplemented: parse unicode escapes in " << std::string(p);
                xreflush(err);
                *r = '\0';
                r++;
                p = p + 6;
                break;
              case 'v':
                *r = '\v';
                r++;
                p = p + 2;
                break;
              case 'x':
                  {
                    char* endp;
                    int i = strtol(p + 2, &endp, 10);
                    if ( 0 < i && i <= 127)
                      {
                        *r = static_cast<char>(i);
                      }
                    else
                      {
                        *err << "*** XRE unimplemented: parse \\x" << i << std::endl;
                        xreflush(err);
                        //fprintf(stderr, "*** XRE unimplemented: "
                        //        "parse \\x%d\n", i);
                        *r = '\0';
                      }
                    r++;
                    assert(endp != p);
                    p = endp;
                   break;
                }
              case '\0':
                *err << "End of line after \\ escape" << std::endl;
                xreflush(err);
                //fprintf(stderr, "End of line after \\ escape\n");
                *r = '\0';
                r++;
                p++;
                break;
              default:
                *r = *(p + 1);
                r++;
                p += 2;
                break;
              }
          }
      }
    *r = '\0';
    free(quoted);

    length = 
      hfst::HfstTokenizer::check_utf8_correctness_and_calculate_length(std::string(rv));

    return rv;
}

// If \a str is of form "@_<foo>_@", insert pair ("@_<foo>_@", "<foo>") into \a substitutions.
static void insert_angle_bracket_substitutions
(const std::string & str, hfst::HfstSymbolSubstitutions & substitutions)
{
  if (str.length() < 6)
    return;
  if (str.substr(0, 3) == "@_<" &&
      str.substr(str.length()-3, 3) == ">_@")
    {
      std::string substituting_str = str.substr(2, str.length()-4);
      substitutions.insert(StringPair(str, substituting_str));
    }
}

char*
escape_enclosing_angle_brackets(char *s)
{
  if (s[0] != '<')
    return s;
  unsigned int i=0;
  while(s[i] != '\0')
    {
      i++;
    }
  i--;
  if (s[i] != '>')
    return s;
  
  std::string retval = std::string("@_") + std::string(s) + std::string("_@");
  free(s);
  return strdup(retval.c_str());
}

HfstTransducer*
unescape_enclosing_angle_brackets(HfstTransducer *t)
{
  hfst::HfstSymbolSubstitutions substitutions;
  StringSet alpha = t->get_alphabet();
  for (StringSet::const_iterator it = alpha.begin();
       it != alpha.end(); it++)
    {
      insert_angle_bracket_substitutions(*it, substitutions);
    }
  if (substitutions.size() == 0)
    return t;

  t->substitute(substitutions);
  t->optimize();
  return t;
}

double
get_weight(const char *s)
{
    double rv = -3.1415;
    const char* weightstart = s;
    while ((*weightstart != '\0') &&
           ((*weightstart == ' ') || (*weightstart == '\t') ||
            (*weightstart == ';')))
    {
        weightstart++;
    }
    char* endp;
    rv = strtod(weightstart, &endp);
    assert(endp != weightstart);
    return rv;
}

HfstTransducer*
compile(const string& xre, map<string,HfstTransducer*>& defs,
        map<string, string>& func_defs,
        map<string, unsigned int > func_args,
        map<string, std::set<string> >& lists,
        ImplementationType impl)
{
    // lock here?
    data = strdup(xre.c_str());
    // use an internal variable startptr_ instead of global startptr
    char * startptr_ = data;
    len = strlen(data);
    definitions = defs;
    function_definitions = func_defs;
    function_arguments = func_args;
    symbol_lists = lists;
    format = impl;

    contains_only_comments = false;

    yyscan_t scanner;
    xrelex_init(&scanner);
    YY_BUFFER_STATE bs = xre_scan_string(startptr_,scanner);
    
    int parse_retval = xreparse(scanner);

    xre_delete_buffer(bs,scanner);
    xrelex_destroy(scanner);

    free(startptr_);
    data = 0;
    len = 0;
    if (parse_retval == 0 && !contains_only_comments) // if (yynerrs == 0)
      {
        HfstTransducer* rv = new HfstTransducer(*last_compiled);
        delete last_compiled;
        return rv;
      }
    else
      {
        return NULL;
      }
}

  // todo: Contains lots of code copied directly from compile(...), should be rewritten..
HfstTransducer*
compile_first(const string& xre, map<string,HfstTransducer*>& defs,
              map<string, string>& func_defs,
              map<string, unsigned int > func_args,
              map<string, std::set<string> >& lists,
              ImplementationType impl,
              unsigned int & chars_read)
{
    // lock here?
    data = strdup(xre.c_str());
    // use an internal variable startptr_ instead of global startptr
    char * startptr_ = data;
    len = strlen(data);
    definitions = defs;
    function_definitions = func_defs;
    function_arguments = func_args;
    symbol_lists = lists;
    format = impl;

    contains_only_comments = false;

    yyscan_t scanner;
    xrelex_init(&scanner);
    YY_BUFFER_STATE bs = xre_scan_string(startptr_,scanner);

    bool tmp = hfst::xre::allow_extra_text_at_end;
    hfst::xre::allow_extra_text_at_end = true;
    hfst::xre::cr = 0;
    hfst::xre::lr = 1;

    int parse_retval = xreparse(scanner);
    chars_read = hfst::xre::cr;
    hfst::xre::allow_extra_text_at_end = tmp;

    xre_delete_buffer(bs,scanner);
    xrelex_destroy(scanner);

    free(startptr_);

    data = 0;
    len = 0;
    if (parse_retval == 0 && !contains_only_comments) // if (yynerrs == 0)
      {
        HfstTransducer* rv = new HfstTransducer(*last_compiled);
        delete last_compiled;
        return rv;
      }
    else
      {
        return NULL;
      }
}

bool is_valid_function_call
(const char * name, const std::vector<HfstTransducer> * args)
{
  std::map<std::string, std::string>::const_iterator name2xre
    = function_definitions.find(name);
  std::map<std::string, unsigned int >::const_iterator name2args
    = function_arguments.find(name);

  if (name2xre == function_definitions.end() ||
      name2args == function_arguments.end())
    {
      std::ostream * err = xreerrstr();
      *err << "No such function defined: '" << name << "'" << std::endl;
      xreflush(err);
      //fprintf(stderr, "No such function defined: '%s'\n", name);
      return false;
    }

  unsigned int number_of_args = name2args->second;

  if ( number_of_args != args->size())
    {
      std::ostream * err = xreerrstr();
      *err << "Wrong number of arguments: function '" << name << "' expects "
                           << (int)number_of_args << ", " << (int)args->size() << " given" << std::endl;
      xreflush(err);
        //fprintf(stderr, "Wrong number of arguments: function '%s' expects %i, %i given\n",
        //       name, (int)number_of_args, (int)args->size());
      return false;
    }

  return true;
}

const char * get_function_xre(const char * name)
{
  std::map<std::string,std::string>::const_iterator it
    = function_definitions.find(name);
  if (it == function_definitions.end())
    {
      return NULL;
    }
  return it->second.c_str();
}

bool define_function_args(const char * name, const std::vector<HfstTransducer> * args)
{
  if (! is_valid_function_call(name, args))
    {
      return false;
    }
  unsigned int arg_number = 1;
  for (std::vector<HfstTransducer>::const_iterator it = args->begin();
       it != args->end(); it++)
    {
      ostringstream os;
      os << arg_number;
      std::string function_arg = "@" + std::string(name) + os.str() + "@";
      definitions[function_arg] = new HfstTransducer(*it);
      //fprintf(stderr, "defined function arg: '%s', %i:\n", name, arg_number); // DEBUG
      //std::cerr << *it << std::endl;
      arg_number++;
    }
  return true;
}

void undefine_function_args(const char * name)
{
  std::map<std::string, unsigned int>::const_iterator it = function_arguments.find(name);
  if (it == function_arguments.end())
    {
      return;
    }
  for (unsigned int arg_number = 1; arg_number <= it->second; arg_number++)
    {
      ostringstream os;
      os << arg_number;
      std::string function_arg = "@" + std::string(name) + os.str() + "@";
      definitions.erase(function_arg);
      //fprintf(stderr, "undefined function arg: '%s', %i:\n", name, arg_number); // DEBUG
    }
}

bool is_definition(const char* symbol)
{
  std::string symbol_(symbol);
  if (definitions.find(symbol_) == definitions.end())
    return false;
  return true;
}

HfstTransducer*
expand_definition(const char* symbol)
{
  if (expand_definitions)
    {
      for (std::map<std::string,hfst::HfstTransducer*>::const_iterator it
             = definitions.begin(); it != definitions.end(); it++)
        {
          if (strcmp(it->first.c_str(), symbol) == 0)
            {
              return new HfstTransducer(*(it->second));
            }
        }
    }
  return new HfstTransducer(symbol, symbol, hfst::xre::format);
}


HfstTransducer*
expand_definition(HfstTransducer* tr, const char* symbol)
{
  if (expand_definitions)
    {
      for (std::map<std::string,hfst::HfstTransducer*>::const_iterator it
             = definitions.begin(); it != definitions.end(); it++)
        {
          if (strcmp(it->first.c_str(), symbol) == 0)
            {
              StringSet alpha = it->second->get_alphabet();
              tr->substitute(hfst::StringPair(symbol,symbol), *(it->second), false); // do not harmonize
              if (alpha.find(symbol) == alpha.end())
                tr->remove_from_alphabet(symbol);
              //if (it->second->get_alphabet().find(symbol) != it->second->get_alphabet().end())
              //  std::cerr << "WARN: symbol '" << std::string(symbol) << "' was removed" << std::endl;
              break;
            }
        }
    }
  return tr;
}

static const char * get_print_format(const char * symbol)
{
  if (strcmp(hfst::internal_identity.c_str(), symbol) == 0)
    return "?";
  if (strcmp(hfst::internal_unknown.c_str(), symbol) == 0)
    return "?";
  if (strcmp(hfst::internal_epsilon.c_str(), symbol) == 0)
    return "0";
  return symbol;
}

HfstTransducer*
xfst_curly_label_to_transducer(const char* input, const char* output)
{
  HfstTransducer * retval = NULL;

  if (strcmp(input, hfst::internal_unknown.c_str()) == 0)
    {
      HfstTokenizer tok;
      StringVector sv = tok.tokenize_one_level(output);
      std::string first_token(sv.at(0));
      retval = new HfstTransducer
        (hfst::internal_unknown, first_token, hfst::xre::format);

      for (StringVector::const_iterator it = sv.begin();
           it != sv.end(); it++)
        {
          HfstTransducer tmp(*it, first_token, hfst::xre::format);
          retval->disjunct(tmp, false);
        }
      for (StringVector::const_iterator it = ++(sv.begin());
           it != sv.end(); it++)
        {
          HfstTransducer tmp(hfst::internal_epsilon, *it, hfst::xre::format);
          retval->concatenate(tmp, false);
        }
    }
  else if (strcmp(output, hfst::internal_unknown.c_str()) == 0)
    {
      HfstTokenizer tok;
      StringVector sv = tok.tokenize_one_level(input);
      std::string first_token(sv.at(0));
      retval = new HfstTransducer
        (first_token, hfst::internal_unknown, hfst::xre::format);

      for (StringVector::const_iterator it = sv.begin();
           it != sv.end(); it++)
        {
          HfstTransducer tmp(first_token, *it, hfst::xre::format);
          retval->disjunct(tmp, false);
        }
      for (StringVector::const_iterator it = ++(sv.begin());
           it != sv.end(); it++)
        {
          HfstTransducer tmp(*it, hfst::internal_epsilon, hfst::xre::format);
          retval->concatenate(tmp, false);
        }
    }
  else
    {
      std::string istr(input);
      std::string ostr(output);
      HfstTokenizer tok;
      tok.add_multichar_symbol(hfst::internal_epsilon);
      retval = new HfstTransducer(istr, ostr, tok, hfst::xre::format);
    }

  retval->minimize(); // it should be safe to minimize
  return retval;
}

HfstTransducer*
xfst_label_to_transducer(const char* input, const char* output)
{
  HfstTransducer * retval = NULL;

  bool input_is_definition = is_definition(input);
  bool output_is_definition = is_definition(output);
  bool input_is_unknown = (strcmp(input, hfst::internal_unknown.c_str()) == 0);
  bool output_is_unknown = (strcmp(output, hfst::internal_unknown.c_str()) == 0);

  // definitions -> use cross-product
  if (input_is_definition || output_is_definition)
    {
      HfstTransducer * tmp = NULL; // temporary transducer for cross-product calculation
      if (input_is_unknown)
        {
          retval = new HfstTransducer(hfst::internal_identity, hfst::xre::format);
          tmp = expand_definition(output);
        }
      else if (output_is_unknown)
        {
          tmp = new HfstTransducer(hfst::internal_identity, hfst::xre::format);
          retval = expand_definition(input);
        }
      else // neither is unknown
        {
          retval = expand_definition(input);
          tmp = expand_definition(output);
        }
      retval->cross_product(*tmp);
      delete tmp;
      return retval;
    }

  // no definitions
  if  (input_is_unknown && output_is_unknown)
    {
      retval = new HfstTransducer(hfst::internal_unknown, hfst::internal_unknown, hfst::xre::format);
      HfstTransducer id(hfst::internal_identity, hfst::internal_identity, hfst::xre::format);
      retval->disjunct(id).minimize(); // it should be safe to minimize
    }
  else if (input_is_unknown)
    {
      retval = new HfstTransducer(hfst::internal_unknown, output, hfst::xre::format);
      HfstTransducer output_tr(output, output, hfst::xre::format);
      retval->disjunct(output_tr).minimize(); // it should be safe to minimize
    }
  else if (output_is_unknown)
    {
      retval = new HfstTransducer(input, hfst::internal_unknown, hfst::xre::format);
      HfstTransducer input_tr(input, input, hfst::xre::format);
      retval->disjunct(input_tr).minimize(); // it should be safe to minimize
    }
  else
    {
      retval = new HfstTransducer(input, output, hfst::xre::format);
    }
  return retval;
}

  HfstTransducer * contains_twolc(const HfstTransducer * t)
  {
    // marker = [0:M ?]*
    HfstTransducer marker("@_EPSILON_SYMBOL_@", "M", t->get_type());
    HfstTransducer id("@_IDENTITY_SYMBOL_@", t->get_type());
    marker.concatenate(id).repeat_star().minimize(); // it should be safe to minimize

    // the rule
    HfstTransducer right_context(*t);
    right_context.insert_freely(StringPair("M", "@_EPSILON_SYMBOL_@")).optimize();
    right_context.insert_freely(StringPair("M", "M")).optimize();
    HfstTransducer left_context("@_EPSILON_SYMBOL_@", t->get_type());
    HfstTransducerPair context(left_context, right_context);

    StringPairSet mappings;
    mappings.insert(StringPair("M", "M"));

    // alphabet list possible mappings: M:M, M:0,
    StringPairSet alphabet;
    alphabet.insert(StringPair("M","M"));
    alphabet.insert(StringPair("M","@_EPSILON_SYMBOL_@"));
    alphabet.insert(StringPair("@_IDENTITY_SYMBOL_@","@_IDENTITY_SYMBOL_@"));

    // and identity pairs of symbols that occur in t
    HfstBasicTransducer basic(*t);
    StringPairSet symbol_pairs = basic.get_transition_pairs();
    for (StringPairSet::const_iterator it = symbol_pairs.begin();
         it != symbol_pairs.end(); it++)
      {
        alphabet.insert(*it);
        alphabet.insert(StringPair(it->first, it->first));
        alphabet.insert(StringPair(it->second, it->second));
      }

    HfstTransducer rule = hfst::rules::two_level_if_and_only_if
      (context, mappings, alphabet);

    return new HfstTransducer(rule);

    marker.compose(rule);
    marker.optimize();

    return new HfstTransducer(marker);
  }

  HfstTransducer * contains(const HfstTransducer * t)
  {
    HfstTransducer any(hfst::internal_identity, hfst::xre::format);
    any.repeat_star().minimize(); // it should be safe to minimize
    HfstTransducer * retval = new HfstTransducer(any);
    retval->concatenate(*t).concatenate(any);
    retval->optimize();
    return retval;
  }

  // [ 0::weight -> 0 || _ [t] ] - [?* - $[t]]
  HfstTransducer * contains_with_weight(const HfstTransducer * t, float weight)
  {
    HfstTransducer weighted_epsilon(hfst::internal_epsilon, hfst::xre::format);
    weighted_epsilon.set_final_weights(weight);
    HfstTransducer epsilon(hfst::internal_epsilon, hfst::xre::format);

    // mapping: 0::weight -> 0
    HfstTransducerPair mappingPair(weighted_epsilon, epsilon);
    HfstTransducerPairVector mappingPairVector;
    mappingPairVector.push_back(mappingPair);
    
    // context: 0 _ [t]
    HfstTransducerPair contextPair(epsilon, *t);
    HfstTransducerPairVector contextPairVector;
    contextPairVector.push_back(contextPair);

    // weighted_rule = [ 0::weight @-> 0 || _ [t] ]
    // (add weight for each occurrence of t)
    hfst::xeroxRules::Rule rule(mappingPairVector, contextPairVector, hfst::xeroxRules::REPL_UP);
    HfstTransducer weighted_rule(hfst::xre::format);
    weighted_rule = replace(rule, false);

    // note that replace_leftmost_longest_match does not work correctly in the case of epsilons

    // noT = ?* - $[t]
    // (strings that do not contain t)
    HfstTransducer noT(hfst::internal_identity, t->get_type());
    noT.repeat_star().minimize(); // it should be safe to minimize
    HfstTransducer * oneOrMoreT = contains(t);
    noT.subtract(*oneOrMoreT);
    noT.optimize();
    delete oneOrMoreT;

    // return [weighted_rule - noT]
    // (subtract strings that do not contain t from weighted rule)
    weighted_rule.subtract(noT);
    weighted_rule.optimize();
    return new HfstTransducer(weighted_rule);
  }

  // [$c - $[ [[?+ c ?*] & [c ?*]] | [[c ?+] & [c]]]]
  HfstTransducer * contains_once(const HfstTransducer * c)
  {
    // any_star = [?*]
    HfstTransducer any_star(hfst::internal_identity, hfst::xre::format);
    any_star.repeat_star().minimize(); // it should be safe to minimize

    // any_plus = [?+]
    HfstTransducer any_plus(hfst::internal_identity, hfst::xre::format);
    any_plus.repeat_plus().minimize(); // it should be safe to minimize

    // t1 = [?+ c ?*]
    HfstTransducer * t1 = new HfstTransducer(any_plus);
    t1->concatenate(*c);
    t1->optimize();
    t1->concatenate(any_star);
    t1->optimize();

    // t2 = [c ?*]
    HfstTransducer t2(*c);
    t2.concatenate(any_star);
    t2.optimize();

    // t1 = [[?+ c ?*] & [c ?*]]
    t1->intersect(t2);

    // t3 = [[c ?+] & c]
    HfstTransducer t3(*c);
    t3.concatenate(any_plus);
    t3.optimize();
    t3.intersect(*c);
    t3.optimize();

    // t1 = [t1 | t3]
    t1->disjunct(t3);
    t1->optimize();
    
    // cont_t1 = $[t1]
    HfstTransducer * cont_t1 = contains(t1);
    delete t1;
    // cont_c = $[c]
    HfstTransducer * cont_c = contains(c);

    // $[c] - $[t1]
    cont_c->subtract(*cont_t1);
    cont_c->optimize();
    delete cont_t1;
    return cont_c;
  }

  HfstTransducer * contains_once_optional(const HfstTransducer * t)
  {
    // neg_t = ~$[t]
    HfstTransducer * cont_t = contains(t);
    HfstTransducer neg_t(hfst::internal_identity, hfst::xre::format);
    neg_t.repeat_star();
    neg_t.optimize();
    neg_t.subtract(*cont_t);
    neg_t.optimize();
    delete cont_t;
    
    HfstTransducer * retval = contains_once(t);
    retval->disjunct(neg_t);
    retval->optimize();
    return retval;
  }

  HfstTransducer * merge_first_to_second(HfstTransducer * tr1, HfstTransducer * tr2)
  {
    // Merge operation creates an XreCompiler that needs this information below. Otherwise, it will overwrite all this.
    struct XreConstructorArguments args(hfst::xre::definitions, hfst::xre::function_definitions, hfst::xre::function_arguments, hfst::xre::symbol_lists, hfst::xre::format);

    tr1->optimize();
    tr2->merge(*tr1, args);
    return tr2;
  }

  void warn(const char * msg)
  {
    if (!verbose_)
      return;
    
    std::ostream * err = xreerrstr();
    *err << msg;
    xreflush(err);
  }

void warn_about_xfst_special_symbol(const char * symbol)
{
  if (strcmp("all", symbol) == 0) {
    if (verbose_) {
      warn("warning: symbol 'all' has no special meaning in hfst\n"); }
    return;
  }

  if (symbol[0] != '<')
    return;
  unsigned int max_index=1;
  while(symbol[max_index] != '\0') {
    max_index++; }
  max_index--;
  if (max_index < 1)
    return;

  if (symbol[max_index] != '>')
    return;
  if (!verbose_)
    return;
  std::ostream * err = xreerrstr();
  *err << "warning: '" << symbol << " ' is an ordinary symbol in hfst" << std::endl;
  xreflush(err);
}

void warn_about_hfst_special_symbol(const char * symbol)
{
  if (symbol[0] != '@')
    return;
  if (symbol[1] != '_')
    return;
  unsigned int max_index=2;
  while(symbol[max_index] != '\0') {
    max_index++; }
  max_index--;
  if (max_index < 3)
    return;
  if (symbol[max_index] != '@')
    return;
  if (symbol[max_index-1] != '_')
    return;
  if (!verbose_)
    return;
  if (hfst::xre::verbose_)
    {
      std::ostream * err = xreerrstr();
      *err << "warning: '" << symbol << "' is not an ordinary symbol in hfst" << std::endl;
      xreflush(err);
    }
}

void warn_about_special_symbols_in_replace(HfstTransducer * t)
{
  if (!verbose_)
    return;

  std::ostream * err = xreerrstr();

  StringSet alphabet = t->get_alphabet();
  for (StringSet::const_iterator it = alphabet.begin();
       it != alphabet.end(); it++)
    {
      if (HfstTransducer::is_special_symbol(*it) &&
          *it != hfst::internal_epsilon &&
          *it != hfst::internal_unknown &&
          *it != hfst::internal_identity)
        {
          *err << "warning: using special symbol '" << *it << "' in replace rule, use substitute instead" << std::endl;
        }
    }
  xreflush(err);
}

void check_multichar_symbol(const char * symbol)
{
  if (defined_multichar_symbols_ == NULL)
    return;
  
  if (defined_multichar_symbols_->find(std::string(symbol)) ==
      defined_multichar_symbols_->end())
    {
      std::ostream * err = xreerrstr();
      *err << "warning: multichar symbol '" << symbol << "' used but not defined" << std::endl;
      xreflush(err);
    }
}

bool has_non_identity_pairs(const HfstTransducer * t)
{
  hfst::implementations::HfstBasicTransducer basic(*t);
  StringPairSet sps = basic.get_transition_pairs();
  for (StringPairSet::const_iterator it = sps.begin(); it != sps.end(); it++)
    {
      if (it->first != it->second)
        {
          return true;
        }
    }
  return false;
}

} }


