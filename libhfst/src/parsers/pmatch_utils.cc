// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

/**
 * @file pmatch.cc
 *
 * @brief implements pmatch routines.
 */

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "pmatch_utils.h"
#include "HfstTransducer.h"
//#include "tools/src/HfstUtf8.h"
#include "implementations/optimized-lookup/pmatch.h"

using std::string;
using std::map;

extern int pmatchlineno;
extern char* pmatchtext;
extern int pmatchparse();
extern int pmatchnerrs;

int
pmatcherror(const char *msg)
{
    fprintf(stderr, "*** pmatch parsing failed: %s\n", msg);
    if (strlen(hfst::pmatch::data) < 60)
    {
        fprintf(stderr, "*** parsing %s [line %d, near %s]\n",
                hfst::pmatch::data, pmatchlineno, pmatchtext);
    }
    else
    {
        char buf[200];
        memcpy(buf, hfst::pmatch::data, 200 - 1);
        buf[200 - 1] = '\0';
        fprintf(stderr, "***    parsing %s [line %d, near %s]...\n",
                buf, pmatchlineno, pmatchtext);
    }
    exit(0);
}

void pmatchwarning(const char *msg)
{
    if (hfst::pmatch::verbose) {
        std::cerr << "pmatch: "<< msg << std::endl;
    }
}

