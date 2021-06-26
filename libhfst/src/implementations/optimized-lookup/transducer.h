// -*- mode: c++; -*-
// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#ifndef _HFST_OL_TRANSDUCER_TRANSDUCER_H_
#define _HFST_OL_TRANSDUCER_TRANSDUCER_H_

#ifndef _MSC_VER
#  include <unistd.h>
#else
#  include <io.h>
#endif

#include <vector>
#include <set>
#include <iostream>
#include <limits>
#include <string>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <utility>
#include <deque>
#include <queue>
#include <stdexcept>
#include <time.h>

#include "../../HfstExceptionDefs.h"
#include "../../HfstFlagDiacritics.h"
#include "../../HfstSymbolDefs.h"
#include "../../HfstDataTypes.h"

#ifdef _MSC_VER
 #include <BaseTsd.h>
 typedef SSIZE_T ssize_t;
#endif


/** \brief A namespace for optimized-lookup functions and datatypes.*/
namespace hfst_ol {
using hfst::FdOperation;
using hfst::FdState;
using hfst::FdTable;
using hfst::HfstTwoLevelPath;
using hfst::HfstOneLevelPath;
using hfst::HfstTwoLevelPaths;
using hfst::HfstOneLevelPaths;
using hfst::StringVector;
using hfst::StringPairVector;

typedef unsigned short SymbolNumber;
typedef unsigned int TransitionTableIndex;
typedef unsigned int TransitionNumber;
typedef unsigned int StateIdNumber;
typedef short ValueNumber;
typedef float Weight;
typedef std::set<SymbolNumber> SymbolNumberSet;
typedef std::vector<SymbolNumber> SymbolNumberVector;
typedef std::set<TransitionTableIndex> TransitionTableIndexSet;
typedef std::vector<std::string> SymbolTable;

// for lookup
typedef std::pair<std::string, std::string> StringPair;

// for ospell
typedef std::vector<short> FlagDiacriticState;
typedef std::map<SymbolNumber, hfst::FdOperation> OperationMap;
typedef std::map<std::string, SymbolNumber> StringSymbolMap;
class STransition;

// for epsilon loop checking
struct TraversalState
{
    TransitionTableIndex index;
    FlagDiacriticState flags;
    TraversalState(TransitionTableIndex i, FlagDiacriticState f):
        index(i), flags(f) {}
    bool operator==(const TraversalState & rhs) const;
    bool operator<(const TraversalState & rhs) const;

};
typedef std::set<TraversalState> TraversalStates;

  // parentheses avoid collision with windows macro 'max'
  const SymbolNumber NO_SYMBOL_NUMBER = (std::numeric_limits<SymbolNumber>::max)();
const TransitionTableIndex NO_TABLE_INDEX =
  (std::numeric_limits<TransitionTableIndex>::max)();
  const unsigned long NO_COUNTER = (std::numeric_limits<unsigned long>::max)();
const Weight INFINITE_WEIGHT = static_cast<float>(NO_TABLE_INDEX);

enum HeaderFlag {Weighted, Deterministic, Input_deterministic, Minimized,
                 Cyclic, Has_epsilon_epsilon_transitions,
                 Has_input_epsilon_transitions, Has_input_epsilon_cycles,
                 Has_unweighted_input_epsilon_cycles};

// This is 2^31, hopefully equal to UINT_MAX/2 rounded up.
// For some profound reason it can't be replaced with (UINT_MAX+1)/2.
const TransitionTableIndex TRANSITION_TARGET_TABLE_START = 2147483648u;
const unsigned int MAX_IO_LEN = 10000;
const unsigned int MAX_RECURSION_DEPTH = 5000;

// This function is queried to check whether we should do the
// single-character ascii lookup tokenization or the regular
// trie-tokenization
bool should_ascii_tokenize(unsigned char c);

inline bool indexes_transition_table(const TransitionTableIndex i)
{
    return i >= TRANSITION_TARGET_TABLE_START;
}
inline bool indexes_transition_index_table(const TransitionTableIndex i)
{
    return i < TRANSITION_TARGET_TABLE_START;
}

class TransducerHeader
{
private:
    SymbolNumber number_of_input_symbols;
    SymbolNumber number_of_symbols;
    TransitionTableIndex size_of_transition_index_table;
    TransitionTableIndex size_of_transition_target_table;
  
    StateIdNumber number_of_states;
    TransitionNumber number_of_transitions;

    bool weighted;
    bool deterministic;
    bool input_deterministic;
    bool minimized;
    bool cyclic;
    bool has_epsilon_epsilon_transitions;
    bool has_input_epsilon_transitions;
    bool has_input_epsilon_cycles;
    bool has_unweighted_input_epsilon_cycles;

    static void header_error()
        {
            HFST_THROW(TransducerHasWrongTypeException);
        }

