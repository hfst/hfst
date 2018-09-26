// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.
#ifndef _HFST_OL_TRANSDUCER_PMATCH_H_
#define _HFST_OL_TRANSDUCER_PMATCH_H_

#include <map>
#include <stack>
#include <sstream>
#include <algorithm>
#include <ctime>
#include "HfstTransducer.h"
#include "HfstExceptionDefs.h"
#include "transducer.h"

namespace hfst_ol {

    class PmatchTransducer;
    class PmatchContainer;
    struct Location;
    struct WeightedDoubleTape;
    struct RtnStackFrame;

    typedef std::vector<RtnStackFrame> RtnCallStack;
    typedef std::vector<RtnCallStack> RtnCallStacks;
    typedef std::vector<PmatchTransducer *> RtnVector;
    typedef std::map<std::string, SymbolNumber> RtnNameMap;
    typedef std::vector<Location> LocationVector;
    typedef std::vector<LocationVector> LocationVectorVector;
    typedef std::vector<WeightedDoubleTape> WeightedDoubleTapeVector;
            
    
    enum SpecialSymbol{entry,
                       exit,
                       LC_entry,
                       LC_exit,
                       RC_entry,
                       RC_exit,
                       NLC_entry,
                       NLC_exit,
                       NRC_entry,
                       NRC_exit,
                       Pmatch_passthrough,
                       boundary,
                       Pmatch_input_mark,
                       SPECIALSYMBOL_NR_ITEMS};

    class PositionStack: public std::vector<unsigned int>
    {
        unsigned int tmp;
    public:
        void push(unsigned int val) { push_back(val); }
        void pop(void) { tmp = back(); pop_back(); }
        void unpop(void) { push_back(tmp); }
        unsigned int top(void) { return back(); }
    };

    class PmatchAlphabet: public TransducerAlphabet {
    protected:
        RtnVector rtns;
        SymbolNumber input_mark_symbol;
        SymbolNumberVector special_symbols;
        std::map<SymbolNumber, std::string> end_tag_map;
        std::map<std::string, SymbolNumber> capture_tag_map;
        std::map<std::string, SymbolNumber> captured_tag_map;
        SymbolNumberVector capture2captured;
        SymbolNumberVector captured2capture;
        RtnNameMap rtn_names;
// For each symbol, either NO_SYMBOL for "no corresponding list" or an index into symbol_lists
        SymbolNumberVector symbol2lists;
// For each a symbol, either NO_SYMBOL for "this is not a list" or an index into symbol_list_members
        SymbolNumberVector list2symbols;
        // For each entry referring to entries in the symbol table, indicate
        // "this symbol is an exclusionary list", ie. symbols not in it
        // will match
        SymbolNumberVector exclusionary_lists;
        std::vector<SymbolNumberVector> symbol_lists;
        std::vector<SymbolNumberVector> symbol_list_members;
        std::vector<unsigned long> counters;
        SymbolNumberVector guards;
        std::vector<bool> global_flags;
        std::vector<bool> printable_vector;
        bool is_end_tag(const SymbolNumber symbol) const;
        bool is_capture_tag(const SymbolNumber symbol) const;
        bool is_captured_tag(const SymbolNumber symbol) const;
        bool is_input_mark(const SymbolNumber symbol) const;
        bool is_guard(const SymbolNumber symbol) const;
        bool is_counter(const SymbolNumber symbol) const;
        bool is_global_flag(const SymbolNumber symbol) const;
        std::string end_tag(const SymbolNumber symbol);
        std::string start_tag(const SymbolNumber symbol);
        PmatchContainer * container;