namespace hfst
{
namespace pmatch
{

char* data;
std::map<std::string, hfst::pmatch::PmatchObject*> definitions;
std::map<std::string, std::string> variables;
std::vector<std::map<std::string, PmatchObject*> > call_stack;
std::map<std::string, PmatchObject*> def_insed_expressions;
std::set<std::string> inserted_names;
std::set<std::string> unsatisfied_insertions;
std::set<std::string> used_definitions;
std::set<std::string> function_names;
char* startptr;
hfst::ImplementationType format;
size_t len;
bool verbose;
bool flatten;
clock_t timer;
int minimization_guard_count;
bool need_delimiters;

std::map<std::string, hfst::HfstTransducer> named_transducers;
PmatchUtilityTransducers* utils=NULL;

void warn(std::string warning)
{
    std::cerr << "pmatch: warning: " << warning;
}

PmatchUtilityTransducers*
get_utils()
{
  if (utils == NULL)
    {
      utils = new PmatchUtilityTransducers();
    }
  return utils;
}

void zero_minimization_guard(void)
{
    minimization_guard_count = 0;
}

PmatchTransducerContainer * make_minimization_guard(void)
{
    std::stringstream guard;
    if(minimization_guard_count == 0) {
        guard << hfst::internal_epsilon;
    } else {
        guard << "@PMATCH_GUARD_" << minimization_guard_count << "@";
    }
    ++minimization_guard_count;
    return epsilon_to_symbol_container(guard.str());
}

bool symbol_in_global_context(std::string & sym)
{
    return definitions.count(sym) != 0;
}

bool symbol_in_local_context(std::string & sym)
{
    if (call_stack.size() == 0) {
        return false;
    }
    return call_stack.back().count(sym) != 0;
}

PmatchObject * symbol_from_global_context(std::string & sym)
{
    if (symbol_in_global_context(sym)) {
        return definitions[sym];
    } else {
        return (PmatchObject *) NULL;
    }
}

PmatchObject * symbol_from_local_context(std::string & sym)
{
    if (symbol_in_local_context(sym)) {
        return call_stack.back()[sym];
    } else {
        return (PmatchObject *) NULL;
    }
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

std::string get_Ins_transition(const char *s)
{
    std::stringstream tmp;
    tmp << "@I." << s << "@";
    return tmp.str();
}

std::string get_RC_transition(const char *s)
{
    std::stringstream tmp;
    tmp << "@RC." << s << "@";
    return tmp.str();
}

std::string get_LC_transition(const char *s)
{
    std::stringstream tmp;
    tmp << "@LC." << s << "@";
    return tmp.str();
}

HfstTransducer * add_pmatch_delimiters(HfstTransducer * regex)
{
    HfstTransducer * delimited_regex = new HfstTransducer(hfst::internal_epsilon,
                                                          ENTRY_SYMBOL,
                                                          regex->get_type());
    delimited_regex->concatenate(*regex);
    delimited_regex->concatenate(HfstTransducer(hfst::internal_epsilon,
                                                EXIT_SYMBOL,
                                                regex->get_type()));
    delete regex;
    return delimited_regex;
}

PmatchTransducerContainer * make_end_tag(std::string tag)
{ return epsilon_to_symbol_container("@PMATCH_ENDTAG_" + tag + "@"); }

PmatchTransducerContainer * make_counter(std::string name)
{ return epsilon_to_symbol_container("@PMATCH_COUNTER_" + name + "@"); }

HfstTransducer * make_list(HfstTransducer * t, ImplementationType f)
{
    std::string arc = "@L.";
    hfst::StringSet alphabet = t->get_alphabet();
    for (hfst::StringSet::const_iterator it = alphabet.begin();
         it != alphabet.end(); ++it) {
        if (!hfst_ol::PmatchAlphabet::is_special(*it) &&
            *it != hfst::internal_epsilon && *it != hfst::internal_unknown &&
            *it != hfst::internal_identity && *it != hfst::internal_default) {
            arc.append(*it);
            arc.append("_");
        }
    }
    arc.append("@");
    return new HfstTransducer(arc, f);
}

HfstTransducer * make_exc_list(HfstTransducer * t, ImplementationType f)
{
    std::string arc = "@X.";
    hfst::StringSet alphabet = t->get_alphabet();
    for (hfst::StringSet::const_iterator it = alphabet.begin();
         it != alphabet.end(); ++it) {
        if (!hfst_ol::PmatchAlphabet::is_special(*it) &&
            *it != hfst::internal_epsilon && *it != hfst::internal_unknown &&
            *it != hfst::internal_identity && *it != hfst::internal_default) {
            arc.append(*it);
            arc.append("_");
        }
    }
    arc.append("@");
    return new HfstTransducer(arc, f);
}

HfstTransducer * make_sigma(HfstTransducer * t)
{
    HfstTransducer * retval =
        new HfstTransducer(format);
    hfst::StringSet alphabet = t->get_alphabet();
    for (hfst::StringSet::const_iterator it = alphabet.begin();
         it != alphabet.end(); ++it) {
        if (!hfst_ol::PmatchAlphabet::is_special(*it) &&
            *it != hfst::internal_epsilon && *it != hfst::internal_unknown &&
            *it != hfst::internal_identity && *it != hfst::internal_default) {
            retval->disjunct(HfstTransducer(*it, format));
        }
    }
    return retval;
}

PmatchTransducerContainer * epsilon_to_symbol_container(std::string s)
{
    HfstTransducer * tmp = new HfstTransducer(hfst::internal_epsilon, s, format);
    return new PmatchTransducerContainer(tmp);
}

PmatchTransducerContainer * make_rc_entry(void)
{ return epsilon_to_symbol_container(RC_ENTRY_SYMBOL); }
PmatchTransducerContainer * make_lc_entry(void)
{ return epsilon_to_symbol_container(LC_ENTRY_SYMBOL); }
PmatchTransducerContainer * make_nrc_entry(void)
{ return epsilon_to_symbol_container(NRC_ENTRY_SYMBOL); }
PmatchTransducerContainer * make_nlc_entry(void)
{ return epsilon_to_symbol_container(NLC_ENTRY_SYMBOL); }
PmatchTransducerContainer * make_rc_exit(void)
{ return epsilon_to_symbol_container(RC_EXIT_SYMBOL); }
PmatchTransducerContainer * make_lc_exit(void)
{ return epsilon_to_symbol_container(LC_EXIT_SYMBOL); }
PmatchTransducerContainer * make_nrc_exit(void)
{ return epsilon_to_symbol_container(NRC_EXIT_SYMBOL); }
PmatchTransducerContainer * make_nlc_exit(void)
{ return epsilon_to_symbol_container(NLC_EXIT_SYMBOL); }
PmatchTransducerContainer * make_passthrough(void)
{ return epsilon_to_symbol_container(PASSTHROUGH_SYMBOL); }

char * get_delimited(const char *s, char delim_left, char delim_right)
{
    const char *qstart = strchr((char*) s, delim_left) + 1;
    const char *qend = strrchr((char*) s, delim_right);
    char* qpart = strdup(qstart);
    *(qpart+ (size_t) (qend - qstart)) = '\0';
    return qpart;
}

char * get_delimited(const char *s, char delim)
{
    return get_delimited(s, delim, delim);
}

char * get_escaped_delimited(const char *s, char delim_left, char delim_right)
{
    return unescape_delimited(get_delimited(s, delim_left, delim_right), delim_right);
}
char * get_escaped_delimited(const char *s, char delim)
{
    return unescape_delimited(get_delimited(s, delim, delim), delim);
}

char * unescape_delimited(char *s, char delim)
{
    char * read = s;
    char * write = s;
    while (*read != '\0') {
        if (*read == '\\' && (*(read + 1) == delim || *(read + 1) == '\\')) {
            *write = *(read + 1);
            read += 2;
            write += 1;
        } else {
            *write = *read;
            ++read;
            ++write;
        }
    }
    *write = '\0';
    return s;
}

char*
parse_quoted(const char *s)
{
    char* quoted = get_delimited(s, '"');
    // Mysteriously, when the quoted string is 24 + n * 16 bytes in length, an
    // extra byte is needed for rv.
    char* rv = static_cast<char*>(malloc(sizeof(char)*(strlen(quoted) + 1)));
    char* p = quoted;
    char* r = rv;
    while (*p != '\0')
      {
        if (*p != '\\')
          {
            *r = *p;
            ++r;
            ++p;
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
                fprintf(stderr, "*** PMATCH unimplemented: "
                        "parse octal escape in %s", p);
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
                  if (strlen(p) < 6) {
                      // Can't be a valid escape sequence
                      *r++ = *p;
                      *r++ = *(p+1);
                      p += 2;
                  } else {
                      char buf[5];
                      memcpy(buf, p+2, 4);
                      buf[4] = '\0';
                      unsigned int codepoint = strtol(buf, NULL, 16);
                      std::string utf8_char = codepoint_to_utf8(codepoint);
                      strcpy(r, utf8_char.c_str());
                      r += utf8_char.size() + 1;
                      p += 6;
                  }
                  break;
              case 'U':
                  if (strlen(p) < 10) {
                      // Can't be a valid escape sequence
                      *r++ = *p;
                      *r++ = *(p+1);
                      p += 2;
                  } else {
                      char buf[9];
                      memcpy(buf, p+2, 8);
                      buf[8] = '\0';
                      unsigned int codepoint = strtol(buf, NULL, 16);
                      std::string utf8_char = codepoint_to_utf8(codepoint);
                      strcpy(r, utf8_char.c_str());
                      r += utf8_char.size() + 1;
                      p += 10;
                  }
                  break;
              case 'v':
                *r = '\v';
                r++;
                p = p + 2;
                break;
              case 'x':
                  {
                    char* endp;
                    int i = strtol(p + 2, &endp, 16);
                    if ( 0 < i && i <= 127)
                      {
                        *r = static_cast<char>(i);
                      }
                    else
                      {
                        fprintf(stderr, "*** PMATCH unimplemented: "
                                "parse \\x%d\n", i);
                        *r = '\0';
                      }
                    r++;
                    assert(endp != p);
                    p = endp;
                   break;
                }
              case '\0':
                fprintf(stderr, "End of line after \\ escape\n");
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
    return rv;
}

unsigned int next_utf8_to_codepoint(unsigned char **c)
{
    unsigned int codepoint = 0;
    int bytes_in_char = 0;
    if (**c <= 127) {
        bytes_in_char = 1;
        codepoint = **c & 127;
    } else if ( (**c & (128 + 64)) == (128 + 64) ) {
        bytes_in_char = 2;
        codepoint = **c & 31;
    } else if ( (**c & (128 + 64 + 32)) == (128 + 64 + 32) ) {
        bytes_in_char = 3;
        codepoint = **c & 15;
    } else if ( (**c & (128 + 64 + 32 + 16)) == (128 + 64 + 32 + 16)) {
        bytes_in_char = 4;
        codepoint = **c & 7;
    } else {
        return 0;
    }
    for (int i = 1; i < bytes_in_char; ++i) {
        codepoint = ((codepoint << 6) | (unsigned long)(*(*c + i) & 63));
    }
    *c += bytes_in_char;
    return codepoint;
}

std::string codepoint_to_utf8(unsigned int codepoint)
{
    char buf[5];
    bool u_parse_err = false;
    // The following is adapted from an answer at
    // http://stackoverflow.com/questions/4607413/c-library-to-convert-unicode-code-points-to-utf8
    // My understanding of the magic numbers:
    // 0x80 = 128 = 2^7
    // 64 = 2^6, 192 = 2^6 + 2^7
    // 0x800 = 2048 = 2^11
    // 0x1000 = 2^16 etc.
    if (codepoint < 0x80) {
        buf[0] = codepoint;
        buf[1] = '\0';
    } else if (codepoint < 0x800) {
        buf[0] = 192 + codepoint / 64;
        buf[1] = 128 + codepoint % 64;
        buf[2] = '\0';
    } else if (codepoint - 0xd800u < 0x800) {
        u_parse_err = true;
    } else if (codepoint < 0x10000) {
        buf[0] = 224 + codepoint / 4096;
        buf[1] = 128 + codepoint / 64 % 64;
        buf[2] = 128 + codepoint % 64;
        buf[3] = '\0';
    } else if (codepoint < 0x110000) {
        buf[0] = 240 + codepoint / 262144;
        buf[1] = 128 + codepoint / 4096 % 64;
        buf[2] = 128 + codepoint / 64 % 64;
        buf[3] = 128 + codepoint % 64;
        buf[4] = '\0';
    } else {
        u_parse_err = true;
    }
    if (u_parse_err) {
        return "";
    } else {
        return std::string(buf);;
    }
}

PmatchTransducerContainer * parse_range(const char * s)
{
    char * quoted = get_delimited(s, '"');
    char * orig_quoted = quoted;
    char ** c = & quoted;
    unsigned char bytes_in_char;
    HfstTransducer * retval = new HfstTransducer(format);
    while (**c != '\0') {
        unsigned int codepoint1 = 0;
        unsigned int codepoint2 = 0;
        if (strlen(*c) >= 6 && **c == '\\' &&
            (*(*c + 1) == 'u' || *(*c + 1) == 'U')) {
            // an escape sequence
            char buf[9];
            if (*(*c + 1) == 'u') {
                memcpy(buf, *c+2, 4);
                buf[4] = '\0';
                *c += 6;
            } else {
                memcpy(buf, *c+2, 8);
                buf[8] = '\0';
                *c += 10;
            }
            codepoint1 = strtol(buf, NULL, 16);
        } else {
            codepoint1 = next_utf8_to_codepoint((unsigned char**) c);
        }
        if (**c != '-') {
            std::string errstring("Could not parse range expression: ");
            errstring.append(std::string(s));
            pmatcherror(errstring.c_str());
        }
        *c += 1;
        if (strlen(*c) >= 6 && **c == '\\' &&
            (*(*c + 1) == 'u' || *(*c + 1) == 'U')) {
            char buf[9];
            if (*(*c + 1) == 'u') {
                memcpy(buf, *c+2, 4);
                buf[4] = '\0';
                *c += 6;
            } else {
                memcpy(buf, *c+2, 8);
                buf[8] = '\0';
                *c += 10;
            }
            codepoint2 = strtol(buf, NULL, 16);
        } else {
            codepoint2 = next_utf8_to_codepoint((unsigned char**) c);
        }
        if (codepoint1 == 0 || codepoint2 == 0) {
            std::string errstring("Malformed character in range expression: ");
            errstring.append(std::string(s));
            pmatcherror(errstring.c_str());
        }
        if (codepoint2 < codepoint1) {
            std::string errstring("Range expression goes from higher to lower: ");
            errstring.append(std::string(s));
            pmatcherror(errstring.c_str());
        }
        while (codepoint1 <= codepoint2) {
            retval->disjunct(HfstTransducer(codepoint_to_utf8(codepoint1), format));
            ++codepoint1;
        }
    }
    free(orig_quoted);
    return new PmatchTransducerContainer(retval);
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

void init_globals(void)
{
    definitions.clear();
    variables.clear();
    variables["count-patterns"] = "off";
    variables["delete-patterns"] = "off";
    variables["extract-patterns"] = "off";
    variables["locate-patterns"] = "off";
    variables["mark-patterns"] = "on";
    variables["max-context-length"] = "254";
    variables["max-recursion"] =  "5000";
    variables["need-separators"] = "on";
    def_insed_expressions.clear();
    inserted_names.clear();
    unsatisfied_insertions.clear();
    used_definitions.clear();
    function_names.clear();
    zero_minimization_guard();
    need_delimiters = false;
}

std::map<std::string, HfstTransducer*>
compile(const string& pmatch, map<string,HfstTransducer*>& defs,
        ImplementationType impl, bool be_verbose, bool do_flatten)
{
    // lock here?
    init_globals();
    data = strdup(pmatch.c_str());
    startptr = data;
    len = strlen(data);
    verbose = be_verbose;
    flatten = do_flatten;
    for (map<string, HfstTransducer*>::iterator it = defs.begin();
         it != defs.end(); ++it) {
        definitions[it->first] = new PmatchTransducerContainer(it->second);
    }
    format = impl;
    if (hfst::pmatch::verbose) {
        timer = clock();
        std::cerr << std::endl;
    }
    pmatchparse();
    free(startptr);
    data = 0;
    len = 0;
    std::map<std::string, hfst::HfstTransducer*> retval;
     for (std::set<std::string>::const_iterator it =
              unsatisfied_insertions.begin();
          it != unsatisfied_insertions.end(); ++it) {
         if (definitions.count(*it) == 0) {
             std::cerr << "Inserted transducer "
                       << *it << " was never defined!\n";
             return retval;
         }
     }
     if (hfst::pmatch::verbose) {
         std::map<std::string, PmatchObject*>::iterator defs_itr;
         for (defs_itr = definitions.begin(); defs_itr != definitions.end();
              ++defs_itr) {
             if (used_definitions.count(defs_itr->first) == 0 &&
                 defs_itr->first.compare("TOP") != 0) {
                 std::cerr << "Warning: " << defs_itr->first << " defined but never used\n";
             }
         }
     }

    if (pmatchnerrs != 0) {
        return retval;
    }
    // Our helper for harmonizing all the networks' alphabets with
    // each other
    if (hfst::pmatch::verbose) {
        std::cerr << "\nCompiling and harmonizing...\n";
        timer = clock();
    }

    if (inserted_names.size() > 0) {
        HfstTransducer dummy(format);
        // We keep TOP and any inserted transducers
        std::map<std::string, PmatchObject *>::iterator defs_it;
        for (defs_it = definitions.begin(); defs_it != definitions.end();
             ++defs_it) {
            if (defs_it->first.compare("TOP") == 0 ||
                inserted_names.count(defs_it->first) != 0) {
                HfstTransducer * tmp = defs_it->second->evaluate();
                tmp->minimize();
                dummy.harmonize(*tmp);
                retval[defs_it->first] = tmp;
            }
        }
        
        // Now that dummy is harmonized with everything, we harmonize everything
        // with dummy and minimize the results
        std::map<std::string, HfstTransducer *>::iterator tr_it;
        for(tr_it = retval.begin(); tr_it != retval.end(); ++tr_it) {
            tr_it->second->harmonize(dummy);
            tr_it->second->minimize();
        }
    } else {
        if (definitions.size() == 0) {
            std::cerr << "warning: pmatch compilation had an empty result\n";
                retval.insert(std::pair<std::string, hfst::HfstTransducer*>("TOP", new HfstTransducer(format)));
        } else if (definitions.count("TOP") == 0) {
            std::cerr << "Pmatch compilation warning: regex or TOP was undefined, using ";
            std::cerr << definitions.begin()->first << " as root\n";
            hfst::HfstTransducer * tmp = definitions.begin()->second->evaluate();
            tmp->minimize();
            retval.insert(std::pair<std::string, hfst::HfstTransducer*>("TOP", tmp));
        } else {
            hfst::HfstTransducer * tmp = definitions["TOP"]->evaluate();
            tmp->minimize();
            retval.insert(std::pair<std::string, hfst::HfstTransducer*>("TOP", tmp));
        }
    }
    if (hfst::pmatch::verbose) {
        double duration = (clock() - hfst::pmatch::timer) /
            (double) CLOCKS_PER_SEC;
        hfst::pmatch::timer = clock();
        std::cerr << "compiled and harmonized in " << duration << " seconds\n";
    }
    if (variables["need-separators"] == "on") {
        HfstTransducer not_whitespace(hfst::internal_identity, format);
        not_whitespace.subtract(*(get_utils()->latin1_whitespace_acceptor));
        HfstTransducer anything(hfst::internal_identity, format);
        anything.repeat_star();
        HfstTransducer begins_and_ends_with_non_whitespace(not_whitespace);
        begins_and_ends_with_non_whitespace.concatenate(anything);
        begins_and_ends_with_non_whitespace.concatenate(not_whitespace);
        begins_and_ends_with_non_whitespace.compose(*(retval["TOP"]));
        HfstTransducer empty(format);
        if (begins_and_ends_with_non_whitespace.compare(empty) == false) {
            HfstTransducer whitespace_punct_context(*(get_utils()->latin1_whitespace_acceptor));
            whitespace_punct_context.disjunct(*(get_utils()->latin1_punct_acceptor));
            whitespace_punct_context.disjunct(HfstTransducer("@BOUNDARY@", format));
            HfstTransducer * top_with_boundaries = new HfstTransducer(hfst::internal_epsilon, LC_ENTRY_SYMBOL, format);
            top_with_boundaries->concatenate(whitespace_punct_context);
            top_with_boundaries->concatenate(HfstTransducer(hfst::internal_epsilon, LC_EXIT_SYMBOL, format));
            HfstTransducer RC(hfst::internal_epsilon, RC_ENTRY_SYMBOL, format);
            RC.concatenate(whitespace_punct_context);
            RC.concatenate(HfstTransducer(hfst::internal_epsilon, RC_EXIT_SYMBOL, format));
            top_with_boundaries->concatenate(*(retval["TOP"]));
            top_with_boundaries->concatenate(RC);
            delete retval["TOP"];
            retval["TOP"] = add_pmatch_delimiters(top_with_boundaries);
        }
    }
    for(std::map<std::string, std::string>::iterator it = variables.begin();
        it != variables.end(); ++it) {
        retval["TOP"]->set_property(it->first, it->second);
    }
    return retval;
}

void print_size_info(HfstTransducer * net)
{
    if (!hfst::pmatch::verbose) {
        return;
    }
    HfstBasicTransducer tmp(*net);
    size_t states = 0;
    size_t arcs = 0;
    for(HfstBasicTransducer::const_iterator state_it = tmp.begin();
        state_it != tmp.end(); ++state_it) {
        ++states;
        for(HfstBasicTransducer::HfstTransitions::const_iterator tr_it =
                state_it->begin(); tr_it != state_it->end(); ++tr_it) {
            ++arcs;
        }
    }
    std::cerr << states <<
        " states and " << arcs << " arcs" << std::endl;
}

HfstTransducer * read_text(char * filename, ImplementationType type)
{
    std::ifstream infile;
    std::string line;
    infile.open(filename);
    HfstTokenizer tok;
    HfstTransducer * retval = new HfstTransducer(type);
    if(!infile.good()) {
        std::cerr << "Pmatch: could not open text file " << filename <<
            " for reading\n";
    } else {
        size_t n = 0;
        while(infile.good()) {
            std::getline(infile, line);
            if(!line.empty()) {
                ++n;
                StringPairVector spv = tok.tokenize(line);
                retval->disjunct(spv);
            }
        }
    }
    infile.close();
    return retval;
}

std::vector<std::vector<std::string> > read_args(char * filename, unsigned int argcount)
{
    std::ifstream infile;
    std::string line;
    infile.open(filename);
    std::vector<std::vector <std::string> > retval;
    std::vector<std::string> current_tokens;
    if(!infile.good()) {
        std::cerr << "Pmatch: could not open text file " << filename <<
            " for reading\n";
    } else {
        size_t n = 0;
        while(infile.good()) {
            std::getline(infile, line);
            if(!line.empty()) {
                current_tokens.clear();
                ++n;
                int curpos;
                int nextpos = -1;
                do {
                    curpos = nextpos + 1;
                    nextpos = line.find_first_of(" ", curpos);
                    current_tokens.push_back(line.substr(curpos, nextpos - curpos));
                } while (nextpos != std::string::npos);
                if (current_tokens.size() != argcount) {
                    std::cerr << "Pmatch: line " << n << " in " << filename << " contained "
                              << current_tokens.size() << " tokens, expected " << argcount << std::endl;
                } else {
                    retval.push_back(current_tokens);
                }
            }
        }
    }
    infile.close();
    return retval;
}

PmatchUtilityTransducers::PmatchUtilityTransducers(void)
{
    latin1_acceptor = make_latin1_acceptor();
    latin1_alpha_acceptor = make_latin1_alpha_acceptor();
    latin1_lowercase_acceptor = make_latin1_lowercase_acceptor();
    latin1_uppercase_acceptor = make_latin1_uppercase_acceptor();
    combining_accent_acceptor = make_combining_accent_acceptor();
    latin1_numeral_acceptor = make_latin1_numeral_acceptor();
    latin1_punct_acceptor = make_latin1_punct_acceptor();
    latin1_whitespace_acceptor = make_latin1_whitespace_acceptor();
    lowerfy = make_lowerfy();
    capify = make_capify();
}

PmatchUtilityTransducers::~PmatchUtilityTransducers(void)
{
    delete latin1_acceptor;
    delete latin1_alpha_acceptor;
    delete latin1_lowercase_acceptor;
    delete latin1_uppercase_acceptor;
    delete combining_accent_acceptor;
    delete latin1_numeral_acceptor;
    delete latin1_punct_acceptor;
    delete latin1_whitespace_acceptor;
    delete lowerfy;
    delete capify;
}

HfstTransducer * PmatchUtilityTransducers::make_latin1_acceptor(ImplementationType type)
  {
      HfstTransducer * retval = make_latin1_alpha_acceptor();
      HfstTransducer * tmp = make_latin1_numeral_acceptor();
      retval->disjunct(*tmp); delete tmp;
      tmp = make_latin1_punct_acceptor(); retval->disjunct(*tmp); delete tmp;
      tmp = make_latin1_whitespace_acceptor(); retval->disjunct(*tmp); delete tmp;
      retval->minimize();
      return retval;
  }

HfstTransducer * PmatchUtilityTransducers::make_latin1_alpha_acceptor(ImplementationType type)
  {
      HfstTransducer * retval = make_latin1_lowercase_acceptor();
      HfstTransducer * tmp = make_latin1_uppercase_acceptor();
      retval->disjunct(*tmp); delete tmp;
      retval->minimize();
      return retval;
  }

HfstTransducer * PmatchUtilityTransducers::make_latin1_lowercase_acceptor(ImplementationType type)
  {
      HfstTransducer * retval = acceptor_from_cstr(latin1_lower, type);
      HfstTransducer * tmp = make_combining_accent_acceptor();
      retval->disjunct(*tmp); delete tmp;
      retval->minimize();
      return retval;
  }

HfstTransducer * PmatchUtilityTransducers::make_latin1_uppercase_acceptor(ImplementationType type)
  {
      HfstTransducer * retval = acceptor_from_cstr(latin1_upper, type);
      HfstTransducer * tmp = make_combining_accent_acceptor();
      retval->disjunct(*tmp); delete tmp;
      retval->minimize();
      return retval;
  }

HfstTransducer * PmatchUtilityTransducers::make_combining_accent_acceptor(ImplementationType type)
  {
      return acceptor_from_cstr(combining_accents, type);
  }

HfstTransducer * PmatchUtilityTransducers::make_latin1_numeral_acceptor(ImplementationType type)
  {
      HfstTransducer * retval = new HfstTransducer(type);
      const std::string num =
          "0123456789";
      for (std::string::const_iterator it = num.begin(); it != num.end();
           ++it) {
          retval->disjunct(HfstTransducer(std::string(1, *it), type));
      }
      return retval;
      retval->minimize();
  }

HfstTransducer * PmatchUtilityTransducers::make_latin1_punct_acceptor(ImplementationType type)
  {
      return acceptor_from_cstr(latin1_punct, type);
  }

HfstTransducer * PmatchUtilityTransducers::make_latin1_whitespace_acceptor(ImplementationType type)
{
    return acceptor_from_cstr(latin1_whitespace, type);
}

HfstTransducer * PmatchUtilityTransducers::make_capify(ImplementationType type)
{
    HfstTransducer * retval = new HfstTransducer(type);
    HfstTokenizer tok;
    for (size_t i = 0; i < array_len(latin1_upper); ++i) {
        retval->disjunct(HfstTransducer(latin1_lower[i], latin1_upper[i],
                                        tok, type));
    }
    HfstTransducer accents(*combining_accent_acceptor);
    accents.optionalize();
    retval->concatenate(accents);
    retval->minimize();
    return retval;
}

HfstTransducer * PmatchUtilityTransducers::make_lowerfy(ImplementationType type)
{
    HfstTransducer * retval = new HfstTransducer(type);
    HfstTokenizer tok;
    for (size_t i = 0; i < array_len(latin1_upper); ++i) {
        retval->disjunct(HfstTransducer(latin1_upper[i], latin1_lower[i],
                                        tok, type));
    }
    HfstTransducer accents(*combining_accent_acceptor);
    accents.optionalize();
    retval->concatenate(accents);
    retval->minimize();
    return retval;
}

HfstTransducer * PmatchUtilityTransducers::cap(HfstTransducer & t, Side side, bool optional)
{
    HfstTransducer * retval;
    HfstTransducer cap(*capify);
    HfstTransducer decap(cap);
    decap.invert();
    HfstTransducer anything(HfstTransducer::identity_pair(t.get_type()));
    HfstTransducer anything_but_whitespace_star(anything);
    anything_but_whitespace_star.subtract(*latin1_whitespace_acceptor);
    anything_but_whitespace_star.repeat_star();
    if (optional == false) {
        // don't let lowercased first letters through
        anything.subtract(*latin1_lowercase_acceptor);
    }
    // As in the regexp
    // [[[["A":"a" [[\" "]* (" " "A":"a")]* ] .o. [{ab ad}:{ef eh}].u]] .o.
    //   [{ab ad}:{ef eh}] ] .o. [[{ab ad}:{ef eh}].l] .o.
    //   ["e":"E" [[\" "]+ (" " "e":"E")]*]
    if (side == Lower) {
        retval = new HfstTransducer(t);
        cap.disjunct(anything);
        // Cap is the first letter to either capitalize or accept if it's not a
        // lowercase letter
        HfstTransducer continuation(anything_but_whitespace_star);
        // continuation is the rest of the first word
        HfstTransducer more_caps(*latin1_whitespace_acceptor);
        // more_caps is more words to capitalize
        more_caps.concatenate(cap);
        more_caps.optionalize();
        continuation.concatenate(more_caps);
        continuation.repeat_star();
        cap.concatenate(continuation);
        retval->compose(cap);
    } else if (side == Upper) {
        decap.disjunct(anything);
        HfstTransducer continuation(anything_but_whitespace_star);
        HfstTransducer more_decaps(*latin1_whitespace_acceptor);
        more_decaps.concatenate(decap);
        more_decaps.optionalize();
        continuation.concatenate(more_decaps);
        continuation.repeat_star();
        retval = new HfstTransducer(decap);
        retval->concatenate(continuation);
        retval->compose(t);
    } else { // both
        decap.disjunct(anything);
        HfstTransducer continuation(anything_but_whitespace_star);
        HfstTransducer more_decaps(*latin1_whitespace_acceptor);
        more_decaps.concatenate(decap);
        more_decaps.optionalize();
        continuation.concatenate(more_decaps);
        continuation.repeat_star();
        retval = new HfstTransducer(decap);
        retval->concatenate(continuation);
        retval->compose(t);
        HfstTransducer continuation2(anything_but_whitespace_star);
        HfstTransducer more_caps(*latin1_whitespace_acceptor);
        cap.disjunct(anything);
        more_caps.concatenate(cap);
        more_caps.optionalize();
        continuation2.concatenate(more_caps);
        continuation2.repeat_star();
        cap.concatenate(continuation2);
        retval->compose(cap);
    }
    retval->minimize();
    return retval;
}

HfstTransducer * PmatchUtilityTransducers::tolower(HfstTransducer & t, Side side, bool optional)
{
    HfstTransducer anything(hfst::internal_identity, hfst::pmatch::format);
    if (optional == false) {
        anything.subtract(*latin1_uppercase_acceptor);
    }
    HfstTransducer * retval;
    if (side == Lower) {
        HfstTransducer lowercase(*lowerfy);
        lowercase.disjunct(anything);
        lowercase.repeat_star();
        retval = new HfstTransducer(t);
        retval->compose(lowercase);
    } else if (side == Upper) {
        retval = new HfstTransducer(*capify);
        retval->disjunct(anything);
        retval->repeat_star();
        retval->compose(t);
    } else { // both
        retval = new HfstTransducer(*capify);
        retval->disjunct(anything);
        retval->repeat_star();
        retval->compose(t);
        HfstTransducer lowercase(*lowerfy);
        lowercase.disjunct(anything);
        lowercase.repeat_star();
        retval->compose(lowercase);
    }
    retval->minimize();
    return retval;
}

HfstTransducer * PmatchUtilityTransducers::toupper(HfstTransducer & t, Side side, bool optional)
{
    HfstTransducer anything(hfst::internal_identity, hfst::pmatch::format);
    if (optional == false) {
        anything.subtract(*latin1_lowercase_acceptor);
    }
    HfstTransducer * retval;
    if (side == Lower) {
        HfstTransducer uppercase(*capify);
        uppercase.disjunct(anything);
        uppercase.repeat_star();
        retval = new HfstTransducer(t);
        retval->compose(uppercase);
    } else if (side == Upper) {
        retval = new HfstTransducer(*lowerfy);
        retval->disjunct(anything);
        retval->repeat_star();
        retval->compose(t);
    } else { // both
        retval = new HfstTransducer(*lowerfy);
        retval->disjunct(anything);
        retval->repeat_star();
        retval->compose(t);
        HfstTransducer uppercase(*capify);
        uppercase.disjunct(anything);
        uppercase.repeat_star();
        retval->compose(uppercase);
    }
    retval->minimize();
    return retval;
}

PmatchObject::PmatchObject(void)
{
    name = "";
    weight = 0.0;
    line_defined = pmatchlineno;
    cache = (HfstTransducer*) (NULL);
}

HfstTransducer * PmatchObject::evaluate(std::vector<PmatchObject *> args)
{
    if (args.size() == 0) {
        if (should_use_cache()) {
            if (cache == NULL) {
                start_timing();
                cache = evaluate();
                report_time();
            }
            return new HfstTransducer(*cache);
        } else {
            start_timing();
            HfstTransducer * retval = evaluate();
            retval->minimize();
            report_time();
            return retval;
        }
    } else {
        std::stringstream errstring;
        errstring << "Object " << name << " on line " << pmatchlineno << " has no argument handling";
        throw std::invalid_argument(errstring.str());
    }
}

HfstTransducer * PmatchSymbol::evaluate(PmatchEvalType eval_type)
{
    start_timing();
    HfstTransducer * retval;
    if (symbol_in_local_context(sym)) {
        retval = symbol_from_local_context(sym)->evaluate();
    } else if (symbol_in_global_context(sym)) {
        if (flatten && def_insed_expressions.count(sym) == 1) {
            retval = def_insed_expressions[sym]->evaluate();
        } else {
            retval = symbol_from_global_context(sym)->evaluate();
        }
        used_definitions.insert(sym);
    } else {
        if (verbose) {
            std::cerr << "Warning: interpreting undefined symbol \"" << sym
                      << "\" as label on line " << line_defined << "\n";
        }
        retval = new HfstTransducer(sym, format);
    }
    retval->set_final_weights(weight, true);
    retval->minimize();
    report_time();
    return retval;
}

HfstTransducer * PmatchString::evaluate(PmatchEvalType eval_type) {
    if (cache != NULL && should_use_cache()) {
        return new HfstTransducer(*cache);
    }
    start_timing();
    HfstTransducer * tmp;
    if(multichar) {
        HfstTokenizer tok;
        tmp = new HfstTransducer(string, tok, format);
    } else {
        tmp = new HfstTransducer(string, format);
    }
    tmp->set_final_weights(weight, true);
    if (cache == NULL && should_use_cache() == true) {
        cache = tmp;
        cache->minimize();
        report_time();
        return new HfstTransducer(*cache);
    }
        report_time();
    return tmp;
}

HfstTransducer * PmatchFunction::evaluate(std::vector<PmatchObject *> funargs)
{
    if (verbose) {
        my_timer = clock();
    }
    if (funargs.size() != args.size()) {
        std::stringstream errstring;
        errstring << "Function " << name << " expected " << args.size() << " args, got " << funargs.size() << std::endl;
        throw std::invalid_argument(errstring.str());
    }
    std::map<std::string, PmatchObject *> local_env;
    if (call_stack.size() != 0) {
        local_env = call_stack.back();
    };
    for (int i = 0; i < args.size(); ++i) {
        local_env[args[i]] = funargs[i];
    }
    call_stack.push_back(local_env);
    HfstTransducer * retval = root->evaluate();
    retval->set_final_weights(weight, true);
    call_stack.pop_back();
    if (verbose) {
        double duration = (clock() - my_timer) /
            (double) CLOCKS_PER_SEC;
        std::cerr << "Call to " << name << " evaluated in " << duration << " seconds\n";
    }
    return retval;
}

HfstTransducer * PmatchFunction::evaluate(PmatchEvalType eval_type)
{
    std::vector<PmatchObject *> funargs;
    return evaluate(funargs);
}

HfstTransducer * PmatchBuiltinFunction::evaluate(PmatchEvalType eval_type)
{
    start_timing();
    HfstTransducer * retval;
    if (type == Interpolate) {
        if (args->size() < 3) {
            std::stringstream errstring;
            errstring << "Builtin function Interpolate called with " << args->size() << " arguments, but it expects at least 3." << std::endl;
            throw std::invalid_argument(errstring.str());
        }
        // arguments are in reverse order after parsing
        retval = (*(args->rbegin() + 1))->evaluate();
        HfstTransducer * interpolator = (*(args->rbegin()))->evaluate();
        for(std::vector<PmatchObject*>::reverse_iterator it = args->rbegin() + 2;
            it != args->rend(); ++it) {
            HfstTransducer * tmp = (*it)->evaluate();
            retval->concatenate(*interpolator);
            retval->concatenate(*tmp);
            delete tmp;
        }
        delete interpolator;
    }
    retval->set_final_weights(weight, true);
    report_time();
    return retval;
}

HfstTransducer * PmatchNumericOperation::evaluate(PmatchEvalType eval_type)
{
    if (cache != NULL && should_use_cache()) {
        return new HfstTransducer(*cache);
    }
    HfstTransducer * tmp;
    start_timing();
    tmp = root->evaluate();
    if (op == RepeatN) {
        tmp->repeat_n(values[0]);
    } else if (op == RepeatNPlus) {
        tmp->repeat_n_plus(values[0]);
    } else if (op == RepeatNMinus) {
        tmp->repeat_n_minus(values[0]);
    } else if (op == RepeatNToK) {
        tmp->repeat_n_to_k(values[0], values[1]);
    }
    tmp->set_final_weights(weight, true);
    if (cache == NULL && should_use_cache() == true) {
        cache = tmp;
        cache->minimize();
        report_time();
        return new HfstTransducer(*cache);
    }
    report_time();
    return tmp;
}

HfstTransducer * PmatchUnaryOperation::evaluate(PmatchEvalType eval_type)
{
    if (cache != NULL && should_use_cache()) {
        return new HfstTransducer(*cache);
    }
    HfstTransducer * retval;
    start_timing();
    retval = root->evaluate();
    if (op == AddDelimiters) {
        retval = add_pmatch_delimiters(retval);
    } else if (op == Optionalize) {
        retval->optionalize();
    } else if (op == RepeatStar) {
        retval->repeat_star();
    } else if (op == RepeatPlus) {
        retval->repeat_plus();
    } else if (op == Reverse) {
        retval->reverse();
    } else if (op == Invert) {
        retval->invert();
    } else if (op == InputProject) {
        retval->input_project();
    } else if (op == OutputProject) {
        retval->output_project();
    } else if (op == Complement) {
        // Defined here only for automata, so can project to input
        HfstTransducer * complement =
            new HfstTransducer(hfst::internal_identity, hfst::pmatch::format);
        complement->repeat_star();
        complement->subtract(*retval);
        delete retval;
        retval = complement;
    } else if (op == Containment) {
        HfstTransducer* left = new HfstTransducer(hfst::internal_identity,
                                                  hfst::pmatch::format);
        HfstTransducer* right = new HfstTransducer(hfst::internal_identity,
                                                   hfst::pmatch::format);
        right->repeat_star();
        left->repeat_star();
        retval->repeat_star();
        left->concatenate(*retval);
        left->concatenate(*right);
        delete retval; delete right;
        retval = left;
    } else if (op == ContainmentOnce) {
        HfstTransducer* left = new HfstTransducer(hfst::internal_identity,
                                                  hfst::pmatch::format);
        HfstTransducer* right = new HfstTransducer(hfst::internal_identity,
                                                   hfst::pmatch::format);
        left->subtract(*retval);
        right->subtract(*retval);
        right->repeat_star();
        left->repeat_star();
        left->concatenate(*retval);
        left->concatenate(*right);
        delete retval; delete right;
        retval = left;
    } else if (op == ContainmentOptional) {
        HfstTransducer* left = new HfstTransducer(hfst::internal_identity,
                                                  hfst::pmatch::format);
        HfstTransducer* right = new HfstTransducer(hfst::internal_identity,
                                                   hfst::pmatch::format);
        left->subtract(*retval);
        right->subtract(*retval);
        right->repeat_star();
        left->repeat_star();
        retval->optionalize();
        left->concatenate(*retval);
        left->concatenate(*right);
        delete retval; delete right;
        retval = left;
    } else if (op == TermComplement) {
        HfstTransducer* any = new HfstTransducer(hfst::internal_identity,
                                                 hfst::pmatch::format);
        any->subtract(*retval);
        delete retval;
        retval = any;
    } else if (op == Cap) {
        HfstTransducer * tmp = get_utils()->cap(*retval);
        delete retval;
        retval = tmp;
    } else if (op == OptCap) {
        HfstTransducer * tmp = get_utils()->cap(*retval, Both, true);
        delete retval;
        retval = tmp;
    } else if (op == ToLower) {
        HfstTransducer * tmp = get_utils()->tolower(*retval);
        delete retval;
        retval = tmp;
    } else if (op == ToUpper) {
        HfstTransducer * tmp = get_utils()->toupper(*retval);
        delete retval;
        retval = tmp;
    } else if (op == OptToLower) {
        HfstTransducer * tmp = get_utils()->tolower(*retval, Both, true);
        tmp->disjunct(*retval);
        delete retval;
        retval = tmp;
    } else if (op == OptToUpper) {
        HfstTransducer * tmp = get_utils()->toupper(*retval, Both, true);
        delete retval;
        retval = tmp;
    } else if (op == AnyCase) {
        HfstTransducer * toupper = get_utils()->toupper(*retval, Both, true);
        HfstTransducer * tolower = get_utils()->tolower(*retval, Both, true);
        retval->disjunct(*toupper);
        retval->disjunct(*tolower);
        delete toupper; delete tolower;
    } else if (op == CapUpper) {
        HfstTransducer * tmp = get_utils()->cap(*retval, Upper);
        delete retval;
        retval = tmp;
    } else if (op == OptCapUpper) {
        HfstTransducer * tmp = get_utils()->cap(*retval, Upper, true);
        delete retval;
        retval = tmp;
    } else if (op == ToLowerUpper) {
        HfstTransducer * tmp = get_utils()->tolower(*retval, Upper);
        delete retval;
        retval = tmp;
    } else if (op == ToUpperUpper) {
        HfstTransducer * tmp = get_utils()->toupper(*retval, Upper);
        delete retval;
        retval = tmp;
    } else if (op == OptToLowerUpper) {
        HfstTransducer * tmp = get_utils()->tolower(*retval, Upper, true);
        tmp->disjunct(*retval);
        delete retval;
        retval = tmp;
    } else if (op == OptToUpperUpper) {
        HfstTransducer * tmp = get_utils()->toupper(*retval, Upper, true);
        delete retval;
        retval = tmp;
    } else if (op == AnyCaseUpper) {
        HfstTransducer * toupper = get_utils()->toupper(*retval, Upper, true);
        HfstTransducer * tolower = get_utils()->tolower(*retval, Upper, true);
        retval->disjunct(*toupper);
        retval->disjunct(*tolower);
        delete toupper; delete tolower;
    } else if (op == CapLower) {
        HfstTransducer * tmp = get_utils()->cap(*retval, Lower);
        delete retval;
        retval = tmp;
    } else if (op == OptCapLower) {
        HfstTransducer * tmp = get_utils()->cap(*retval, Lower, true);
        delete retval;
        retval = tmp;
    } else if (op == ToLowerLower) {
        HfstTransducer * tmp = get_utils()->tolower(*retval, Lower);
        delete retval;
        retval = tmp;
    } else if (op == ToUpperLower) {
        HfstTransducer * tmp = get_utils()->toupper(*retval, Lower);
        delete retval;
        retval = tmp;
    } else if (op == OptToLowerLower) {
        HfstTransducer * tmp = get_utils()->tolower(*retval, Lower, true);
        tmp->disjunct(*retval);
        delete retval;
        retval = tmp;
    } else if (op == OptToUpperLower) {
        HfstTransducer * tmp = get_utils()->toupper(*retval, Lower, true);
        delete retval;
        retval = tmp;
    } else if (op == AnyCaseLower) {
        HfstTransducer * toupper = get_utils()->toupper(*retval, Lower, true);
        HfstTransducer * tolower = get_utils()->tolower(*retval, Lower, true);
        retval->disjunct(*toupper);
        retval->disjunct(*tolower);
        delete toupper; delete tolower;
    } else if (op == MakeSigma) {
        HfstTransducer * tmp = make_sigma(retval);
        delete retval;
        retval = tmp;
    } else if (op == MakeList) {
        if (!flatten) {
            HfstTransducer * tmp = make_list(retval);
            delete retval;
            retval = tmp;
        }
    } else if (op == MakeExcList) {
        if (!flatten) {
            HfstTransducer * tmp = make_exc_list(retval);
            delete retval;
            retval = tmp;
        }
    }
    retval->set_final_weights(weight, true);
    if (cache == NULL && should_use_cache() == true) {
        cache = retval;
        cache->minimize();
        report_time();
        print_size_info(cache);
        return new HfstTransducer(*cache);
    }
    report_time();
    return retval;
}

HfstTransducer * PmatchBinaryOperation::evaluate(PmatchEvalType eval_type)
{
    if (cache != NULL && should_use_cache()) {
        return new HfstTransducer(*cache);
    }
    start_timing();
    HfstTransducer * retval;
    HfstTransducer * lhs = left->evaluate();
    HfstTransducer * rhs = right->evaluate();
    if (op == Concatenate) {
        lhs->concatenate(*rhs);
    } else if (op == Compose) {
        lhs->compose(*rhs);
    } else if (op == CrossProduct) {
        lhs->cross_product(*rhs);
    } else if (op == LenientCompose) {
        lhs->lenient_composition(*rhs);
    } else if (op == Disjunct) {
        lhs->disjunct(*rhs);
    } else if (op == Intersect) {
        lhs->intersect(*rhs);
    } else if (op == Subtract) {
        lhs->subtract(*rhs);
    } else if (op == UpperSubtract) {
        pmatcherror("Upper subtraction not implemented.");
        return lhs;
    } else if (op == LowerSubtract) {
        pmatcherror("Lower subtraction not implemented.");
        return lhs;
    } else if (op == UpperPriorityUnion) {
        lhs->priority_union(*rhs);
    } else if (op == LowerPriorityUnion) {
        lhs->invert();
        rhs->invert();
        lhs->priority_union(*rhs);
        lhs->invert();
    } else if (op == Shuffle) {
        try {
            lhs->shuffle(*rhs);
        } catch (const TransducersAreNotAutomataException & e) {
            pmatchwarning("tried to shuffle with non-automaton transducers,\n"
                          "    shuffling with their input projection instead.");
            lhs->input_project();
            rhs->input_project();
            lhs->shuffle(*rhs);
        }
    } else if (op == Before) {
        HfstTransducer * tmp = new HfstTransducer(hfst::xeroxRules::before(*lhs, *rhs));
        delete lhs;
        lhs = tmp;
    } else if (op == After) {
        HfstTransducer * tmp = new HfstTransducer(hfst::xeroxRules::after(*lhs, *rhs));
        delete lhs;
        lhs = tmp;
    } else if (op == InsertFreely) {
        lhs->insert_freely(*rhs, false);
    } else if (op == IgnoreInternally) {
        HfstTransducer * right_part = new HfstTransducer(*lhs);
        HfstTransducer * middle_part = new HfstTransducer(*lhs);
        middle_part->disjunct(*rhs);
        middle_part->repeat_star();
        lhs->concatenate(*middle_part);
        lhs->concatenate(*right_part);
        delete middle_part;
        delete right_part;
    } else if (op == Merge) {
        HfstTransducer * tmp;
        try {
            tmp = hfst::xre::merge_first_to_second(lhs, rhs);
        }
        catch (const TransducersAreNotAutomataException & e) {
            pmatcherror("Error: transducers must be automata in merge operation.");
        }
        delete lhs; lhs = tmp;
    }
    delete rhs;
    lhs->set_final_weights(weight, true);
    retval = lhs;
    if (cache == NULL && should_use_cache() == true) {
        cache = retval;
        cache->minimize();
        print_size_info(cache);
        report_time();
        return new HfstTransducer(*cache);
    }
    report_time();
    return retval;
}

StringPair PmatchBinaryOperation::as_string_pair(void)
{
    if (op == CrossProduct) {
        std::string left_string = left->as_string();
        std::string right_string = right->as_string();
        return StringPair(left_string, right_string);
    }
    return StringPair("", "");
}

HfstTransducer * PmatchTernaryOperation::evaluate(PmatchEvalType eval_type)
{
    if (cache != NULL && should_use_cache()) {
        return new HfstTransducer(*cache);
    }
    start_timing();
    HfstTransducer * retval;
    if (op == Substitute) {
        retval = left->evaluate();
        StringPair middle_pair = middle->as_string_pair();
        StringPair right_pair = right->as_string_pair();
        if (right_pair.first != "" || right_pair.second != "") {
            retval->substitute(middle_pair, right_pair);
        } else {
            HfstTransducer * tmp = right->evaluate();
            retval->substitute(middle_pair, *tmp);
            delete tmp;
        }
    }
    retval->set_final_weights(weight, true);
    if (cache == NULL && should_use_cache() == true) {
        cache = retval;
        cache->minimize();
        report_time();
        return new HfstTransducer(*cache);
    }
    report_time();
    return retval;
}

HfstTransducer * PmatchAcceptor::evaluate(PmatchEvalType eval_type)
{
    start_timing();
    HfstTransducer * retval;
    switch(set) {
    case Alpha:
        retval = new HfstTransducer(* get_utils()->latin1_alpha_acceptor);
        break;
    case UppercaseAlpha:
        retval = new HfstTransducer(* get_utils()->latin1_uppercase_acceptor);
        break;
    case LowercaseAlpha:
        retval = new HfstTransducer(* get_utils()->latin1_lowercase_acceptor);
        break;
    case Numeral:
        retval = new HfstTransducer(* get_utils()->latin1_numeral_acceptor);
        break;
    case Punctuation:
        retval = new HfstTransducer(* get_utils()->latin1_punct_acceptor);
        break;
    case Whitespace:
        retval = new HfstTransducer(* get_utils()->latin1_whitespace_acceptor);
    }
    retval->set_final_weights(weight, true);
    report_time();
    return retval;
}

HfstTransducer * PmatchParallelRulesContainer::evaluate(PmatchEvalType eval_type)
{
    if (cache != NULL && should_use_cache()) {
        return new HfstTransducer(*cache);
    }
    start_timing();
    HfstTransducer * retval;
    switch (arrow) {
    case hfst::xeroxRules::E_REPLACE_RIGHT:
        retval = new HfstTransducer(replace(make_mappings(), false));
        break;
    case hfst::xeroxRules::E_OPTIONAL_REPLACE_RIGHT:
        retval = new HfstTransducer(replace(make_mappings(), true));
        break;
    case hfst::xeroxRules::E_REPLACE_LEFT:
        retval = new HfstTransducer(replace_left(make_mappings(), false));
        break;
    case hfst::xeroxRules::E_OPTIONAL_REPLACE_LEFT:
        retval = new HfstTransducer(replace_left(make_mappings(), true));
        break;
    case hfst::xeroxRules::E_RTL_LONGEST_MATCH:
        retval = new HfstTransducer(replace_rightmost_longest_match(make_mappings()));
        break;
    case hfst::xeroxRules::E_RTL_SHORTEST_MATCH:
        retval = new HfstTransducer(replace_rightmost_shortest_match(make_mappings()));
        break;
    case hfst::xeroxRules::E_LTR_LONGEST_MATCH:
        retval = new HfstTransducer(replace_leftmost_longest_match(make_mappings()));
        break;
    case hfst::xeroxRules::E_LTR_SHORTEST_MATCH:
        retval = new HfstTransducer(replace_leftmost_shortest_match(make_mappings()));
        break;
    case hfst::xeroxRules::E_REPLACE_RIGHT_MARKUP:
    default:
        pmatcherror("Unrecognized arrow type");
        return (HfstTransducer *) NULL;
    }
    retval->set_final_weights(weight, true);
    report_time();
    if (cache == NULL && should_use_cache() == true) {
        cache = retval;
        cache->minimize();
        return new HfstTransducer(*cache);
    }
    return retval;
}

std::vector<hfst::xeroxRules::Rule> PmatchParallelRulesContainer::make_mappings(void)
{
    std::vector<hfst::xeroxRules::Rule> retval;
    for(std::vector<PmatchReplaceRuleContainer *>::iterator it = rules.begin();
        it != rules.end(); ++it) {
        retval.push_back((*it)->make_mapping());
    }
    
    return retval;
}

HfstTransducer * PmatchReplaceRuleContainer::evaluate(PmatchEvalType eval_type)
{
    if (cache != NULL && should_use_cache()) {
        return new HfstTransducer(*cache);
    }
    start_timing();
    HfstTransducer * retval;
    switch (arrow) {
    case hfst::xeroxRules::E_REPLACE_RIGHT:
        retval = new HfstTransducer(replace(make_mapping(), false));
        break;
    case hfst::xeroxRules::E_OPTIONAL_REPLACE_RIGHT:
        retval = new HfstTransducer(replace(make_mapping(), true));
        break;
    case hfst::xeroxRules::E_REPLACE_LEFT:
        retval = new HfstTransducer(replace_left(make_mapping(), false));
        break;
    case hfst::xeroxRules::E_OPTIONAL_REPLACE_LEFT:
        retval = new HfstTransducer(replace_left(make_mapping(), true));
        break;
    case hfst::xeroxRules::E_RTL_LONGEST_MATCH:
        retval = new HfstTransducer(replace_rightmost_longest_match(make_mapping()));
        break;
    case hfst::xeroxRules::E_RTL_SHORTEST_MATCH:
        retval = new HfstTransducer(replace_rightmost_shortest_match(make_mapping()));
        break;
    case hfst::xeroxRules::E_LTR_LONGEST_MATCH:
        retval = new HfstTransducer(replace_leftmost_longest_match(make_mapping()));
        break;
    case hfst::xeroxRules::E_LTR_SHORTEST_MATCH:
        retval = new HfstTransducer(replace_leftmost_shortest_match(make_mapping()));
        break;
    case hfst::xeroxRules::E_REPLACE_RIGHT_MARKUP:
    default:
        pmatcherror("Unrecognized arrow");
        return (HfstTransducer *) NULL;
    }
    retval->set_final_weights(weight, true);
    report_time();
    if (cache == NULL && should_use_cache() == true) {
        cache = retval;
        cache->minimize();
        return new HfstTransducer(*cache);
    }
    return retval;
}

hfst::xeroxRules::Rule PmatchReplaceRuleContainer::make_mapping(void)
{
    HfstTransducerPairVector pair_vector;
    for(MappingPairVector::iterator it = mapping.begin();
        it != mapping.end(); ++it) {
        HfstTransducer * l = it->first->evaluate();
        HfstTransducer * r = it->second->evaluate();
        pair_vector.push_back(HfstTransducerPair(HfstTransducer(*l),
                                                 HfstTransducer(*r)));
        delete l; delete r;
    }
    if (context.size() == 0) {
        return hfst::xeroxRules::Rule(pair_vector);
    }
    HfstTransducerPairVector context_vector;
    for (MappingPairVector::iterator it = context.begin();
         it != context.end(); ++it) {
        HfstTransducer * l = it->first->evaluate();
        HfstTransducer * r = it->second->evaluate();
        context_vector.push_back(HfstTransducerPair(HfstTransducer(*l),
                                                    HfstTransducer(*r)));
        delete l; delete r;
    }
    return hfst::xeroxRules::Rule(pair_vector, context_vector, type);
}

HfstTransducer * PmatchQuestionMark::evaluate(PmatchEvalType eval_type)
{
    start_timing();
    HfstTransducer * retval;
    if (eval_type == Transducer) {
        retval = new HfstTransducer(hfst::internal_identity, format);
    } else {
        retval = new HfstTransducer(hfst::internal_unknown, format);
    }
    retval->set_final_weights(weight, true);
    report_time();
    return retval;
}

HfstTransducer * PmatchRestrictionContainer::evaluate(PmatchEvalType eval_type)
{
    if (cache != NULL && should_use_cache()) {
        return new HfstTransducer(*cache);
    }
    start_timing();
    HfstTransducer * retval;
    HfstTransducerPairVector pair_vector;
    for (MappingPairVector::iterator it = contexts->begin();
         it != contexts->end(); ++it) {
        HfstTransducer * lside = it->first->evaluate();
        HfstTransducer * rside = it->second->evaluate();
        pair_vector.push_back(HfstTransducerPair(HfstTransducer(*lside),
                                                 HfstTransducer(*rside)));
        delete lside; delete rside;
    }
    HfstTransducer * l = left->evaluate();
    retval = new HfstTransducer(hfst::xeroxRules::restriction(*l, pair_vector));
    delete l;
    retval->set_final_weights(weight, true);
    report_time();
    if (cache == NULL && should_use_cache() == true) {
        cache = retval;
        cache->minimize();
        return new HfstTransducer(*cache);
    }
    return retval;
}

HfstTransducer * PmatchMarkupContainer::evaluate(PmatchEvalType eval_type) { pmatcherror("Should never happen\n"); throw 1; }
HfstTransducer * PmatchMappingPairsContainer::evaluate(PmatchEvalType eval_type) { pmatcherror("Should never happen\n"); throw 1; }
HfstTransducer * PmatchContextsContainer::evaluate(PmatchEvalType eval_type) { pmatcherror("Should never happen\n"); throw 1; }

} }
