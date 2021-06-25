//       This program is free software: you can redistribute it and/or modify
//       it under the terms of the GNU General Public License as published by
//       the Free Software Foundation, version 3 of the License.
//
//       This program is distributed in the hope that it will be useful,
//       but WITHOUT ANY WARRANTY; without even the implied warranty of
//       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//       GNU General Public License for more details.
//
//       You should have received a copy of the GNU General Public License
//       along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "lookup-state.h"

void
LookupState::init(LookupPath* initial)
{
  clear_paths();
  paths.push_back(initial);
  try_epsilons();
}

void
LookupState::lookup(const SymbolNumberVector& input, CapitalizationMode mode)
{
  for(unsigned short it : input)
    step(it, mode);
}

void
LookupState::step(const SymbolNumber input, const SymbolNumber altinput)
{
  if(input == NO_SYMBOL_NUMBER)
  {
    clear_paths();
    return;
  }
  
  if(printDebuggingInformationFlag)
  {
    std::cout << "Stepping with '" << transducer.get_alphabet().symbol_to_string(input) << "'";
    if(altinput != NO_SYMBOL_NUMBER)
      std::cout << " and '" << transducer.get_alphabet().symbol_to_string(altinput) << "'";
    std::cout << std::endl;
  }
  
  apply_input(input, altinput);
  try_epsilons();
}

void
LookupState::step(const SymbolNumber input, CapitalizationMode mode)
{
  if(input==NO_SYMBOL_NUMBER || !transducer.get_alphabet().has_case(input) ||
     transducer.get_alphabet().is_lower(input) || mode==CaseSensitive || mode==CaseSensitiveDictionaryCase)
    step(input);
  else
    step(input, transducer.get_alphabet().to_lower(input));
}

  
void
LookupState::clear_paths()
{
  for(auto path : paths)
    delete path;
  paths.clear();
}

bool
LookupState::is_final() const
{
  for(auto path : paths)
  {
    TransitionTableIndex index = path->get_index();
    if(indexes_transition_index_table(index))
    {
      if(transducer.get_index(index).final())
        return true;
    }
    else
    {
      if(transducer.get_transition(index).final())
        return true;
    }
  }
  return false;
}

const LookupPathVector
LookupState::get_finals() const
{
  LookupPathVector finals;
  for(auto path : paths)
  {
    TransitionTableIndex index = path->get_index();
    if(indexes_transition_index_table(index))
    {
      if(transducer.get_index(index).final())
        finals.push_back(path);
    }
    else
    {
      if(transducer.get_transition(index).final())
        finals.push_back(path);
    }
  }
  return finals;
}

const LookupPathSet
LookupState::get_finals_set() const
{
  if(printDebuggingInformationFlag)
    std::cout << "Calculating final paths" << std::endl;
  LookupPathSet finals(LookupPath::compare_pointers);
  for(auto path : paths)
  {
    bool is_final;
    TransitionTableIndex index = path->get_index();
    
    if(indexes_transition_index_table(index))
      is_final = transducer.get_index(index).final();
    else
      is_final = transducer.get_transition(index).final();
    
    if(is_final)
    {
      if(printDebuggingInformationFlag)
      {
        std::cout << "  Final path found:";
        for(unsigned short itr : path->get_output_symbols())
          std::cout << " " << itr;
        std::cout << std::endl;
      }
      std::pair<LookupPathSet::iterator,bool> loc = finals.insert(path);
      
      if(loc.second == false) // if this form was already in the set
      {
        if(printDebuggingInformationFlag)
          std::cout << "  Duplicate LookupPath found" << std::endl;
        if(path < *(loc.first)) // if this form has a lower weight than the one there
        {
          finals.erase(loc.first);
          finals.insert(path);
        }
      }
    }
  }
  return finals;
}

void
LookupState::add_path(LookupPath& path)
{
  paths.push_back(&path);
}

void
LookupState::replace_paths(const LookupPathVector& new_paths)
{
  clear_paths();
  paths = new_paths;
}



