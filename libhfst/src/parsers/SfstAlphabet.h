// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

/*
   HFST has a written agreement with the author of SFST, Helmut Schmid,
   that this file, though derived from SFST which is GPLv2+,
   may be distributed under the LGPLv3+ license as part of HFST.
*/

#ifndef _SFST_ALPHABET_H_
#define _SFST_ALPHABET_H_

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <unordered_map>
using std::unordered_map;

#include <set>
#include <vector>
#include <string.h>
#include <stdio.h>

/* @file SfstAlphabet.h
   \brief Declaration of class SfstAlphabet. */

namespace hfst {
  namespace implementations {

    /* copied from SFST's alphabet.h|C */
    class SfstAlphabet {

    public:
      typedef std::pair<unsigned int,unsigned int> NumberPair;
      // used to map the codes back to the symbols
      typedef unordered_map<unsigned int, char*> CharMap;

    private:
      // string comparison operators needed ???
      struct eqstr {
    bool operator()(const char* s1, const char* s2) const {
      return strcmp(s1, s2) == 0;
    }
      };

      // used to map the symbols to their codes
      typedef unordered_map<const char*, unsigned int> SymbolMap;

      // set of symbol pairs
      typedef std::set<NumberPair> NumberPairSet;

      SymbolMap sm; // maps symbols to codes
      CharMap  cm; // maps codes to symbols


      // The set of string pairs
      NumberPairSet pairs;

    public:
      SfstAlphabet();
      SfstAlphabet(const SfstAlphabet &alpha);
      ~SfstAlphabet();

      typedef NumberPairSet::const_iterator const_iterator;
      const_iterator begin() const;
      const_iterator end() const;
      size_t size() const;

      //bool contains_special_symbols(StringPair sp);

      void print();
      void print_pairs(FILE *file);

      void insert(NumberPair sp);
      void clear_pairs();
      CharMap get_char_map();

      void add( const char *symbol, unsigned int c );
      void add_symbol( const char *symbol, unsigned int c );
      int symbol2code( const char * s ) const;
      const char *code2symbol( unsigned int c ) const;
      unsigned int add_symbol(const char * symbol);
      void complement( std::vector<unsigned int> &sym );

      std::pair<unsigned int, unsigned int> next_label(char *&, bool extended=true);
      int next_code( char* &string, bool extended=true, bool insert=true );
      int next_mcsym( char* &string, bool insert=true );
    };

  }
}

#endif
