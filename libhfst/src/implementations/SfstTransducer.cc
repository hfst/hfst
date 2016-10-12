// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include "back-ends/sfst/interface.h"
#include "back-ends/sfst/fst.h"
#include "SfstTransducer.h"
#include "HfstSymbolDefs.h"
#include <time.h>

using namespace SFST;

#ifndef MAIN_TEST
namespace hfst { namespace implementations {

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
    };

    float sfst_seconds_in_harmonize=0;

    float SfstTransducer::get_profile_seconds() {
      return sfst_seconds_in_harmonize;
    }

  void sfst_set_hopcroft(bool value) {
    SFST::Transducer::hopcroft_minimisation=value;
  }

    /** Create an SfstInputStream that reads from stdin. */
  SfstInputStream::SfstInputStream(void):
      is_minimal(false)
  {
    this->input_file = stdin;
  }
    /** Create an SfstInputStream that reads from file \a filename. */
    SfstInputStream::SfstInputStream(const std::string &filename_):
      filename(std::string(filename_)), is_minimal(false)
  {
    if (filename == std::string())
      { input_file = stdin; }
    else {
      input_file = fopen(filename.c_str(),"r");
      if (input_file == NULL)
        {
          HFST_THROW(StreamNotReadableException); }
    }
  }

    /** Close the stream. */
  void SfstInputStream::close(void)
  {
    if (input_file == NULL)
      { return; }
    if (filename.c_str()[0] != 0)
      {
        fclose(input_file);
        input_file = NULL;
      }
  }

  char SfstInputStream::stream_get() {
    return (char) fgetc(input_file); }

  short SfstInputStream::stream_get_short()
  {
    short i;
    assert(1 == fread(&i,sizeof(i),1,input_file));
    return i;
  }

  void SfstInputStream::stream_unget(char c) {
    ungetc ( (int)c, input_file ); }
  
  bool SfstInputStream::is_eof(void)
  {
    int c = getc(input_file);
    bool retval = (feof(input_file) != 0);
    ungetc(c, input_file);
    return retval;
  }
  
  bool SfstInputStream::is_bad(void)
  {
    return is_eof();
  }
  
  bool SfstInputStream::is_good(void)
  {
    return not is_bad();
  };

  bool SfstInputStream::is_fst(void)
  {
    return is_fst(input_file);
  }
  
  bool SfstInputStream::is_fst(FILE * f)
  {
    if (f == NULL)
      { return false; }
    int c = getc(f);
    ungetc(c, f);
    return c == (int)'a';
  }
  
  bool SfstInputStream::is_fst(std::istream &s)
  {
    return s.good() && (s.peek() == (int)'a');
  }

  void SfstInputStream::add_symbol(StringNumberMap &string_number_map,
                                   Character c,
                                   Alphabet &alphabet)
  {
    const char * string_symbol =
      alphabet.code2symbol(c);
    if (string_number_map.find(string_symbol) == string_number_map.end())
      { string_number_map[string_symbol] = c; }
    else if (string_number_map[string_symbol] != c)
      {
        HFST_THROW_MESSAGE
          (HfstFatalException,
           "SfstInputStream: symbol redefined"); }
  }

  Transducer * SfstTransducer::expand_arcs(Transducer * t, StringSet &unknown)
  {
    Transducer &tc = t->copy();
    SfstTransducer::expand(&tc, unknown);
     return &tc;
  }


  std::pair<Transducer*, Transducer*> SfstTransducer::harmonize
  (Transducer *t1, Transducer *t2, bool unknown_symbols_in_use)
  {

    try {

    // 1. Calculate the set of unknown symbols for transducers t1 and t2.

    StringSet unknown_t1;    // symbols known to another but not this
    StringSet unknown_t2;    // and vice versa

      if (unknown_symbols_in_use) {
    StringSet t1_symbols = get_alphabet(t1);
    StringSet t2_symbols = get_alphabet(t2);
    hfst::symbols::collect_unknown_sets(t1_symbols, unknown_t1,
                        t2_symbols, unknown_t2);
      }


      Transducer * new_t1 = &t1->copy(false, &t2->alphabet);
      new_t1->alphabet.insert_symbols(t2->alphabet);
      SFST::Alphabet::CharMap cm = t1->alphabet.get_char_map();
      for (SFST::Alphabet::CharMap::const_iterator it = cm.begin();
       it != cm.end(); it++) {
    new_t1->alphabet.add_symbol(it->second);
      }
      
      t2->alphabet.insert_symbols(new_t1->alphabet);
      delete t1;
      t1 = new_t1;

    // 3. Calculate the set of symbol pairs to which a non-identity "?:?"
    //    transition is expanded for both transducers.
    
    Transducer *harmonized_t1;
    Transducer *harmonized_t2;

    if (unknown_symbols_in_use) {
      harmonized_t1 = expand_arcs(t1, unknown_t1);
      delete t1;
      
      harmonized_t2 = expand_arcs(t2, unknown_t2);
      delete t2;
    }
    else {
      harmonized_t1 = &t1->copy();
      harmonized_t2 = &t2->copy();
    }


    return std::pair<Transducer*, Transducer*>(harmonized_t1, harmonized_t2);

    }
    catch (const char *msg) {
      HFST_THROW_MESSAGE(HfstFatalException, std::string(msg));
    }

  }

