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

#include "SfstAlphabet.h"
#include "SfstBasic.h"
#include "SfstUtf8.h"
#include "HfstSymbolDefs.h"
#include <cstdlib>

#ifndef DEBUG_MAIN

namespace hfst {
  namespace implementations {

    SfstAlphabet::SfstAlphabet() {
      add(hfst::internal_epsilon.c_str(),0);
      add(hfst::internal_unknown.c_str(),1);
      add(hfst::internal_identity.c_str(),2);
    }

    SfstAlphabet::SfstAlphabet(const SfstAlphabet &alpha)
    {
      for (auto it : alpha.cm)
        {
          cm[it.first] = sfst_basic::fst_strdup(it.second);
        }
      for (auto it : alpha.sm)
        {
          sm[sfst_basic::fst_strdup(it.first)] = it.second;
        }
      for (const auto& it : alpha.pairs)
        {
          pairs.insert(NumberPair(it.first, it.second));
        }
    }

    SfstAlphabet::~SfstAlphabet() {
      char **s=new char*[cm.size()];
      pairs.clear();
      sm.clear();

      size_t i, n = 0;
      for (auto& it : cm)
        s[n++] = it.second;
      cm.clear();

      for (i = 0; i < n; i++)
        free(s[i]);
      delete[] s;
    }

    SfstAlphabet::const_iterator SfstAlphabet::begin() const { return pairs.begin(); }
    SfstAlphabet::const_iterator SfstAlphabet::end() const { return pairs.end(); };
    size_t SfstAlphabet::size() const { return pairs.size(); };

    //bool SfstAlphabet::contains_special_symbols(StringPair sp);

    void SfstAlphabet::print_pairs(FILE* file)
    {
      for (const auto& pair : pairs)
        fprintf(file, "%s:%s\n", code2symbol(pair.first), code2symbol(pair.second));
    }

    void SfstAlphabet::print()
    {
      printf("alphabet..\n");
      for (auto& it : cm)
        printf("%i\t%s\n", it.first, it.second);
      printf("..alphabet\n");
    }

    void SfstAlphabet::insert(NumberPair sp) {
      /* check special symbols */ pairs.insert(sp); };
    void SfstAlphabet::clear_pairs() { pairs.clear(); };
    SfstAlphabet::CharMap SfstAlphabet::get_char_map() { return cm; };

    void SfstAlphabet::add( const char *symbol, unsigned int c ) {
      char *s = sfst_basic::fst_strdup(symbol);
      cm[c] = s;
      sm[s] = c;
    }

    int SfstAlphabet::symbol2code( const char * s ) const {
      SymbolMap::const_iterator p = sm.find(s);
      if (p != sm.end()) return p->second;
      return EOF;
    }

    const char *SfstAlphabet::code2symbol( unsigned int c ) const {
      CharMap::const_iterator p=cm.find(c);
      if (p == cm.end())
    return NULL;
      else
    return p->second;
    }

    unsigned int SfstAlphabet::add_symbol(const char * symbol) {
      if (sm.find(symbol) != sm.end())
    return sm[symbol];

      // assign the symbol to some unused character
      for(unsigned int i=1; i!=0; i++)
    if (cm.find(i) == cm.end()) {
      add(symbol, i);
      return i;
    }

      throw "Error: too many symbols in transducer definition";
    }

    void SfstAlphabet::add_symbol( const char *symbol, unsigned int c )

    {
      // check whether the symbol was previously defined
      int sc=symbol2code(symbol);
      if (sc != EOF) {
    if ((unsigned int)sc == c)
      return;

    if (strlen(symbol) < 60) {
      static char message[100];
      sprintf(message, "Error: reinserting symbol '%s' in alphabet with incompatible character value %u %u", symbol, (unsigned)sc, (unsigned)c);
      throw message;
    }
    else
      throw "reinserting symbol in alphabet with incompatible character value";
      }

      // check whether the character is already in use
      const char *s=code2symbol(c);
      if (s == NULL)
    add(symbol, c);
      else {
    if (strcmp(s, symbol) != 0) {
      static char message[100];
      if (strlen(symbol) < 70)
        sprintf(message,"Error: defining symbol %s as character %d (previously defined as %s)", symbol, (unsigned)c, s);
      else
        sprintf(message,"Error: defining a (very long) symbol with previously used character");
      throw message;
    }
      }
    }

