// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#ifndef _LOG_WEIGHT_TRANSDUCER_H_
#define _LOG_WEIGHT_TRANSDUCER_H_

#include "HfstSymbolDefs.h"
#include "HfstExceptionDefs.h"
#include "HfstFlagDiacritics.h"

#if HAVE_CONFIG_H
  #include "../../../config.h"
#endif

//#ifdef _MSC_VER
// #include "back-ends/openfstwin/src/include/fst/fstlib.h"
//#else
// #include "back-ends/openfst/src/include/fst/fstlib.h"
//#endif // _MSC_VER

#include "HfstExtractStrings.h"
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
//#include "HfstAlphabet.h"

#ifdef OPENFST_VERSION_1_5_4
  #include "back-ends/openfst/src/include/fst/fst-decl.h"
#else
namespace fst
{
  template <class W> class TropicalWeightTpl;
  typedef TropicalWeightTpl<float> TropicalWeight;

  template <class W> class LogWeightTpl;
  typedef LogWeightTpl<float> LogWeight;

  template <class W> class ArcTpl;
  typedef ArcTpl<TropicalWeight> StdArc;
  typedef ArcTpl<LogWeight> LogArc;

  template <class A> class VectorFst;
  typedef VectorFst<StdArc> StdVectorFst;
  typedef VectorFst<LogArc> LogFst;

  class SymbolTable;
}
#endif

#ifdef _MSC_VER
typedef __int64 int64;
#else
typedef int64_t int64;
#endif // _MSC_VER

/** @file LogWeightTransducer.h
    \brief Declarations of functions and datatypes that form a bridge between
    HFST API and OpenFst's transducers with logarithmic weights. */

namespace hfst {
namespace implementations
{
  using namespace fst;
  ;
  //typedef LogArc::StateId StateId;
  typedef unsigned int StateId;
  typedef VectorFst<LogArc> LogFst;

  typedef std::vector<LogArc> LogArcVector;
  struct LogArcLessThan {
    bool operator() (const LogArc &arc1,const LogArc &arc2) const; };

  using std::ostream;
  using std::ostringstream;
  using std::stringstream;

  void openfst_log_set_hopcroft(bool value);

  class LogWeightInputStream
  {
  private:
    std::string filename;
    std::ifstream i_stream;
    std::istream &input_stream;
    void skip_identifier_version_3_0(void);
    void skip_hfst_header(void);
  public:
    LogWeightInputStream(void);
    LogWeightInputStream(const std::string &filename);
    void close(void);
    bool is_eof(void) const;
    bool is_bad(void) const;
    bool is_good(void) const;
    bool is_fst(void) const;
    bool operator() (void) const;
    void ignore(unsigned int);
    LogFst * read_transducer();

    char stream_get();
    short stream_get_short();
    void stream_unget(char c);
    
    static bool is_fst(FILE * f);
    static bool is_fst(std::istream &s);
  };

  class LogWeightOutputStream
  {
  private:
    std::string filename;
    std::ofstream o_stream;
    std::ostream &output_stream;
    //void write_3_0_library_header(std::ostream &out);
  public:
    LogWeightOutputStream(void);
    LogWeightOutputStream(const std::string &filename);
    void close(void);
    void write(const char &c);
    void write_transducer(LogFst * transducer);
  };

  /*  class LogWeightTransitionIterator;

  typedef StateId LogWeightState;

  class LogWeightStateIterator
    {
    protected:
      StateIterator<LogFst> * iterator;
    public:
      LogWeightStateIterator(LogFst * t);
      ~LogWeightStateIterator(void);
      void next(void);
      bool done(void);
      LogWeightState value(void);
    };
 

  class LogWeightTransition
    {
    protected:
      LogArc arc;
      LogFst * t;
    public:
      LogWeightTransition(const LogArc &arc, LogFst *t);
      ~LogWeightTransition(void);
      std::string get_input_symbol(void) const;
      std::string get_output_symbol(void) const;
      LogWeightState get_target_state(void) const;
      LogWeight get_weight(void) const;
    };


  class LogWeightTransitionIterator
    {
    protected:
      ArcIterator<LogFst> * arc_iterator;
      LogFst * t;
    public:
      LogWeightTransitionIterator(LogFst * t, StateId state);
      ~LogWeightTransitionIterator(void);
      void next(void);
      bool done(void);
      LogWeightTransition value(void);
    };
  */

  class LogWeightTransducer
    {
    public:
      static LogFst * create_empty_transducer(void);
      static LogFst * create_epsilon_transducer(void);
      void delete_transducer(LogFst * t);

      // string versions
      static LogFst * define_transducer(const std::string &symbol);
      static LogFst * define_transducer
        (const std::string &isymbol, const std::string &osymbol);
      static LogFst * define_transducer
        (const hfst::StringPairVector &spv);
      static LogFst * define_transducer
        (const hfst::StringPairSet &sps, bool cyclic=false);
      static LogFst * define_transducer(const std::vector<StringPairSet> &spsv);

      // number versions
      static LogFst * define_transducer(unsigned int number);
      static LogFst * define_transducer
        (unsigned int inumber, unsigned int onumber);
      static LogFst * define_transducer(const hfst::NumberPairVector &npv);
      static LogFst * define_transducer
        (const hfst::NumberPairSet &nps, bool cyclic=false);
      static LogFst * define_transducer
        (const std::vector<NumberPairSet> &npsv);