    void SfstInputStream::ignore(unsigned int n)
    {
      for (unsigned int i=0; i<n; i++)
        fgetc(input_file);
    }

    bool SfstInputStream::set_implementation_specific_header_data
    (StringPairVector &header_data, unsigned int index)
    {
      if (index != (header_data.size()-1) )
        return false;

      if ( not ( strcmp("minimal", header_data[index].first.c_str()) == 0) )
        return false;

      if ( strcmp("true", header_data[index].second.c_str()) == 0 )
        is_minimal=true;
      else if ( strcmp("false", header_data[index].second.c_str()) == 0 )
        is_minimal=false;
      else
        return false;

      return true;
    }
    
    unsigned int SfstTransducer::number_of_states(Transducer* t)
    {
      std::vector<SFST::Node*> indexing;
      std::pair<size_t, size_t> number_of_nodes_and_transitions =
        t->nodeindexing(&indexing);
      return number_of_nodes_and_transitions.first;
    }

    unsigned int SfstTransducer::number_of_arcs(Transducer* t)
    {
      std::vector<SFST::Node*> indexing;
      std::pair<size_t, size_t> number_of_nodes_and_transitions =
        t->nodeindexing(&indexing);
      return number_of_nodes_and_transitions.second;
    }


    Transducer * SfstInputStream::read_transducer()
  {
    if (is_eof())
      {
        HFST_THROW(StreamIsClosedException); }
    Transducer * t = NULL;
    try
      {
        // DEBUGGING
        assert (stream_get() == 'a');
        stream_unget('a');

        Transducer * t = new Transducer(input_file,true);

        if (not is_minimal) {
          t->minimised = false;
          t->deterministic = false;
        }
        return t;
      }
    catch (const char * p)
      {
        delete t;
        fprintf(stderr, "caught message: \"%s\"\n", p);
        HFST_THROW(TransducerHasWrongTypeException);
      }
    return NULL;
  }


  // ---------- SfstOutputStream functions ----------

  SfstOutputStream::SfstOutputStream(void)
  { ofile = stdout; }

    SfstOutputStream::SfstOutputStream(const std::string &str):
    filename(std::string(str))
  {
    if (filename != std::string()) {
      ofile = fopen(filename.c_str(), "wb");
      if (ofile == NULL)
        HFST_THROW(StreamNotReadableException);
    }
    else
      ofile = stdout;
  }

  void SfstOutputStream::close(void)
  {
    if (filename != std::string())
      { fclose(ofile); }
  }

    void SfstOutputStream::append_implementation_specific_header_data
    (std::vector<char> &header, Transducer *t)
    {
      std::string min("minimal");
      for (unsigned int i=0; i<min.length(); i++)
        header.push_back(min[i]);
      header.push_back('\0');

      std::string min_value;
      if (t->minimised && t->deterministic)
        min_value = std::string("true");
      else
        min_value = std::string("false");

      for (unsigned int i=0; i<min_value.length(); i++)
        header.push_back(min_value[i]);
      header.push_back('\0');
    }

    void SfstOutputStream::write(const char &c)
    {
      fputc(c,ofile);
    }

    void SfstOutputStream::write_transducer(Transducer * transducer)
  {
    transducer->store(ofile);
    if (fflush(ofile) != 0) {
      HFST_THROW_MESSAGE(HfstFatalException,
                         "An error happened when writing an SfstTransducer.");
    }
  }

  void SfstTransducer::print_test(Transducer *t)
  {
    std::cerr << *t;
  }

    unsigned int SfstTransducer::get_biggest_symbol_number(Transducer * t)
    {
      unsigned int biggest_number=0;
      SFST::Alphabet::CharMap cm = t->alphabet.get_char_map();
      for (SFST::Alphabet::CharMap::const_iterator it = cm.begin();
           it != cm.end(); it++) {
        if (it->first > biggest_number)
          biggest_number = it->first;
      }
      return biggest_number;
    }

    StringVector SfstTransducer::get_symbol_vector
    (Transducer * t)
    {
      unsigned int biggest_symbol_number = get_biggest_symbol_number(t);
      StringVector symbol_vector;
      symbol_vector.reserve(biggest_symbol_number+1);
      symbol_vector.resize(biggest_symbol_number+1,"");
      
      StringSet alphabet = get_alphabet(t);
      for (StringSet::const_iterator it = alphabet.begin(); it != alphabet.end(); it++)
        {
          unsigned int symbol_number = get_symbol_number(t, it->c_str());
          symbol_vector.at(symbol_number) = *it;
        }
      return symbol_vector;
    }
    
    std::map<std::string, unsigned int> SfstTransducer::get_symbol_map
    (Transducer * t)
    {
      StringSet alphabet = get_alphabet(t);
      std::map<std::string, unsigned int> symbol_map;
      for (StringSet::const_iterator it = alphabet.begin(); it != alphabet.end(); it++)
        {
          symbol_map[*it] = get_symbol_number(t, it->c_str());
        }
      return symbol_map;
    }

  void SfstTransducer::print_alphabet(Transducer *t) {
    fprintf(stderr, "alphabet..\n");
    SFST::Alphabet::CharMap cm = t->alphabet.get_char_map();
    for (SFST::Alphabet::CharMap::const_iterator it = cm.begin();
         it != cm.end(); it++)
      fprintf(stderr, "%i\t%s\n",it->first,it->second);
    fprintf(stderr, "..alphabet\n");
  }