    void SfstAlphabet::complement(std::vector<unsigned int>& sym)
    {
      std::vector<unsigned int> result;
      for (auto it : cm)
        {
          unsigned int c = it.first;
          if (c != 0 && c != 1 && c != 2)
            {  // no special symbols
              size_t i;
              for (i = 0; i < sym.size(); i++)
                if (sym[i] == c)
                  break;
              if (i == sym.size())
                result.push_back(c);
            }
        }
      sym.swap(result);
    }

    /*******************************************************************/
    /*                                                                 */
    /*  Alphabet::next_mcsym                                           */
    /*                                                                 */
    /*  recognizes multi-character symbols which are enclosed with     */
    /*  angle brackets <...>. If the argument flag insert is true,     */
    /*  the multi-character symbol must be already in the lexicon in   */
    /*  order to be recognized.                                        */
    /*                                                                 */
    /*******************************************************************/

    int SfstAlphabet::next_mcsym( char* &string, bool insert )

    {
      char *start=string;

      if (*start == '<')
    // symbol might start here
    for( char *end=start+1; *end; end++ )
      if (*end == '>') {
        // matching pair of angle brackets found
        // mark the end of the substring with \0
        char lastc = *(++end);
        *end = 0;

        int c;

        // handle epsilon symbol "<>" here
        if (strcmp(start,"<>") == 0) {
          c = 0;
        }
        else {
          if (insert)
        c = add_symbol( start );
          else
        c = symbol2code(start);
        }
        // restore the original string
        *end = lastc;

        if (c != EOF) {
          // symbol found
          // return its code
          string = end;
          return (unsigned int)c;
        }
        else
          // not a complex character
          break;
      }
      return EOF;
    }

    int SfstAlphabet::next_code( char* &string, bool extended, bool insert )

    {
      if (*string == 0)
    return EOF; // finished

      int c = next_mcsym(string, insert);
      if (c != EOF)
    return c;

      if (extended && *string == '\\')
    string++; // remove quotation

      //if (utf8) {
      {
    unsigned int c = sfst_utf8::utf8toint( &string );
    return (int)add_symbol(sfst_utf8::int2utf8(c));
      }
      //}
      /*else {
    char buffer[2];
    buffer[0] = *string;
    buffer[1] = 0;
    string++;
    return (int)add_symbol(buffer);
    }*/
    }

    std::pair<unsigned int, unsigned int> SfstAlphabet::next_label(char * &string, bool extended)
    {
      // read first character
      int c = next_code( string, extended );
      if (c == EOF) {
    return std::pair<unsigned int, unsigned int>(0,0); // end of string reached
      }

      unsigned int lc=(unsigned int)c;
      if (!extended || *string != ':') { // single character?
    if (lc == 0)
      return next_label(string, extended); // ignore epsilon
    return std::pair<unsigned int, unsigned int>(lc,lc);
      }

      // read second character
      string++; // jump over ':'
      c = next_code( string );
      if (c == EOF) {
    static char buffer[1000];
    sprintf(buffer,"Error: incomplete symbol in input file: %s", string);
    throw buffer;
      }

      std::pair<unsigned int, unsigned int> retval(lc, (unsigned int)c);
      if (retval.first == 0 && retval.second == 0)
    return next_label(string, extended); // ignore epsilon transitions
      return retval;
    }

  }
}
#else
#include <iostream>
int
main(int argc, char** argv)
{
  std::cout << "Unit tests for " __FILE__ ":";
  std::cout << std::endl << "constructors: ";
  std::cout << " default()...";
  SfstAlphabet defaultAlpha;
  std::cout << " (copy)...";
  SfstAlphabet copyAlpha(defaultAlpha);
  std::cout << std::endl << "destructor: ";
    delete new SfstAlphabet();
    delete new SfstAlphabet(defaultAlpha);
    std::cout << std::endl << "rest skipped...";
    std::cout << "ok" << std::endl;
    return EXIT_SUCCESS;
}
#endif