      static LogFst * copy(LogFst * t);
      static LogFst * determinize(LogFst * t);
      static LogFst * minimize(LogFst * t);
      static LogFst * remove_epsilons(LogFst * t);
      static LogFst * n_best(LogFst * t, unsigned int n);
      static LogFst * repeat_star(LogFst * t);
      static LogFst * repeat_plus(LogFst * t);
      static LogFst * repeat_n(LogFst * t, unsigned int n);
      static LogFst * repeat_le_n(LogFst * t, unsigned int n);
      static LogFst * optionalize(LogFst * t);
      static LogFst * invert(LogFst * t);
      static LogFst * reverse(LogFst * transducer);
      static LogFst * extract_input_language(LogFst * t);
      static LogFst * extract_output_language(LogFst * t);
      static void extract_paths
        (LogFst * t, hfst::ExtractStringsCb& callback,
         int cycles=-1, FdTable<int64>* fd=NULL, bool filter_fd=false
         /*bool include_spv=false*/);
      static void extract_random_paths
    (const LogFst *t, HfstTwoLevelPaths &results, int max_num);
      static LogFst * compose(LogFst * t1,
                                   LogFst * t2);
      static LogFst * concatenate(LogFst * t1,
                                        LogFst * t2);
      static LogFst * disjunct(LogFst * t1,
                              LogFst * t2);

      static LogFst * disjunct(LogFst * t, const StringPairVector &spv);
      static LogFst * disjunct(LogFst * t, const NumberPairVector &npv);

      static LogFst * intersect(LogFst * t1,
                             LogFst * t2);
      static LogFst * subtract(LogFst * t1,
                            LogFst * t2);
      static LogFst * set_weight(LogFst * t,float f);
      static LogFst * set_final_weights(LogFst * t, float weight);
      static LogFst * transform_weights(LogFst * t,float (*func)(float f));
      static LogFst * push_weights(LogFst * t, bool to_initial_state);

      static std::pair<LogFst*, LogFst*> harmonize
        (LogFst *t1, LogFst *t2, bool unknown_symbols_in_use=true);

      static void write_in_att_format(LogFst * t, FILE *ofile);
      static void write_in_att_format_number(LogFst * t, FILE *ofile);
      
      static void test_minimize(void);

      static void write_in_att_format(LogFst * t, std::ostream &os);
      static void write_in_att_format_number(LogFst * t, std::ostream &os);

      static LogFst * read_in_att_format(FILE *ifile);
      
      static bool are_equivalent(LogFst *one, LogFst *another);
      static bool is_cyclic(LogFst * t);
      static bool is_automaton(LogFst * t);

      static FdTable<int64>* get_flag_diacritics(LogFst * t);

      // string versions
      static LogFst * insert_freely(LogFst * t, const StringPair &symbol_pair);
      static LogFst * substitute
        (LogFst * t, std::string old_symbol, std::string new_symbol);
      static LogFst * substitute(LogFst * t,
                                 StringPair old_symbol_pair,
                                 StringPair new_symbol_pair);
      static LogFst * substitute(LogFst * t,
                                 StringPair old_symbol_pair,
                                 StringPairSet new_symbol_pair_set);
      static LogFst * substitute(LogFst * t,
                                 const StringPair old_symbol_pair,
                                 LogFst *transducer);

      // number versions
      static LogFst * insert_freely(LogFst * t, const NumberPair &number_pair);
      static LogFst * substitute(LogFst * t, unsigned int, unsigned int);
      static LogFst * substitute(LogFst * t,
                                       NumberPair old_number_pair,
                                       NumberPair new_number_pair);
      static LogFst * substitute(LogFst * t,
                                       const NumberPair old_number_pair,
                                       LogFst *transducer);

      static void insert_to_alphabet
        (LogFst *t, const std::string &symbol);
      static void remove_from_alphabet
        (LogFst *t, const std::string &symbol);
      static StringSet get_alphabet(LogFst *t);
      static unsigned int get_symbol_number(LogFst *t,
                        const std::string &symbol);

      static NumberNumberMap create_mapping(LogFst * t1, LogFst * t2);
      static void recode_symbol_numbers(LogFst * t, hfst::NumberNumberMap &km);

      static LogFst * expand_arcs
        (LogFst * t, hfst::StringSet &unknown, bool unknown_symbols_in_use);

      float get_profile_seconds();

      static unsigned int number_of_states(const LogFst * t);

    private:
      static fst::SymbolTable create_symbol_table(std::string name);
      static void initialize_symbol_tables(LogFst *t);
      static void remove_symbol_table(LogFst *t);
      
      /* Maps state numbers in AT&T text format to state ids used by
         OpenFst transducers. */
      typedef std::map<int, StateId> StateMap;

      static StateId add_and_map_state
        (LogFst *t, int state_number, StateMap &state_map);

      static int has_arc(LogFst &t,
                         /*LogArc::StateId*/ int sourcestate,
                         /*LogArc::Label*/ int ilabel,
                         /*LogArc::Label*/ int olabel);
      static void disjunct_as_tries(LogFst &t1,
                             StateId t1_state,
                             const LogFst * t2,
                             StateId t2_state);
      static void add_sub_trie(LogFst &t1,
                        StateId t1_state,
                        const LogFst * t2,
                        StateId t2_state);

    public:
      static StateId add_state(LogFst *t);
      static void set_final_weight(LogFst *t, StateId s, float w);
      static void add_transition
        (LogFst *t, StateId source,
         std::string &isymbol, std::string &osymbol, float w, StateId target);
      static float get_final_weight(LogFst *t, StateId s);
      static float is_final(LogFst *t, StateId s);
      static StateId get_initial_state(LogFst *t);
      static void represent_empty_transducer_as_having_one_state(LogFst *t);

    };

} }
#endif