void
LookupState::try_epsilons()
{
  for(auto & i : paths)
  {
    const LookupPath& path = *i;
    
    if(indexes_transition_index_table(path.get_index()))
      try_epsilon_index(path);
    else // indexes transition table
      try_epsilon_transitions(path);
  }
}

void
LookupState::try_epsilon_index(const LookupPath& path)
{
  // if this path points to an entry in the transition index table
  // which indexes one or more epsilon transtions
  const TransitionIndex& index = transducer.get_index(path.get_index()+1);
  
  if(index.matches(0))
  {
    // copy the current path, follow the index, add the new path to the list
    LookupPath& epsilon_path = *path.clone();
    epsilon_path.follow(index);
    add_path(epsilon_path);
  }
}

void
LookupState::try_epsilon_transitions(const LookupPath& path)
{
  TransitionTableIndex transition_index;
  
  // if the path is pointing to the "state" entry before the transitions
  if(transducer.get_transition(path.get_index()).get_input_symbol() == NO_SYMBOL_NUMBER)
    transition_index = path.get_index()+1;
  else // the path is pointing directly to a transition
    transition_index = path.get_index();
  
  while(true)
  {
    const Transition& transition = transducer.get_transition(transition_index);
    
    if(transducer.is_epsilon(transition))
    {
      // copy the path, follow the transition, add the new path to the list
      LookupPath& epsilon_path = *path.clone();
      if(epsilon_path.follow(transition))
        add_path(epsilon_path);
      else
      {
        // destroy the new path instead of pushing it
        delete &epsilon_path;
      }
    }
    else
      return;
    
    transition_index++;
  }
}


void
LookupState::apply_input(const SymbolNumber input, const SymbolNumber altinput)
{
  LookupPathVector new_paths;
  if(input == 0)
  {
    replace_paths(new_paths);
    return;
  }
  
  for(auto & i : paths)
  {
    LookupPath& path = *i;
    
    if(indexes_transition_index_table(path.get_index()))
    {
      try_index(new_paths, path, input);
      if(altinput != NO_SYMBOL_NUMBER)
        try_index(new_paths, path, altinput);
      //if(!try_index(new_paths, path, input) && altinput != NO_SYMBOL_NUMBER)
      //  try_index(new_paths, path, altinput);
    }
    else // indexes transition table
    {
      try_transitions(new_paths, path, input);
      if(altinput != NO_SYMBOL_NUMBER)
        try_transitions(new_paths, path, altinput);
      //if(!try_transitions(new_paths, path, input) && altinput != NO_SYMBOL_NUMBER)
      //  try_transitions(new_paths, path, altinput);
    }
  }
  
  replace_paths(new_paths);
}

bool
LookupState::try_index(LookupPathVector& new_paths,
                         const LookupPath& path,
                         const SymbolNumber input) const
{
  //??? is the +1 here correct?
  TransitionIndex index = transducer.get_index(path.get_index()+input+1);
  
  if(index.matches(input))
  {
    // copy the path, follow the index, and handle the new transitions
    LookupPath& extended_path = *path.clone();
    extended_path.follow(index);
    bool res = try_transitions(new_paths, extended_path, input);
    delete &extended_path;
    return res;
  }
  return false;
}

bool
LookupState::try_transitions(LookupPathVector& new_paths,
                               const LookupPath& path,
                               const SymbolNumber input) const
{
  bool found = false;
  TransitionTableIndex transition_index;
  
  // if the path is pointing to the "state" entry before the transitions
  if(transducer.get_transition(path.get_index()).get_input_symbol() == NO_SYMBOL_NUMBER)
    transition_index = path.get_index()+1;
  else // the path is pointing directly to a transition
    transition_index = path.get_index();
  
  while(true)
  {
    const Transition& transition = transducer.get_transition(transition_index);
    
    if(transition.matches(input))
    {
      // copy the path, follow the transition, add the new path to the list
      LookupPath& extended_path = *path.clone();
      extended_path.follow(transition);
      new_paths.push_back(&extended_path);
      found = true;
    }
    else
      return found;
    
    transition_index++;
  }
}