  void SfstTransducer::initialize_alphabet(Transducer *t) {
    t->alphabet.clear();
    t->alphabet.utf8 = true;
    t->alphabet.add_symbol("<>", 0);
    t->alphabet.add_symbol(internal_unknown.c_str(), 1);
    t->alphabet.add_symbol(internal_identity.c_str(), 2);
  }

  Transducer * SfstTransducer::create_empty_transducer(void)
  { Transducer * retval = new Transducer();
    initialize_alphabet(retval);
    return retval;
  }
  
  Transducer * SfstTransducer::create_epsilon_transducer(void)
  { Transducer * t = new Transducer;
    initialize_alphabet(t);
    t->root_node()->set_final(1);
    return t; }
  
  Transducer * SfstTransducer::define_transducer(unsigned int number)
  { Transducer * t = new Transducer;
    initialize_alphabet(t);
    Node * n = t->new_node();
    t->root_node()->add_arc(Label(number),n,t);
    n->set_final(1);
    return t; }
  
    Transducer * SfstTransducer::define_transducer
    (unsigned int inumber, unsigned int onumber)
  { Transducer * t = new Transducer;
    initialize_alphabet(t);
    Node * n = t->new_node();
    t->root_node()->add_arc(Label(inumber,
                                  onumber),n,t);
    n->set_final(1);
    return t; }


    Transducer * SfstTransducer::define_transducer(const std::string &symbol)
  { Transducer * t = new Transducer;
    initialize_alphabet(t);
    Node * n = t->new_node();

    unsigned int number;
    if (is_epsilon(symbol))
      number=0;
    else
      number=t->alphabet.add_symbol(symbol.c_str());

    t->root_node()->add_arc(Label(number),n,t);
    n->set_final(1);
    return t; }
  
    Transducer * SfstTransducer::define_transducer
    (const std::string &isymbol, const std::string &osymbol)
  { Transducer * t = new Transducer;
    initialize_alphabet(t);
    Node * n = t->new_node();

    unsigned int inumber,onumber;
    if (is_epsilon(isymbol))
      inumber=0;
    else
      inumber=t->alphabet.add_symbol(isymbol.c_str());
    if (is_epsilon(osymbol))
      onumber=0;
    else
      onumber=t->alphabet.add_symbol(osymbol.c_str());

    t->root_node()->add_arc(Label(inumber,onumber),n,t);
    n->set_final(1);
    return t; }

  Transducer * SfstTransducer::define_transducer(const StringPairVector &spv)
  { Transducer * t = new Transducer;
    initialize_alphabet(t);
    Node * n = t->root_node();
    for (StringPairVector::const_iterator it = spv.begin();
         it != spv.end();
         ++it)
      {
        Node * temp = t->new_node();

        unsigned int inumber,onumber;
        if (is_epsilon(it->first) ||
            strcmp(it->first.c_str(),"<>") == 0 )
          inumber=0;
        else
          inumber=t->alphabet.add_symbol(it->first.c_str());
        if (is_epsilon(it->second) ||
            strcmp(it->second.c_str(),"<>") == 0 )
          onumber=0;
        else
          onumber=t->alphabet.add_symbol(it->second.c_str());

        n->add_arc(Label(inumber,onumber),temp,t);
        n = temp;
      }
    n->set_final(1);
    return t; }

    Transducer * SfstTransducer::define_transducer
    (const StringPairSet &sps, bool cyclic)
  { Transducer * t = new Transducer;
    initialize_alphabet(t);
    Node * n = t->root_node();
    Node * new_node = n;
    if (not sps.empty()) {
      if (not cyclic)
        new_node = t->new_node();
      for (StringPairSet::const_iterator it = sps.begin();
           it != sps.end();
           ++it)
        {
          unsigned int inumber,onumber;
          if (is_epsilon(it->first))
            inumber=0;
          else
            inumber=t->alphabet.add_symbol(it->first.c_str());
          if (is_epsilon(it->second))
            onumber=0;
          else
            onumber=t->alphabet.add_symbol(it->second.c_str());

          n->add_arc(Label(inumber,onumber),new_node,t);
        }
    }
    new_node->set_final(1);
    return t; }

  Transducer * SfstTransducer::define_transducer
  (const std::vector<StringPairSet> &spsv)
  { Transducer * t = new Transducer;
    initialize_alphabet(t);
    Node * n = t->root_node();
    for (std::vector<StringPairSet>::const_iterator it = spsv.begin();
         it != spsv.end();
         ++it)
      {
        Node * temp = t->new_node();

        for (StringPairSet::const_iterator it2 = (*it).begin();
             it2 != (*it).end(); it2++ )
          {
            unsigned int inumber,onumber;
            if (is_epsilon(it2->first) ||
                strcmp(it2->first.c_str(),"<>") == 0 )
              inumber=0;
            else
              inumber=t->alphabet.add_symbol(it2->first.c_str());
            if (is_epsilon(it2->second) ||
                strcmp(it2->second.c_str(),"<>") == 0 )
              onumber=0;
            else
              onumber=t->alphabet.add_symbol(it2->second.c_str());
            
            n->add_arc(Label(inumber,onumber),temp,t);
          }

        n = temp;
      }
    n->set_final(1);
    return t; }

    void SfstTransducer::delete_transducer(Transducer * t)
    { delete t; }

