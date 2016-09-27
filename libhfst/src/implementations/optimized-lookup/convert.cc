// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include "convert.h"

#ifdef _MSC_VER
#include "back-ends/openfstwin/src/include/fst/fstlib.h"
#else
#include "back-ends/openfst/src/include/fst/fstlib.h"
#endif // _MSC_VER

namespace fst
{
  extern template class TropicalWeightTpl<float>;
  extern template class ArcTpl<TropicalWeight>;
  extern template class VectorFst<StdArc>;
  extern template class ArcIterator<StdVectorFst>;
}


#ifndef MAIN_TEST

namespace hfst_ol {

void write_transitions_from_state_placeholders(
    TransducerTable<TransitionW> & transition_table,
    std::vector<hfst_ol::StatePlaceholder>
    & state_placeholders,
    std::set<SymbolNumber> & flag_symbols)
{
    for (std::vector<StatePlaceholder>::iterator it =
             state_placeholders.begin(); it!=state_placeholders.end(); ++it) {

        // Insert a finality marker unless this is the first state,
        // the finality of which is determined by the index table
        if (it->state_number != 0) {
            transition_table.append(
                TransitionW(
                    it->final, it->final_weight));
        }
        
        // Then we iterate through the symbols each state has.
    // First we do a pass for epsilon and flags (they have to come
    // first), then everything else.
        if (it->input_present(0)) {
            add_transitions_with(0, it->get_transition_placeholders(0),
                                 transition_table,
                                 state_placeholders,
                                 flag_symbols);
    }
    for (std::set<hfst_ol::SymbolNumber>::iterator flag_it =
         flag_symbols.begin(); flag_it != flag_symbols.end();
         ++flag_it) {
        if (it->input_present(*flag_it)) {
            hfst_ol::add_transitions_with(
                *flag_it,
                it->get_transition_placeholders(*flag_it),
                transition_table,
                state_placeholders,
                flag_symbols);
            
        }
    }
    for (unsigned int i = 1; i < it->symbol_to_transition_placeholder_v.size();
         ++i) {
        if (!it->input_present(i) ||
            flag_symbols.count(i) != 0) {
        continue;
        }
        hfst_ol::add_transitions_with(i,
                                      it->get_transition_placeholders(i),
                                      transition_table,
                                      state_placeholders,
                                      flag_symbols);
    }
    }

        
    // one final padding transition
    transition_table.append(hfst_ol::TransitionW(
                false, hfst_ol::INFINITE_WEIGHT));

}

void add_transitions_with(SymbolNumber symbol,
              std::vector<TransitionPlaceholder> & transitions,
              TransducerTable<TransitionW> & transition_table,
              std::vector<hfst_ol::StatePlaceholder>
              & state_placeholders,
              std::set<SymbolNumber> & flag_symbols)
{
    for (std::vector<TransitionPlaceholder>::iterator it = transitions.begin();
     it != transitions.end(); ++it) {
    // before writing each transition, find out whether its
    // target is simple (ie. should point directly to TA entry)
    unsigned int target;
    if (state_placeholders[it->target].is_simple()) {
        target = state_placeholders[it->target].first_transition +
        TRANSITION_TARGET_TABLE_START - 1;
    } else {
        target = state_placeholders[it->target].start_index;
    }
    transition_table.append(TransitionW(
                    symbol,
                    it->output,
                    target,
                    it->weight));
    }
}

bool compare_states_by_input_size(
    const StatePlaceholder & lhs, const StatePlaceholder & rhs)
{
    // descending by input size
    return lhs.inputs > rhs.inputs;
}

bool compare_states_by_state_number(
    const StatePlaceholder & lhs, const StatePlaceholder & rhs)
{
    // ascending by number
    return lhs.state_number < rhs.state_number;
}

#if HAVE_OPENFST

bool check_finality(TransduceR * tr, StateId s)
{
  return tr->Final(s) != fst::TropicalWeight::Zero();
}

void ConvertIdNumberMap::add_node(StateId n, TransduceR *tr)
{
  if (node_to_id.find(n) == node_to_id.end())
  {
    node_to_id[n] = node_counter;
    id_to_node[node_counter] = n;
    ++node_counter;
    for(ArcIterator aiter(*tr,n); !aiter.Done(); aiter.Next())
    {
      StdArc a = aiter.Value();
      add_node(a.nextstate,tr);
    }
  }
}

void ConvertIdNumberMap::set_node_maps(TransduceR * t)
{
  StateId n = t->Start();
  add_node(n,t);
}

StateIdNumber ConvertIdNumberMap::get_node_id(StateId n) const
{
  StateIdsToIdNumbers::const_iterator i = node_to_id.find(n);
  if(i == node_to_id.end())
    return NO_ID_NUMBER;
  return i->second;
}

StateId ConvertIdNumberMap::get_id_node(StateIdNumber n) const
{
  IdNumbersToStateIds::const_iterator i = id_to_node.find(n);
  if(i == id_to_node.end())
    return NO_STATE_ID;
  return i->second;
}

void ConvertTransducerAlphabet::inspect_node(
    StateId n,
    StateIdSet& visited_nodes,
    OfstSymbolCountMap& symbol_count_map, SymbolSet& all_symbol_set)
{
  if(visited_nodes.find(n) != visited_nodes.end())
    return;
  visited_nodes.insert(n);
  
  std::set<std::string> input_symbols;
  for(ArcIterator aiter(*transducer,n); !aiter.Done(); aiter.Next())
  {
    StdArc arc = aiter.Value();
    std::string input_symbol_string =
    transducer->InputSymbols()->Find(arc.ilabel);
    
    if(!FdOperation::is_diacritic(input_symbol_string))
      input_symbols.insert(input_symbol_string);
    all_symbol_set.insert(input_symbol_string);
    if(transducer->OutputSymbols() != NULL)
      all_symbol_set.insert(transducer->OutputSymbols()->Find(arc.olabel));
    else
      all_symbol_set.insert(transducer->InputSymbols()->Find(arc.olabel));
    
    inspect_node(
    arc.nextstate, visited_nodes, symbol_count_map, all_symbol_set);
  }
  
  for(std::set<std::string>::const_iterator it=input_symbols.begin();
      it!=input_symbols.end(); it++)
    symbol_count_map[ofst_symbol_table->Find(*it)]++;
}
void ConvertTransducerAlphabet::get_symbol_info(
    OfstSymbolCountMap &symbol_count_map, SymbolSet& all_symbol_set)
{
  symbol_count_map[0] = 1;
  StateIdSet visited_nodes;
  inspect_node(
      transducer->Start(), visited_nodes, symbol_count_map, all_symbol_set);
}

void ConvertTransducerAlphabet::populate_symbol_table(
    OfstSymbolCountMap &input_symbol_counts, SymbolSet& all_symbol_set)
{
  // a reverse mapping of input_symbol_counts, to sort symbols by frequency
  std::multimap<unsigned int,int64> count_keys;
  for (OfstSymbolCountMap::iterator it=input_symbol_counts.begin();
       it!=input_symbol_counts.end(); ++it)
  {
    if(!FdOperation::is_diacritic(ofst_symbol_table->Find(it->first)))
      count_keys.insert(std::pair<unsigned int,int64>(it->second,it->first));
    else
      count_keys.insert(std::pair<unsigned int,int64>(0,it->first));
  }
  
  symbol_table.push_back(ofst_symbol_table->Find((int64)0));
  for (std::multimap<unsigned int,int64>::reverse_iterator it =
       count_keys.rbegin();
       it!=count_keys.rend();++it)
  {
    if (it->second != 0)
      symbol_table.push_back(ofst_symbol_table->Find(it->second));
  }
  for(fst::SymbolTableIterator it(*ofst_symbol_table); !it.Done(); it.Next())
  {
    if(input_symbol_counts.find(it.Value()) == input_symbol_counts.end())
    {
      if(all_symbol_set.find(it.Symbol()) != all_symbol_set.end())
        symbol_table.push_back(it.Symbol());
    }
  }
}

void ConvertTransducerAlphabet::set_maps()
{
  for(fst::SymbolTableIterator it(*(transducer->InputSymbols())); !it.Done()
      ; it.Next())
  {
    for(size_t i=0; i<symbol_table.size(); i++)
    {
      if(symbol_table[i] == it.Symbol())
      {
        input_symbols_map[it.Value()] = i;
        break;
      }
    }
  }
  
  if(transducer->OutputSymbols() != NULL)
  {
    for(fst::SymbolTableIterator it(*(transducer->OutputSymbols())); !it.Done()
        ; it.Next())
    {
      for(size_t i=0; i<symbol_table.size(); i++)
      {
        if(symbol_table[i] == it.Symbol())
        {
          output_symbols_map[it.Value()] = i;
          break;
        }
      }
    }
  }
  else
    output_symbols_map = input_symbols_map;
}

ConvertTransducerAlphabet::ConvertTransducerAlphabet(TransduceR* t):
  transducer(t)
{
  // add an epsilon symbol here??
  ofst_symbol_table = t->InputSymbols()->Copy();
  if(t->OutputSymbols() != NULL)
    ofst_symbol_table->AddTable(*(t->OutputSymbols()));
  
  OfstSymbolCountMap symbol_count_map;
  SymbolSet all_symbol_set;
  
  get_symbol_info(symbol_count_map, all_symbol_set);
  populate_symbol_table(symbol_count_map, all_symbol_set);
  set_maps();
  
  delete ofst_symbol_table;
}

void ConvertTransducerAlphabet::display() const
{
  std::cout << "Final reordered symbol table:" << std::endl;
  for(size_t i=0;i<symbol_table.size();i++)
    std::cout << i << ": " << symbol_table[i] << std::endl;
  
  std::cout << "Initial input symbols (old/new: string):" << std::endl;
  for(fst::SymbolTableIterator i(*(transducer->InputSymbols()));
      !i.Done(); i.Next())
    std::cout << i.Value() << "/" << lookup_ofst_input_symbol(i.Value())
          << ": " << i.Symbol() << std::endl;
  std::cout << "Initial output symbols: (old/new: string)" << std::endl;
  if(transducer->OutputSymbols() != NULL)
  {
    for(fst::SymbolTableIterator i(*(transducer->InputSymbols()));
    !i.Done(); i.Next())
      std::cout << i.Value() << "/" << lookup_ofst_output_symbol(i.Value())
        << ": " << i.Symbol() << std::endl;
  }
  else
    std::cout << "[NULL]" << std::endl;
}

SymbolNumber ConvertTransducerAlphabet::lookup_ofst_input_symbol(int64 s) const
{
  std::map<int64, SymbolNumber>::const_iterator i = input_symbols_map.find(s);
  return (i==input_symbols_map.end()) ? NO_SYMBOL_NUMBER : i->second;
}
SymbolNumber ConvertTransducerAlphabet::lookup_ofst_output_symbol(
    int64 s) const
{
  std::map<int64, SymbolNumber>::const_iterator i = output_symbols_map.find(s);
  return (i==output_symbols_map.end()) ? NO_SYMBOL_NUMBER : i->second;
}

bool ConvertTransducerAlphabet::is_flag_diacritic(SymbolNumber symbol) const
{
  return FdOperation::is_diacritic(symbol_table[symbol]);
}

TransducerAlphabet ConvertTransducerAlphabet::to_alphabet() const
{
  return TransducerAlphabet(symbol_table);
}


ConvertTransition::ConvertTransition(const StdArc &a):
    input_symbol(ConvertTransducer::constructing_transducer->
         get_alphabet().lookup_ofst_input_symbol(a.ilabel)),
    output_symbol(ConvertTransducer::constructing_transducer->
          get_alphabet().lookup_ofst_output_symbol(a.olabel)),
    target_state_id(ConvertTransducer::constructing_transducer->
            get_id_number_map().get_node_id(a.nextstate)),
    weight(a.weight.Value()), table_index(NO_TABLE_INDEX) {}

void ConvertTransition::display() const
{
  std::cout << "  " << input_symbol << ":" << output_symbol << " at "
        << table_index << " ->" << target_state_index << " (" << weight
        << ")" << std::endl;
}

void ConvertTransition::set_target_state_index()
{
  ConvertFstState& state = ConvertTransducer::constructing_transducer->
      get_state(target_state_id);
  target_state_index = state.get_table_index();
}

bool ConvertTransition::numerical_cmp(
    const ConvertTransition &another_transition) const
{
  if (input_symbol == another_transition.input_symbol)
  {
    if(output_symbol == another_transition.output_symbol)
      return target_state_id < another_transition.target_state_id;
    else
      return output_symbol < another_transition.output_symbol;
  }
  return input_symbol < another_transition.input_symbol;
}

bool ConvertTransition::operator<(
    const ConvertTransition &another_transition) const
{
  if((input_symbol == 0) || ConvertTransducer::constructing_transducer->
     get_alphabet().is_flag_diacritic(input_symbol))
  {
    if((another_transition.input_symbol == 0) ||
       ConvertTransducer::constructing_transducer->get_alphabet()
       .is_flag_diacritic(another_transition.input_symbol))
      return numerical_cmp(another_transition);
    else
      return true;
  }
  else
  {
    if((another_transition.input_symbol != 0) &&
       !ConvertTransducer::constructing_transducer->
       get_alphabet().is_flag_diacritic(another_transition.input_symbol))
      return numerical_cmp(another_transition);
    else
      return false;
  }
}

template<class T>
T ConvertTransition::to_transition() const
{
  return T(input_symbol, output_symbol, target_state_index, weight);
}


void ConvertTransitionIndex::display() const
{
  std::cout << "  input_symbol: " << input_symbol
        << " to transitions starting at " << first_transition_index
        << std::endl;
}

bool ConvertTransitionIndex::operator<(
    const ConvertTransitionIndex &another_index) const
{
  return input_symbol < another_index.input_symbol;
}

template<class T>
T ConvertTransitionIndex::to_transition_index() const
{
  return T(input_symbol, first_transition_index);
}


ConvertFstState::ConvertFstState(StateId n, TransduceR * tr):
    table_index(NO_TABLE_INDEX), final(check_finality(tr,n)),
    weight(INFINITE_WEIGHT),
    id(ConvertTransducer::constructing_transducer->
       get_id_number_map().get_node_id(n))
{
  set_transitions(n,tr);
  set_transition_indices();
  if(final)
  {
    if(ConvertTransducer::constructing_transducer->is_weighted())
      weight = tr->Final(n).Value();
    else
    {
      TransitionTableIndex finality = 1;
      weight = *reinterpret_cast<Weight*>(&finality);
    }
  }
};
ConvertFstState::~ConvertFstState()
{
  for(ConvertTransitionSet::const_iterator it=transitions.begin();
      it!=transitions.end(); it++)
    delete *it;
  for(ConvertTransitionIndexSet::const_iterator it=transition_indices.begin();
      it!=transition_indices.end(); it++)
    delete *it;
}

void ConvertFstState::display() const
{
  std::cout << id << " at index " << table_index;
  if(final)
    std::cout << " (final, " << weight << ")";
  std::cout << ":" << std::endl;
  std::cout << " Transition indices:" << std::endl;
  for(ConvertTransitionIndexSet::const_iterator i=transition_indices.begin();
      i!=transition_indices.end(); i++)
    (*i)->display();
  std::cout << " Transitions:" << std::endl;
  for(ConvertTransitionSet::const_iterator i=transitions.begin();
      i!=transitions.end(); i++)
    (*i)->display();
}

void ConvertFstState::set_transitions(StateId n, TransduceR * tr)
{
  for(ArcIterator aiter(*tr,n); !aiter.Done(); aiter.Next())
  {
    const StdArc a = aiter.Value();
    ConvertTransition * t = new ConvertTransition(a);
    transitions.insert(t);
  }
}

void ConvertFstState::set_transition_indices(void)
{
  SymbolNumber previous_symbol = NO_SYMBOL_NUMBER;
  SymbolNumber position = 0;
  
  bool zero_transitions = false;
  for (ConvertTransitionSet::iterator it=transitions.begin();
       it!=transitions.end(); ++it)
  {
    ConvertTransition * t = *it;
    SymbolNumber input_symbol = t->get_input_symbol();
    if(previous_symbol != input_symbol)
    {
      if(ConvertTransducer::constructing_transducer->
     get_alphabet().is_flag_diacritic(input_symbol))
      {
        if(!zero_transitions)
        {
          transition_indices.insert(new ConvertTransitionIndex(0, t));
          
          previous_symbol = input_symbol;
          zero_transitions =true;
        }
      }
      else
      {
        transition_indices.insert(new ConvertTransitionIndex(input_symbol, t));
        
        previous_symbol = input_symbol;
      }
    }
    if(input_symbol == 0) { zero_transitions = true; }
    ++position;
  }
}

SymbolNumberSet * ConvertFstState::get_input_symbols(void) const
{
  SymbolNumberSet * input_symbols = new SymbolNumberSet;
  
  for(ConvertTransitionIndexSet::const_iterator it=transition_indices.begin();
      it!=transition_indices.end(); ++it)
  {
    ConvertTransitionIndex * i = *it;
    input_symbols->insert(i->get_input_symbol());
  }
  
  return input_symbols;
}

TransitionTableIndex ConvertFstState::set_transition_table_indices(
    TransitionTableIndex place)
{
  first_transition_index = place;
  
  // lay out the transitions sequentially with a space between each state
  for(ConvertTransitionSet::iterator it=transitions.begin();
      it!=transitions.end(); ++it)
  {
    ConvertTransition * t = *it;
    t->set_table_index(place);
    ++place;
  }
  ++place;
  
  // update the TransitionIndex's to store the table location of the
  // associated transition
  for(ConvertTransitionIndexSet::iterator it=transition_indices.begin();
      it!=transition_indices.end(); ++it)
  {
    ConvertTransitionIndex* i = *it;
    i->set_first_transition_index(
    i->get_first_transition()->get_table_index());
  }
  
  return place;
}

void ConvertFstState::set_transition_target_indices()
{
  for(ConvertTransitionSet::iterator i=transitions.begin();
      i!=transitions.end(); i++)
    (*i)->set_target_state_index();
}

template<class T>
void ConvertFstState::insert_transition_indices(
    TransducerTable<T>& index_table) const
{
  // only the start state and big states have
  // entries in the transition index table
  if(!is_big_state() && !is_start_state())
    return;
  
  TransitionTableIndex i = table_index;
  
  if(final)
    index_table.set(
    i, T(index_table[i].get_input_symbol(),
         *reinterpret_cast<const TransitionTableIndex*>(&weight)));
  
  ++i;
  
  for(ConvertTransitionIndexSet::const_iterator it=transition_indices.begin();
      it!=transition_indices.end(); ++it)
  {
    ConvertTransitionIndex * ind = *it;
    index_table.set(i+ind->get_input_symbol(), ind->to_transition_index<T>());
  }
}

template<class T>
TransitionTableIndex ConvertFstState::append_transitions(
    TransducerTable<T>& transition_table,
    TransitionTableIndex place) const
{
  while(place < get_first_transition_index())
  {
    transition_table.append(T(final, weight));
    ++place;
  }
  
  for(ConvertTransitionSet::iterator it=transitions.begin();
      it!=transitions.end(); ++it)
  {
    transition_table.append((*it)->to_transition<T>());
    ++place;
  }
  return place;
}


struct fst_state_compare {
  bool operator() (
      const ConvertFstState * s1, const ConvertFstState * s2) const
  {
    if (s1->transition_indices.size() < s2->transition_indices.size())
      return true;
    return s1->id < s2->id;
  }
};

typedef std::set<ConvertFstState*,fst_state_compare> StateSet;



void ConvertTransitionTableIndices::get_more_space(void)
{
  for(SymbolNumber i = 0; i < number_of_input_symbols + 1; ++i)
  {
    indices.push_back(EMPTY);
  }
}

bool ConvertTransitionTableIndices::state_fits(SymbolNumberSet * input_symbols,
                    bool final_state,
                    PlaceHolderVector::size_type index)
{
  if((indices.at(index) == EMPTY_START) ||
     (indices.at(index) == OCCUPIED_START))
    return false;
  
  if(final_state && (indices.at(index) == OCCUPIED))
    return false;
  
  // The input symbols start after the finality indicator.
  PlaceHolderVector::size_type input_symbol_start = index+1;
  
  // The node fits, if every one of its input symbols goes on
  // an EMPTY or EMPTY_START index.
  for(SymbolNumberSet::iterator it=input_symbols->begin();
      it!=input_symbols->end(); ++it)
  {
    SymbolNumber input_symbol = *it;
    
    if((indices.at(input_symbol_start + input_symbol) == OCCUPIED) ||
       (indices.at(input_symbol_start + input_symbol) == OCCUPIED_START))
      return false;
  }

  return true;
}

void ConvertTransitionTableIndices::insert_state(
    SymbolNumberSet * input_symbols,
    bool final_state,
    PlaceHolderVector::size_type index)
{
  if (final_state || (indices.at(index) == OCCUPIED))
    indices.at(index) = OCCUPIED_START;
  else
    indices.at(index) = EMPTY_START;
  
  // The input symbols start after the finality indicator.
  PlaceHolderVector::size_type input_symbol_start = index+1;
  
  for(SymbolNumberSet::iterator it=input_symbols->begin();
      it!=input_symbols->end(); ++it)
  {
    SymbolNumber input_symbol = *it;
    if(indices.at(input_symbol_start + input_symbol) == EMPTY)
      indices.at(input_symbol_start + input_symbol) = OCCUPIED;
    else
      indices.at(input_symbol_start + input_symbol) = OCCUPIED_START;
  }
}

PlaceHolderVector::size_type ConvertTransitionTableIndices::last_full_index(
    void) const
{
  for(PlaceHolderVector::size_type i = indices.size()-1; i != 0; --i)
  {
    if(indices.at(i) != EMPTY)
      return i;
  }
  return 0;
}
PlaceHolderVector::size_type ConvertTransitionTableIndices::add_state(
    ConvertFstState * state)
{
  if(lower_bound_test_count >= 1)
  {
    lower_bound_test_count = 0;
    if(indices.size() > 2000 && lower_bound < (indices.size() - 2000))
      lower_bound = indices.size() - 1000;
    
    ++lower_bound;
  }

  //  if (lower_bound_test_count >= 10)
  //  {
  //    lower_bound_test_count = 0;
  //    ++lower_bound;
  //  }
#ifdef DEBUG
  std::cerr << lower_bound << " " << lower_bound_test_count << " "
        << indices.size() << " " << state->number_of_input_symbols()
        << "\r";
#endif
  bool final_state = state->is_final();
  
  SymbolNumberSet * state_input_symbols =
    state->get_input_symbols();
  
  ++lower_bound_test_count;
  
  for(PlaceHolderVector::size_type index=lower_bound; index<indices.size();
      ++index)
  {
    // Only try the first 100 indices.
    //    if (index > lower_bound+100000)
    //{
    //  index = last_full_index()+1;
    //}
    if((index + number_of_input_symbols + 1) >= indices.size())
      get_more_space();
    
    if(state_fits(state_input_symbols,final_state,index))
    {
      insert_state(state_input_symbols,final_state,index);
      delete state_input_symbols;
      return index;
    }
  }
  return UINT_MAX;
}

void ConvertTransducerHeader::full_traversal(
    TransducerHeader& h, TransduceR* tr,
    StateId n, StateIdSet& visited_nodes, StateIdSet& nodes_in_path,
    OfstSymbolSet& all_input_symbols)
{
  if(visited_nodes.find(n) != visited_nodes.end())
    return;
  visited_nodes.insert(n);
  nodes_in_path.insert(n);
  
  if(h.weighted && !h.has_unweighted_input_epsilon_cycles)
  {
    StateIdSet epsilon_nodes;
    find_input_epsilon_cycles(n,n,epsilon_nodes,true,tr,h);
  }
  if(!h.has_input_epsilon_cycles)
  {
    StateIdSet epsilon_nodes;
    find_input_epsilon_cycles(n,n,epsilon_nodes,false,tr,h);
  }
  
  OfstSymbolSet node_input_symbols;
  LabelSet transition_labels;
  
  for(ArcIterator aiter(*tr,n); !aiter.Done(); aiter.Next())
  {
    StdArc a = aiter.Value();
    transition_label l;
    l.input_symbol = a.ilabel;
    l.output_symbol = a.olabel;
    StateId target = a.nextstate;
    
    h.number_of_transitions++;
    if(!FdOperation::is_diacritic(tr->InputSymbols()->Find(a.ilabel)))
      all_input_symbols.insert(a.ilabel);
    
    if(l.input_symbol == 0)
    {
      h.has_input_epsilon_transitions = true;
      if(l.output_symbol == 0)
        h.has_epsilon_epsilon_transitions = true;
    }
    
    if(node_input_symbols.find(l.input_symbol) != node_input_symbols.end())
      h.input_deterministic = false;
    else
      node_input_symbols.insert(l.input_symbol);
    
    if(transition_labels.find(l) != transition_labels.end())
      h.deterministic = false;
    else
      transition_labels.insert(l);
    
    if(nodes_in_path.find(target) != nodes_in_path.end())
      h.cyclic = true;
    
    full_traversal(h, tr, target, visited_nodes, nodes_in_path,
           all_input_symbols);
  }
  nodes_in_path.erase(n);
}

void ConvertTransducerHeader::find_input_epsilon_cycles(
    StateId n, StateId start, StateIdSet &epsilon_targets,
    bool unweighted_only, TransduceR * tr, TransducerHeader& h)
{
  for (ArcIterator aiter(*tr,n); !aiter.Done(); aiter.Next())
  {
    StdArc a = aiter.Value();
    if(a.ilabel != 0 ||
       FdOperation::is_diacritic(tr->InputSymbols()->Find(a.ilabel)))
      continue;
    else
    {
      if(a.weight != StdArc::Weight::Zero())
        continue;
    }
    
    StateId target = a.nextstate;
    if (start == target )
    {
      if(unweighted_only)
        h.has_unweighted_input_epsilon_cycles = true;
      h.has_input_epsilon_cycles = true;
      return;
    }
    
    if(epsilon_targets.find(target) != epsilon_targets.end())
    {
      epsilon_targets.insert(target);
      find_input_epsilon_cycles(
      target, start, epsilon_targets, unweighted_only, tr, h);
    }

    if(h.has_input_epsilon_cycles || h.has_unweighted_input_epsilon_cycles)
      return;
  }
}

void ConvertTransducerHeader::compute_header(TransducerHeader& header,
        TransduceR * t, SymbolNumber symbol_count,
        TransitionTableIndex number_of_index_table_entries,
        TransitionTableIndex number_of_target_table_entries,
        bool weighted)
{
  //Initial values, many will be modified by the following function calls
  header.number_of_input_symbols = 0;
  header.number_of_symbols = symbol_count;
  header.size_of_transition_index_table = number_of_index_table_entries;
  header.size_of_transition_target_table = number_of_target_table_entries;
  header.number_of_states = 0;
  header.number_of_transitions = 0;
  header.weighted = weighted;
  header.deterministic = true;
  header.input_deterministic = true;
  header.minimized = false; // TODO: determine this somehow
  header.cyclic = false;
  header.has_epsilon_epsilon_transitions = false;
  header.has_input_epsilon_transitions = false;
  header.has_input_epsilon_cycles = false;
  header.has_unweighted_input_epsilon_cycles = false;
  
  StateIdSet nodes, nodes_in_path;
  OfstSymbolSet input_symbols;
  input_symbols.insert(0);
  full_traversal(header, t, t->Start(), nodes, nodes_in_path, input_symbols);
  
  header.number_of_input_symbols = input_symbols.size();
  header.number_of_states = nodes.size();
  if(!header.weighted)
    header.has_unweighted_input_epsilon_cycles =
    header.has_input_epsilon_cycles;
}


ConvertTransducer* ConvertTransducer::constructing_transducer = NULL;

void ConvertTransducer::read_nodes(void)
{
  for(StateIdNumber id=0; id<id_number_map->get_number_of_nodes(); ++id)
  {
    StateId n = id_number_map->get_id_node(id);
    ConvertFstState * state = new ConvertFstState(n,fst);
    states.push_back(state);
  }
}

void ConvertTransducer::set_transition_table_indices(void)
{
  TransitionTableIndex place = TRANSITION_TARGET_TABLE_START;
  for (ConvertFstStateVector::iterator it=states.begin();
       it!=states.end(); ++it)
  {
    ConvertFstState * state = *it;
    place = state->set_transition_table_indices(place);
  }
}

void ConvertTransducer::set_index_table_indices(void)
{
  StateSet state_set;
  
  for(ConvertFstStateVector::iterator it=states.begin()+1;
      it != states.end(); ++it)
      state_set.insert(*it);
  
  ConvertFstState * start_state = *states.begin();
  TransitionTableIndex start_state_index = fst_indices->add_state(start_state);
  
  start_state->set_table_index(start_state_index);
  
  for(StateSet::reverse_iterator it=state_set.rbegin();
      it!=state_set.rend(); ++it)
  {
    ConvertFstState * state = *it;
    TransitionTableIndex state_index;
    if (state->is_big_state())
      state_index = fst_indices->add_state(state);
    else
    {
      state_index = state->get_first_transition_index()-1;
      if(state_index < TRANSITION_TARGET_TABLE_START)
      {
        std::cerr << "FIXME!" << std::endl;
        throw;
      }
    }
    
    state->set_table_index(state_index);
  }
  
  // now that the state object's all know their table location, update the
  // transition objects with that information
  for(ConvertFstStateVector::const_iterator it=states.begin();
      it!=states.end(); it++)
    (*it)->set_transition_target_indices();
}

void ConvertTransducer::add_input_symbols(StateId n,
                      SymbolNumberSet &input_symbols,
                      StateIdSet &visited_nodes)
{
  for(ArcIterator aiter(*fst,n); !aiter.Done(); aiter.Next())
  {
    StdArc a = aiter.Value();
    input_symbols.insert(a.ilabel);
    if (visited_nodes.find(a.nextstate) == visited_nodes.end())
    {
      visited_nodes.insert(a.nextstate);
      add_input_symbols(a.nextstate, input_symbols, visited_nodes);
    }
  }
}

SymbolNumber ConvertTransducer::number_of_input_symbols(void)
{
  SymbolNumberSet input_symbol_set;
  input_symbol_set.insert(0);
  StateIdSet visited_nodes;
  add_input_symbols(fst->Start(), input_symbol_set, visited_nodes);
  return input_symbol_set.size();
}

TransitionTableIndex ConvertTransducer::count_transitions(void) const
{
  TransitionTableIndex transition_count = 0;
  for(ConvertFstStateVector::const_iterator it=states.begin();
      it!=states.end(); ++it)
  {
    // Separator between states;
    ++transition_count;
    
    transition_count += (*it)->number_of_transitions();
  }
  return transition_count;
}

void ConvertTransducer::display_states() const
{
  std::cout << "Transducer states:" << std::endl;
  for(ConvertFstStateVector::const_iterator it=states.begin();
      it!=states.end(); it++)
    (*it)->display();
}
void ConvertTransducer::display_tables() const
{
  std::cout << "Transducer tables:" << std::endl;
  std::cout << "----------" << std::endl;
  if(is_weighted())
  {
    std::cout << " Transition index table:" << std::endl;
    make_index_table<TransitionWIndex>(index_table_size).display(false);
    std::cout << " Transition table:" << std::endl;
    make_transition_table<TransitionW>().display(true);
  }
  else
  {
    std::cout << " Transition index table:" << std::endl;
    make_index_table<TransitionIndex>(index_table_size).display(false);
    std::cout << " Transition table:" << std::endl;
    make_transition_table<Transition>().display(true);
  }
  std::cout << "----------" << std::endl;
}

template<class T>
TransducerTable<T> ConvertTransducer::make_index_table(
    TransitionTableIndex index_table_size) const
{
  TransducerTable<T> index_table(index_table_size, T());
  
  for(ConvertFstStateVector::const_iterator it=states.begin();
      it!=states.end(); ++it)
  {
    ConvertFstState * state = *it;
    state->insert_transition_indices(index_table);
  }
  
  return index_table;
}

template<class T>
TransducerTable<T> ConvertTransducer::make_transition_table() const
{
  TransducerTable<T> transition_table;
  TransitionTableIndex place = TRANSITION_TARGET_TABLE_START;
  for(ConvertFstStateVector::const_iterator it=states.begin();
      it!=states.end(); it++)
  {
    place = (*it)->append_transitions(transition_table, place);
  }
  transition_table.append(T(false,INFINITE_WEIGHT));
  
  return transition_table;
}

ConvertTransducer::ConvertTransducer(TransduceR * tr, bool weighted):
    fst(tr),
    id_number_map(new ConvertIdNumberMap(tr)),
    fst_indices(new ConvertTransitionTableIndices(number_of_input_symbols())),
    header(weighted), alphabet(tr)
{
  constructing_transducer = this;
  id_number_map = new ConvertIdNumberMap(tr);
  
  //std::cout << "Creating state structures" << std::endl;
  read_nodes();
  //std::cout << "Laying out transition table" << std::endl;
  set_transition_table_indices();
  //std::cout << "Laying out transition index table" << std::endl;
  set_index_table_indices();
  
  index_table_size = fst_indices->size();
  delete fst_indices;
  
  //std::cout << "Computing header properties" << std::endl;
  ConvertTransducerHeader::compute_header(
      header, tr, alphabet.get_symbol_table().size(),
      index_table_size,count_transitions(), weighted);
  
  //header.display();
  //alphabet.display();
  //display_states();
  //display_tables();
  
  delete id_number_map;
  id_number_map = NULL;
  constructing_transducer = NULL;
}

ConvertTransducer::~ConvertTransducer()
{
  for(ConvertFstStateVector::const_iterator it=states.begin();
      it!=states.end(); it++)
    delete *it;
}

Transducer* ConvertTransducer::to_transducer() const
{
  //std::cout << "Building new transducer" << std::endl;
  if(is_weighted())
    return new Transducer(
    header, alphabet.to_alphabet(),
    make_index_table<TransitionWIndex>(index_table_size),
    make_transition_table<TransitionW>());
  else
    return new Transducer(header, alphabet.to_alphabet(),
              make_index_table<TransitionIndex>(index_table_size),
              make_transition_table<Transition>());
}
#endif // HAVE_OPENFST

}

#else // MAIN_TEST was defined

#include <iostream>

int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":" << std::endl;
    
    std::cout << "ok" << std::endl;
    return 0;
}

#endif // MAIN_TEST
