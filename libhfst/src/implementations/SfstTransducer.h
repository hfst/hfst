// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#ifndef _SFST_TRANSDUCER_H_
#define _SFST_TRANSDUCER_H_

#include "HfstExceptionDefs.h"
#include "HfstFlagDiacritics.h"
#include "HfstSymbolDefs.h"
#include "HfstExtractStrings.h"
//#include "back-ends/sfst/interface.h"
//#include "back-ends/sfst/fst.h"

namespace SFST { class Transducer; typedef short unsigned int Character; class Alphabet; class Label; }

#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>

/** @file SfstTransducer.h
    \brief Declarations of functions and datatypes that form a bridge between
    HFST API and SFST. */

namespace hfst {
namespace implementations
{
  //using namespace SFST;
  typedef SFST::Transducer Transducer;
  ;
  using std::ostream;
  using std::ostringstream;

  void sfst_set_hopcroft(bool);

  class SfstInputStream
  {
  private:
    std::string filename;
    FILE * input_file;
    bool is_minimal;  // whether the next transducer in the stream is minimal
                      // this can be said in the header
    void add_symbol(StringNumberMap &string_number_map,
                    SFST::Character c,
                    SFST::Alphabet &alphabet);

  public:
    SfstInputStream(void);
    SfstInputStream(const std::string &filename);
    void close(void);
    bool is_eof(void);
    bool is_bad(void);
    bool is_good(void);
    bool is_fst(void);
    void ignore(unsigned int);

    char stream_get();
    short stream_get_short();
    void stream_unget(char c);

    bool set_implementation_specific_header_data
      (StringPairVector &data, unsigned int index);
    SFST::Transducer * read_transducer();
    
    static bool is_fst(FILE * f);
    static bool is_fst(std::istream &s);
  };

  class SfstOutputStream
  {
  private:
    std::string filename;
    FILE *ofile;
    //void write_3_0_library_header(FILE *file, bool is_minimal);
  public:
    SfstOutputStream(void);
    SfstOutputStream(const std::string &filename);
    void close(void);
    void write(const char &c);
    void append_implementation_specific_header_data
      (std::vector<char> &header, SFST::Transducer *t);
    void write_transducer(SFST::Transducer * transducer);
  };

  /*
  class HfstNode2Int {
    
    struct hashf {
      size_t operator()(const SFST::Node *node) const {
    return (size_t)node;
      }
    };
    struct equalf {
      int operator()(const SFST::Node *n1, const SFST::Node *n2) const {
    return (n1 == n2);
      }
    };
    typedef SFST::hash_map<SFST::Node*, int, hashf, equalf> NL;
    
  private:
    NL number;
    
  public:
    int &operator[]( SFST::Node *node ) {
      NL::iterator it=number.find(node);
      if (it == number.end())
    return number.insert(NL::value_type(node, 0)).first->second;
      return it->second;
    };
    };*/
  
  
  class SfstTransducer
    {
    public:
      static SFST::Transducer * create_empty_transducer(void);
      static SFST::Transducer * create_epsilon_transducer(void);

      static SFST::Transducer * define_transducer(unsigned int number);
      static SFST::Transducer * define_transducer
        (unsigned int inumber, unsigned int onumber);

      static void delete_transducer(SFST::Transducer * t);