  Transducer * SfstTransducer::copy(Transducer * t)
  { return &t->copy(); }
  
  Transducer * SfstTransducer::determinize(Transducer * t)
  { return &t->determinise(); }
  
  Transducer * SfstTransducer::minimize(Transducer * t)
  { Transducer * retval = &t->minimise(false);
    retval->alphabet.copy(t->alphabet);
    return retval; }
  
  Transducer * SfstTransducer::remove_epsilons(Transducer * t)
  { return &t->remove_epsilons(); }
  
  Transducer * SfstTransducer::repeat_star(Transducer * t)
  { return &t->kleene_star(); }
  
  Transducer * SfstTransducer::repeat_plus(Transducer * t)
  { Transducer * star = repeat_star(t);
    t = &(*t + *star);
    delete star;
    return t; }
  
  Transducer * SfstTransducer::repeat_n(Transducer * t, unsigned int n)
  {
    Transducer * power = create_epsilon_transducer();
    for (unsigned int i = 0; i < n; ++i)
      {
        Transducer * temp = &(*power + *t);
        delete power;
        power = temp;
      }
    return power; }
  
  Transducer * SfstTransducer::repeat_le_n(Transducer * t, unsigned int n)
  {
    Transducer * result = create_empty_transducer();
    for (unsigned int i = 0; i < n+1; ++i)
      {
        Transducer * power = repeat_n(t,i);
        Transducer * temp = &(*power | *result);
        delete power;
        delete result;
        result = temp;
      }
    return result; }
  
  Transducer * SfstTransducer::optionalize(Transducer * t)
  { Transducer *eps = create_epsilon_transducer();
    Transducer *opt = &(*t | *eps);
    delete eps;
    return opt; }
  
  Transducer * SfstTransducer::invert(Transducer * t)
  { return &t->switch_levels(); }
  
  Transducer * SfstTransducer::reverse(Transducer * t)
  { return &t->reverse(); }
  
  Transducer * SfstTransducer::extract_input_language(Transducer * t)
  {
    Transducer * retval = &t->lower_level();

    // projection includes in the alphabet only symbols that
    // occur in the input side, which we do not want

    SFST::Alphabet::CharMap _cm = t->alphabet.get_char_map();
    for (SFST::Alphabet::CharMap::const_iterator it
           = _cm.begin(); it != _cm.end(); it++) {
      retval->alphabet.add_symbol(it->second, it->first);
    }

    // unknowns must be replaced with identities
    Transducer * tmp = retval;
    retval = substitute(retval, internal_unknown, internal_identity);
    delete tmp;

    return retval;
  }
  
  Transducer * SfstTransducer::extract_output_language(Transducer * t)
  { t->complete_alphabet();

    Transducer * retval = &t->upper_level();

    // projection includes in the alphabet only symbols that
    // occur in the output side, which we do not want

    SFST::Alphabet::CharMap _cm = t->alphabet.get_char_map();
    for (SFST::Alphabet::CharMap::const_iterator it
           = _cm.begin(); it != _cm.end(); it++) {
      retval->alphabet.add_symbol(it->second, it->first);
    }

    // unknowns must be replaced with identities
    Transducer * tmp = retval;
    retval = substitute(retval, internal_unknown, internal_identity);
    delete tmp;

    return retval; }

  std::vector<Transducer*> SfstTransducer::extract_path_transducers
  (Transducer *t)
  { vector<Transducer*> paths;

    t->enumerate_paths(paths);

    // paths contains vectors whose alphabet does not have special symbols
    Transducer *foo = define_transducer(internal_epsilon);
    for (unsigned int i=0; i<(unsigned int)paths.size(); i++) {
      (paths[i])->alphabet.copy(t->alphabet);
      std::pair<Transducer*,Transducer*> harm = harmonize(paths[i],foo,false);
      paths[i] = harm.first;
    }
    delete foo;
    return paths;
  }

  static bool extract_paths
  (Transducer * t, Node *node,
   HfstNode2Int &all_visitations, HfstNode2Int &path_visitations,
   hfst::ExtractStringsCb& callback, int cycles,
   std::vector<hfst::FdState<Character> >* fd_state_stack, bool filter_fd,
   StringPairVector &spv)
  {
    if(cycles >= 0 && path_visitations[node] > cycles)
      return true;
    all_visitations[node]++;
    path_visitations[node]++;

    if (spv.size() != 0)
      {
        bool final = node->is_final();
        hfst::HfstTwoLevelPath path(0, spv);
        hfst::ExtractStringsCb::RetVal ret = callback(path, final);
        if(!ret.continueSearch || !ret.continuePath)
          {
            path_visitations[node]--;
            return ret.continueSearch;
          }
      }
    
    // sort arcs by number of visitations
    vector<Arc*> arc;
    for( ArcsIter p(node->arcs()); p; p++ ) {
      Arc *a=p;
      Node *n=a->target_node();
      size_t i;
      for( i=0; i<arc.size(); i++ )
        if (all_visitations[n] < all_visitations[arc[i]->target_node()])
          break;
      arc.push_back(NULL);
      for( size_t k=arc.size()-1; k>i; k-- )
        arc[k] = arc[k-1];
      arc[i] = a;
    }
    
    bool res = true;
    for( size_t i=0; i<arc.size() && res == true; i++ ) {
      Label l = arc[i]->label();
      bool added_fd_state = false;
      
      if (fd_state_stack) {
        if(fd_state_stack->back().get_table().get_operation(l.lower_char())
           != NULL) {
          fd_state_stack->push_back(fd_state_stack->back());
          if(fd_state_stack->back().apply_operation(l.lower_char()))
            added_fd_state = true;
          else {
            fd_state_stack->pop_back();
            continue; // don't follow the transition
          }
        }
      }
            
      /* Handle spv here. Special symbols (flags, epsilons) are always
         inserted. */
      Character lc=l.lower_char();
      Character uc=l.upper_char();
      std::string istring("");
      std::string ostring("");

      if (!filter_fd ||
          fd_state_stack->back().get_table().get_operation(lc) == NULL)
        istring = std::string(t->alphabet.write_char(lc));

      if (!filter_fd ||
          fd_state_stack->back().get_table().get_operation(uc) == NULL)
        ostring = std::string(t->alphabet.write_char(uc));

      if (istring.compare("<>") == 0)
        istring = std::string(internal_epsilon);
      if (ostring.compare("<>") == 0)
        ostring = std::string(internal_epsilon);

      spv.push_back(StringPair(istring, ostring));
    
      res = extract_paths(t, arc[i]->target_node(), all_visitations,
                            path_visitations,
                            callback, cycles,
                            fd_state_stack, filter_fd, spv);
      spv.pop_back();
      
      if(added_fd_state)
        fd_state_stack->pop_back();
    }

    path_visitations[node]--;
    return res;
  }
  
