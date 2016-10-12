// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#ifndef _FOMA_TRANSDUCER_H_
#define _FOMA_TRANSDUCER_H_

#include "HfstSymbolDefs.h"
#include "HfstExceptionDefs.h"
#include "HfstExtractStrings.h"
#include "HfstFlagDiacritics.h"
#include <stdlib.h>

//#ifndef _FOMALIB_H_
//#define _FOMALIB_H_
//#include "back-ends/foma/fomalib.h"
//#endif

struct fsm;

#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>

#include "../FormatSpecifiers.h"

/** @file FomaTransducer.h
    \brief Declarations of functions and datatypes that form a bridge between
    HFST API and foma. */

namespace hfst {
  namespace implementations
{
  ;
  using std::ostream;
  using std::ostringstream;

  class FomaInputStream
  {
  private:
    std::string filename;
    FILE * input_file;
    void skip_identifier_version_3_0(void);
    void skip_hfst_header(void);
  public:
    FomaInputStream(void);
    FomaInputStream(const std::string &filename);
    void close(void);
    bool is_eof(void);
    bool is_bad(void);
    bool is_good(void);
    bool is_fst(void);
    void ignore(unsigned int);
    fsm * read_transducer();

    char stream_get();
    short stream_get_short();
    void stream_unget(char c);
    
    static bool is_fst(FILE * f);
    static bool is_fst(std::istream &s);
  };

  class FomaOutputStream
  {
  private:
    std::string filename;
    FILE *ofile;
    //void write_3_0_library_header(FILE *file);
  public:
    FomaOutputStream(void);
    FomaOutputStream(const std::string &filename);
    void close(void);
    void write(const char &c);
    void write_transducer(fsm * transducer);
  };

  class FomaTransducer
    {
    public:
      static fsm * create_empty_transducer(void);
      static fsm * create_epsilon_transducer(void);
      static fsm * define_transducer(const hfst::StringPairVector &spv);
      static fsm * define_transducer
        (const hfst::StringPairSet &sps, bool cyclic=false);
      static fsm * define_transducer(const std::vector<StringPairSet> &spsv);
      static fsm * define_transducer
        (const std::string &symbol);
      static fsm * define_transducer
        (const std::string &isymbol, const std::string &osymbol);
      static fsm * copy(fsm * t);
      static fsm * determinize(fsm * t);
      static fsm * minimize(fsm * t);
      static fsm * remove_epsilons(fsm * t);
      static fsm * repeat_star(fsm * t);
      static fsm * repeat_plus(fsm * t);
      static fsm * repeat_n(fsm * t, unsigned int n);
      static fsm * repeat_le_n(fsm * t, unsigned int n);
      static fsm * optionalize(fsm * t);
      static fsm * invert(fsm * t);
      static fsm * reverse(fsm * t);
      static fsm * extract_input_language(fsm * t);
      static fsm * extract_output_language(fsm * t);

      static fsm * insert_freely(fsm * t, const StringPair &symbol_pair);

      static bool are_equivalent(fsm *t1, fsm *t2);
      static bool is_cyclic(fsm * t);

      static fsm * substitute
        (fsm * t,hfst::String old_symbol,hfst::String new_symbol);

      static fsm * compose(fsm * t1,
                           fsm * t2);
      static fsm * concatenate(fsm * t1,
                               fsm * t2);
      static fsm * disjunct(fsm * t1,
                            fsm * t2);
      static fsm * intersect(fsm * t1,
                             fsm * t2);
      static fsm * subtract(fsm * t1,
                            fsm * t2);
                        
      static void extract_paths(fsm * t, hfst::ExtractStringsCb& callback,
                                  int cycles=-1, FdTable<int>* fd=NULL,
                                  bool filter_fd=false);
      static void extract_random_paths
    (const fsm *t, HfstTwoLevelPaths &results, int max_num);

      static FdTable<int>* get_flag_diacritics(fsm * t);

      static unsigned int get_biggest_symbol_number(fsm * t);
      static StringVector get_symbol_vector(fsm * t);
      static std::map<std::string, unsigned int> get_symbol_map(fsm * t);

      static void insert_to_alphabet(fsm *t, const std::string &symbol);
      static void remove_from_alphabet(fsm *t, const std::string &symbol);
      static StringSet get_alphabet(fsm *t);
      static unsigned int get_symbol_number(fsm *t,
                        const std::string &symbol);

      static void harmonize(fsm *net1, fsm *net2);

      static fsm * read_net(FILE * file);
      static int write_net(fsm * net, FILE * file);
                        
      static void delete_foma(fsm * net);
      static void print_test(fsm * t);

#if GENERATE_LEXC_WRAPPER
      static fsm * read_lexc(const std::string &filename, bool verbose);
#endif

      static unsigned int number_of_states(fsm * net);
      static unsigned int number_of_arcs(fsm * net);

      static fsm * eliminate_flags(fsm * t);
      static fsm * eliminate_flag(fsm * t, const std::string & flag);

    };

} }
#endif