      static SFST::Transducer * define_transducer(const std::string &symbol);
      static SFST::Transducer * define_transducer
        (const std::string &isymbol, const std::string &osymbol);
      static SFST::Transducer * define_transducer
        (const StringPairVector &spv);
      static SFST::Transducer * define_transducer
        (const StringPairSet &sps, bool cyclic=false);
      static SFST::Transducer * define_transducer
        (const std::vector<StringPairSet> &spsv);
      static SFST::Transducer * copy(SFST::Transducer * t);
      static SFST::Transducer * determinize(SFST::Transducer * t);
      static SFST::Transducer * minimize(SFST::Transducer * t);
      static SFST::Transducer * remove_epsilons(SFST::Transducer * t);
      static SFST::Transducer * repeat_star(SFST::Transducer * t);
      static SFST::Transducer * repeat_plus(SFST::Transducer * t);
      static SFST::Transducer * repeat_n(SFST::Transducer * t,unsigned int n);
      static SFST::Transducer * repeat_le_n(SFST::Transducer * t,unsigned int n);
      static SFST::Transducer * optionalize(SFST::Transducer * t);
      static SFST::Transducer * invert(SFST::Transducer * t);
      static SFST::Transducer * reverse(SFST::Transducer * transducer);
      static SFST::Transducer * extract_input_language(SFST::Transducer * t);
      static SFST::Transducer * extract_output_language(SFST::Transducer * t);
      static std::vector<SFST::Transducer*> extract_path_transducers
        (SFST::Transducer *t);
      static void extract_paths
        (SFST::Transducer * t, hfst::ExtractStringsCb& callback, int cycles=-1,
         FdTable<SFST::Character>* fd=NULL, bool filter_fd=false);

      static void extract_random_paths
    (SFST::Transducer *t, HfstTwoLevelPaths &results, int max_num);

      static SFST::Transducer * insert_freely
        (SFST::Transducer *t , const StringPair &symbol_pair);
      static SFST::Transducer * substitute
        (SFST::Transducer * t, String old_symbol, String new_symbol);
      static SFST::Transducer * substitute
        (SFST::Transducer *t, const StringPair &symbol_pair, SFST::Transducer *tr);

      static SFST::Transducer * compose(SFST::Transducer * t1,
                           SFST::Transducer * t2);
      static SFST::Transducer * concatenate(SFST::Transducer * t1,
                               SFST::Transducer * t2);
      static SFST::Transducer * disjunct(SFST::Transducer * t1,
                            SFST::Transducer * t2);
      static SFST::Transducer * intersect(SFST::Transducer * t1,
                             SFST::Transducer * t2);
      static SFST::Transducer * subtract(SFST::Transducer * t1,
                            SFST::Transducer * t2);
      static std::pair<SFST::Transducer*, SFST::Transducer*> harmonize
        (SFST::Transducer *t1, SFST::Transducer *t2, bool unknown_symbols_in_use=true);

      static bool are_equivalent(SFST::Transducer * t1, SFST::Transducer * t2);
      static bool is_cyclic(SFST::Transducer * t);
      static bool is_automaton(SFST::Transducer * t);
      
      static FdTable<SFST::Character>* get_flag_diacritics(SFST::Transducer * t);

      static void print_test(SFST::Transducer *t);
      static void print_alphabet(SFST::Transducer *t);

      static unsigned int get_biggest_symbol_number(SFST::Transducer * t);

      static StringVector get_symbol_vector(SFST::Transducer * t);

      static std::map<std::string, unsigned int> get_symbol_map(SFST::Transducer * t);

      static SFST::Transducer * disjunct(SFST::Transducer * t, const StringPairVector &spv);

      static StringPairSet get_symbol_pairs(SFST::Transducer *t);

      float get_profile_seconds();
      static unsigned int number_of_states(SFST::Transducer *t);
      static unsigned int number_of_arcs(SFST::Transducer *t);

      static StringSet get_alphabet(SFST::Transducer *t);
      static void insert_to_alphabet(SFST::Transducer *t, const std::string &symbol);
      static void remove_from_alphabet
    (SFST::Transducer *t, const std::string &symbol);
      static unsigned int get_symbol_number(SFST::Transducer *t,
                        const std::string &symbol);

    protected:
      static void initialize_alphabet(SFST::Transducer *t);
      static SFST::Transducer * expand_arcs(SFST::Transducer * t, StringSet &unknown);

      static void expand_node( SFST::Transducer *t, SFST::Node *origin, SFST::Label &l,
                               SFST::Node *target, hfst::StringSet &s );
      static void expand2
        ( SFST::Transducer *t, SFST::Node *node,
          hfst::StringSet &new_symbols, std::set<SFST::Node*> &visited_nodes );
      static void expand(SFST::Transducer *t, hfst::StringSet &new_symbols);

    };
} }
#endif