    public:
        PmatchAlphabet(std::istream& is, SymbolNumber symbol_count, PmatchContainer * cont);
        PmatchAlphabet(TransducerAlphabet const & a, PmatchContainer * cont);
        PmatchAlphabet(void);
        ~PmatchAlphabet(void);
        virtual void add_symbol(const std::string & symbol);
        static bool is_end_tag(const std::string & symbol);
        static bool is_capture_tag(const std::string & symbol);
        static bool is_captured_tag(const std::string & symbol);
        static bool is_insertion(const std::string & symbol);
        static bool is_guard(const std::string & symbol);
        static bool is_list(const std::string & symbol);
        static bool is_underscored_list(const std::string & symbol);
        static bool is_counter(const std::string & symbol);
        static bool is_special(const std::string & symbol);
        static bool is_printable(const std::string & symbol);
        static bool is_global_flag(const std::string & symbol);
        static std::string name_from_insertion(
            const std::string & symbol);
        bool is_printable(SymbolNumber symbol);
        bool is_global_flag(SymbolNumber symbol);
        void add_special_symbol(const std::string & str, SymbolNumber symbol_number);
        void process_underscored_symbol_list(const std::string & str, SymbolNumber sym);
        void process_symbol_list(const std::string & str, SymbolNumber sym);
        void process_counter(std::string str, SymbolNumber sym);
        void count(SymbolNumber sym);
        void add_rtn(PmatchTransducer * rtn, std::string const & name);
        bool has_rtn(std::string const & name) const;
        bool has_rtn(SymbolNumber symbol) const;
        PmatchTransducer * get_rtn(SymbolNumber symbol);
        PmatchTransducer * get_rtn(std::string name);
        std::string get_counter_name(SymbolNumber symbol);
        SymbolNumber get_special(SpecialSymbol special) const;
        SymbolNumberVector get_specials(void) const;
        std::string stringify(const DoubleTape & str);
        Location locatefy(unsigned int input_offset,
                          const WeightedDoubleTape & str);

        friend class PmatchTransducer;
        friend class PmatchContainer;
    };

    struct RtnStackFrame
    {
        PmatchTransducer * caller;
        TransitionTableIndex caller_index;
    };

    struct Capture
    {
        unsigned int begin;
        unsigned int end;
        SymbolNumber name;
    };

    class PmatchContainer
    {
    protected:
        PmatchAlphabet alphabet;
        Encoder * encoder;
        SymbolNumber orig_symbol_count;
        SymbolNumber symbol_count;
        PmatchTransducer * toplevel;
        SymbolNumberVector input;
        // This tracks the ENTRY and EXIT tags
        PositionStack entry_stack;
        RtnCallStacks rtn_stacks;
        DoubleTape tape;
        DoubleTape best_result;
        DoubleTape result;
        LocationVectorVector locations;
        WeightedDoubleTapeVector tape_locations;
        std::vector<Capture> captures;
        std::vector<Capture> best_captures;
        std::vector<Capture> old_captures;
        std::vector<bool> possible_first_symbols;
        // The flag state for global flags
        hfst::FdState<SymbolNumber> global_flag_state;
        bool verbose;
        
        bool count_patterns;
        bool delete_patterns;
        bool extract_patterns;
        bool locate_mode;
        bool mark_patterns;
        size_t max_context_length;
        size_t max_recursion;
        bool need_separators;
        bool xerox_composition;

        unsigned long line_number;
        std::map<std::string, size_t> pattern_counts;
        bool profile_mode;
        bool single_codepoint_tokenization;
        unsigned int recursion_depth_left;
        // An optional time limit for operations
        double max_time;
        // When we started work
        clock_t start_clock;
        // A counter to avoid checking the clock too often
        unsigned long call_counter;
        // A flag to set for when time has been overstepped
        bool limit_reached;
        // Weight cutoff
        Weight max_weight;
        // The global running weight
        Weight running_weight;
        Weight weight_limit;
        // This is the depth of the stack from the point of view of the
        // container. When it's 0, we're in the toplevel, even if the
        // stack of variables is bigger due to having passed through a RTN.
        unsigned int stack_depth;
        // Where in the input the best candidate so far has gotten to
        unsigned int best_input_pos;
        Weight best_weight;

    public:

        PmatchContainer(std::istream & is);
        PmatchContainer(Transducer * toplevel);
        PmatchContainer(std::vector<hfst::HfstTransducer> transducers);
        PmatchContainer(void);
        ~PmatchContainer(void);