  static const int BUFFER_START_SIZE = 64;
  
    void SfstTransducer::extract_paths
    (Transducer * t, hfst::ExtractStringsCb& callback, int cycles,
     FdTable<SFST::Character>* fd, bool filter_fd)
    {
    if(!t->root_node())
      return;
    
    HfstNode2Int all_visitations;
    HfstNode2Int path_visitations;
    vector<hfst::FdState<Character> >* fd_state_stack =
      (fd==NULL) ? NULL :
      new std::vector<hfst::FdState<Character> >
      (1, hfst::FdState<Character>(*fd));
    
    StringPairVector spv;
    hfst::implementations::extract_paths
      (t, t->root_node(), all_visitations, path_visitations,
       callback, cycles, fd_state_stack, filter_fd,
       spv);

    // add epsilon path, if needed
    if (t->root_node() && t->root_node()->is_final()) {
      StringPairVector empty_spv;
      HfstTwoLevelPath epsilon_path(0, empty_spv);
      callback(epsilon_path, true /* final*/);
    }

  }

  /* Get a random path from transducer \a t. */
  static HfstTwoLevelPath random_path(Transducer *t) {
    
    HfstTwoLevelPath path;
    Node * current_t_node = t->root_node();

    /* If we cannot proceed, all elements in \a path whose index is smaller
       that \a last_index constitute the longest path that is recognized by
       transducer \a t so far. */
    int last_index=0;

    std::vector<SFST::Node*> indexing;
    std::pair<size_t, size_t> number_of_nodes_and_transitions =
      t->nodeindexing(&indexing);
    unsigned int number_of_nodes =
      (unsigned int) number_of_nodes_and_transitions.first;
    
    /* Whether a state has been visited. */
    std::vector<int> visited;
    visited.reserve(number_of_nodes);

    /* Whether the state is marked as broken, i.e. we cannot proceed from
       that state. These arrays are used for giving more probability for
       shorter paths if \a t is cyclic. */
    std::vector<int> broken;
    broken.reserve(number_of_nodes);

    for ( unsigned int i = 0; i < number_of_nodes; ++i ) {
      visited.push_back(0);
      visited.push_back(0);
    }

    while (1) {

      visited[ current_t_node->index ] = 1;
      
      vector<Arc> t_transitions;
      for ( ArcsIter it( current_t_node->arcs() ); it; it++) {
    t_transitions.push_back(*it);
      }
      
      /* If we cannot proceed, return the longest path so far. */
      if (t_transitions.empty() || broken[current_t_node->index]) {
    for (int i=(int)path.second.size()-1; i>=last_index; i--) {
      path.second.pop_back();
    }
    return path;
      }

      /* Go through all transitions in a random order.
     (If \a t is pruned, only one transition is proceeded.) */
      while ( not t_transitions.empty() ) {
    unsigned int index = rand() % t_transitions.size();
    Arc arc = t_transitions.at(index);
    t_transitions.erase(t_transitions.begin()+index);
    
    Node * t_target = arc.target_node();

    std::string istring
      = t->alphabet.code2symbol(arc.label().lower_char());
    std::string ostring
      = t->alphabet.code2symbol(arc.label().upper_char());
    if (istring.compare("<>") == 0)
      istring = std::string(internal_epsilon);
    if (ostring.compare("<>") == 0)
      ostring = std::string(internal_epsilon);

    path.second.push_back
      (StringPair(istring, ostring));
    
    /* If the target state is final, */
    if ( t_target->is_final() ) {
      if ( (rand() % 4) == 0 ) {  // randomly return the path so far,
        return path;
      } // or continue.
      last_index = (int)path.second.size();
    }

    /* Give more probability for shorter paths. */

    if ( broken[ t_target->index ] == 0 ) {
      if ( visited[ t_target->index ] == 1 )
        if ( (rand() % 4) == 0 )
          broken[ t_target->index ] = 1;
    }
    
    if ( visited[ t_target->index ] == 1 ) {
      if ( (rand() % 4) == 0 )
        broken[ t_target->index ] = 1;
    }

    /* Proceed to the target state. */
    current_t_node = t_target;
    break;
      }
    }
    return path;
  };
    
    static bool is_minimal_and_empty(Transducer *t)
    {
      for( ArcsIter p(t->root_node()->arcs()); p; p++ ) {
        return false;
      }
      return true;
    }

    // FIX: keeps returning epsilon paths...
  void SfstTransducer::extract_random_paths
  (Transducer *t, HfstTwoLevelPaths &results, int max_num)
  {
    // empty transducer
    if (is_minimal_and_empty(t))
      {
        return;
      }
    bool is_epsilon_path_accepted =
      t->root_node()->is_final();

    srand((unsigned int)(time(0)));

    while (max_num > 0) {
      HfstTwoLevelPath path = random_path(t);

      /* If we extract the same path again, try at most 5 times
     to extract another one. */
      unsigned int i = 0;
      while ( (results.find(path) != results.end()) and (i < 5) ) {
        path = random_path(t);
        ++i;
      }
      if (!is_epsilon_path_accepted && path.second.size() == 0)
        {
          fprintf(stderr, "wrong epsilon path returned, retrying...\n");
          continue;
        }
      results.insert(path);

      --max_num;
    }
  }

  Transducer * SfstTransducer::insert_freely
  (Transducer * t, const StringPair &symbol_pair)
  {
    std::string isymbol = symbol_pair.first;
    std::string osymbol = symbol_pair.second;
    if (is_epsilon(isymbol))
      isymbol = std::string("<>");
    if (is_epsilon(osymbol))
      osymbol = std::string("<>");

    return &t->freely_insert
      ( Label( t->alphabet.add_symbol(isymbol.c_str()),
               t->alphabet.add_symbol(osymbol.c_str()) ));
  }

  
  Transducer * SfstTransducer::substitute
  (Transducer * t,String old_symbol,String new_symbol)
  {
    std::string old_symbol_ = old_symbol;
    std::string new_symbol_ = new_symbol;
    if (is_epsilon(old_symbol))
      old_symbol_ = std::string("<>");
    if (is_epsilon(new_symbol))
      new_symbol_ = std::string("<>");

    Transducer * retval =
      &t->replace_char(t->alphabet.add_symbol(old_symbol_.c_str()),
                       t->alphabet.add_symbol(new_symbol_.c_str()));
    retval->alphabet.copy(t->alphabet);
    return retval; }

  Transducer * SfstTransducer::substitute
  (Transducer *t, const StringPair &symbol_pair, Transducer *tr)
  {
    std::string isymbol = symbol_pair.first;
    std::string osymbol = symbol_pair.second;
    if (is_epsilon(isymbol))
      isymbol = std::string("<>");
    if (is_epsilon(osymbol))
      osymbol = std::string("<>");

    Transducer * retval
      = &t->splice( Label(
                          t->alphabet.add_symbol(isymbol.c_str()),
                          t->alphabet.add_symbol(osymbol.c_str()) ), tr );
    retval->alphabet.copy(t->alphabet);
    return retval;
  }

  
  Transducer * SfstTransducer::compose
  (Transducer * t1, Transducer * t2)
  {
    return &t1->operator||(*t2); }

  Transducer * SfstTransducer::concatenate
  (Transducer * t1, Transducer * t2)
  { return &t1->operator+(*t2); }

  Transducer * SfstTransducer::disjunct
  (Transducer * t1, Transducer * t2)
  { return &t1->operator|(*t2); }

  Transducer * SfstTransducer::disjunct
  (Transducer * t, const StringPairVector &spv)
  {
    Node *node= t->root_node();
    for (StringPairVector::const_iterator it = spv.begin();
         it != spv.end(); it++)
      {
        unsigned int inumber,onumber;
        if (is_epsilon(it->first) ||
            strcmp(it->first.c_str(),"<>") == 0 )
          inumber=0;
        else
          inumber=t->alphabet.add_symbol(it->first.c_str());
        if (is_epsilon(it->second) ||
            strcmp(it->second.c_str(),"<>") == 0 )
          onumber=0;
        else
          onumber=t->alphabet.add_symbol(it->second.c_str());

        Label l(inumber, onumber);
        t->alphabet.insert(l);
        Arcs *arcs=node->arcs();
        node = arcs->target_node( l );
        if (node == NULL) {
          node = t->new_node();
          arcs->add_arc( l, node, t );
        }
    }
    node->set_final(1);
    return t;
  }

  Transducer * SfstTransducer::intersect
  (Transducer * t1, Transducer * t2)
  { return &t1->operator&(*t2); }

  Transducer * SfstTransducer::subtract
  (Transducer * t1, Transducer * t2)
  {
    try {
      unsigned int t1_alphabet_size = t1->alphabet.size();
      // This will cause an exception when SFST calculates the negation
      // that is needed in subtraction.
      if (t1_alphabet_size == 0)  {
    t1->alphabet.insert(Label(1,1)); // insert a dummy symbol pair
      }
      Transducer * retval = &t1->operator/(*t2);
      if (t1_alphabet_size == 0) {
    t1->alphabet.clear_char_pairs(); // remove the dummy symbol pair
    t1->complete_alphabet();
      }
      return retval;
    }
    catch (const char *msg) {
      fprintf(stderr, "ERROR: %s\n", msg);
      HFST_THROW_MESSAGE(HfstFatalException, std::string(msg));
    }
  }

  bool SfstTransducer::are_equivalent(Transducer * t1, Transducer * t2)
  {
    return (*t1 == *t2);
  }
  
  bool SfstTransducer::is_cyclic(Transducer * t)
  {
    return t->is_cyclic();
  }

  bool SfstTransducer::is_automaton(Transducer * t)
  {
    return t->is_automaton();
  }


    FdTable<SFST::Character>* SfstTransducer::get_flag_diacritics(Transducer * t)
  {
    FdTable<SFST::Character>* table = new FdTable<SFST::Character>();
    SFST::Alphabet::CharMap cm = t->alphabet.get_char_map();
    for (SFST::Alphabet::CharMap::const_iterator it
           = cm.begin(); it != cm.end(); it++) {
      if(FdOperation::is_diacritic(it->second))
        table->define_diacritic(it->first, it->second);
    }
    return table;
  }

  void SfstTransducer::insert_to_alphabet
    (Transducer * t, const std::string &symbol)
  {
    t->alphabet.add_symbol(symbol.c_str());
  }

  void SfstTransducer::remove_from_alphabet
  (Transducer * t, const std::string &symbol)
  {
    SFST::Alphabet & alpha = t->alphabet;
    const char * symbol_to_remove = symbol.c_str();

    std::vector<char*> sym;
    std::vector<SFST::Character> code;
    std::vector<SFST::Label> label;

    SFST::Alphabet::CharMap cm = alpha.get_char_map();
    for( SFST::Alphabet::CharMap::const_iterator it=cm.begin();
     it!=cm.end(); it++ ) {
      SFST::Character c=it->first;
      char *s=it->second;
      if (strcmp(s, symbol_to_remove) != 0) {
    sym.push_back(fst_strdup(s));
    code.push_back(c);
      }
    }
    
    for( std::set<SFST::Label>::const_iterator it=alpha.begin();
     it!=alpha.end(); it++ ) {
      SFST::Label l=*it;
      if (strcmp(alpha.code2symbol(l.upper_char()), symbol_to_remove) != 0 &&
      strcmp(alpha.code2symbol(l.lower_char()), symbol_to_remove) != 0  ) {
    label.push_back(l);
      }
    }
    
    alpha.clear();

    for( size_t i=0; i<sym.size(); i++ ) {
      alpha.add_symbol(sym[i], code[i]);
      free(sym[i]);
    }
    for( size_t i=0; i<label.size(); i++ )
      alpha.insert( label[i] );
  }

  StringSet SfstTransducer::get_alphabet(Transducer * t)
  {
    StringSet s;
    SFST::Alphabet::CharMap cm = t->alphabet.get_char_map();
    for ( SFST::Alphabet::CharMap::const_iterator it = cm.begin();
          it != cm.end(); it++ ) {
      if (strcmp(it->second, "<>") == 0)
        s.insert(internal_epsilon);
      else
        s.insert( std::string(it->second) );
    }
    return s;
  }

    unsigned int SfstTransducer::get_symbol_number
    (Transducer *t,
     const std::string &symbol)
  {
    if (symbol == "@_EPSILON_SYMBOL_@")
      return 0;
    int i = t->alphabet.symbol2code(symbol.c_str());
    if (i == EOF) {
      HFST_THROW(SymbolNotFoundException);
    }
    return (unsigned int)i;
  }

  StringPairSet SfstTransducer::get_symbol_pairs(Transducer *t)
  {
    StringPairSet s;
    t->alphabet.clear_char_pairs();
    t->complete_alphabet();
    for (SFST::Alphabet::const_iterator it = t->alphabet.begin();
         it != t->alphabet.end(); it++)
      {
        const char * isymbol = t->alphabet.code2symbol(it->lower_char());
        const char * osymbol = t->alphabet.code2symbol(it->upper_char());

        if (isymbol == NULL) {
          HFST_THROW_MESSAGE(HfstFatalException, "input number not found");
          //fprintf(stderr, "ERROR: input number %i not found\n",
          //        it->lower_char());
          //exit(1);
        }
        if (osymbol == NULL) {
          HFST_THROW_MESSAGE(HfstFatalException, "output number not found");
          //fprintf(stderr, "ERROR: input number %i not found\n",
          //        it->upper_char());
          //exit(1);
        }

        std::string istring(isymbol);
        std::string ostring(osymbol);
        if (istring.compare("<>") == 0)
          istring = std::string("<>");
        if (ostring.compare("<>") == 0)
          ostring = std::string("<>");
        
          s.insert(StringPair(istring,
                              ostring
                              ));
      }
    return s;
  }