    template<class T>
    static T read_property(std::istream& is)
        {
            T p;
            is.read(reinterpret_cast<char*>(&p), sizeof(T));
            return p;
        }
    template<class T>
    static void write_property(T prop, std::ostream& os)
        { os.write(reinterpret_cast<const char*>(&prop), sizeof(prop)); }
    static bool read_bool_property(std::istream& is)
        {
            unsigned int prop;
            is.read(reinterpret_cast<char*>(&prop), sizeof(unsigned int));
            if(prop == 0)
                return false;
            if(prop == 1)
                return true;
            header_error();
            return false;
        }
    static void write_bool_property(bool value, std::ostream& os)
        {
            unsigned int prop = (value?1:0);
            os.write(reinterpret_cast<char*>(&prop), sizeof(prop));
        }
public:
    TransducerHeader(bool weights):
        number_of_input_symbols(0),
        number_of_symbols(1), // epsilon
        size_of_transition_index_table(1),
        size_of_transition_target_table(0),
        number_of_states(1),
        number_of_transitions(0),
        weighted(weights),
        deterministic(true),
        input_deterministic(true),
        minimized(true),
        cyclic(false),
        has_epsilon_epsilon_transitions(false),
        has_input_epsilon_transitions(false),
        has_input_epsilon_cycles(false),
        has_unweighted_input_epsilon_cycles(false)
        {}

    // a basic constructor that's only told information we
    // actually use at the moment
    TransducerHeader(
        SymbolNumber input_symbols,
        SymbolNumber symbols,
        TransitionTableIndex transition_index_table,
        TransitionTableIndex transition_table,
        bool weights):
        number_of_input_symbols(input_symbols),
        number_of_symbols(symbols), // epsilon
        size_of_transition_index_table(transition_index_table),
        size_of_transition_target_table(transition_table),
        number_of_states(0),
        number_of_transitions(0),
        weighted(weights),
        deterministic(true),
        input_deterministic(true),
        minimized(true),
        cyclic(false),
        has_epsilon_epsilon_transitions(false),
        has_input_epsilon_transitions(false),
        has_input_epsilon_cycles(false),
        has_unweighted_input_epsilon_cycles(false)
        { }


    TransducerHeader(std::istream& is):
        number_of_input_symbols(read_property<SymbolNumber>(is)),
        number_of_symbols(read_property<SymbolNumber>(is)),
        size_of_transition_index_table(
            read_property<TransitionTableIndex>(is)),
        size_of_transition_target_table(
            read_property<TransitionTableIndex>(is)),
        number_of_states(read_property<StateIdNumber>(is)),
        number_of_transitions(read_property<TransitionNumber>(is)),
        weighted(read_bool_property(is)),
        deterministic(read_bool_property(is)),
        input_deterministic(read_bool_property(is)),
        minimized(read_bool_property(is)),
        cyclic(read_bool_property(is)),
        has_epsilon_epsilon_transitions(read_bool_property(is)),
        has_input_epsilon_transitions(read_bool_property(is)),
        has_input_epsilon_cycles(read_bool_property(is)),
        has_unweighted_input_epsilon_cycles(read_bool_property(is))
        {
            if(!is) {
                HFST_THROW(TransducerHasWrongTypeException);
            }
        }
    
    SymbolNumber symbol_count(void) const { return number_of_symbols; }
    SymbolNumber input_symbol_count(void) const {
        return number_of_input_symbols;
    }
    void increment_symbol_count(void)
        {++number_of_symbols; ++number_of_input_symbols;}
  
    TransitionTableIndex index_table_size(void) const
        { return size_of_transition_index_table; }
    TransitionTableIndex target_table_size(void) const
        { return size_of_transition_target_table; }

    bool probe_flag(HeaderFlag flag) const
        {
            switch (flag) {
            case Weighted:
                return weighted;
            case Deterministic:
                return deterministic;
            case Input_deterministic:
                return input_deterministic;
            case Minimized:
                return minimized;
            case Cyclic:
                return cyclic;
            case Has_epsilon_epsilon_transitions:
                return has_epsilon_epsilon_transitions;
            case Has_input_epsilon_transitions:
                return has_input_epsilon_transitions;
            case Has_input_epsilon_cycles:
                return has_input_epsilon_cycles;
            case Has_unweighted_input_epsilon_cycles:
                return has_unweighted_input_epsilon_cycles;
            }
            return false;
        }
  
    void set_flag(HeaderFlag flag, bool value)
        {
          (void)value;
            switch (flag) {
            case Weighted:
                weighted = true;
                break;
            case Deterministic:
                deterministic = true;
                break;
            case Input_deterministic:
                input_deterministic = true;
                break;
            case Minimized:
                minimized = true;
                break;
            case Cyclic:
                cyclic = true;
                break;
            case Has_epsilon_epsilon_transitions:
                has_epsilon_epsilon_transitions = true;
                break;
            case Has_input_epsilon_transitions:
                has_input_epsilon_transitions = true;
                break;
            case Has_input_epsilon_cycles:
                has_input_epsilon_cycles = true;
                break;
            case Has_unweighted_input_epsilon_cycles:
                has_unweighted_input_epsilon_cycles = true;
            default:
                return;
            }
        }
  
    void display() const
        {
            std::cout << "Transducer properties:" << std::endl
                      << " number_of_symbols: "
                      << number_of_symbols << std::endl
                      << " number_of_input_symbols: "
                      << number_of_input_symbols << std::endl
                      << " size_of_transition_index_table: "
                      << size_of_transition_index_table << std::endl
                      << " size_of_transition_target_table: "
                      << size_of_transition_target_table << std::endl
                      << " number_of_states: "
                      << number_of_states << std::endl
                      << " number_of_transitions: "
                      << number_of_transitions << std::endl
                      << " weighted: "
                      << weighted << std::endl
                      << " deterministic: "
                      << deterministic << std::endl
                      << " input_deterministic: "
                      << input_deterministic << std::endl
                      << " minimized: "
                      << minimized << std::endl
                      << " cyclic: "
                      << cyclic << std::endl
                      << " has_epsilon_epsilon_transitions: "
                      << has_epsilon_epsilon_transitions << std::endl
                      << " has_input_epsilon_transitions: "
                      << has_input_epsilon_transitions << std::endl
                      << " has_input_epsilon_cycles: "
                      << has_input_epsilon_cycles << std::endl
                      << " has_unweighted_input_epsilon_cycles: "
                      << has_unweighted_input_epsilon_cycles << std::endl;
        }
  
    void write(std::ostream& os) const
        {
            write_property(number_of_input_symbols, os);
            write_property(number_of_symbols, os);
            write_property(size_of_transition_index_table, os);
            write_property(size_of_transition_target_table, os);
            write_property(number_of_states, os);
            write_property(number_of_transitions, os);
            write_bool_property(weighted, os);
            write_bool_property(deterministic, os);
            write_bool_property(input_deterministic, os);
            write_bool_property(minimized, os);
            write_bool_property(cyclic, os);
            write_bool_property(has_epsilon_epsilon_transitions, os);
            write_bool_property(has_input_epsilon_transitions, os);
            write_bool_property(has_input_epsilon_cycles, os);
            write_bool_property(has_unweighted_input_epsilon_cycles, os);
        }
  
    friend class ConvertTransducerHeader;
};

class TransducerAlphabet
{
protected:
    SymbolTable symbol_table;
    hfst::FdTable<SymbolNumber> fd_table;
    SymbolNumber unknown_symbol;
    SymbolNumber default_symbol;
    SymbolNumber identity_symbol;
    SymbolNumber orig_symbol_count;
  
public:
    TransducerAlphabet()
        {
            symbol_table.push_back("@_EPSILON_SYMBOL_@");
            unknown_symbol = NO_SYMBOL_NUMBER;
            default_symbol = NO_SYMBOL_NUMBER;
            identity_symbol = NO_SYMBOL_NUMBER;
            orig_symbol_count = 1;
        }
    TransducerAlphabet(std::istream& is,
                       SymbolNumber symbol_count,
                       bool preserve_diacritic_strings = true);
    TransducerAlphabet(const SymbolTable& st);
    virtual ~TransducerAlphabet() {}
  
    void display() const;
  
    void write(std::ostream& os) const
        {
            for(SymbolTable::const_iterator i = symbol_table.begin();
                i != symbol_table.end(); i++)
            {
                os << *i;
                os.put('\0');
            }
        }
  
    bool has_flag_diacritics() const
        { return fd_table.num_features() > 0; }
    bool is_flag_diacritic(SymbolNumber symbol) const
        { return fd_table.is_diacritic(symbol); }
    bool is_like_epsilon(SymbolNumber symbol) const;
    bool is_meta_arc(SymbolNumber symbol) const;
    
    const SymbolTable& get_symbol_table() const
        { return symbol_table; }
    
    const std::string string_from_symbol(const SymbolNumber symbol) const
    // represent epsilon as blank string
        { return (symbol == 0) ? "" : symbol_table[symbol]; }

    SymbolNumber symbol_from_string(const std::string symbol_string) const;
    StringSymbolMap build_string_symbol_map(void) const;
    const hfst::FdTable<SymbolNumber>& get_fd_table() const
        { return fd_table; }
    const hfst::FdOperation * get_operation(SymbolNumber symbol) const
        {
            return fd_table.get_operation(symbol);
        }
    SymbolNumber get_unknown_symbol(void) const
        { return unknown_symbol; }
    SymbolNumber get_default_symbol(void) const
        { return default_symbol; }
    SymbolNumber get_identity_symbol(void) const
        { return identity_symbol; }
    SymbolNumber get_orig_symbol_count(void) const
        { return orig_symbol_count; }
    virtual void add_symbol(char * symbol);
    virtual void add_symbol(const std::string & symbol);

    
};

class TransitionIndex
{
protected:
    SymbolNumber input_symbol;
    TransitionTableIndex first_transition_index;
public:
    static const size_t size =
        sizeof(SymbolNumber) + sizeof(TransitionTableIndex);
    TransitionIndex(): input_symbol(NO_SYMBOL_NUMBER),
                       first_transition_index(NO_TABLE_INDEX) {}
    TransitionIndex(SymbolNumber input,
                    TransitionTableIndex first_transition):
        input_symbol(input), first_transition_index(first_transition) {}
  
    TransitionIndex(std::istream& is):
        input_symbol(NO_SYMBOL_NUMBER), first_transition_index(0)
        {
            is.read(reinterpret_cast<char*>(&input_symbol),
                    sizeof(SymbolNumber));
            is.read(reinterpret_cast<char*>(&first_transition_index),
                    sizeof(TransitionTableIndex));
        }
    // A constructor for reading from a char array at p
    TransitionIndex(char * p):
        input_symbol(*((SymbolNumber*) p)),
        first_transition_index((*(TransitionTableIndex*)
                                (p + sizeof(SymbolNumber)))) {}
    virtual ~TransitionIndex() {}
  
    void write(std::ostream& os, bool weighted) const
        {
            os.write(reinterpret_cast<const char*>(&input_symbol),
                     sizeof(SymbolNumber));
            if(!weighted && input_symbol == NO_SYMBOL_NUMBER &&
               first_transition_index != NO_TABLE_INDEX) {
                // Make sure that we write the correct type of final index
                unsigned int unweighted_final_index = 1;
                os.write(reinterpret_cast<const char*>(&unweighted_final_index),
                         sizeof(first_transition_index));
            } else {
                os.write(reinterpret_cast<const char*>(
                             &first_transition_index),
                         sizeof(first_transition_index));
            }
        }
  
    void display() const;
  
    TransitionTableIndex get_target(void) const
        { return first_transition_index; }
    SymbolNumber get_input_symbol(void) const
        { return input_symbol; }
  
    bool matches(const SymbolNumber s) const;
    virtual bool final(void) const;
    virtual Weight final_weight(void) const { return 0.0; }
  
    static TransitionIndex create_final()
        { return TransitionIndex(NO_SYMBOL_NUMBER, 1); }
};

class TransitionWIndex : public TransitionIndex
{
public:
    TransitionWIndex(): TransitionIndex() {}
    TransitionWIndex(SymbolNumber input,
                     TransitionTableIndex first_transition):
        TransitionIndex(input, first_transition) {}
    TransitionWIndex(std::istream& is):
        TransitionIndex(is) {}
    TransitionWIndex(char * p):
        TransitionIndex(p) {}
    
    Weight final_weight(void) const;
  
    static TransitionWIndex create_final()
        { return TransitionWIndex(NO_SYMBOL_NUMBER, 0); }
    
    static TransitionWIndex create_final(Weight w)
        {
            union to_weight
            {
                TransitionTableIndex i;
                Weight w;
            } weight;
            weight.w = w;
            return TransitionWIndex(NO_SYMBOL_NUMBER, weight.i);
        }
};
    
class Transition
{
protected:
    SymbolNumber input_symbol;
    SymbolNumber output_symbol;
    TransitionTableIndex target_index;
public:
    static const size_t size = 2 * sizeof(SymbolNumber) +
        sizeof(TransitionTableIndex);
    Transition(SymbolNumber input, SymbolNumber output,
               TransitionTableIndex target, Weight bogus=0.0f):
        input_symbol(input), output_symbol(output), target_index(target)
        {(void)bogus; bogus=0.0f;}
    Transition(bool final, Weight bogus=0.0f):
        input_symbol(NO_SYMBOL_NUMBER), output_symbol(NO_SYMBOL_NUMBER),
        target_index(final?1:NO_TABLE_INDEX) {(void)bogus; bogus=0.0f;}
    Transition(std::istream& is):
        input_symbol(NO_SYMBOL_NUMBER), output_symbol(NO_SYMBOL_NUMBER),
        target_index(0)
        {
            is.read(reinterpret_cast<char*>(&input_symbol),
                    sizeof(SymbolNumber));
            is.read(reinterpret_cast<char*>(&output_symbol),
                    sizeof(SymbolNumber));
            is.read(reinterpret_cast<char*>(&target_index),
                    sizeof(target_index));
        }
    // A constructor for reading from char array
    Transition(char * p):
        input_symbol(*(SymbolNumber*) p),
        output_symbol(*(SymbolNumber*) (p + sizeof(SymbolNumber))),
        target_index(*(TransitionTableIndex*) (p + 2 * sizeof(SymbolNumber)))
        {}
  
    virtual ~Transition() {}
  
    virtual void write(std::ostream& os, bool weighted) const
        {
            os.write(reinterpret_cast<const char*>(&input_symbol),
                     sizeof(input_symbol));
            os.write(reinterpret_cast<const char*>(&output_symbol),
                     sizeof(output_symbol));
            os.write(reinterpret_cast<const char*>(&target_index),
                     sizeof(target_index));
            if (weighted) {
                os << 0.0f;
            }
        }

    virtual void display() const;

    TransitionTableIndex get_target(void) const {return target_index;}
    SymbolNumber get_output_symbol(void) const {return output_symbol;}
    SymbolNumber get_input_symbol(void) const {return input_symbol;}
  
    bool matches(const SymbolNumber s) const;
    virtual bool final(void) const;
    virtual Weight get_weight(void) const { return 0.0; }
};

class TransitionW : public Transition
{
protected:
    Weight transition_weight;
public:
    static const size_t size = 2 * sizeof(SymbolNumber) +
        sizeof(TransitionTableIndex) + sizeof(Weight);

    TransitionW(SymbolNumber input, SymbolNumber output,
                TransitionTableIndex target, Weight w):
        Transition(input, output, target), transition_weight(w) {}
    TransitionW(bool final, Weight w):
        Transition(final), transition_weight(w) {}
    TransitionW(std::istream& is): Transition(is), transition_weight(0.0f)
        {is.read(reinterpret_cast<char*>(&transition_weight), sizeof(Weight));}
    TransitionW(char * p):
        Transition(p),
        transition_weight(*((Weight*)
                            (p + 2 * sizeof(SymbolNumber)
                             + sizeof(TransitionTableIndex))))
        {}
  
    void write(std::ostream& os, bool weighted) const
        {
            Transition::write(os, false);
            if (weighted) {
                os.write(reinterpret_cast<const char*>(&transition_weight),
                         sizeof(transition_weight));
            }
        }
  
    void display() const;
  
    Weight get_weight(void) const { return transition_weight; }
};


template <class T>
class TransducerTable
{
protected:
    std::vector<T> table;
public:
    TransducerTable(): table() {}
    TransducerTable(size_t size, const T& entry): table(size, entry) {}
    TransducerTable(
        std::istream& is, TransitionTableIndex index_count): table()
        {
            char * p = (char*) malloc(T::size * index_count);
            is.read(p, T::size * index_count);
            char * p_orig = p;
            while(index_count) {
                table.push_back(T(p));
                --index_count;
                p += T::size;
            }
            free(p_orig);
        }
    TransducerTable(const TransducerTable& t): table(t.table) {}
  