        void set_properties(void);
        void set_properties(std::map<std::string, std::string> & properties);
        void collect_first_symbols(const std::string & symbol_list);
        SymbolNumberVector symbol_vector_from_symbols(const std::string & symbols);
        void initialize_input(const char * input);
        bool has_unsatisfied_rtns(void) const;
        std::string get_unsatisfied_rtn_name(void) const;
        void add_rtn(Transducer * rtn, const std::string & name);
        void process(const std::string & input);
        std::string match(const std::string & input,
                          double time_cutoff = 0.0,
                          Weight weight_cutoff = INFINITE_WEIGHT);
        LocationVectorVector locate(const std::string & input,
                                    double time_cutoff = 0.0,
                                    Weight weight_cutoff = INFINITE_WEIGHT);
        void note_analysis(unsigned int input_pos, unsigned int tape_pos);
        void grab_location(unsigned int input_pos, unsigned int tape_pos);
        std::pair<SymbolNumberVector::iterator,
                  SymbolNumberVector::iterator>
        get_longest_matching_capture(SymbolNumber key, unsigned int input_pos);
        std::string get_profiling_info(void);
        std::string get_pattern_count_info(void);
        bool has_queued_input(unsigned int input_pos);
        bool input_matches_at(unsigned int pos,
                              SymbolNumberVector::iterator begin,
                              SymbolNumberVector::iterator end);
        bool not_possible_first_symbol(SymbolNumber sym)
        {
            if (possible_first_symbols.size() == 0) {
                return false;
            }
            return sym >= possible_first_symbols.size() ||
                possible_first_symbols[sym] == false;
        }
        void copy_to_result(const DoubleTape & best_result);
        void copy_to_result(SymbolNumber input, SymbolNumber output);
        static std::map<std::string, std::string> parse_hfst3_header(std::istream & f);
        void set_verbose(bool b) { verbose = b; }
        void set_locate_mode(bool b) { locate_mode = b; }
        void set_extract_patterns(bool b)
            { extract_patterns = b; }
        void set_single_codepoint_tokenization(bool b)
            { single_codepoint_tokenization = b; }
        void set_count_patterns(bool b)
            { count_patterns = b; }
        void set_delete_patterns(bool b)
            { delete_patterns = b; }
        void set_mark_patterns(bool b)
            { mark_patterns = b; }
        void set_max_recursion(size_t max)
            { max_recursion = max; }
        void set_max_context(size_t max)
            { max_context_length = max; }
        bool is_in_locate_mode(void) { return locate_mode; }
        void set_profile(bool b) { profile_mode = b; }
        void set_weight(Weight w) { running_weight = w; }
        void increment_weight(Weight w) { running_weight += w; }
        Weight get_weight(void) { return running_weight; }
        void increase_stack_depth(void) { ++stack_depth; }
        void decrease_stack_depth(void)
            {
                if (stack_depth == 0) {
                    HFST_THROW_MESSAGE(HfstException, "pmatch: negative stack depth");
                }
                --stack_depth;
            }
        void push_rtn_call(unsigned int return_index, PmatchTransducer * caller);
        RtnStackFrame rtn_stack_top(void);
        PmatchTransducer * get_latest_rtn_caller(void);
        void rtn_stack_pop(void);
        unsigned int get_stack_depth(void) { return stack_depth; }
        bool candidate_found(void)
            {
                if (locate_mode) {
                    return tape_locations.size() != 0;
                } else {
                    return best_result.size() != 0;
                }
            }
        bool try_recurse(void)
        {
            if (recursion_depth_left > 0) {
                --recursion_depth_left;
                return true;
            } else {
                return false;
            }
        }
        void unrecurse(void) { ++recursion_depth_left; }
        void reset_recursion(void) { recursion_depth_left = (unsigned int)max_recursion; }

        friend class PmatchTransducer;
        friend class PmatchAlphabet;
    };

    struct Location
    {
        unsigned int start;
        unsigned int length;
        std::string input;
        std::string output;
        std::string tag;
        Weight weight;
        std::vector<size_t> input_parts; // indices in input_symbol_strings
        std::vector<size_t> output_parts; // indices in output_symbol_strings
        std::vector<std::string> input_symbol_strings;
        std::vector<std::string> output_symbol_strings;

        bool operator<(Location rhs) const
            { return this->weight < rhs.weight; }
    };

    struct ContextMatchedTrap
    {
        bool polarity;
    ContextMatchedTrap(bool p): polarity(p) {}
    };