  /* Expand a transition according to the previously unknown symbols in s. */
  void SfstTransducer::expand_node
  ( Transducer *t, Node *origin, Label &l, Node *target, hfst::StringSet &s )
  {
    if ( l.lower_char() == 1 && l.upper_char() == 1 )     // cross product "?:?"
      {
        for (hfst::StringSet::iterator it1 = s.begin(); it1 != s.end(); it1++)
          {
        if (not FdOperation::is_diacritic(*it1)) {

          int inumber = t->alphabet.symbol2code(it1->c_str());
          if (inumber == -1) {
        std::cerr << "ERROR: no number for symbol " << *it1
              << std::endl;
        assert(false);
          }
          for (hfst::StringSet::iterator it2 = s.begin();
           it2 != s.end(); it2++)
        {
          if (not FdOperation::is_diacritic(*it2)) {
            int onumber = t->alphabet.symbol2code(it2->c_str());
            if (onumber == -1) {
              std::cerr << "ERROR: no number for symbol " << *it2
                << std::endl;
              assert(false);
            }
            if (inumber != onumber) {
              // add transitions of type x:y
              // (non-identity cross-product of symbols in s)
              origin->add_arc( Label(inumber, onumber), target, t );
            }
          }
        }
          // add transitions of type x:? and ?:x here
          origin->add_arc( Label(inumber, 1), target, t );
          origin->add_arc( Label(1, inumber), target, t );
        }
      }
      }
    else if (l.lower_char() == 2 || l.upper_char() == 2 )  // identity "?:?"
      {
        for (hfst::StringSet::iterator it = s.begin(); it != s.end(); it++)
          {
        if (not FdOperation::is_diacritic(*it)) {
          int number = t->alphabet.symbol2code(it->c_str());
          if (number == -1) {
        std::cerr << "ERROR: no number for symbol " << *it
              << std::endl;
        assert(false);
          }
          // add transitions of type x:x
          origin->add_arc( Label(number, number), target, t );
        }
      }
      }
    else if (l.lower_char() == 1)  // "?:x"
      {
        for (hfst::StringSet::iterator it = s.begin(); it != s.end(); it++)
          {
        if (not FdOperation::is_diacritic(*it)) {
          int number = t->alphabet.symbol2code(it->c_str());
          if (number == -1) {
        std::cerr << "ERROR: no number for symbol " << *it
              << std::endl;
        assert(false);
          }
          origin->add_arc( Label(number, l.upper_char()), target, t );
        }
          }
      }
    else if (l.upper_char() == 1)  // "x:?"
      {
        for (hfst::StringSet::iterator it = s.begin(); it != s.end(); it++)
          {
        if (not FdOperation::is_diacritic(*it)) {
          int number = t->alphabet.symbol2code(it->c_str());
          if (number == -1) {
        std::cerr << "ERROR: no number for symbol " << *it
              << std::endl;
        assert(false);
          }
          origin->add_arc( Label(l.lower_char(), number), target, t );
        }
      }
      }
    // keep the original transition in all cases
    return;
  }

  /*******************************************************************/
  /*                                                                 */
  /*  HFST addition                                                  */
  /*  Transducer::expand_nodes                                       */
  /*                                                                 */
  /*******************************************************************/

  void SfstTransducer::expand2
  ( Transducer *t, Node *node,
    hfst::StringSet &new_symbols, std::set<Node*> &visited_nodes )
  {
    if (visited_nodes.find(node) == visited_nodes.end()) {
      visited_nodes.insert(node);
      // iterate over all outgoing arcs of node
      for( ArcsIter p(node->arcs()); p; p++ ) {
        Arc *arc=p;
        expand2(t, arc->target_node(), new_symbols, visited_nodes);
        Label l = arc->label();
        expand_node( t, node, l, arc->target_node(), new_symbols);
      }
    }
    return;
  }
    
  /* Expand all transitions according to the previously unknown symbols
     listed in new_symbols. */
  void SfstTransducer::expand(Transducer *t, hfst::StringSet &new_symbols)
  {
    std::set<Node*> visited_nodes;
    expand2(t, t->root_node(), new_symbols, visited_nodes);
  }


} }

#else // MAIN_TEST was defined
using namespace hfst::implementations;
#include <iostream>

bool does_sfst_alphabet_contain(SFST::Transducer *t, const char *str)
{
  SFST::Alphabet::CharMap cm = t->alphabet.get_char_map();
  for (SFST::Alphabet::CharMap::const_iterator it = cm.begin();
       it != cm.end(); it++) {
    if (strcmp(str, it->second) == 0) {
      return true; }
  }
  return false;
}

int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":";

    // Test alphabet pruning
    SFST::Transducer * t = SfstTransducer::define_transducer("a", "b");

    SFST::Transducer * t_input = SfstTransducer::extract_input_language(t);
    assert( does_sfst_alphabet_contain(t_input, "a") &&
        does_sfst_alphabet_contain(t_input, "b")  );

    SFST::Transducer * t_output = SfstTransducer::extract_output_language(t);
    assert( does_sfst_alphabet_contain(t_output, "a") &&
        does_sfst_alphabet_contain(t_output, "b")  );

    SFST::Transducer * t_min = SfstTransducer::minimize(t_input);
    assert( does_sfst_alphabet_contain(t_min, "a") &&
        does_sfst_alphabet_contain(t_min, "b")  );

    SFST::Transducer * t_eps_free = SfstTransducer::remove_epsilons(t_output);
    assert( does_sfst_alphabet_contain(t_eps_free, "a") &&
        does_sfst_alphabet_contain(t_eps_free, "b")  );

    SFST::Transducer * t_subst = SfstTransducer::substitute(t, "a", "c");
    assert( does_sfst_alphabet_contain(t_subst, "a") &&
        does_sfst_alphabet_contain(t_subst, "b")  &&
        does_sfst_alphabet_contain(t_subst, "c") );

    std::cout << std::endl << "ok" << std::endl;
    return EXIT_SUCCESS;
}
#endif // MAIN_TEST