    void append(const T& v) {table.push_back(v);}
    void set(size_t index, const T& v) {table[index] = v;}
  
    const T& operator[](TransitionTableIndex i) const
        {
            return (i < TRANSITION_TARGET_TABLE_START) ?
                table[i] : table[i-TRANSITION_TARGET_TABLE_START];
        }

    std::vector<T> get_vector(void) const { return std::vector<T>(table); } ;
  
    void display(bool transition_table) const
        {
            for(size_t i=0;i<table.size();i++)
            {
                std::cout << i;
                if(transition_table)
                    std::cout << "/" << i+TRANSITION_TARGET_TABLE_START;
                std::cout << ": ";
                table[i].display();
            }
        }
  
    unsigned int size() const {return hfst::size_t_to_uint(table.size());}
};

class TransducerTablesInterface
{
public:
    virtual ~TransducerTablesInterface() {}
  
    virtual const TransitionIndex& get_index(
        TransitionTableIndex i) const = 0;
    virtual const Transition& get_transition(
        TransitionTableIndex i) const = 0;
    virtual Weight get_weight(
        TransitionTableIndex i) const = 0;
    virtual SymbolNumber get_transition_input(
        TransitionTableIndex i) const = 0;
    virtual SymbolNumber get_transition_output(
        TransitionTableIndex i) const = 0;
    virtual TransitionTableIndex get_transition_target(
        TransitionTableIndex i) const = 0;
    virtual bool get_transition_finality(
        TransitionTableIndex i) const = 0;
    virtual SymbolNumber get_index_input(
        TransitionTableIndex i) const = 0;
    virtual TransitionTableIndex get_index_target(
        TransitionTableIndex i) const = 0;
    virtual bool get_index_finality(
        TransitionTableIndex i) const = 0;
    virtual Weight get_final_weight(
        TransitionTableIndex i) const = 0;
  
    virtual void display() const {}
};

template <class T1, class T2>
class TransducerTables : public TransducerTablesInterface
{
protected:
    TransducerTable<T1> index_table;
    TransducerTable<T2> transition_table;
public:
    TransducerTables(std::istream& is, TransitionTableIndex index_table_size,
                     TransitionTableIndex transition_table_size):
        index_table(
            is, index_table_size),
        transition_table(is, transition_table_size) { }
    
    TransducerTables(): index_table(1, T1::create_final()),
                        transition_table() {}
    TransducerTables(const TransducerTable<T1>& index_table,
                     const TransducerTable<T2>& transition_table):
        index_table(index_table), transition_table(transition_table) {}

    const TransitionIndex& get_index(TransitionTableIndex i) const
        {return index_table[i];}
    const Transition& get_transition(TransitionTableIndex i) const
        {return transition_table[i];}
    Weight get_weight(TransitionTableIndex i) const
        { return transition_table[i].get_weight(); }
    SymbolNumber get_transition_input(TransitionTableIndex i) const
        { return transition_table[i].get_input_symbol(); }
    SymbolNumber get_transition_output(TransitionTableIndex i) const
        { return transition_table[i].get_output_symbol(); }
    TransitionTableIndex get_transition_target(TransitionTableIndex i) const
        { return transition_table[i].get_target(); }
    bool get_transition_finality(TransitionTableIndex i) const
        { return transition_table[i].final(); }
    SymbolNumber get_index_input(TransitionTableIndex i) const
        { return index_table[i].get_input_symbol(); }
    TransitionTableIndex get_index_target(TransitionTableIndex i) const
        { return index_table[i].get_target(); }
    bool get_index_finality(TransitionTableIndex i) const
        { return index_table[i].final(); }
    Weight get_final_weight(TransitionTableIndex i) const
        { return index_table[i].final_weight(); }
  
    void display() const
        {
            std::cout << "Transition index table:" << std::endl;
            index_table.display(false);
            std::cout << "Transition table:" << std::endl;
            transition_table.display(true);
        }
};


// There follow some classes for implementing lookup
    
class OlLetterTrie;
typedef std::vector<OlLetterTrie*> OlLetterTrieVector;

class OlLetterTrie
{
private:
    OlLetterTrieVector letters;
    SymbolNumberVector symbols;
    
public:
    OlLetterTrie():
        letters(UCHAR_MAX+1, static_cast<OlLetterTrie*>(NULL)),
        symbols(UCHAR_MAX+1,NO_SYMBOL_NUMBER)
        {}

    ~OlLetterTrie() {
        for (size_t i=0 ; i<letters.size() ; ++i) {
            delete letters[i];
            letters[i] = 0;
        }
    }
    
    void add_string(const char * p,SymbolNumber symbol_key);
    bool has_key_starting_with(const char c) const;
    
    SymbolNumber find_key(char ** p);
    
};

class Encoder {
    
protected:
    SymbolNumber number_of_input_symbols;
    OlLetterTrie letters;
    SymbolNumberVector ascii_symbols;
    
    void read_input_symbols(const SymbolTable & kt);
    void read_input_symbol(const char * symbol, const int symbol_number);
    
public:
    Encoder(const SymbolTable & st, SymbolNumber input_symbol_count):
        number_of_input_symbols(input_symbol_count),
        ascii_symbols(128, NO_SYMBOL_NUMBER)
        {
            read_input_symbols(st);
        }

    SymbolNumber find_key(char ** p);

    friend class Transducer;
    friend class PmatchContainer;
};

struct SymbolPair
{
    SymbolNumber input;
    SymbolNumber output;
    SymbolPair(void): input(0), output(0) {}
    SymbolPair(SymbolNumber i, SymbolNumber o): input(i), output(o) {}
};

// A vector that can be written to at any position, so that it
// adds new elements if the desired element isn't already present.
struct DoubleTape: public std::vector<SymbolPair>
{
    void write(unsigned int pos, SymbolNumber in, SymbolNumber out)
        {
            while (pos >= this->size()) {
                this->push_back(SymbolPair());
            }
            this->operator[](pos) = SymbolPair(in, out);
        }

    void write(unsigned int pos, std::vector<SymbolNumber> & vec)
        {
            while (pos + vec.size() >= this->size()) {
                this->push_back(SymbolPair());
            }
            for (size_t i = 0; i < vec.size(); ++i) {
                this->operator[](pos + i) = SymbolPair(vec[i], vec[i]);
            }
        }

    void write(unsigned int pos, std::pair<SymbolNumberVector::iterator,
               SymbolNumberVector::iterator> start_and_end)
        {
            size_t size = start_and_end.second - start_and_end.first;
            while (pos + size >= this->size()) {
                this->push_back(SymbolPair());
            }
            for (size_t i = 0; i < size; ++i) {
                this->operator[](pos + i) =
                    SymbolPair(*(start_and_end.first + i),
                               *(start_and_end.first + i));
            }
        }

    DoubleTape extract_slice(unsigned int start, unsigned int stop)
        {
            DoubleTape retval;
            while(start < stop) {
                retval.push_back(this->at(start));
                ++start;
            }
            return retval;
        }
};

struct WeightedDoubleTape: public DoubleTape
{
    Weight weight;
    WeightedDoubleTape(DoubleTape dt, Weight w): DoubleTape(dt), weight(w) {}
};

class Tape: public SymbolNumberVector
{
public:
    void write(unsigned int i, SymbolNumber s)
        {
            if (this->size() > i) {
                this->operator[](i) = s;
            } else {
                while (this->size() <= i) {
                    this->push_back(NO_SYMBOL_NUMBER);
                }
                this->operator[](i) = s;
            }
        }
};

/** \brief A compiled transducer format, suitable for fast lookup operations.
 */
class Transducer
{
protected:
    TransducerHeader* header;
    TransducerAlphabet* alphabet;
    TransducerTablesInterface* tables;
    void load_tables(std::istream& is);

    // for lookup
    Weight current_weight;
    HfstTwoLevelPaths * lookup_paths;
    Encoder * encoder;
    Tape input_tape;
    DoubleTape output_tape;
    hfst::FdState<SymbolNumber> flag_state;
    // This is to keep track of whether we're going to take a default transition
    bool found_transition;
    // For keeping a tally of previously epsilon-visited states to control
    // going into loops
    TraversalStates traversal_states;

    ssize_t max_lookups;
    unsigned int recursion_depth_left;
    double max_time;
    clock_t start_clock;

    void try_epsilon_transitions(unsigned int input_tape_pos,
                                 unsigned int output_tape_pos,
                                 TransitionTableIndex i);
  
    void try_epsilon_indices(unsigned int input_tape_pos,
                             unsigned int output_tape_pos,
                             TransitionTableIndex i);

    void find_transitions(SymbolNumber input,
                          unsigned int input_tape_pos,
                          unsigned int output_tape_pos,
                          TransitionTableIndex i);

    void find_index(SymbolNumber input,
                    unsigned int input_tape_pos,
                    unsigned int output_tape_pos,
                    TransitionTableIndex i);
    
    void get_analyses(unsigned int input_tape_pos,
                      unsigned int output_tape_pos,
                      TransitionTableIndex i);
    
    void find_loop_epsilon_transitions(unsigned int input_pos,
                                       TransitionTableIndex i);
    void find_loop_epsilon_indices(unsigned int input_pos,
                                   TransitionTableIndex i);
    void find_loop_transitions(SymbolNumber input,
                               unsigned int input_pos,
                               TransitionTableIndex i);
    void find_loop_index(SymbolNumber input,
                         unsigned int input_pos,
                         TransitionTableIndex i);
    void find_loop(unsigned int input_pos,
                   TransitionTableIndex i);


public:
    Transducer(std::istream& is);
    Transducer(bool weighted);
    Transducer(Transducer * t);
    Transducer();
    Transducer(const TransducerHeader& header,
               const TransducerAlphabet& alphabet,
               const TransducerTable<TransitionIndex>& index_table,
               const TransducerTable<Transition>& transition_table);
    Transducer(const TransducerHeader& header,
               const TransducerAlphabet& alphabet,
               const TransducerTable<TransitionWIndex>& index_table,
               const TransducerTable<TransitionW>& transition_table);
    virtual ~Transducer();

    void write(std::ostream& os) const;
    Transducer * copy(Transducer * t, bool weighted = false);
    void display() const;

    const TransducerHeader& get_header() const
        { return *header; }
    const TransducerAlphabet& get_alphabet() const
        { return *alphabet; }
    const Encoder& get_encoder(void) const
        { return *encoder; }
    const hfst::FdTable<SymbolNumber>& get_fd_table() const
        { return alphabet->get_fd_table(); }
    const SymbolTable& get_symbol_table() const
        { return alphabet->get_symbol_table(); }

