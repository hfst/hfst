// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

 #ifndef _HFST_BASIC_TRANSDUCER_H_
 #define _HFST_BASIC_TRANSDUCER_H_

 /** @file HfstBasicTransducer.h
     @brief Class HfstBasicTransducer */

 #include <cstdio>
 #include <string>
 #include <set>
 #include <cassert>
 #include <iostream>
 #include <algorithm>
 #include <stack>

 #include "../HfstSymbolDefs.h"
 #include "../HfstExceptionDefs.h"
 #include "../HfstDataTypes.h"
 #include "../HarmonizeUnknownAndIdentitySymbols.h"
 #include "../HfstFlagDiacritics.h"
 #include "../HfstLookupFlagDiacritics.h"
 #include "../HfstEpsilonHandler.h"
 #include "ConvertTransducerFormat.h"
 #include "HfstBasicTransition.h"
 #include "HfstTropicalTransducerTransitionData.h"
 #include "../string-utils.h"

 #include "../hfstdll.h"

 namespace hfst {

   /** @brief A namespace for all code that forms a bridge between
       backend libraries and HFST.
       
       Most code in this namespace is not intended to be accessed
       by the user of the HFST interface. An exception is HFST's
       own transducer class hfst::HfstTransitionGraph and classes that
       are needed to use it.
   */
   namespace implementations {
     
     /** @brief The number of a state in an HfstTransitionGraph. */
     typedef unsigned int HfstState;
     
     typedef std::pair<HfstState, std::vector<std::pair<std::string, std::string> > > HfstReplacement;
     typedef std::vector<HfstReplacement> HfstReplacements;
     typedef std::map<HfstState, HfstReplacements > HfstReplacementsMap;
     
     typedef std::vector<std::vector<hfst::implementations::HfstBasicTransition> > HfstBasicStates;
     
     /** @brief A simple transition graph format that consists of
         states and transitions between those states.
         
         Probably the easiest way to use this template is to choose
         the implementations #HfstBasicTransducer
         (HfstTransitionGraph<HfstTropicalTransducerTransitionData>)
         and #HfstBasicTransition
         (HfstTransition<HfstTropicalTransducerTransitionData>).
         The class HfstTropicalTransducerTransitionData contains an input string,
         an output string and a float weight. HfstBasicTransducer is the
         implementation that is used as an example in this documentation.
         
         An example of creating a HfstBasicTransducer [foo:bar baz:baz]
         with weight 0.4 from scratch:
         
 \verbatim
   // Create an empty transducer
   // The transducer has initially one start state (number zero)
   // that is not final
   HfstBasicTransducer fsm;
   // Add two states to the transducer
   fsm.add_state(1);
   fsm.add_state(2);
   // Create a transition [foo:bar] leading to state 1 with weight 0.1 ...
   HfstBasicTransition tr(1, "foo", "bar", 0.1);
   // ... and add it to state zero
   fsm.add_transition(0, tr);
   // Add a transition [baz:baz] with weight 0 from state 1 to state 2
   fsm.add_transition(1, HfstBasicTransition(2, "baz", "baz", 0.0));
   // Set state 2 as final with weight 0.3
   fsm.set_final_weight(2, 0.3);
 \endverbatim

        An example of iterating through a HfstBasicTransducer's states
        and transitions when printing it in AT&T format to stderr:

 \verbatim
   // The first state is always number zero.
   unsigned int source_state=0;

   // Go through all states
     for (HfstBasicTransducer::const_iterator it = fsm.begin();
      it != fsm.end(); it++ )
       {
         // Go through all transitions
     for (HfstBasicTransducer::HfstTransitions::const_iterator tr_it
            = it->begin(); tr_it != it->end(); tr_it++)
       {
         std::cerr << source_state << "\t"
               << tr_it->get_target_state() << "\t"
               << tr_it->get_input_symbol() << "\t"
               << tr_it->get_output_symbol() << "\t"
               << tr_it->get_weight() << std::endl;
       }

     if (fsm.is_final_state(source_state))
       {
         std::cerr << source_state << "\t"
               << fsm.get_final_weight(source_state) << std::endl;
       }

     // the next state is numbered source_state + 1
     source_state++;
       }
 \endverbatim

 @see #HfstBasicTransducer HfstBasicTransition */
     class HfstBasicTransducer
     {
       
       // --- Datatypes and variables ---
       
     public:
       /** @brief Datatype for a symbol in a transition. */
       typedef HfstTropicalTransducerTransitionData::SymbolType HfstSymbol;
       /** @brief Datatype for a symbol pair in a transition. */
       typedef std::pair<HfstSymbol, HfstSymbol>
         HfstSymbolPair;
       /** @brief A set of symbol pairs. */
       typedef std::set<HfstSymbolPair> HfstSymbolPairSet;
       /** @brief A set of symbol pairs. */
       typedef std::set<HfstSymbol> HfstSymbolSet;
       /** @brief A vector of symbol pairs. */
       typedef std::vector<HfstSymbolPair> HfstSymbolPairVector;
       /** @brief Datatype for the alphabet of a graph. */
       typedef std::set<HfstSymbol> HfstAlphabet;
       
       /** @brief Datatype for the states of a transition in a graph. */
       typedef std::vector<HfstBasicTransition> HfstTransitions;
       
       /* Datatype for the states of a graph and their transitions.
          Each index of the vector is a state and the transitions
          on that index are the transitions of that state. */
       typedef std::vector<HfstTransitions> HfstStates;
       
       /* States of the graph and their transitions. */
       HfstStates state_vector;
       
     protected:
       /* The initial state number. */
       static const HfstState INITIAL_STATE = 0;
       
       /* Datatype for the final states and their weights in a graph. */
       typedef std::map<HfstState,HfstTropicalTransducerTransitionData::WeightType> FinalWeightMap;
       /* The final states and their weights in the graph. */
       FinalWeightMap final_weight_map;
       
       /* The alphabet of the graph. */
       HfstAlphabet alphabet;
       
       /* Used by substitute function. */
       typedef unsigned int HfstNumber;
       typedef std::vector<HfstNumber> HfstNumberVector;
       typedef std::pair<HfstNumber, HfstNumber> HfstNumberPair;
       typedef std::map<HfstNumberPair, HfstNumberPair> HfstNumberPairSubstitutions;
       
     protected:
       /* @brief An iterator type that points to a state in a graph.
          
          The value pointed by the iterator is of type HfstTransitions. */
       typedef HfstStates::iterator iterator;
       
     public:
       /** @brief A const iterator type that points a state in a graph.
           
           The value pointed by the iterator is of type HfstTransitions. */
       typedef HfstStates::const_iterator const_iterator;

       /** @brief The name of the graph. */
       std::string name;
       
       /** @brief The states of the graph. */
       std::vector<HfstState> states() const;
       
       /** @brief The states of the graph and their transitions. */
       HfstBasicStates states_and_transitions() const;
       
       // --------------------------------------------------------
       // --- Construction, assignment, copying and conversion ---
       // --------------------------------------------------------
       
       /** @brief Create a graph with one initial state that has state number
           zero and is not a final state, i.e. create an empty graph. */
       HFSTDLL HfstBasicTransducer(void);
       
       HFSTDLL HfstBasicTransducer(FILE *file);
       
       
       /** @brief The assignment operator. */
       HFSTDLL HfstBasicTransducer &operator=(const HfstBasicTransducer &graph);
       
       HFSTDLL HfstBasicTransducer &assign(const HfstBasicTransducer &graph);
       
       /** @brief Create a deep copy of HfstBasicTransducer \a graph. */
       HFSTDLL HfstBasicTransducer(const HfstBasicTransducer &graph);
       
       /** @brief Create an HfstBasicTransducer equivalent to HfstTransducer
           \a transducer. FIXME: move to a separate file */
       HFSTDLL HfstBasicTransducer(const hfst::HfstTransducer &transducer);
       
       // --------------------------------------------------
       // --- Initialization, optimization and debugging ---
       // --------------------------------------------------
       
     protected:
       /* Add epsilon, unknown and identity symbols to the alphabet
          \a alpha. */
       void initialize_alphabet(HfstAlphabet &alpha);
       
       /* Check that all symbols that occur in the transitions of the graph
          are also in the alphabet. */
       bool check_alphabet();
       
     public:
       /* Print the alphabet of the graph to standard error stream. */
       HFSTDLL void print_alphabet() const;
       
     protected:
       /* Get the number of the \a symbol. */
       unsigned int get_symbol_number
         (const HfstSymbol &symbol) const;
       
       /* For internal optimization: Reserve space for
          \a number_of_states states. */
       void initialize_state_vector
         (unsigned int number_of_states);
       
       /* For internal optimization: Reserve space for
          \a number_of_transitions transitions for state number
          \a state_number. */
       void initialize_transition_vector
         (unsigned int state_number, unsigned int number_of_transitions);
       
       // -----------------------------------
       // ---------- The alphabet -----------
       // -----------------------------------
       
     public:
       /** @brief Explicitly add \a symbol to the alphabet of the graph.
           
           @note Usually the user does not have to take care of the alphabet
           of a graph. This function can be useful in some special cases. */
       HFSTDLL void add_symbol_to_alphabet(const HfstSymbol &symbol);
       
       /** @brief Remove symbol \a symbol from the alphabet of the graph.
           
           @note Use with care, removing symbols that occur in the transitions
           of the graph can have unexpected results. */
       HFSTDLL void remove_symbol_from_alphabet(const HfstSymbol &symbol);
       
       HFSTDLL void remove_symbols_from_alphabet(const HfstSymbolSet &symbols);
       
       /** @brief Same as #add_symbol_to_alphabet for each symbol in
           \a symbols. */
       HFSTDLL void add_symbols_to_alphabet(const HfstSymbolSet &symbols);
       
       HFSTDLL void add_symbols_to_alphabet(const HfstSymbolPairSet &symbols);
       
       /* Remove all symbols that are given in \a symbols but do not occur
          in transitions of the graph from its alphabet. */
       HFSTDLL void prune_alphabet_after_substitution(const std::set<unsigned int> &symbols);
       
       HFSTDLL HfstAlphabet symbols_used();
       
       /** @brief Remove all symbols that do not occur in transitions of
           the graph from its alphabet.
           
           @param force Whether unused symbols are removed even if
           unknown or identity symbols occur in transitions.
           
           Epsilon, unknown and identity \link hfst::String symbols\endlink
           are always included in the alphabet. */
       HFSTDLL void prune_alphabet(bool force=true);
       
       /** @brief Get the set of HfstSymbols in the alphabet
           of the graph.
           
           The HfstSymbols do not necessarily occur in any transitions
           of the graph. Epsilon, unknown and identity \link
           hfst::String symbols\endlink are always included in the alphabet. */
       HFSTDLL const HfstAlphabet &get_alphabet() const;
       
       HFSTDLL StringPairSet get_transition_pairs() const;
       
       // ----------------------------------------------------------------
       // --- Adding states and transitions and iterating through them ---
       // ----------------------------------------------------------------
       
       /** @brief Add a new state to this graph and return its number.
           
           @return The next (smallest) free state number. */
       HFSTDLL HfstState add_state(void);
       
         /** @brief Add a state \a s to this graph.
             
             If the state already exists, it is not added again.
             All states with state number smaller than \a s are also
             added to the graph if they did not exist before.
             @return \a s*/
       HFSTDLL HfstState add_state(HfstState s);

       /** @brief Get the biggest state number in use. */
       HFSTDLL HfstState get_max_state() const;
       
       /** @brief Add a transition \a transition to state \a s.
           
           If state \a s does not exist, it is created. */
     HFSTDLL void add_transition(HfstState s, const HfstBasicTransition & transition,
                                 bool add_symbols_to_alphabet=true);
     
     /** @brief Remove transition \a transition from state \a s.
         \a remove_symbols_from_alphabet defines whether
         symbols in \a transition are removed from the alphabet
         if they are no longer used in the graph.
         
         If \a state or \a transition does not exist, nothing is done. */
     HFSTDLL void remove_transition(HfstState s, const HfstBasicTransition & transition,
                                    bool remove_symbols_from_alphabet=false);
     
     /** @brief Whether state \a s is final.
         FIXME: return positive infinity instead if not final. */
     HFSTDLL bool is_final_state(HfstState s) const;

     /** Get the final weight of state \a s in this graph. */
     HFSTDLL HfstTropicalTransducerTransitionData::WeightType get_final_weight(HfstState s) const;
     
     /** @brief Set the final weight of state \a s in this graph
         to \a weight.
         
         If the state does not exist, it is created. */
     HFSTDLL void set_final_weight(HfstState s,
                                   const HfstTropicalTransducerTransitionData::WeightType & weight);
     
     /** @brief Sort the arcs of this transducer according to input and
         output symbols. */
     HFSTDLL HfstBasicTransducer &sort_arcs(void);
     
     /** @brief Get an iterator to the beginning of the states in
         the graph.
         
         For an example, see #HfstTransitionGraph */
     HFSTDLL iterator begin();
     
     /** @brief Get a const iterator to the beginning of
         states in the graph. */
     HFSTDLL const_iterator begin() const;
     
     /** @brief Get an iterator to the end of states (last state + 1)
         in the graph. */
     HFSTDLL iterator end();
     
     /** @brief Get a const iterator to the end of states (last state + 1)
         in the graph. */
     HFSTDLL const_iterator end() const;
     
     
     /** @brief Get the set of transitions of state \a s in this graph.
         
             If the state does not exist, a @a StateIndexOutOfBoundsException
             is thrown.
     */
     HFSTDLL const HfstTransitions & operator[](HfstState s) const;
     
     /** @brief Alternative name for operator[].
         
         Python interface uses this function as '[]' is not a legal name.
         
         @see operator[]
     */
     HFSTDLL const HfstTransitions & transitions(HfstState s) const;
     
     /** @brief Get mutable transitions.
      */
     HFSTDLL HfstTransitions & transitions(HfstState s);
     
     // --------------------------------------------------
     // -----   Reading and writing in AT&T format   -----
     // --------------------------------------------------
     
     protected:
     /* Change state numbers s1 to s2 and vice versa. */
     void swap_state_numbers(HfstState s1, HfstState s2);

     static void write_weight(FILE * file, float weight);
     
     static void write_weight(std::ostream & os, float weight);
     
     static void xfstize(std::string & symbol);
     
     static void xfstize_symbol(std::string & symbol);
     
     void print_xfst_state(std::ostream & os, HfstState state);
     
     void print_xfst_state(FILE * file, HfstState state);
     
     void print_xfst_arc(std::ostream & os, HfstTropicalTransducerTransitionData data);
     
     void print_xfst_arc(FILE * file, HfstTropicalTransducerTransitionData data);
     
     public:
     
     /** @brief Write the graph in xfst text format to ostream \a os.
         \a write_weights defines whether weights are printed (todo). */
     HFSTDLL void write_in_xfst_format(std::ostream &os, bool write_weights=true);
     
     // note: unknown and identity are both '?'
     HFSTDLL static std::string prologize_symbol(const std::string & symbol);
     
     // caveat: '?' is always unknown
     HFSTDLL static std::string deprologize_symbol(const std::string & symbol);
     
     HFSTDLL static void print_prolog_arc_symbols(FILE * file, HfstTropicalTransducerTransitionData data);
     
     HFSTDLL static void print_prolog_arc_symbols(std::ostream & os, HfstTropicalTransducerTransitionData data);
     
     /** @brief Write the graph in prolog format to FILE \a file.
         \a write_weights defines whether weights are printed (todo). */
     HFSTDLL void write_in_prolog_format(FILE * file, const std::string & name,
                                         bool write_weights=true);
     
     /** @brief Write the graph in prolog format to ostream \a os.
         \a write_weights defines whether weights are printed (todo). */
     HFSTDLL void write_in_prolog_format(std::ostream & os, const std::string & name,
                                         bool write_weights=true);
     
     // If \a str is of format ".+", change it to .+ and return true.
     // Else, return false.
     HFSTDLL static bool strip_quotes_from_both_sides(std::string & str);
     
     // If \a str is of format .+)\.", change it to .+ and return true.
     // Else, return false.
     HFSTDLL static bool strip_ending_parenthesis_and_comma(std::string & str);
     
     HFSTDLL static bool parse_prolog_network_line(const std::string & line, HfstBasicTransducer & graph);

     // Get positions of \a c in \a str. If \a esc is precedes
     // \a c, \a c is not included.
     HFSTDLL static std::vector<unsigned int> get_positions_of_unescaped_char
       (const std::string & str, char c, char esc);
     
     // Extract input and output symbols, if possible, from prolog arc
     // \a str and store them to \a isymbol and \a osymbol.
     // Return whether symbols were successfully extracted.
     // \a str must be of format "foo":"bar" or "foo"
     HFSTDLL static bool get_prolog_arc_symbols
       (const std::string & str, std::string & isymbol, std::string & osymbol);
     
     HFSTDLL static bool extract_weight(std::string & symbol, float & weight);

     HFSTDLL static bool parse_prolog_arc_line(const std::string & line, HfstBasicTransducer & graph);
     
     HFSTDLL static bool parse_prolog_final_line(const std::string & line, HfstBasicTransducer & graph);
     
     HFSTDLL static bool parse_prolog_symbol_line(const std::string & line, HfstBasicTransducer & graph);
     
     // Erase newlines from the end of \a str and return \a str.
     HFSTDLL static std::string strip_newlines(std::string & str);
     
     // Try to get a line from \a is (if \a file == NULL)
     // or from \a file. If successfull, strip the line from newlines,
     // increment \a linecount by one and return the line.
     // Else, throw an EndOfStreamException.
     HFSTDLL static std::string get_stripped_line
       (std::istream & is, FILE * file, unsigned int & linecount);
     
     /* Create an HfstTransitionGraph as defined in prolog format
        in istream \a is or FILE \a file.
        
        The functions is called by functions
        read_in_prolog_format(istream&) and
        read_in_prolog_format(FILE*).
        If \a file is NULL, it is ignored and \a is is used.
        If \a file is not NULL, it is used and \a is is ignored. */
     HFSTDLL static HfstBasicTransducer read_in_prolog_format
       (std::istream &is, FILE *file, unsigned int & linecount);
     
     HFSTDLL static HfstBasicTransducer read_in_prolog_format
       (std::istream &is,
        unsigned int & linecount);
     
     HFSTDLL static HfstBasicTransducer read_in_prolog_format
       (FILE *file,
        unsigned int & linecount);
     
     /** @brief Write the graph in xfst text format to FILE \a file.
         \a write_weights defines whether weights are printed (todo). */
     HFSTDLL void write_in_xfst_format(FILE * file, bool write_weights=true);
     
     /** @brief Write the graph in AT&T format to ostream \a os.
         \a write_weights defines whether weights are printed. */
     HFSTDLL void write_in_att_format(std::ostream &os, bool write_weights=true);
     
     /** @brief Write the graph in AT&T format to FILE \a file.
         \a write_weights defines whether weights are printed. */
     HFSTDLL void write_in_att_format(FILE *file, bool write_weights=true);
     
     HFSTDLL void write_in_att_format(char * ptr, bool write_weights=true);
     
     /** @brief Write the graph in AT&T format to FILE \a file using numbers
         instead of symbol names.
         \a write_weights defines whether weights are printed. */
     HFSTDLL void write_in_att_format_number(FILE *file, bool write_weights=true);
     
     bool add_att_line(char * line, const std::string & epsilon_symbol);
     
     /* Create an HfstTransitionGraph as defined in AT&T format
        in istream \a is or FILE \a file. \a epsilon_symbol defines
        how epsilon is represented.
        
        The functions is called by functions
        read_in_att_format(istream&, std::string) and
        read_in_att_format(FILE*, std::string).
        If \a file is NULL, it is ignored and \a is is used.
        If \a file is not NULL, it is used and \a is is ignored. */
     HFSTDLL static HfstBasicTransducer read_in_att_format
       (std::istream &is,
        FILE *file,
        std::string epsilon_symbol,
        unsigned int & linecount);
     
     /** @brief Create an HfstTransitionGraph as defined in AT&T
         transducer format in istream \a is. \a epsilon_symbol
         defines how epsilon is represented.
         @pre \a is not at end, otherwise an exception is thrown.
         @note Multiple AT&T transducer definitions are separated with
         the line "--". */
     HFSTDLL static HfstBasicTransducer read_in_att_format
       (std::istream &is,
        std::string epsilon_symbol,
        unsigned int & linecount);
     
     /** @brief Create an HfstTransitionGraph as defined
         in AT&T transducer format in FILE \a file.
         \a epsilon_symbol defines how epsilon is represented.
         @pre \a is not at end, otherwise an exception is thrown.
         @note Multiple AT&T transducer definitions are separated with
         the line "--". */
     HFSTDLL static HfstBasicTransducer read_in_att_format
       (FILE *file,
        std::string epsilon_symbol,
        unsigned int & linecount);
     
     // ----------------------------------------------
     // -----       Substitution functions       -----
     // ----------------------------------------------
     
       protected:
     
     /* A function that performs in-place-substitution in the graph. */
     
     void substitute_(HfstSymbol old_symbol,
                      HfstSymbol new_symbol,
                      bool input_side=true,
                      bool output_side=true);
     
     /* A function that performs in-place substitutions in the graph
        as defined in \a substitutions.
        
        substitutions[from_number] = to_number,
        if substitutions[from_number] = no_substitution, no substitution is made */
     void substitute_(const HfstNumberVector &substitutions,
                      unsigned int no_substitution);
     
     /* A function that performs in-place substitutions in the graph
        as defined in \a substitutions. */
     void substitute_(const HfstNumberPairSubstitutions &substitutions);
     
     public:
     
     /* A function that performs in-place removal of all transitions
        equivalent to \a sp in the graph. */
     HFSTDLL void remove_transitions(const HfstSymbolPair &sp);
     
     protected:
     
     /* A function that performs in-place-substitution in the graph. */
     void substitute_(const HfstSymbolPair &old_sp,
                      const HfstSymbolPairSet &new_sps);
     
     /* A function that performs in-place-substitution in the graph. */
     void substitute_(bool (*func)
                      (const HfstSymbolPair &sp, HfstSymbolPairSet &sps));
     
     public:
     
     /* ----------------------------------------
               The public substitution functions.
               ---------------------------------------- */
     
     /** @brief Substitute \a old_symbol with \a new_symbol in
         all transitions. \a input_side and \a output_side define
         whether the substitution is made on input and output sides. */
     HFSTDLL HfstBasicTransducer &
       substitute(const HfstSymbol &old_symbol,
                      const HfstSymbol  &new_symbol,
                  bool input_side=true,
                  bool output_side=true);
     
     HFSTDLL HfstBasicTransducer &substitute_symbols
       (const HfstSymbolSubstitutions &substitutions);
     
     /** @brief Substitute all transitions as defined in \a substitutions */
     HfstBasicTransducer &substitute
       (const HfstSymbolSubstitutions &substitutions);
     
     HFSTDLL HfstBasicTransducer &substitute_symbol_pairs
       (const HfstSymbolPairSubstitutions &substitutions);
     
     /** @brief Substitute all transitions as defined in \a substitutions.
         
         For each transition x:y, \a substitutions is searched and if
         a mapping x:y -> X:Y is found, the transition x:y is replaced
         with X:Y. If no mapping is found, the transition remains the same.
     */
     HFSTDLL HfstBasicTransducer &substitute
       (const HfstSymbolPairSubstitutions &substitutions);
     
     /** @brief Substitute all transitions \a sp with a set of transitions
         \a sps. */
     HFSTDLL HfstBasicTransducer &substitute
       (const HfstSymbolPair &sp, const HfstSymbolPairSet &sps);
     
     /** @brief Substitute all transitions \a old_pair with
         \a new_pair. */
     HFSTDLL HfstBasicTransducer &substitute
       (const HfstSymbolPair &old_pair,
        const HfstSymbolPair &new_pair);
     
     /** @brief Substitute all transitions with a set of transitions as
         defined by function \a func.
         
         \a func takes as its argument a transition \a sp and inserts
         into the set of transitions \a sps the transitions with which
         the original transition \a sp must be replaced. \a func returns
         a value indicating whether any substitution must be made, i.e.
         whether any transition was inserted into \a sps. */
     HFSTDLL HfstBasicTransducer &
       substitute(bool (*func)
                  (const HfstSymbolPair &sp, HfstSymbolPairSet &sps) );
     
     /* ----------------------------------------------------
        Substitute string pair with a transition graph
        ---------------------------------------------------- */
     
     /* Used in function
        substitute(const StringPair&, HfstTransitionGraph&) */
     struct substitution_data
     {
       HfstState origin_state;
       HfstState target_state;
       HfstTropicalTransducerTransitionData::WeightType weight;
       HfstBasicTransducer * substituting_graph;
       
       substitution_data(HfstState origin,
                         HfstState target,
                         HfstTropicalTransducerTransitionData::WeightType weight,
                         HfstBasicTransducer * substituting)
       {
         origin_state=origin;
         target_state=target;
         this->weight=weight;
         substituting_graph=substituting;
       }
     };
         
     /* Used in function substitute(const StringPair&,
        HfstTransitionGraph&)
        Add a copy of substituting graph with epsilon transitions between
        states and with weight as defined in \a sub. */
     void add_substitution(const substitution_data &sub);
     
     public:
     
     /** @brief Substitute all transitions \a old_symbol : \a new_symbol
         with a copy of \a graph.
         
         Copies of \a graph are attached to this graph with
         epsilon transitions.
         
         The weights of the transitions to be substituted are copied
             to epsilon transitions leaving from the source state of
             the transitions to be substituted to the initial state
             of a copy of \a graph.
             
             The final weights in \a
             graph are copied to epsilon transitions leading from
             the final states (after substitution non-final states)
             of \a graph to target states of transitions
             \a old_symbol : \a new_symbol (that are substituted)
             in this graph.
     */
     HFSTDLL HfstBasicTransducer &
       substitute(const HfstSymbolPair &sp,
                  const HfstBasicTransducer &graph);
     
     HFSTDLL std::string weight2marker(float weight);
     
     HFSTDLL HfstBasicTransducer & substitute_weights_with_markers();
     
     // ####
     // another version of substitute for internal use..
     // ####
     typedef std::map<HfstSymbol, HfstBasicTransducer> SubstMap;
     
     HFSTDLL HfstBasicTransducer &
       substitute(SubstMap & substitution_map,
                  bool harmonize);
     
     HFSTDLL bool marker2weight(const std::string & str, float & weight);
     
     HFSTDLL HfstBasicTransducer & substitute_markers_with_weights();
     
     // aliases
     HFSTDLL HfstBasicTransducer & substitute_symbol(const std::string &old_symbol, const std::string &new_symbol, bool input_side=true, bool output_side=true);
     
     HFSTDLL HfstBasicTransducer & substitute_symbol_pair(const StringPair &old_symbol_pair, const StringPair &new_symbol_pair);
     
     HFSTDLL HfstBasicTransducer & substitute_symbol_pair_with_set(const StringPair &old_symbol_pair, const hfst::StringPairSet &new_symbol_pair_set);
     
     HFSTDLL HfstBasicTransducer & substitute_symbol_pair_with_transducer(const StringPair &symbol_pair, HfstBasicTransducer &transducer);
     
     
     /* ----------------------------
        Insert freely functions
        ---------------------------- */
     
     
     /** @brief Insert freely any number of \a symbol_pair in
         the graph with weight \a weight. */
     HFSTDLL HfstBasicTransducer &insert_freely
       (const HfstSymbolPair &symbol_pair, HfstTropicalTransducerTransitionData::WeightType weight);
     
     /** @brief Insert freely any number of any symbol in \a symbol_pairs in
         the graph with weight \a weight. */
     HFSTDLL HfstBasicTransducer &insert_freely
       (const HfstSymbolPairSet &symbol_pairs,
        HfstTropicalTransducerTransitionData::WeightType weight);
     
     /** @brief Insert freely any number of \a graph in this
         graph. */
     HFSTDLL HfstBasicTransducer &insert_freely
       (const HfstBasicTransducer &graph);
     
     /* -------------------------------
        Harmonization function
        ------------------------------- */
     
     /** @brief Harmonize this HfstTransitionGraph and \a another.
         
         In harmonization the unknown and identity symbols in
         transitions of both graphs are expanded according to
         the symbols that are previously unknown to the graph.
         
         For example the graphs
         \verbatim
         [a:b ?:?]
         [c:d ? ?:c]
         \endverbatim
         are expanded to
         \verbatim
         [ a:b [?:? | ?:c | ?:d | c:d | d:c | c:? | d:?] ]
         [ c:d [? | a | b] [?:c| a:c | b:?] ]
         \endverbatim
         when harmonized.
         The symbol "?" means \@_UNKNOWN_SYMBOL_\@ in either or
         both sides of a transition
         (transitions of type [?:x], [x:?] and [?:?]).
         The transition [?] means [\@_IDENTITY_SYMBOL_\@].
         
         @note This function is always called for arguments of functions
         that take two or more graphs as their arguments, unless otherwise
         said.
     */
     HFSTDLL HfstBasicTransducer &harmonize(HfstBasicTransducer &another);
     
     /* -------------------------------
        Disjunction functions
        ------------------------------- */
     
       protected:
     /* Disjunct the transition of path \a spv pointed by \a it
        to state \a s. If the transition does not exist in the graph,
        it is created as well as its target state.
        
        @return The final state of path \a spv, when \a it is at end. */
     HfstState disjunct(const StringPairVector &spv,
                        StringPairVector::const_iterator &it,
                        HfstState s);
     
     public:
     
     /** @brief Disjunct this graph with a one-path graph
         defined by string pair vector \a spv with weight \a weight.
         
         @pre This graph must be a trie where all weights are in
         final states, i.e. all transitions have a zero weight.
         
         There is no way to test whether a graph is a trie, so the use
         of this function is probably limited to fast construction
         of a lexicon. Here is an example:
         
         \verbatim
         HfstBasicTransducer lexicon;
         HfstTokenizer TOK;
         lexicon.disjunct(TOK.tokenize("dog"), 0.3);
         lexicon.disjunct(TOK.tokenize("cat"), 0.5);
         lexicon.disjunct(TOK.tokenize("elephant"), 1.6);
         \endverbatim
         
     */
     HFSTDLL HfstBasicTransducer &disjunct
       (const StringPairVector &spv, HfstTropicalTransducerTransitionData::WeightType weight);
     
     HFSTDLL bool is_special_symbol(const std::string & symbol);
     
     HFSTDLL HfstBasicTransducer &complete();
     
     HFSTDLL StringSet get_flags() const;
     
     // Whether symbol \a symbol must be purged from transitions and alphabet
     // of a transducer after \a flag has been eliminated from the transducer.
     // If \a flag is the empty string, all flags have been eliminated.
     HFSTDLL bool purge_symbol(const std::string & symbol, const std::string & flag);
     
     // Replace arcs in \a transducer that use flag \a flag with epsilon arcs
     // and remove \a flag from alphabet of \a transducer. If \a flag is the empty
     // string, replace/remove all flags.
     HFSTDLL void flag_purge(const std::string & flag);
     
     /* A topological sort. */
     struct TopologicalSort
     {
       std::vector<int> distance_of_state;
       std::vector<std::set<HfstState> > states_at_distance;
       
       /* Initialize the TopologicalSort by reserving space for a transducer
          with biggest state number \a biggest_state_number, */
       HFSTDLL void set_biggest_state_number(unsigned int biggest_state_number)
       {
         distance_of_state = std::vector<int>(biggest_state_number+1, -1);
       }
       
       /* Set the maximum distance of \a state to \a distance, */
       HFSTDLL void set_state_at_distance(HfstState state, unsigned int distance,
                                          bool overwrite)
       {
         // see that 'state' does not exceed the maximum state number given in initialization
         if (state > (distance_of_state.size() - 1))
           {
             std::cerr << "ERROR in TopologicalSort::set_state_at_distance: first argument ("
                       << state << ") is out of range (should be < " << distance_of_state.size()
                       << ")" << std::endl;
           }
         // if there is nothing on index 'state',
         // push back empty sets of states up to index 'state', including
         while (distance + 1 > (unsigned int)states_at_distance.size())
           {
             std::set<HfstState> empty_set;
             states_at_distance.push_back(empty_set);
           }
         // if there was previous distance defined for 'state', erase it, if needed
         int previous_distance = distance_of_state.at(state);
         if (previous_distance != -1 && previous_distance != (int)distance && overwrite)
           {
             states_at_distance.at(previous_distance).erase(state);
           }
         // set state and distance
         states_at_distance.at(distance).insert(state);
         distance_of_state.at(state) = distance;
       }
       
       /* The states that have a maximum distance of \a distance. */
       HFSTDLL const std::set<HfstState> & get_states_at_distance(unsigned int distance)
       {
         // if there is nothing on index 'state',
         // push back empty sets of states up to index 'state', including
         while (distance > (states_at_distance.size() - 1))
           {
             std::set<HfstState> empty_set;
             states_at_distance.push_back(empty_set);
           }
         return states_at_distance.at(distance);
       }
     };

     enum SortDistance { MaximumDistance, MinimumDistance };
     
     /*
       Get a topological (maximum distance) sort of this graph.
       @return A vector of sets of states. At each vector index ind, the
       result contains the set of all states whose (maximum) distance from
       the start state is ind.
     */
     HFSTDLL std::vector<std::set<HfstState> > topsort(SortDistance dist) const;
     
     /** The length of longest string accepted by this graph.
         If no string is accepted, return -1. */
     HFSTDLL int longest_path_size();
     
     /** The lengths of strings accepted by this graph, in descending order.
         If not string is accepted, return an empty vector. */
     HFSTDLL std::vector<unsigned int> path_sizes();
     
     bool has_negative_epsilon_cycles
       (HfstState state,
        float total_weight,
        std::map<HfstState, float> & state_weights);
     
     bool has_negative_epsilon_cycles();
     
     HFSTDLL bool is_infinitely_ambiguous
       (HfstState state,
        std::set<HfstState> &epsilon_path_states,
        std::vector<unsigned int> &states_handled);
     
     HFSTDLL bool is_infinitely_ambiguous();
     
     bool is_possible_flag(std::string symbol, StringVector & fds, bool obey_flags);
     
     HFSTDLL bool is_lookup_infinitely_ambiguous
       (const HfstOneLevelPath& s,
        unsigned int& index, HfstState state,
        std::set<HfstState> &epsilon_path_states,
        StringVector & fds, bool obey_flags);
     
     HFSTDLL bool is_lookup_infinitely_ambiguous(const HfstOneLevelPath & s, bool obey_flags=false);
     
     HFSTDLL bool is_lookup_infinitely_ambiguous(const StringVector & s, bool obey_flags=false);
     
     HFSTDLL static void push_back_to_two_level_path
       (HfstTwoLevelPath &path,
        const StringPair &sp,
        const float &weight,
        StringVector * fds_so_far = NULL);
     
     HFSTDLL static void pop_back_from_two_level_path
       (HfstTwoLevelPath &path,
        const float &weight,
        StringVector * fds_so_far = NULL);
     
     HFSTDLL static void add_to_results
       (HfstTwoLevelPaths &results,
        HfstTwoLevelPath &path_so_far,
        const float &final_weight,
        float * max_weight);
     
     HFSTDLL static bool is_possible_transition
       (const HfstBasicTransition &transition,
        const StringVector &lookup_path,
        const unsigned int &lookup_index,
        const StringSet &alphabet,
        bool &input_symbol_consumed,
        StringVector * fds_so_far = NULL);
     
     HFSTDLL void lookup
       (const StringVector &lookup_path,
        HfstTwoLevelPaths &results,
        HfstState state,
        unsigned int lookup_index, // an iterator instead?
        HfstTwoLevelPath &path_so_far,
        StringSet &alphabet,
        HfstEpsilonHandler Eh,
        size_t max_epsilon_cycles,
        float * max_weight = NULL,
        StringVector * flag_diacritic_path = NULL);
     
     HFSTDLL void lookup
       (const StringVector &lookup_path,
        HfstTwoLevelPaths &results,
        size_t * max_epsilon_cycles = NULL,
        float * max_weight = NULL,
        bool obey_flags = false);
     
     HFSTDLL void check_regexp_state_for_cycle(HfstState s, const std::set<HfstState> & states_visited);
     
     // Returns whether tr is "^]":"^]". If tr is not allowed, throws an error message.
     HFSTDLL bool check_regexp_transition_end(const HfstBasicTransition & tr, bool input_side);
     
     // If there is a "^[":"^[" transition leading to state \a s from some state
     // S and state S is included in \a states_visited and \a path and \a full_paths
     // are empty, this function can be used to find all (sub-)paths of form
     // [x:y]* "^]" (x and y cannot be "^]" or "^[") starting from state \a s. The resulting
     // paths are stored in \a full_paths. \a path is used to keep track of each path so
     // far. Weights are currently ignored.
     HFSTDLL void find_regexp_paths
       (HfstState s,
        std::set<HfstState> & states_visited,
        std::vector<std::pair<std::string, std::string> > & path,
        HfstReplacements & full_paths, bool input_side);
     
     // For each "^[":"^[" transition in state \a s, find continuing paths of form [x:y]* "^]":"^]"
     // (where x and y can freely be any symbols except "^]" or "^[") and store those paths in \a full_paths
     // vector where the first member of each element is the state where the ending "^]":"^]" transition
     // leads to and the second element is a vector of transitions (i.e. string pairs) without the ending
     // "^]":"^]" transition.
     // An error is thrown if mismatched "^[" or "^]" symbols, special symbols (epsilon, unknown, identity),
     // or loops are encountered on a regexp path. Final states are allowed on regexp paths as they are also
     // allowed by Xerox tools.
     // Weights are currently ignored.
     HFSTDLL void find_regexp_paths
       (HfstState s,
        std::vector<std::pair<HfstState, std::vector<std::pair<std::string, std::string> > > > & full_paths,
        bool input_side);
     
     // Find all subpaths of form "^[" [x:y]* "^]" (x and y cannot be "^[" or "^]") and return them.
     // retval[start_state] == vector(pair(end_state, vector(pair(isymbol,osymbol) ) ) )
     // Weights are currently ignored.
     HFSTDLL HfstReplacementsMap find_replacements(bool input_side);
     
     // Attach a copy of \a graph between states \a state1 and \a state2 with epsilon transitions.
     // There will be an epsilon transition with weight zero from state \a state1 to the
     // initial state of \a graph and one epsilon transition from each final state of
     // \a graph to state \a state2 with a weight of that state's final weight. Final states of
     // \a graph as well as its initial state are made normal non-final states when copying \a graph.
     // Todo: copy alphabet? harmonize graphs?
     HFSTDLL void insert_transducer(HfstState state1, HfstState state2, const HfstBasicTransducer & graph);
     
     typedef std::pair<HfstState, HfstState> StatePair;
     typedef std::map<StatePair, HfstState> StateMap;
     
     // Find target state corresponding to state pair \a target1, \a target2 in \a state_map and return that state.
     // If not found, add a new state to \a intersection, add it to \a state_map and return it.
     // \a was_new_state specifies whether a new state was added.
     static HfstState find_target_state
       (HfstState target1, HfstState target2, StateMap & state_map,
        HfstBasicTransducer & intersection, bool & was_new_state);
     
     // A function used by find_matches.
     // Copy matching transition tr1/tr2 to state \a state in \a intersection and return
     // the target state of that transition. Also make that state final, if needed.
     HFSTDLL static HfstState handle_match(const HfstBasicTransducer & graph1, const HfstBasicTransition & tr1,
                                           const HfstBasicTransducer & graph2, const HfstBasicTransition & tr2,
                                           HfstBasicTransducer & intersection, HfstState state, StateMap & state_map);
     
     // A recursive function used by function intersect.
     //
     // @param graph1        The first transducer argument of intersection.
     // @param state1        The current state of \a graph1.
     // @param graph2        The second transducer argument of intersection.
     // @param state2        The current state of \a graph2.
     // @param intersection  The intersection of \a graph1 and \a graph2.
     // @param state         The current state of \a intersection.
     // @param state_map     State pairs from \a graph1 and \a graph2 mapped to states in \a intersection.
     // @param agenda        States in \a intersection already handled or scheduled to be handled.
     //
     // @pre \a graph1 and \a graph2 must be arc-sorted (via sort_arcs()) to make transition matching faster.
     // @pre \a graph1 and \a graph2 must be deterministic. (todo: handle equivalent transitions, maybe even epsilons?)
     HFSTDLL static void find_matches
       (HfstBasicTransducer & graph1, HfstState state1, HfstBasicTransducer & graph2, HfstState state2,
        HfstBasicTransducer & intersection, HfstState state, StateMap & state_map, std::set<HfstState> & agenda);
     
     HFSTDLL static HfstBasicTransducer intersect
       (HfstBasicTransducer & graph1, HfstBasicTransducer & graph2);
     
     // A function used by find_matches_for_merge
     // Copy matching transition graph_tr/merger_tr to state \a result_state in \a result and return
     // the target state of that transition. Also make that state final, if needed.
     HFSTDLL static HfstState handle_non_list_match(const HfstBasicTransducer & graph, const HfstBasicTransition & graph_transition,
                                                    const HfstBasicTransducer & merger, HfstState merger_target,
                                                    HfstBasicTransducer & result, HfstState result_state, StateMap & state_map);
     
     // A function used by find_matches_for_merge
     // Copy matching transition graph_tr/merger_tr to state \a result_state in \a result and return
     // the target state of that transition. Also make that state final, if needed.
     HFSTDLL static HfstState handle_list_match(const HfstBasicTransducer & graph, const HfstBasicTransition & graph_transition,
                                                const HfstBasicTransducer & merger, const HfstBasicTransition & merger_transition,
                                                HfstBasicTransducer & result, HfstState result_state, StateMap & state_map, std::set<std::string> & markers_added);
     
     HFSTDLL static bool is_list_symbol(const HfstTropicalTransducerTransitionData & transition_data, const std::map<std::string, std::set<std::string> > & list_symbols);
     
     // A recursive function used by function intersect.
     //
     // @param graph          The first transducer argument of intersection.
     // @param graph_state    The current state of \a graph1.
     // @param merger         The second transducer argument of intersection.
     // @param merger_state   The current state of \a graph2.
     // @param result         The intersection of \a graph1 and \a graph2.
     // @param result_state   The current state of \a intersection.
     // @param state_map      State pairs from \a graph and \a merger mapped to states in \a result.
     // @param agenda         States in \a result already handled or scheduled to be handled.
     //
     // @pre \a graph and \a merger must be arc-sorted (via sort_arcs()) to make transition matching faster.
     // @pre \a graph and \a merger must be deterministic. (todo: handle equivalent transitions, maybe even epsilons?)
     HFSTDLL static void find_matches_for_merge
       (HfstBasicTransducer & graph, HfstState graph_state, HfstBasicTransducer & merger, HfstState merger_state,
        HfstBasicTransducer & result, HfstState result_state, StateMap & state_map, std::set<HfstState> & agenda,
        const std::map<std::string, std::set<std::string> > & list_symbols, std::set<std::string> & markers_added);
     
     HFSTDLL static HfstBasicTransducer merge
       (HfstBasicTransducer & graph, HfstBasicTransducer & merger, const std::map<std::string, std::set<std::string> > & list_symbols, std::set<std::string> & markers_added);
     
     // --- Friends ---
     
     friend class ConversionFunctions;
     friend class hfst::HarmonizeUnknownAndIdentitySymbols;
     };
     
   }
   
 }

#endif // #ifndef _HFST_BASIC_TRANSDUCER_H_
