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

#include "HfstTransducer.h"
#include "HfstExceptionDefs.h"

#include "pmatch_utils.h"
#include "xre_utils.h"
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
    std::string parsedata;
    if (strlen(hfst::pmatch::data) < 60) {
        parsedata = hfst::pmatch::data;
    }
    else {
        parsedata = std::string(hfst::pmatch::data, 59) + "... [truncated]";
    }
    std::string errmsg = "pmatch parsing failed: ";
    errmsg.append(msg);
    errmsg.append("\n*** parsing ");
    errmsg.append(parsedata);
    errmsg.append(" at line ");
    std::ostringstream ss;
    ss << pmatchlineno;
    errmsg.append(ss.str());
    errmsg.append(" near ");
    errmsg.append(pmatchtext);
    errmsg.append("\n");

    // TODO: clean the potentially large amounts of data we're leaking in case
    // the caller isn't the command line utility that exits after this

    HFST_THROW_MESSAGE(HfstException, errmsg);
}

void pmatchwarning(const char *msg)
{
    if (hfst::pmatch::verbose) {
        std::string warnmsg = "pmatch: ";
        warnmsg.append(msg);
        warnmsg.append(" on line ");
        std::ostringstream ss;
        ss << pmatchlineno;
        warnmsg.append(ss.str());
        warnmsg.append("\n");
        std::cerr << warnmsg;
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
std::set<std::string> capture_names;
std::vector<WordVector> word_vectors;
char* startptr;
hfst::ImplementationType format;
size_t len;
bool verbose;
bool flatten;
bool include_cosine_distances;
std::string includedir;
clock_t timer;
int minimization_guard_count;
bool need_delimiters;
WordVecFloat vector_similarity_projection_factor;

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

bool string_set_has_meta_arc(StringSet & ss)
{
    return ss.count(hfst::internal_unknown) == 1 ||
        ss.count(hfst::internal_identity) == 1 ||
        ss.count(hfst::internal_default) == 1;
}

bool is_special(const std::string & symbol)
{
    if (symbol.size() < 3) {
        return false;
    }
    return symbol.find("@") == 0 && symbol.rfind("@") == symbol.size() - 1;
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
    if ( maxlen > (int)len ) {
        maxlen = hfst::size_t_to_int(len);
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

PmatchTransducerContainer * make_capture_tag(std::string tag)
{ return epsilon_to_symbol_container("@PMATCH_CAPTURE_" + tag + "@"); }

PmatchTransducerContainer * make_captured_tag(std::string tag)
{ return epsilon_to_symbol_container("@PMATCH_CAPTURED_" + tag + "@"); }

PmatchObject * make_with_tag_entry(std::string key, std::string value)
{
    return new PmatchString("@P.PMATCH_GLOBAL_" + key + "." + value + "@");
}

PmatchObject * make_with_tag_exit(std::string key)
{
    return new PmatchString("@C.PMATCH_GLOBAL_" + key + "@");
}

// Get the n best candidates in the original space using an insertion sort
std::vector<std::pair<WordVector, WordVecFloat> > get_top_n(size_t n,
                                                            std::vector<WordVector> & vecs,
                                                            WordVector & comparison_point)
{
    std::vector<std::pair<WordVector, WordVecFloat> > retval;
    for (std::vector<WordVector>::const_iterator it = vecs.begin();
         it != vecs.end(); ++it) {
        WordVecFloat cosdist = cosine_distance(*it, comparison_point);
        for (size_t i = 0; i <= retval.size(); ++i) {
            if (i == retval.size()) {
                // We made it to the top
                retval.push_back(std::pair<WordVector, WordVecFloat>(
                                     WordVector(*it), cosdist));
                break;
            } else {
                // Walking the list
                if (cosdist >= retval[i].second) {
                    if (i == 0 && retval.size() == n) {
                        break;
                    }
                    retval.insert(retval.begin() + i, std::pair<WordVector, WordVecFloat>(
                                      WordVector(*it), cosdist));
                    break;
                } else {
                    continue;
                }
            }
        }
        if (retval.size() > n) {
            retval.erase(retval.begin());
        }
    }
    return retval;
}

// Get the n best candidates in the transformed space using an insertion sort
std::vector<std::pair<WordVector, WordVecFloat> > get_top_n_transformed(
    size_t n,
    const std::vector<WordVector> & vecs,
    std::vector<WordVecFloat> plane_vec,
    std::vector<WordVecFloat> comparison_point,
    WordVecFloat translation_term,
    bool negative)
{
    std::vector<std::pair<WordVector, WordVecFloat> > retval;
    WordVecFloat plane_vec_square_sum = square_sum(plane_vec);
    WordVecFloat comparison_point_norm = norm(comparison_point);
    for (std::vector<WordVector>::const_iterator it = vecs.begin();
         it != vecs.end(); ++it) {
        WordVector transformed_vec(*it);

        /*
         * First, given a plane "plane_vec = translation term" and a point,
         * find the multiple of plane_vec which produces a vector going
         * from point to the nearest point in the plane.
         */

        WordVecFloat transformed_vec_scaler =
            (translation_term - dot_product(transformed_vec.vector, plane_vec))
            / plane_vec_square_sum;
        transformed_vec_scaler *= vector_similarity_projection_factor;
        if(negative) {
            transformed_vec.vector =
                pointwise_minus(transformed_vec.vector,
                                pointwise_multiplication(transformed_vec_scaler, plane_vec));
        } else {
            transformed_vec.vector =
                pointwise_plus(transformed_vec.vector,
                               pointwise_multiplication(transformed_vec_scaler, plane_vec));
        }
        transformed_vec.norm = norm(transformed_vec.vector);
        WordVecFloat cosdist = 1 - dot_product(transformed_vec.vector, comparison_point)
            / (transformed_vec.norm * comparison_point_norm);
        for (size_t i = 0; i <= retval.size(); ++i) {
            if (i == retval.size()) {
                // We made it to the top
                retval.push_back(std::pair<WordVector, WordVecFloat>(transformed_vec,
                                                                     cosdist));
                break;
            } else {
                // Walking the list
                if (cosdist >= retval[i].second) {
                    if (i == 0 && retval.size() == n) {
                        break;
                    }
                    retval.insert(retval.begin() + i,
                                  std::pair<WordVector, WordVecFloat>(transformed_vec, cosdist));
                    break;
                } else {
                    continue;
                }
            }
        }
        if (retval.size() > n) {
            retval.erase(retval.begin());
        }
    }
    return retval;
}

template<typename T> std::vector<T> pointwise_minus(std::vector<T> l,
                                                    std::vector<T> r)
{
    std::vector<T> ret(l.size(), 0);
    for(size_t i = 0; i < l.size(); ++i) {
        ret[i] = l[i] - r[i];
    }
    return ret;
}

template<typename T> std::vector<T> pointwise_plus(std::vector<T> l,
                                                   std::vector<T> r)
{
    std::vector<T> ret(l.size(), 0);
    for(size_t i = 0; i < l.size(); ++i) {
        ret[i] = l[i] + r[i];
    }
    return ret;
}

template<typename T> std::vector<T> pointwise_multiplication(T l,
                                                             std::vector<T> r)
{
    std::vector<T> ret(r.size(), 0);
    for(size_t i = 0; i < r.size(); ++i) {
        ret[i] = l * r[i];
    }
    return ret;
}

template<typename T> T dot_product(std::vector<T> l,
                                   std::vector<T> r)
{
    T ret = 0;
    for(size_t i = 0; i < l.size(); ++i) {
        ret += l[i] * r[i];
    }
    return ret;
}

template<typename T> T square_sum(std::vector<T> v)
{
    T ret = 0;
    for(size_t i = 0; i < v.size(); ++i) {
        ret += v[i] * v[i];
    }
    return ret;
}

template<typename T> T norm(std::vector<T> v)
{
    return sqrt(square_sum(v));
}

WordVecFloat cosine_distance(WordVector left, WordVector right)
{
    // Sometimes very nearby vectors combined with rounding error will produce
    // a slightly negative distance, so make sure to return at least 0.0
    WordVecFloat retval = 1.0 - dot_product(left.vector, right.vector) /
        (left.norm * right.norm);
    return std::max(static_cast<WordVecFloat>(0.0), retval);
}

WordVecFloat cosine_distance(std::vector<WordVecFloat> left, std::vector<WordVecFloat> right)
{
    WordVecFloat retval = 1.0 - dot_product(left, right) / (norm(left) * norm(right));
    return std::max(static_cast<WordVecFloat>(0.0), retval);
}

// Single-word Like()
PmatchObject * compile_like_arc(std::string word,
                                unsigned int nwords)
{
    WordVector this_word;
    for (std::vector<WordVector>::iterator it = word_vectors.begin();
         it != word_vectors.end(); ++it) {
        if (word == it->word) {
            this_word = *it;
            break;
        }
    }
    if (this_word.word == "") {
        // got no matches
        PmatchString * word_o = new PmatchString(word);
        word_o->multichar = true;
        pmatchwarning("no matches for argument to Like() operation");
        return word_o;
    }

    std::vector<std::pair<WordVector, WordVecFloat> > top_n = get_top_n(nwords, word_vectors, this_word);

    HfstTokenizer tok;
    HfstTransducer * retval = new HfstTransducer(format);
    if (verbose) {
        std::cerr << "Inserting into Like(" << word << "):" << std::endl;
    }
    for (size_t i = 0; i < top_n.size(); ++i) {
        if (verbose) {
            std::cerr << "  " << top_n[i].first.word << std::endl;
        }
        HfstTransducer tmp(top_n[i].first.word, tok, format);
        if (include_cosine_distances) {
            tmp.set_final_weights(top_n[i].second);
        }
        retval->disjunct(tmp);
    }
    return new PmatchTransducerContainer(retval);
}

// the general case
PmatchObject * compile_like_arc(std::string word1, std::string word2,
                                unsigned int nwords, bool is_negative)
{
    WordVector this_word1;
    WordVector this_word2;
    for (std::vector<WordVector>::iterator it = word_vectors.begin();
         (it != word_vectors.end() &&
          (this_word1.word == "" || this_word2.word == "")); ++it) {
        if (word1 == it->word) {
            this_word1 = *it;
        }
        if (word2 == it->word) {
            this_word2 = *it;
        }
    }
    if (this_word1.word == "" && this_word2.word == "") {
        // got no matches
        PmatchString * word1_o = new PmatchString(word1);
        PmatchString * word2_o = new PmatchString(word2);
        word1_o->multichar = true; word2_o->multichar = true;
        pmatchwarning("no matches for arguments to Like() operation");
        return new PmatchBinaryOperation(Disjunct, word1_o, word2_o);
    }

    if (this_word1.word == "" || this_word2.word == "") {
        // just one match
        pmatchwarning("only one match for arguments to Like() operation, using nearest neighbours");
        WordVector this_word = (this_word1.word == "" ? this_word2 : this_word1);
        std::vector<std::pair<WordVector, WordVecFloat> > top_n = get_top_n(nwords, word_vectors, this_word);
        HfstTokenizer tok;
        HfstTransducer * retval = new HfstTransducer(format);
        if (verbose) {
            std::cerr << "Inserting into Like(" << this_word.word << "):" << std::endl;
        }

        for (size_t i = 0; i < top_n.size(); ++i) {
            if (verbose) {
                std::cerr << "  " << top_n[i].first.word << std::endl;
            }
            HfstTransducer tmp(top_n[i].first.word, tok, format);
            if (include_cosine_distances) {
                tmp.set_final_weights(top_n[i].second);
            }
            retval->disjunct(tmp);
        }
        return new PmatchTransducerContainer(retval);
    }

    if(variables["vector-similarity-projection-factor"] != "1.0") {
        vector_similarity_projection_factor =
            strtod(variables["vector-similarity-projection-factor"].c_str(), NULL);
    }
    /*
     * When there are two vectors A and B, we compute the vector A - B that
     * goes from one to the other, and define a hyperplane orthogonal to that
     * vector that intersects the vector at the midpoint between the
     * two. We then add to all vectors a multiple of A - B to move them closer
     * to the plane, reducing the distance that is due to the difference
     * between A and B. (This is like projecting the space to the hyperplane
     * if we go all the way to the plane)
     *
     * The hyperplane is defined by the equation |B - A| = d, where d is a
     * translation term. |B - A| = 0 would be the set of vectors orthogonal to
     * |B - A|. We set d so that the distance from the hyperplane to A is
     * half of the norm of |B - A|.
     *
     */

    std::vector<WordVecFloat> B_minus_A = pointwise_minus(
        this_word1.vector, this_word2.vector);
    WordVecFloat hyperplane_translation_term = dot_product(B_minus_A, this_word1.vector)
        - square_sum(B_minus_A) * 0.5;

    std::vector<WordVecFloat> comparison_point;
    if (is_negative == true) {
        if (verbose) {
            std::cerr << "Inserting into Unlike(" << this_word1.word << ", " << this_word2.word << "):" << std::endl;
        }
        WordVecFloat comparison_scaler =
            (hyperplane_translation_term - dot_product(this_word1.vector, B_minus_A)) / square_sum(B_minus_A);
        comparison_scaler *= vector_similarity_projection_factor;
        comparison_point = pointwise_minus(this_word1.vector, pointwise_multiplication(comparison_scaler, B_minus_A));
    } else {
        if (verbose) {
            std::cerr << "Inserting into Like(" << this_word1.word << ", " << this_word2.word << "):" << std::endl;
        }
        comparison_point = pointwise_plus(this_word2.vector, pointwise_multiplication(
                                              static_cast<WordVecFloat>(0.5), B_minus_A));
    }

    std::vector<std::pair<WordVector, WordVecFloat> > top_n = get_top_n_transformed(nwords,
                                                                                    word_vectors,
                                                                                    B_minus_A,
                                                                                    comparison_point,
                                                                                    hyperplane_translation_term,
                                                                                    is_negative);
    HfstTokenizer tok;
    HfstTransducer * retval = new HfstTransducer(format);
    for (size_t i = 0; i < top_n.size() && i <= nwords; ++i) {
        if (verbose) {
            std::cerr << "  " << top_n[i].first.word << std::endl;
        }
        HfstTransducer tmp(top_n[i].first.word, tok, format);
        if (include_cosine_distances) {
            tmp.set_final_weights(top_n[i].second);
        }
        retval->disjunct(tmp);
        // if (include_cosine_distances) {
        //     for (size_t j = i + 1; j < word_vectors.size() && j <= nwords; ++j) {
        //         HfstTransducer tmp2(word_vectors[i].word + "_cos_" + word_vectors[j].word, tok, format);
        //         tmp2.set_final_weights(cosine_distance(projected_i,
        //                                                get_projected_vector(word_vectors[j].vector, B_minus_A, hyperplane_translation_term)));
        //         retval->disjunct(tmp2);
        //     }
        // }
    }
    return new PmatchTransducerContainer(retval);
}

PmatchTransducerContainer * make_counter(std::string name)
{ return epsilon_to_symbol_container("@PMATCH_COUNTER_" + name + "@"); }

hfst::StringSet get_non_special_alphabet(HfstTransducer * t)
{
    hfst::StringSet retval;
    hfst::StringSet const & alphabet = t->get_alphabet();
    for (hfst::StringSet::const_iterator it = alphabet.begin();
         it != alphabet.end(); ++it) {
        if (hfst_ol::PmatchAlphabet::is_printable(*it)) {
            retval.insert(*it);
        }
    }
    return retval;
}

HfstTransducer * make_list(HfstTransducer * t, ImplementationType f)
{
    std::string arc = "@L.";
    hfst::StringSet alphabet = get_non_special_alphabet(t);
    for (hfst::StringSet::const_iterator it = alphabet.begin();
         it != alphabet.end(); ++it) {
        arc.append(*it);
        arc.append("_");
    }
    arc.append("@");
    return new HfstTransducer(arc, f);
}

HfstTransducer * make_exc_list(HfstTransducer * t, ImplementationType f)
{
    std::string arc = "@X.";
    hfst::StringSet alphabet = get_non_special_alphabet(t);
    for (hfst::StringSet::const_iterator it = alphabet.begin();
         it != alphabet.end(); ++it) {
        arc.append(*it);
        arc.append("_");
    }
    arc.append("@");
    return new HfstTransducer(arc, f);
}

HfstTransducer * make_sigma(HfstTransducer * t)
{
    HfstTransducer * retval =
        new HfstTransducer(format);
    hfst::StringSet alphabet = get_non_special_alphabet(t);
    for (hfst::StringSet::const_iterator it = alphabet.begin();
         it != alphabet.end(); ++it) {
            retval->disjunct(HfstTransducer(*it, format));
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
    variables["xerox-composition"] = "on";
    variables["vector-similarity-projection-factor"] = "1.0";
    call_stack.clear();
    def_insed_expressions.clear();
    inserted_names.clear();
    unsatisfied_insertions.clear();
    used_definitions.clear();
    function_names.clear();
    capture_names.clear();
    zero_minimization_guard();
    need_delimiters = false;
    pmatchnerrs = 0;
}

string expand_includes(const string & script)
{
    if (script.find("@include\"") == string::npos) {
        return string(script);
    }
    bool in_quoted_literal = false;
    bool in_curly_literal = false;
    bool in_comment = false;
    string::const_iterator it = script.begin();
    string retval = "";
    while (it != script.end()) {
        if (in_quoted_literal && *it == '"' && *(it-1) != '\\')
            in_quoted_literal = false;
        else if (in_curly_literal && *it == '}' && *(it-1) != '\\')
            in_curly_literal = false;
        else if (in_comment && *it == '\n')
            in_comment = false;
        else if (*it == '"')
            in_quoted_literal = true;
        else if (*it == '{')
            in_curly_literal = true;
        else if (*it == '!')
            in_comment = true;
        else if (*it == '%') {
            retval.push_back(*it);
            ++it;
            if (it != script.end()) {
                retval.push_back(*it);
                ++it;
            }
            continue;
        } else if (script.compare(it - script.begin(), 9, "@include\"") == 0) {
            size_t terminating_quote_pos = script.find('"', it - script.begin() + 9);
            if (terminating_quote_pos != string::npos) {
                size_t filename_start_pos = it - script.begin() + 9;
                size_t filename_len = terminating_quote_pos - filename_start_pos;
                string filepath = path_from_filename(script.substr(filename_start_pos,
                                                                   filename_len).c_str());
                std::ifstream infile;
                infile.open(filepath.c_str());
                if(!infile.good()) {
                    std::stringstream errstring;
                    errstring << "could not open file " << filepath << " for @include\n";
                    pmatcherror(errstring.str().c_str());
                }
                char c = infile.get();
                while(infile.good()) {
                    retval.push_back(c);
                    c = infile.get();
                }
                infile.close();
                it += 10 + filename_len;
                continue;
            }
        }
        retval.push_back(*it);
        ++it;
    }
    return retval;
}

std::map<std::string, HfstTransducer*>
compile(const string& pmatch, map<string,HfstTransducer*>& defs,
        ImplementationType impl, bool be_verbose, bool do_flatten,
        bool do_include_cosine_distances,
        std::string includedir_)
{
    // lock here?
    init_globals();
    string expanded_script = expand_includes(pmatch);
    data = strdup(expanded_script.c_str());
    startptr = data;
    len = strlen(data);
    verbose = be_verbose;
    flatten = do_flatten;
    include_cosine_distances = do_include_cosine_distances;
    includedir = includedir_;
    vector_similarity_projection_factor = 1.0;
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
        data = 0;
        len = 0;
        return retval;
    }
    // Our helper for harmonizing all the networks' alphabets with
    // each other
    if (hfst::pmatch::verbose) {
        std::cerr << "\nCompiling and harmonizing...\n";
        timer = clock();
    }

    if (inserted_names.size() > 0 || def_insed_expressions.size() > 0) {
        HfstTransducer dummy(format);
        // We keep TOP and any inserted transducers
        std::map<std::string, PmatchObject *>::iterator defs_it;
        for (defs_it = definitions.begin(); defs_it != definitions.end();
             ++defs_it) {
            if (defs_it->first.compare("TOP") == 0 ||
                inserted_names.count(defs_it->first) != 0 ||
                def_insed_expressions.count(defs_it->first) != 0) {
                HfstTransducer * tmp = NULL;
                if (def_insed_expressions.count(defs_it->first) != 0) {
                    tmp = def_insed_expressions[defs_it->first]->evaluate();
                } else {
                    tmp = defs_it->second->evaluate();
                }
                tmp->minimize();
                dummy.harmonize(*tmp);
                // This is what it will be called in the archive
                tmp->set_name(defs_it->first);
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
            tmp->set_name("TOP");
            retval.insert(std::pair<std::string, hfst::HfstTransducer*>("TOP", tmp));
        } else {
            hfst::HfstTransducer * tmp = definitions["TOP"]->evaluate();
            tmp->minimize();
            tmp->set_name("TOP");
            retval.insert(std::pair<std::string, hfst::HfstTransducer*>("TOP", tmp));
        }
    }

    if (hfst::pmatch::verbose) {
        double duration = (clock() - hfst::pmatch::timer) /
            (double) CLOCKS_PER_SEC;
        hfst::pmatch::timer = clock();
        std::cerr << "compiled and harmonized in " << duration << " seconds\n";
    }

    StringSet allowed_initial_symbols;
    StringSet disallowed_initial_symbols;
    definitions["TOP"]->collect_initial_symbols_into(
        allowed_initial_symbols, disallowed_initial_symbols);
    std::string initial_symbols_list;
    std::string disallowed_initial_symbols_list;
    // Use this to bail out if there's something suspicious in the final lists
    bool initial_symbols_ok = true;
    for (StringSet::iterator it = allowed_initial_symbols.begin();
         it != allowed_initial_symbols.end(); ++it) {
        if (is_special(*it)) {
            if (hfst::pmatch::verbose) {
                std::cerr << "Not setting initial symbol list due to special symbol " << *it << std::endl;
            }
            initial_symbols_ok = false;
        }
        initial_symbols_list.append(*it);
    }
    for (StringSet::iterator it = disallowed_initial_symbols.begin();
         it != disallowed_initial_symbols.end(); ++it) {
        if (is_special(*it)) {
            if (hfst::pmatch::verbose) {
                std::cerr << "Not setting initial symbol list due to special symbol " << *it << std::endl;
            }
            initial_symbols_ok = false;
        }
        disallowed_initial_symbols_list.append(*it);
    }
    if (allowed_initial_symbols.size() > 200) {
        if (hfst::pmatch::verbose) {
            std::cerr << "Not setting initial symbol list due to excess length: " << allowed_initial_symbols.size() << std::endl;
        }
        initial_symbols_ok = false;
    }
    if (disallowed_initial_symbols.size() > 200) {
        if (hfst::pmatch::verbose) {
            std::cerr << "Not setting initial symbol list due to excess length: " << disallowed_initial_symbols.size() << std::endl;
        }
        initial_symbols_ok = false;
    }
    if (initial_symbols_ok && initial_symbols_list.size() != 0) {
        variables["initial-symbols"] = initial_symbols_list;
    }
    if (initial_symbols_ok && disallowed_initial_symbols_list.size() != 0) {
        variables["disallowed-initial-symbols"] = disallowed_initial_symbols_list;
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
        HfstTransducer is_single_non_whitespace(not_whitespace);
        is_single_non_whitespace.compose(*(retval["TOP"]));
        HfstTransducer empty(format);
        if (begins_and_ends_with_non_whitespace.compare(empty) == false ||
            is_single_non_whitespace.compare(empty) == false) {
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
            (retval["TOP"])->minimize();
            if (hfst::pmatch::verbose) {
                double duration = (clock() - hfst::pmatch::timer) /
                    (double) CLOCKS_PER_SEC;
                hfst::pmatch::timer = clock();
                std::cerr << "added automatic context separators in " << duration << " seconds\n";
            }
        }
    }
    for(std::map<std::string, std::string>::iterator it = variables.begin();
        it != variables.end(); ++it) {
        retval["TOP"]->set_property(it->first, it->second);
    }
    data = 0;
    len = 0;
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
        for(hfst::implementations::HfstBasicTransitions::const_iterator tr_it =
                state_it->begin(); tr_it != state_it->end(); ++tr_it) {
            ++arcs;
        }
    }
    std::cerr << states <<
        " states and " << arcs << " arcs" << std::endl;
}

HfstTransducer * read_text(std::string filename, ImplementationType type,
                           bool spaced_text)
{
    std::ifstream infile;
    std::string line;
    infile.open(filename.c_str());
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
                if (spaced_text) {
                    StringPairVector spv = tok.tokenize_space_separated(line);
                } else {
                    StringPairVector spv = tok.tokenize(line);
                    retval->disjunct(spv);
                }
            }
        }
    }
    infile.close();
    return retval;
}

HfstTransducer * read_spaced_text(std::string filename, ImplementationType type)
{ return read_text(filename, type, true); }

std::string path_from_filename(const char * filename)
{
    std::string retval(filename);
    if (includedir.size() > 0 && retval.size() > 0) {
        // includedir won't be > 0 under Windows until this mechanism is ported
        if (retval[0] != '/') {
            // not an absolute dir
            retval.insert(0, includedir);
        }
    }
    return retval;
}

void read_vec(std::string filename)
{
    bool binary_format = false;
    if (filename.rfind(".bin") == filename.size() - 4) {
        binary_format = true;
    }
    if (word_vectors.size() != 0) {
        word_vectors.clear();
        std::cerr << "pmatch: vector model file " << filename
                  << " overrides earlier one\n";
    }
    char separator = ' ';
    std::ifstream infile;
    std::string line;
    size_t lexicon_size;
    size_t dimension;
    infile.open(filename.c_str());
    if(!infile.good()) {
        std::cerr << "pmatch: could not open vector file " << filename <<
            " for reading\n";
        return;
    }
    std::getline(infile, line);
    std::stringstream ss(line);
    ss >> lexicon_size;
    ss.ignore(1);
    ss >> dimension;
    word_vectors.reserve(lexicon_size + 1);
    size_t words_read = 0;
    if (binary_format) {
        size_t vector_data_size = sizeof(float) * dimension;
        std::vector<char> vector_data(vector_data_size);
        while (infile.good() && words_read <= lexicon_size) {
            // The actual number of vectors is 1 more than lexicon_size
            // due to <s>
            std::getline(infile, line, separator);
            infile.read(&vector_data[0], vector_data_size);
            infile.ignore(1);
            WordVector wv;
            wv.word = line;
            // This will not compile is WordVectorFloat is not float,
            // in which case a conversion needs to happen, but
            // we can reasonably expect it to be a float for the
            // foreseeable future
            wv.vector.assign((float*) vector_data.data(), (float*) (vector_data.data() + vector_data_size));
            wv.norm = norm(wv.vector);
            word_vectors.push_back(wv);
            ++words_read;
        }
    } else {
        while(infile.good() && words_read <= lexicon_size) {
            std::getline(infile, line);
            if (line.empty()) { continue; }
            ++words_read;
            size_t pos = line.find(separator);
            if (pos == std::string::npos) {
                separator = '\t';
                pos = line.find(separator);
                if (pos == std::string::npos) {
                    std::cerr << "pmatch warning: vector file " << filename <<
                        " doesn't appear to be tab- or space-separated\n  (reading line " << words_read + 1 << ")\n";
                    break;
                }
            }
            std::string word = line.substr(0, pos);
            std::vector<WordVecFloat> components;
            size_t nextpos;
            while (std::string::npos != (nextpos = line.find(separator, pos + 1))) {
                components.push_back(strtod(line.substr(pos + 1, nextpos - pos).c_str(), NULL));
                pos = nextpos;
            }
            // there can be one more from pos to the newline if there isn't a
            // separator at the end
            if (line.back() != separator) {
#if defined _MSC_VER && 1200 <= _MSC_VER
                components.push_back((float)strtod(line.substr(pos + 1).c_str(), NULL));
            }
#else
                components.push_back(strtof(line.substr(pos + 1).c_str(), NULL));
            }
#endif
            if (word_vectors.size() != 0 && word_vectors[0].vector.size() != components.size()) {
                std::cerr << "pmatch warning: vector file " << filename <<
                    " appears malformed\n  (reading line " << words_read + 1 << ")\n";
                continue;
            }
            WordVector wv;
            wv.word = word;
            wv.vector = components;
            wv.norm = norm(components);
            word_vectors.push_back(wv);
        }
    }
    infile.close();
    if (verbose) {
        if (word_vectors.size() == 0) {
            std::cerr << "Tried to read word vector file, empty result\n";
        }
        std::cerr << "Read " << word_vectors.size() << " vectors of dimensionality " << word_vectors[0].vector.size() << std::endl;
    }
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
                    nextpos = hfst::size_t_to_int(line.find_first_of(" ", curpos));
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
      //retval->minimize(); ?
      return retval;
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

#if USE_GLIB_UNICODE
std::string PmatchUtilityTransducers::string_from_g_unichar(gunichar ch)
{
    char buf[7];
    int buflen = g_unichar_to_utf8(ch, buf);
    buf[buflen] = '\0';
    return std::string(buf);
}
#endif

HfstTransducer PmatchUtilityTransducers::get_lowercase_acceptor_from_transducer(HfstTransducer & t)
{
#if USE_GLIB_UNICODE
    HfstTransducer lowercase(t.get_type());
    StringSet ss = t.get_alphabet();
    for (StringSet::const_iterator it = ss.begin(); it != ss.end(); ++it) {
        if (g_utf8_strlen(it->c_str(), -1) == 1) {
            gunichar this_unichar = g_utf8_get_char_validated(it->c_str(), -1);
            if (g_unichar_islower(this_unichar)) {
                lowercase.disjunct(HfstTransducer(*it, t.get_type()));
            }
        }
    }
#elif USE_ICU_UNICODE
    HfstTransducer lowercase(t.get_type());
    StringSet ss = t.get_alphabet();
    for (StringSet::const_iterator it = ss.begin(); it != ss.end(); ++it) {
        icu::UnicodeString us(it->c_str());
        if (us.countChar32() == 1) {
            if (u_islower(us.char32At(0))) {
                lowercase.disjunct(HfstTransducer(*it, t.get_type()));
            }
        }
    }
#else
    HfstTransducer lowercase(*latin1_lowercase_acceptor);
#endif
    return lowercase;
}

HfstTransducer PmatchUtilityTransducers::get_uppercase_acceptor_from_transducer(HfstTransducer & t)
{
#if USE_GLIB_UNICODE
    HfstTransducer uppercase(t.get_type());
    StringSet ss = t.get_alphabet();
    for (StringSet::const_iterator it = ss.begin(); it != ss.end(); ++it) {
        if (g_utf8_strlen(it->c_str(), -1) == 1) {
            gunichar this_unichar = g_utf8_get_char_validated(it->c_str(), -1);
            if (g_unichar_isupper(this_unichar)) {
                uppercase.disjunct(HfstTransducer(*it, t.get_type()));
            }
        }
    }
#elif USE_ICU_UNICODE
    HfstTransducer uppercase(t.get_type());
    StringSet ss = t.get_alphabet();
    for (StringSet::const_iterator it = ss.begin(); it != ss.end(); ++it) {
        icu::UnicodeString us(it->c_str());
        if (us.countChar32() == 1) {
            if (u_isupper(us.char32At(0))) {
                uppercase.disjunct(HfstTransducer(*it, t.get_type()));
            }
        }
    }
#else
    HfstTransducer uppercase(*latin1_uppercase_acceptor);
#endif
    return uppercase;
}

HfstTransducer PmatchUtilityTransducers::lowercaser_from_transducer(HfstTransducer & t)
{
#if USE_GLIB_UNICODE
    HfstTransducer lowercase(t.get_type());
    StringSet ss = t.get_alphabet();
    StringSet uppercases_seen;
    for (StringSet::const_iterator it = ss.begin(); it != ss.end(); ++it) {
        if (g_utf8_strlen(it->c_str(), -1) == 1) {
            gunichar this_unichar = g_utf8_get_char_validated(it->c_str(), -1);
            if (g_unichar_isalpha(this_unichar)) {
                std::string upper = string_from_g_unichar(g_unichar_toupper(this_unichar));
                if (uppercases_seen.count(upper) != 0) {
                    continue;
                }
                uppercases_seen.insert(upper);
                std::string lower = string_from_g_unichar(g_unichar_tolower(this_unichar));
                lowercase.disjunct(HfstTransducer(upper, lower, t.get_type()));
            }
        }
    }
#elif USE_ICU_UNICODE
    HfstTransducer lowercase(t.get_type());
    StringSet ss = t.get_alphabet();
    StringSet uppercases_seen;
    for (StringSet::const_iterator it = ss.begin(); it != ss.end(); ++it) {
        icu::UnicodeString us(it->c_str());
        if (us.countChar32() == 1) {
            UChar32 this_unichar = us.char32At(0);
            if (u_isalpha(this_unichar)) {
                icu::UnicodeString upper_u = us;
                upper_u.toUpper();
                std::string upper;
                upper_u.toUTF8String(upper);
                if (uppercases_seen.count(upper) != 0) {
                    continue;
                }
                uppercases_seen.insert(upper);
                icu::UnicodeString lower_u = us;
                lower_u.toLower();
                std::string lower;
                lower_u.toUTF8String(lower);
                lowercase.disjunct(HfstTransducer(upper, lower, t.get_type()));
            }
        }
    }
#else
    HfstTransducer lowercase(*lowerfy);
#endif
    return lowercase;
}

HfstTransducer PmatchUtilityTransducers::uppercaser_from_transducer(HfstTransducer & t)
{
#if USE_GLIB_UNICODE
    HfstTransducer uppercase(t.get_type());
    StringSet ss = t.get_alphabet();
    StringSet uppercases_seen;
    for (StringSet::const_iterator it = ss.begin(); it != ss.end(); ++it) {
        if (g_utf8_strlen(it->c_str(), -1) == 1) {
            gunichar this_unichar = g_utf8_get_char_validated(it->c_str(), -1);
            if (g_unichar_isalpha(this_unichar)) {
                std::string upper = string_from_g_unichar(g_unichar_toupper(this_unichar));
                if (uppercases_seen.count(upper) != 0) {
                    continue;
                }
                uppercases_seen.insert(upper);
                std::string lower = string_from_g_unichar(g_unichar_tolower(this_unichar));
                uppercase.disjunct(HfstTransducer(lower, upper, t.get_type()));
            }
        }
    }
#elif USE_ICU_UNICODE
    HfstTransducer uppercase(t.get_type());
    StringSet ss = t.get_alphabet();
    StringSet uppercases_seen;
    for (StringSet::const_iterator it = ss.begin(); it != ss.end(); ++it) {
        icu::UnicodeString us(it->c_str());
        if (us.countChar32() == 1) {
            UChar32 this_unichar = us.char32At(0);
            if (u_isalpha(this_unichar)) {
                icu::UnicodeString upper_u = us;
                upper_u.toUpper();
                std::string upper;
                upper_u.toUTF8String(upper);
                if (uppercases_seen.count(upper) != 0) {
                    continue;
                }
                uppercases_seen.insert(upper);
                icu::UnicodeString lower_u = us;
                lower_u.toLower();
                std::string lower;
                lower_u.toUTF8String(lower);
                uppercase.disjunct(HfstTransducer(lower, upper, t.get_type()));
            }
        }
    }
#else
    HfstTransducer uppercase(*capify);
#endif
    return uppercase;
}

HfstTransducer * PmatchUtilityTransducers::cap(HfstTransducer & t, Side side, bool optional)
{
    bool orig_xerox_composition_value = hfst::get_xerox_composition();
    // This is to match flags in t with ?'s in "anything"
    hfst::set_xerox_composition(true);

    HfstTransducer * retval = NULL;
    HfstTransducer cap = uppercaser_from_transducer(t);
    HfstTransducer decap(cap);
    decap.invert();
    HfstTransducer anything(HfstTransducer::identity_pair(t.get_type()));
    HfstTransducer anything_but_whitespace_star(anything);
    anything_but_whitespace_star.subtract(*latin1_whitespace_acceptor);
    anything_but_whitespace_star.repeat_star();
    if (optional == false) {
        // don't let lowercased first letters through
        anything.subtract(get_lowercase_acceptor_from_transducer(t));
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
        retval->output_project();
    }
    retval->minimize();
    hfst::set_xerox_composition(orig_xerox_composition_value);
    return retval;
}

HfstTransducer * PmatchUtilityTransducers::tolower(HfstTransducer & t, Side side, bool optional)
{
    bool orig_xerox_composition_value = hfst::get_xerox_composition();
    // This is to match flags in t with ?'s in "anything"
    hfst::set_xerox_composition(true);

    HfstTransducer anything(hfst::internal_identity, hfst::pmatch::format);
    if (optional == false) {
        anything.subtract(get_uppercase_acceptor_from_transducer(t));
    }
    HfstTransducer * retval = NULL;
    if (side == Lower) {
        HfstTransducer lowercase = lowercaser_from_transducer(t);
        lowercase.disjunct(anything);
        lowercase.repeat_star();
        retval = new HfstTransducer(t);
        retval->compose(lowercase);
    } else if (side == Upper) {
        retval = new HfstTransducer(uppercaser_from_transducer(t));
        retval->disjunct(anything);
        retval->repeat_star();
        retval->compose(t);
    } else { // both
        retval = new HfstTransducer(uppercaser_from_transducer(t));
        retval->disjunct(anything);
        retval->repeat_star();
        retval->compose(t);
        HfstTransducer lowercase = lowercaser_from_transducer(t);
        lowercase.disjunct(anything);
        lowercase.repeat_star();
        retval->compose(lowercase);
    }
    retval->minimize();
    hfst::set_xerox_composition(orig_xerox_composition_value);
    return retval;
}

HfstTransducer * PmatchUtilityTransducers::toupper(HfstTransducer & t, Side side, bool optional)
{

    bool orig_xerox_composition_value = hfst::get_xerox_composition();
    // This is to match flags in t with ?'s in "anything"
    hfst::set_xerox_composition(true);

    HfstTransducer anything(hfst::internal_identity, hfst::pmatch::format);
    if (optional == false) {
        anything.subtract(get_lowercase_acceptor_from_transducer(t));
    }
    HfstTransducer * retval = NULL;
    if (side == Lower) {
        HfstTransducer uppercase = uppercaser_from_transducer(t);
        uppercase.disjunct(anything);
        uppercase.repeat_star();
        retval = new HfstTransducer(t);
        retval->compose(uppercase);
    } else if (side == Upper) {
        retval = new HfstTransducer(lowercaser_from_transducer(t));
        retval->disjunct(anything);
        retval->repeat_star();
        retval->compose(t);
    } else { // both
        retval = new HfstTransducer(lowercaser_from_transducer(t));
        retval->disjunct(anything);
        retval->repeat_star();
        retval->compose(t);
        HfstTransducer uppercase = uppercaser_from_transducer(t);
        uppercase.disjunct(anything);
        uppercase.repeat_star();
        retval->compose(uppercase);
    }
    retval->minimize();
    hfst::set_xerox_composition(orig_xerox_composition_value);
    return retval;
}

PmatchObject::PmatchObject(void)
{
    name = "";
    weight = 0.0;
    line_defined = pmatchlineno;
    cache = (HfstTransducer*) (NULL);
    parent_is_context = false;
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

PmatchObject * PmatchObject::evaluate_as_arg(void)
{
    return new PmatchTransducerContainer(evaluate());
}

void PmatchObject::expand_Ins_arcs(StringSet & ss)
{
    bool did_no_expansions = false;
    StringSet expansions_done;
    StringSet expanded_symbols;
    if ((this->name).size() != 0) {
        std::string this_name_insed = "@I.";
        this_name_insed.append(this->name);
        this_name_insed.append("@");
        expansions_done.insert(this_name_insed);
    }
    while (!did_no_expansions) {
        did_no_expansions = true;
        for (StringSet::const_iterator it = ss.begin(); it != ss.end(); ++it) {
            if (it->find("@I.") == 0 && it->rfind("@") == it->size() - 1) {
                // it's an Ins
                if (expansions_done.count(*it) == 0) {
                    std::string ins_name = it->substr(3, it->size() - 3 - 1);
                    did_no_expansions = false;
                    expansions_done.insert(*it);
                    if (definitions.count(ins_name) != 0) {
                        StringSet allowed, disallowed;
                        if (def_insed_expressions.count(ins_name) != 0) {
                            def_insed_expressions[ins_name]->collect_initial_symbols_into(allowed, disallowed);
                        } else {
                            definitions[ins_name]->collect_initial_symbols_into(allowed, disallowed);
                        }
                        if (allowed.size() != 0) {
                            expanded_symbols.insert(allowed.begin(), allowed.end());
                        } else {
                            expanded_symbols.insert(hfst::internal_identity);
                        }
                    }
                }
            }
        }
    }
    for (StringSet::const_iterator it = expansions_done.begin();
         it != expansions_done.end(); ++it) {
        ss.erase(*it);
    }
    ss.insert(expanded_symbols.begin(), expanded_symbols.end());
}

StringSet PmatchObject::get_real_initial_symbols(void)
{
    if (this->is_left_concatenation_with_context()) {
        return this->get_real_initial_symbols_from_right();
    }
    if (this->is_delimiter()) {
        return this->get_initial_symbols_from_unary_root();
    }
    HfstTransducer * tmp = this->evaluate();
    StringSet retval(tmp->get_initial_input_symbols());
    delete tmp;
    return retval;
}

StringSet PmatchObject::get_real_initial_symbols_from_right(void)
{
    return StringSet();
}

StringSet PmatchBinaryOperation::get_real_initial_symbols_from_right(void)
{
    return right->get_real_initial_symbols();
}

StringSet PmatchObject::get_initial_symbols_from_unary_root(void)
{
    return StringSet();
}

StringSet PmatchUnaryOperation::get_initial_symbols_from_unary_root(void)
{
    return root->get_real_initial_symbols();
}

bool PmatchObject::is_left_concatenation_with_context(void)
{
    return false;
}

bool PmatchBinaryOperation::is_left_concatenation_with_context(void)
{
    return op == Concatenate && left->is_context();
}

bool PmatchObject::is_context(void)
{
    return false;
}

bool PmatchObject::is_delimiter(void)
{
    return false;
}

bool PmatchUnaryOperation::is_context(void)
{
    return op == LC || op == NLC || op == RC || op == NRC;
}

bool PmatchUnaryOperation::is_delimiter(void)
{
    return op == AddDelimiters;
}

StringSet PmatchObject::get_initial_RC_initial_symbols(void)
{
    return StringSet();
}

StringSet PmatchObject::get_initial_NRC_initial_symbols(void)
{
    return StringSet();
}

StringSet PmatchBinaryOperation::get_initial_RC_initial_symbols(void)
{
    StringSet retval;
    if (op == Concatenate) {
        StringSet left_ss = left->get_initial_RC_initial_symbols();
        StringSet right_ss;
        if (right->is_context() || right->is_delimiter()) {
            right_ss = right->get_initial_NRC_initial_symbols();
        }
        retval.insert(left_ss.begin(), left_ss.end());
        retval.insert(right_ss.begin(), right_ss.end());
        return retval;
    }
    return retval;
}

StringSet PmatchBinaryOperation::get_initial_NRC_initial_symbols(void)
{
    StringSet retval;
    if (op == Concatenate) {
        StringSet left_ss = left->get_initial_NRC_initial_symbols();
        StringSet right_ss;
        if (right->is_context() || right->is_delimiter()) {
            right_ss = right->get_initial_NRC_initial_symbols();
        }
        retval.insert(left_ss.begin(), left_ss.end());
        retval.insert(right_ss.begin(), right_ss.end());
        return retval;
    }
    return retval;
}

StringSet PmatchUnaryOperation::get_initial_RC_initial_symbols(void)
{
    if (op == RC) {
        HfstTransducer * tmp = this->root->evaluate();
        StringSet retval(tmp->get_initial_input_symbols());
        delete tmp;
        return retval;
    }
    if (op == AddDelimiters) {
        return root->get_initial_RC_initial_symbols();
    }
    return StringSet();
}

StringSet PmatchUnaryOperation::get_initial_NRC_initial_symbols(void)
{
    if (op == NRC) {
        HfstTransducer * tmp = this->root->evaluate();
        StringSet retval(tmp->get_initial_input_symbols());
        delete tmp;
        return retval;
    }
    if (op == AddDelimiters) {
        return root->get_initial_NRC_initial_symbols();
    }
    return StringSet();
}

void PmatchObject::collect_initial_symbols_into(StringSet & allowed_initial_symbols,
                                                StringSet & disallowed_initial_symbols)
{
    // One or neither of allowed_initial_symbols and disallowed_initial_symbols
    // will have some symbols inserted to it.

    StringSet allowed = this->get_real_initial_symbols();
    StringSet required = this->get_initial_RC_initial_symbols();
    StringSet disallowed = this->get_initial_NRC_initial_symbols();

    // The first input symbols collected in this way may include:
    // - insertion arcs
    // - unknown, identity, default
    // - symbols after flag diacritics
    // - symbols from contexts
    expand_Ins_arcs(allowed);
    expand_Ins_arcs(required);
    expand_Ins_arcs(disallowed);

    if (allowed.size() == 0) {
        // Probably something went wrong, we'll just not make no judgement
        return;
    }

    if (string_set_has_meta_arc(allowed)) {
        if (required.size() != 0 && !string_set_has_meta_arc(required)) {
            // RC sets a constraint
            for (StringSet::iterator it = required.begin(); it != required.end(); ++it) {
                if (disallowed.count(*it) == 0) {
                    allowed_initial_symbols.insert(*it);
                }
            }
            return;
        } else {
            // Anything goes except what is disallowed
            if (disallowed.size() == 0 || string_set_has_meta_arc(disallowed)) {
                return;
            } else {
                disallowed_initial_symbols.insert(disallowed.begin(), disallowed.end());
                return;
            }
        }
    }

    // Now we can assume that "allowed" is nonempty and non-meta.

    if (required.size() == 0 || string_set_has_meta_arc(required)) {
        // RC poses no constraint
        for (StringSet::iterator it = allowed.begin(); it != allowed.end(); ++it) {
            if (disallowed.count(*it) == 0) {
                allowed_initial_symbols.insert(*it);
            }
        }
        return;
    }

    // Now we can assume that there is a genuine RC constraint.

    for (StringSet::iterator it = required.begin(); it != required.end(); ++it) {
        if (allowed.count(*it) == 1 && disallowed.count(*it) == 0) {
            allowed_initial_symbols.insert(*it);
        }
    }
    return;
}


void PmatchString::collect_strings_into(StringVector & strings)
{
    strings.push_back(string);
}

void PmatchBinaryOperation::collect_strings_into(StringVector & strings)
{
    left->collect_strings_into(strings);
    right->collect_strings_into(strings);
}

HfstTransducer * PmatchSymbol::evaluate(void)
{
    start_timing();
    HfstTransducer * retval = NULL;
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
    retval->set_final_weights(hfst::double_to_float(weight), true);
    retval->minimize();
    report_time();
    return retval;
}

PmatchObject * PmatchSymbol::evaluate_as_arg(void)
{
    if (symbol_in_local_context(sym)) {
        return symbol_from_local_context(sym)->evaluate_as_arg();
    } else if (symbol_in_global_context(sym)) {
        used_definitions.insert(sym);
        if (flatten && def_insed_expressions.count(sym) == 1) {
            return def_insed_expressions[sym]->evaluate_as_arg();
        } else {
            return symbol_from_global_context(sym)->evaluate_as_arg();
        }
    } else {
        if (verbose) {
            std::cerr << "Warning: interpreting undefined symbol \"" << sym
                      << "\" as label on line " << line_defined << "\n";
        }
        return new PmatchString(sym);
    }
}

void PmatchSymbol::collect_strings_into(StringVector & strings)
{
    if (symbol_in_local_context(sym)) {
        symbol_from_local_context(sym)->collect_strings_into(strings);
    } else if (symbol_in_global_context(sym)) {
        symbol_from_global_context(sym)->collect_strings_into(strings);
        used_definitions.insert(sym);
    } else {
        strings.push_back(sym);
    }
    return;
}

HfstTransducer * PmatchString::evaluate(void) {
    if (cache != NULL) {
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
    tmp->set_final_weights(hfst::double_to_float(weight), true);
    if (cache == NULL && should_use_cache() == true) {
        cache = tmp;
        cache->minimize();
        report_time();
        return new HfstTransducer(*cache);
    }
        report_time();
    return tmp;
}

PmatchObject * PmatchString::evaluate_as_arg(void) {
    return new PmatchString(*this);
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
    for (int i = 0; i < (int)args.size(); ++i) {
        local_env[args[i]] = funargs[i];
    }
    call_stack.push_back(local_env);
    HfstTransducer * retval = root->evaluate();
    retval->set_final_weights(hfst::double_to_float(weight), true);
    call_stack.pop_back();
    if (verbose) {
        double duration = (clock() - my_timer) /
            (double) CLOCKS_PER_SEC;
        std::cerr << "Call to " << name << " evaluated in " << duration << " seconds\n";
    }
    return retval;
}

HfstTransducer * PmatchFunction::evaluate(void)
{
    std::vector<PmatchObject *> funargs;
    return evaluate(funargs);
}

HfstTransducer * PmatchFuncall::evaluate(void)
{
    std::vector<PmatchObject * > evaluated_args;
    for (std::vector<PmatchObject *>::iterator it = args->begin();
         it != args->end(); ++it) {
        evaluated_args.push_back(
            (*it)->evaluate_as_arg());
    }
    HfstTransducer * retval = fun->evaluate(evaluated_args);
    for (std::vector<PmatchObject *>::iterator it =
             evaluated_args.begin(); it != evaluated_args.end();
         ++it) {
        delete *it;
    }
    return retval;
}

HfstTransducer * PmatchBuiltinFunction::evaluate(void)
{
    start_timing();
    HfstTransducer * retval = NULL;
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
    retval->set_final_weights(hfst::double_to_float(weight), true);
    report_time();
    return retval;
}

HfstTransducer * PmatchNumericOperation::evaluate(void)
{
    if (cache != NULL) {
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
    tmp->set_final_weights(hfst::double_to_float(weight), true);
    if (cache == NULL && should_use_cache() == true) {
        cache = tmp;
        cache->minimize();
        report_time();
        return new HfstTransducer(*cache);
    }
    report_time();
    return tmp;
}

HfstTransducer * PmatchUnaryOperation::evaluate(void)
{
    if (cache != NULL) {
        return new HfstTransducer(*cache);
    }
    HfstTransducer * retval = NULL;
    start_timing();

    // Special optimization cases
    if (op == Implode) {
        std::vector<std::string> strings;
        root->collect_strings_into(strings);
        std::string whole_string;
        for (std::vector<std::string>::iterator it = strings.begin();
             it != strings.end(); ++it) {
            whole_string += *it;
        }
        if (whole_string.size() > 0) {
            retval = new HfstTransducer(whole_string, format);
        } else {
            retval = new HfstTransducer(format);
        }
        retval->set_final_weights(hfst::double_to_float(weight), true);
        if (cache == NULL && should_use_cache() == true) {
            cache = retval;
            report_time();
            print_size_info(cache);
            return new HfstTransducer(*cache);
        }
        report_time();
        return retval;
    } else if (op == Explode) {
        std::vector<std::string> strings;
        root->collect_strings_into(strings);
        std::string whole_string;
        for (std::vector<std::string>::iterator it = strings.begin();
             it != strings.end(); ++it) {
            whole_string += *it;
        }
        HfstTokenizer tok;
        if (whole_string.size() > 0) {
            retval = new HfstTransducer(whole_string, tok, format);
        } else {
            retval = new HfstTransducer(format);
        }
        retval->set_final_weights(hfst::double_to_float(weight), true);
        if (cache == NULL && should_use_cache() == true) {
            cache = retval;
            report_time();
            print_size_info(cache);
            return new HfstTransducer(*cache);
        }
        report_time();
        return retval;
    }

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
        HfstTransducer any(hfst::internal_identity, hfst::pmatch::format);
        any.repeat_star();
        HfstTransducer * left = new HfstTransducer(any);
        left->concatenate(*retval);
        left->concatenate(any);
        delete retval;
        retval = left;
    } else if (op == ContainmentOnce) {
        hfst::ImplementationType xre_orig_type = hfst::xre::format;
        hfst::xre::format = hfst::pmatch::format;
        HfstTransducer * new_retval = hfst::xre::contains_once(retval);
        hfst::xre::format = xre_orig_type;
        delete retval;
        retval = new_retval;
    } else if (op == ContainmentOptional) {
        hfst::ImplementationType xre_orig_type = hfst::xre::format;
        hfst::xre::format = hfst::pmatch::format;
        HfstTransducer * new_retval = hfst::xre::contains_once_optional(retval);
        hfst::xre::format = xre_orig_type;
        delete retval;
        retval = new_retval;
    } else if (op == TermComplement) {
        HfstTransducer* any = new HfstTransducer(hfst::internal_identity,
                                                 hfst::pmatch::format);
        hfst::StringSet alphabet = get_non_special_alphabet(retval);
        for (hfst::StringSet::iterator it = alphabet.begin(); it != alphabet.end(); ++it) {
            HfstTransducer symbol(*it, hfst::pmatch::format);
            any->subtract(symbol);
        }
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
        HfstTransducer * tmp = make_list(retval);
        delete retval;
        retval = tmp;
    } else if (op == MakeExcList) {
        HfstTransducer * tmp = make_exc_list(retval);
        delete retval;
        retval = tmp;
    } else if (op == LC) {
        if (!parent_is_context) {
            retval->reverse();
            HfstTransducer * tmp = new HfstTransducer(hfst::internal_epsilon, LC_ENTRY_SYMBOL, format);
            tmp->concatenate(*retval);
            HfstTransducer lc_exit(hfst::internal_epsilon, LC_EXIT_SYMBOL, format);
            tmp->concatenate(lc_exit);
            delete retval;
            retval = tmp;
            }
    } else if (op == NLC) {
        if (!parent_is_context) {
            retval->reverse();
            PmatchTransducerContainer * tmp = make_minimization_guard();
            HfstTransducer * head = tmp->evaluate(); delete tmp;
            HfstTransducer passthrough(PASSTHROUGH_SYMBOL, format);
            HfstTransducer nlc_entry(hfst::internal_epsilon, NLC_ENTRY_SYMBOL, format);
            HfstTransducer nlc_exit(hfst::internal_epsilon, NLC_EXIT_SYMBOL, format);
            nlc_entry.concatenate(*retval);
            nlc_entry.concatenate(nlc_exit);
            nlc_entry.disjunct(passthrough);
            head->concatenate(nlc_entry);
            delete retval;
            retval = head;
            }
    } else if (op == RC) {
        if (!parent_is_context) {
            HfstTransducer * tmp = new HfstTransducer(hfst::internal_epsilon, RC_ENTRY_SYMBOL, format);
            tmp->concatenate(*retval);
            HfstTransducer rc_exit(hfst::internal_epsilon, RC_EXIT_SYMBOL, format);
            tmp->concatenate(rc_exit);
            delete retval;
            retval = tmp;
        }
    } else if (op == NRC) {
        if (!parent_is_context) {
            PmatchTransducerContainer * tmp = make_minimization_guard();
            HfstTransducer * head = tmp->evaluate(); delete tmp;
            HfstTransducer passthrough(PASSTHROUGH_SYMBOL, format);
            HfstTransducer nrc_entry(hfst::internal_epsilon, NRC_ENTRY_SYMBOL, format);
            HfstTransducer nrc_exit(hfst::internal_epsilon, NRC_EXIT_SYMBOL, format);
            nrc_entry.concatenate(*retval);
            nrc_entry.concatenate(nrc_exit);
            nrc_entry.disjunct(passthrough);
            head->concatenate(nrc_entry);
            delete retval;
            retval = head;
            }
    }

    retval->set_final_weights(hfst::double_to_float(weight), true);
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

HfstTransducer * PmatchBinaryOperation::evaluate(void)
{
    if (cache != NULL) {
        return new HfstTransducer(*cache);
    }
    start_timing();
    HfstTransducer * retval = NULL;

    // Special optimization cases
    if (op == Disjunct) {
        if (left->is_unweighted_disjunction_of_strings() &&
            right->is_unweighted_disjunction_of_strings()) {
            StringVector strings;
            left->collect_strings_into(strings);
            right->collect_strings_into(strings);
            HfstTokenizer tok;
            retval = new HfstTransducer(format);
            for (StringVector::iterator it = strings.begin(); it != strings.end(); ++it) {
                StringPairVector spv = tok.tokenize(*it);
                retval->disjunct(spv);
            }
            retval->set_final_weights(hfst::double_to_float(weight), true);
            if (cache == NULL && should_use_cache() == true) {
                cache = retval;
                // No minimization because we did it the clever way!
                print_size_info(cache);
                report_time();
                return new HfstTransducer(*cache);
            }
            report_time();
            return retval;
        }
    }

    // General cases
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
            (void)e;
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
            (void)e;
            pmatcherror("Error: transducers must be automata in merge operation.");
        }
        delete lhs; lhs = tmp;
    }
    delete rhs;
    lhs->set_final_weights(hfst::double_to_float(weight), true);
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

bool PmatchBinaryOperation::is_unweighted_disjunction_of_strings(void)
{
    return weight == 0.0 &&
        op == Disjunct &&
        left->is_unweighted_disjunction_of_strings() &&
        right->is_unweighted_disjunction_of_strings();
}


HfstTransducer * PmatchTernaryOperation::evaluate(void)
{
    if (cache != NULL) {
        return new HfstTransducer(*cache);
    }
    start_timing();
    HfstTransducer * retval = NULL;
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
    retval->set_final_weights(hfst::double_to_float(weight), true);
    if (cache == NULL && should_use_cache() == true) {
        cache = retval;
        cache->minimize();
        report_time();
        return new HfstTransducer(*cache);
    }
    report_time();
    return retval;
}

HfstTransducer * PmatchAcceptor::evaluate(void)
{
    start_timing();
    HfstTransducer * retval = NULL;
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
    retval->set_final_weights(hfst::double_to_float(weight), true);
    report_time();
    return retval;
}

HfstTransducer * PmatchParallelRulesContainer::evaluate(void)
{
    if (cache != NULL) {
        return new HfstTransducer(*cache);
    }
    start_timing();
    HfstTransducer * retval = NULL;
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
    retval->set_final_weights(hfst::double_to_float(weight), true);
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

HfstTransducer * PmatchReplaceRuleContainer::evaluate(void)
{
    if (cache != NULL) {
        return new HfstTransducer(*cache);
    }
    start_timing();
    HfstTransducer * retval = NULL;
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
    retval->set_final_weights(hfst::double_to_float(weight), true);
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
        TransducerPointerPair pp = (*it)->evaluate_pair();
        HfstTransducerPair p(HfstTransducer(*(pp.first)), HfstTransducer(*(pp.second)));
        delete pp.first;
        delete pp.second;
        pair_vector.push_back(p);
    }
    if (context.size() == 0) {
        return hfst::xeroxRules::Rule(pair_vector);
    }
    HfstTransducerPairVector context_vector;
    for (MappingPairVector::iterator it = context.begin();
         it != context.end(); ++it) {
        TransducerPointerPair pp = (*it)->evaluate_pair();
        HfstTransducerPair p(HfstTransducer(*(pp.first)), HfstTransducer(*(pp.second)));
        delete pp.first;
        delete pp.second;
        context_vector.push_back(p);
    }
    return hfst::xeroxRules::Rule(pair_vector, context_vector, type);
}

HfstTransducer * PmatchQuestionMark::evaluate(void)
{
    start_timing();
    HfstTransducer * retval = NULL;
    retval = new HfstTransducer(hfst::internal_identity, format);
    retval->set_final_weights(hfst::double_to_float(weight), true);
    report_time();
    return retval;
}

HfstTransducer * PmatchRestrictionContainer::evaluate(void)
{
    if (cache != NULL) {
        return new HfstTransducer(*cache);
    }
    start_timing();
    HfstTransducer * retval = NULL;
    HfstTransducerPairVector pair_vector;
    for (MappingPairVector::iterator it = contexts->begin();
         it != contexts->end(); ++it) {
        TransducerPointerPair pp = (*it)->evaluate_pair();
        HfstTransducerPair p(HfstTransducer(*(pp.first)), HfstTransducer(*(pp.second)));
        delete pp.first;
        delete pp.second;
        pair_vector.push_back(p);
    }
    HfstTransducer * l = left->evaluate();
    retval = new HfstTransducer(hfst::xeroxRules::restriction(*l, pair_vector));
    delete l;
    retval->set_final_weights(hfst::double_to_float(weight), true);
    report_time();
    if (cache == NULL && should_use_cache() == true) {
        cache = retval;
        cache->minimize();
        return new HfstTransducer(*cache);
    }
    return retval;
}

TransducerPointerPair PmatchMarkupContainer::evaluate_pair(void) {
    TransducerPointerPair retval;
    HfstTransducer * loa = left_of_arrow->evaluate();
    HfstTransducer * lom = left->evaluate();
    HfstTransducer * rom = right->evaluate();
    HfstTransducerPair tmpMappingPair(*loa, HfstTransducer(format));
    HfstTransducerPair marks(*lom, *rom);
    HfstTransducerPair MappingPair = hfst::xeroxRules::create_mapping_for_mark_up_replace(tmpMappingPair, marks);
    delete loa; delete lom; delete rom;
    retval.first = new HfstTransducer(MappingPair.first);
    retval.second = new HfstTransducer(MappingPair.second);
    return retval;
}

HfstTransducer * PmatchMappingPairsContainer::evaluate(void) { pmatcherror("Should never happen\n"); return 0; }
HfstTransducer * PmatchContextsContainer::evaluate(void) { pmatcherror("Should never happen\n"); return 0; }

} }