    const TransitionIndex& get_index(TransitionTableIndex i) const
        { return tables->get_index(i); }
    const Transition& get_transition(TransitionTableIndex i) const
        { return tables->get_transition(i); }
    
    bool final_index(TransitionTableIndex i) const
        {
            if (indexes_transition_table(i)) {
                return tables->get_transition_finality(i);
            } else {
                return tables->get_index_finality(i);
            }
        }
    bool is_infinitely_ambiguous(void) const
        {
            return header->probe_flag(Has_input_epsilon_cycles);
        }

    bool is_lookup_infinitely_ambiguous(const StringVector & s);
    bool is_lookup_infinitely_ambiguous(const std::string & input);
    
    TransducerTable<TransitionWIndex> copy_windex_table();
    TransducerTable<TransitionW> copy_transitionw_table();
    TransducerTable<TransitionIndex> copy_index_table();
    TransducerTable<Transition> copy_transition_table();
    
    // state_index must be an index to a state which is defined as either:
    // (1) the start of a set of entries in the transition index table, or
    // (2) the boundary before a set of entries in the transition table, in
    //     which case the following entries will all have the same input symbol
    // This function will return a vector of indices to the transition table,
    // i.e. the arcs from the given state
    TransitionTableIndexSet get_transitions_from_state(
        TransitionTableIndex state_index) const;


    bool initialize_input(const char * input_str);
    void include_symbol_in_alphabet(const std::string & sym);
    HfstOneLevelPaths * lookup_fd(const StringVector & s, ssize_t limit = -1,
        double time_cutoff = 0.0);
    /* Tokenize and lookup, accounting for flag diacritics, the surface string
       \a s. The return value, a pointer to HfstOneLevelPaths
       (which is a set) of analyses, is newly allocated.
    */
    HfstOneLevelPaths * lookup_fd(const std::string & s, ssize_t limit = -1,
                                  double time_cutoff = 0.0);
    HfstOneLevelPaths * lookup_fd(const char * s, ssize_t limit = -1,
                                  double time_cutoff = 0.0);
    HfstTwoLevelPaths * lookup_fd_pairs(const std::string & s, ssize_t limit = -1,
                                        double time_cutoff = 0.0);
    HfstTwoLevelPaths * lookup_fd_pairs(const char * s, ssize_t limit = -1,
                                        double time_cutoff = 0.0);
    void note_analysis(void);

    // Methods for supporting ospell
    SymbolNumber get_unknown_symbol(void) const
        { return alphabet->get_unknown_symbol(); }
    StringSymbolMap get_string_symbol_map(void) const
        { return alphabet->build_string_symbol_map(); }
    STransition take_epsilons(const TransitionTableIndex i) const;
    STransition take_epsilons_and_flags(const TransitionTableIndex i);
    STransition take_non_epsilons(const TransitionTableIndex i,
                                  const SymbolNumber symbol) const;
    TransitionTableIndex next(const TransitionTableIndex i,
                              const SymbolNumber symbol) const;
    TransitionTableIndex next_e(const TransitionTableIndex i) const;
    bool has_transitions(const TransitionTableIndex i,
                         const SymbolNumber symbol) const;
    bool has_epsilons_or_flags(const TransitionTableIndex i);
    Weight final_weight(const TransitionTableIndex i) const;
    bool is_flag(const SymbolNumber symbol)
        { return alphabet->is_flag_diacritic(symbol); }
    bool is_weighted(void)
        { return header->probe_flag(Weighted);}

    
    friend class ConvertTransducer;
};

class STransition{
public:
    TransitionTableIndex index;
    SymbolNumber symbol;
    Weight weight;

    STransition(TransitionTableIndex i,
                SymbolNumber s):
        index(i),
        symbol(s),
        weight(0.0)
        {}

    STransition(TransitionTableIndex i,
                SymbolNumber s,
                Weight w):
        index(i),
        symbol(s),
        weight(w)
        {}

};

typedef std::pair<std::string, Weight> StringWeightPair;

class StringWeightComparison
/* results are reversed by default because greater weights represent
   worse results - to reverse the reversal, give a true argument*/

{
    bool reverse;
public:
    StringWeightComparison(bool reverse_result=false):
        reverse(reverse_result)
        {}
    
    bool operator() (StringWeightPair lhs, StringWeightPair rhs)
        { // return true when we want rhs to appear before lhs
            if (reverse) {
                return (lhs.second < rhs.second);
            } else {
                return (lhs.second > rhs.second);
            }
        }
};

typedef std::priority_queue<StringWeightPair,
                            std::vector<StringWeightPair>,
                            StringWeightComparison> CorrectionQueue;
typedef std::priority_queue<StringWeightPair,
                            std::vector<StringWeightPair>,
                            StringWeightComparison> AnalysisQueue;
typedef std::priority_queue<StringWeightPair,
                            std::vector<StringWeightPair>,
                            StringWeightComparison> HyphenationQueue;

/*class STransducer: public Transducer
  {
  protected:
  bool final_transition(TransitionTableIndex i)
  {
  return transitions[i]->final();
  }
    
  bool final_index(TransitionTableIndex i)
  {
  return indices[i]->final();
  }

  
  public:
  Transducer(FILE * f):
  header(TransducerHeader(f)),
  alphabet(TransducerAlphabet(f, header.symbol_count())),
  keys(alphabet.get_key_table()),
  index_reader(f,header.index_table_size()),
  transition_reader(f,header.target_table_size()),
  encoder(keys,header.input_symbol_count()),
  indices(index_reader()),
  transitions(transition_reader())
  {}

  TransitionIndexVector &indices;
  
  TransitionVector &transitions;
    
  SymbolNumber find_next_key(char ** p)
  {
  return encoder.find_key(p);
  }


  unsigned int get_state_size(void)
  {
  return alphabet.get_state_size();
  }

  std::vector<const char*> * get_symbol_table(void)
  {
  return &symbol_table;
  }

  SymbolNumber get_other(void)
  {
  return alphabet.get_other();
  }

  TransducerAlphabet * get_alphabet(void)
  {
  return &alphabet;
  }

  OperationMap * get_operations(void)
  {
  return alphabet.get_operation_map();
  }

  STransition take_epsilons(const TransitionTableIndex i) const;
  STransition take_epsilons_and_flags(const TransitionTableIndex i);
  STransition take_non_epsilons(const TransitionTableIndex i,
  const SymbolNumber symbol) const;
  TransitionTableIndex next(const TransitionTableIndex i,
  const SymbolNumber symbol) const;
  TransitionTableIndex next_e(const TransitionTableIndex i) const;
  bool has_transitions(const TransitionTableIndex i,
  const SymbolNumber symbol) const;
  bool has_epsilons_or_flags(const TransitionTableIndex i);
  bool is_final(const TransitionTableIndex i);
  Weight final_weight(const TransitionTableIndex i) const;
  bool is_flag(const SymbolNumber symbol)
  { return alphabet.is_flag(symbol); }
  bool is_weighted(void)
  { return header.probe_flag(Weighted);}

  };*/

class TreeNode
{
public:
    SymbolNumberVector string;
    unsigned int input_state;
    TransitionTableIndex mutator_state;
    TransitionTableIndex lexicon_state;
    hfst::FdState<SymbolNumber> flag_state;
    Weight weight;

    TreeNode(SymbolNumberVector prev_string,
             unsigned int i,
             TransitionTableIndex mutator,
             TransitionTableIndex lexicon,
             hfst::FdState<SymbolNumber> state,
             Weight w):
        string(prev_string),
        input_state(i),
        mutator_state(mutator),
        lexicon_state(lexicon),
        flag_state(state),
        weight(w)
        { }

    TreeNode(hfst::FdState<SymbolNumber> start_state): // starting state node
        string(SymbolNumberVector()),
        input_state(0),
        mutator_state(0),
        lexicon_state(0),
        flag_state(start_state),
        weight(0.0)
        { }

    TreeNode update_lexicon(SymbolNumber next_symbol,
                            TransitionTableIndex next_lexicon,
                            Weight weight);

    TreeNode update_mutator(SymbolNumber next_symbol,
                            TransitionTableIndex next_mutator,
                            Weight weight);

    void increment_mutator(void);

    TreeNode update(SymbolNumber next_symbol,
                    unsigned int next_input,
                    TransitionTableIndex next_mutator,
                    TransitionTableIndex next_lexicon,
                    Weight weight);

    TreeNode update(SymbolNumber next_symbol,
                    TransitionTableIndex next_mutator,
                    TransitionTableIndex next_lexicon,
                    Weight weight);


};

typedef std::deque<TreeNode> TreeNodeQueue;

int nByte_utf8(unsigned char c);

class InputString
{
  
private:
    SymbolNumberVector s;

public:
    InputString():
        s(SymbolNumberVector())
        { }

    bool initialize(const Encoder & encoder, char * input, SymbolNumber other);
    
    unsigned int len(void)
        {
            return (unsigned int)s.size();
        }

    SymbolNumber operator[](unsigned int i)
        {
            return s[i];
        }

};

class AlphabetTranslationException: public std::runtime_error
{ // "what" should hold the first untranslatable symbol
public:
    
    AlphabetTranslationException(const std::string what):
        std::runtime_error(what)
        { }
};

/** \brief A spellchecker, constructed from two optimized-lookup transducer
    instances. An alphabet translator is built at construction time.
*/
class Speller
{
public:
    Transducer * mutator;
    Transducer * lexicon;
    InputString input;
    TreeNodeQueue queue;
    SymbolNumberVector alphabet_translator;
//    hfst::FdTable<SymbolNumber> operations;
    std::vector<std::string> symbol_table;
    
    Speller(Transducer * mutator_ptr, Transducer * lexicon_ptr):
        mutator(mutator_ptr),
        lexicon(lexicon_ptr),
        input(InputString()),
        queue(TreeNodeQueue()),
        alphabet_translator(SymbolNumberVector()),
//  operations(lexicon->get_fd_table()),
        symbol_table(lexicon->get_symbol_table())
        {
            build_alphabet_translator();
        }
    
    bool init_input(char * str, const Encoder & encoder, SymbolNumber other);

    void build_alphabet_translator(void);
    void lexicon_epsilons(void);
    void mutator_epsilons(void);
    void consume_input(void);
    void lexicon_consume(void);
    /** See if \a line is in the lexicon.
     */
    bool check(char * line);
    /** Return a priority queue of corrections of \a line.
     */
    CorrectionQueue correct(char * line);
    std::string stringify(SymbolNumberVector symbol_vector);
};

}

#endif