    class PmatchTransducer
    {
    protected:
        std::string name;
        enum ContextChecking{none, LC, NLC, RC, NRC};

// Transducers have static data, ie. tables for describing the states and
// transitions, and dynamic data, which is altered during lookup.
// In pmatch several instances of the same transducer may be operating
// in a stack, so this dynamic data is put in a class of its own.
        struct LocalVariables
        {
            hfst::FdState<SymbolNumber> flag_state;

            // Used for context checks
            char tape_step;
            size_t max_context_length_remaining;
            unsigned int context_placeholder;
            ContextChecking context;
            bool default_symbol_trap;
            bool negative_context_success;
            bool pending_passthrough;
        };

        std::stack<LocalVariables> local_stack;
    
        std::vector<TransitionW> transition_table;
        std::vector<TransitionWIndex> index_table;

        PmatchAlphabet & alphabet;
        SymbolNumber orig_symbol_count;
        PmatchContainer * container;

        bool is_final(TransitionTableIndex i)
        {
            if (indexes_transition_table(i)) {
                return transition_table[i - TRANSITION_TARGET_TABLE_START].final();
            } else {
                return index_table[i].final();
            }
        }

        Weight get_weight(TransitionTableIndex i)
        {
            if (indexes_transition_table(i)) {
                return transition_table[i - TRANSITION_TARGET_TABLE_START].get_weight();
            } else {
                return index_table[i].final_weight();
            }
        }

        TransitionTableIndex make_transition_table_index(
            TransitionTableIndex i, SymbolNumber input) {
            if (indexes_transition_table(i)) {
                return i - TRANSITION_TARGET_TABLE_START;
            } else {
                if (index_table[i + input].get_input_symbol() == input) {
                    return index_table[i + input].get_target() - TRANSITION_TARGET_TABLE_START;
                } else {
                    return TRANSITION_TARGET_TABLE_START;
                }
            }
        }

        // The mutually recursive lookup-handling functions

        void take_epsilons(unsigned int input_pos,
                           unsigned int tape_pos,
                           TransitionTableIndex i);

        void check_context(unsigned int input_pos,
                           unsigned int tape_pos,
                           TransitionTableIndex i);
  
        void take_flag(SymbolNumber input,
                       unsigned int input_pos,
                       unsigned int tape_pos,
                       TransitionTableIndex i);
  
        void take_transitions(SymbolNumber input,
                              unsigned int input_pos,
                              unsigned int tape_pos,
                              TransitionTableIndex i);

        void get_analyses(unsigned int input_pos,
                          unsigned int tape_pos,
                          TransitionTableIndex index);

        bool checking_context(void) const;
        bool try_entering_context(SymbolNumber symbol);
        bool try_exiting_context(SymbolNumber symbol);
        void exit_context(void);

    public:
        PmatchTransducer(std::istream& is,
                         TransitionTableIndex index_table_size,
                         TransitionTableIndex transition_table_size,
                         PmatchAlphabet & alphabet,
                         std::string name,
                         PmatchContainer * container);

        PmatchTransducer(std::vector<TransitionW> transition_vector,
                         std::vector<TransitionWIndex> index_vector,
                         PmatchAlphabet & alphabet,
                         std::string name,
                         PmatchContainer * container);

        bool final_index(TransitionTableIndex i) const
        {
            if (indexes_transition_table(i)) {
                return transition_table[i].final();
            } else {
                return index_table[i].final();
            }
        }

        static bool indexes_transition_table(TransitionTableIndex i)
        { return  i >= TRANSITION_TARGET_TABLE_START; }

        static bool is_good(TransitionTableIndex i)
        { return  i < TRANSITION_TARGET_TABLE_START; }

        void match(unsigned int input_pos, unsigned int tape_pos);
        void rtn_call(unsigned int input_pos, unsigned int tape_pos,
                      PmatchTransducer * caller, TransitionTableIndex caller_index);
        void rtn_call_in_context(unsigned int input_pos, unsigned int tape_pos,
                                 PmatchTransducer * caller, TransitionTableIndex caller_index,
                                 LocalVariables locals);
        void rtn_return(unsigned int input_pos, unsigned int tape_pos);
        void handle_final_state(unsigned int input_pos, unsigned int tape_pos);

        friend class PmatchContainer;
    };

}

#endif //_HFST_OL_TRANSDUCER_PMATCH_H_
