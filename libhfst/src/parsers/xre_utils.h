// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

/**
 * @file xre_utils.h
 *
 * @brief auxiliary functions for handling Xerox compatible regular expressions
 * (XRE).
 */

#ifndef GUARD_xre_utils_h
#define GUARD_xre_utils_h

#include <map>
#include "HfstTransducer.h"

namespace hfst { namespace xre {

extern char* data;
extern char* startptr;
extern size_t len;
extern std::map<std::string,hfst::HfstTransducer*> definitions;
extern std::map<std::string,std::string> function_definitions;
extern std::map<std::string,unsigned int> function_arguments;
extern std::map<std::string, std::set<std::string> > symbol_lists;
extern HfstTransducer* last_compiled;
extern bool contains_only_comments;
extern ImplementationType format;

void set_substitution_function_symbol(const std::string &symbol);

bool substitution_function(const hfst::StringPair &p, hfst::StringPairSet &sps);

/**
 * @brief input handling function for flex that parses strings.
 */
int getinput(char *buf, int maxlen);


/**
 * @brief remove starting and ending curly bracket from given string @a.
 */
char* strip_curly(const char *s);

/**
 * @brief remove percent escaping from given string @a s.
 */
char* strip_percents(const char *s);

 char* strip_newline(char *s);

 void count_lines(const char * s);

/**
 * @brief add percents to string to form valid XRE symbol.
 */
char* add_percents(const char* s);

/**
 * @brief find first quoted segment from strign @a s.
 */
char* get_quoted(const char *s);

 char* parse_quoted(const char *s, unsigned int & length);

int* get_n_to_k(const char* s);

double get_weight(const char* s);
 
/**
 * @brief If \a s is of form "<foo>", free \a s and return an escaped version
 * of it: "@_<foo>_@". Else, return \a s.
 */
char* escape_enclosing_angle_brackets(char *s);

/**
 * @brief Substitute all symbols of form "@_<foo>_@" with "<foo>" in \a t and return \a t.
 */
HfstTransducer* unescape_enclosing_angle_brackets(HfstTransducer *t);

/**
 * @brief compile new transducer
 */
HfstTransducer* compile(const std::string& xre,
                        std::map<std::string,hfst::HfstTransducer*>& defs,
                        std::map<std::string,std::string>& func_defs,
                        std::map<std::string,unsigned int> func_args,
                        std::map<std::string, std::set<std::string> >& lists,
                        hfst::ImplementationType type);

/**
 * @brief compile new transducer defined by the first regex in @a xre.
 */
HfstTransducer* compile_first(const std::string& xre,
                              std::map<std::string,hfst::HfstTransducer*>& defs,
                              std::map<std::string,std::string>& func_defs,
                              std::map<std::string,unsigned int> func_args,
                              std::map<std::string, std::set<std::string> >& lists,
                              hfst::ImplementationType type,
                              unsigned int & chars_read);

/**
 * @brief For a single-transition transducer, if the transition symbol is a name for
 * transducer definition, expand the transition into the corresponding transducer.
 */
HfstTransducer* expand_definition(HfstTransducer* tr, const char* symbol);

// the same but simpler..
HfstTransducer* expand_definition(const char* symbol);

 bool define_function_args(const char * name, const std::vector<HfstTransducer> * args);
 void undefine_function_args(const char * name);

 const char * get_function_xre(const char * name);

bool is_definition(const char* symbol);

bool is_valid_function_call(const char * name, const std::vector<HfstTransducer> * args);

/** @brief Parse "input:output", ":output", "input:" or ":". */
 HfstTransducer* xfst_label_to_transducer(const char* input, const char* output);

 HfstTransducer* xfst_curly_label_to_transducer(const char* input, const char* output);

 HfstTransducer * contains(const HfstTransducer * t);

 HfstTransducer * contains_with_weight(const HfstTransducer * t, float weight);

 // doesn't work at the moment
 HfstTransducer * contains_twolc(const HfstTransducer * t);

 HfstTransducer * contains_once(const HfstTransducer * t);

 HfstTransducer * contains_once_optional(const HfstTransducer * t);

 HfstTransducer * merge_first_to_second(HfstTransducer * tr1, HfstTransducer * tr2);

 void warn(const char * msg);
 void warn_about_special_symbols_in_replace(HfstTransducer *t);
 /* Warn about \a symbol if it is of form "@_.*_@" and verbose mode is on. */
 void warn_about_hfst_special_symbol(const char * symbol);
 /* Warn about \a symbol if it is of form "<.*>" or "all" and verbose mode is on. */
 void warn_about_xfst_special_symbol(const char * symbol);

 void check_multichar_symbol(const char * symbol);

 bool has_non_identity_pairs(const HfstTransducer * t);

} } // namespaces
#endif
// vim: set ft=cpp.doxygen:
