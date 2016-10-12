// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#ifndef _FOMALIB_H_
#define _FOMALIB_H_
#include "back-ends/foma/fomalib.h"
#endif

#include "FomaTransducer.h"

#ifndef MAIN_TEST
namespace hfst { namespace implementations {


  // ---------- FomaInputStream functions ----------

    /** Create a FomaInputStream that reads from stdin. */
  FomaInputStream::FomaInputStream(void)
  {
    input_file = stdin;
  }
    /** Create a FomaInputStream that reads from file \a filename. */
    FomaInputStream::FomaInputStream(const std::string &filename_):
      filename(std::string(filename_))
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
  void FomaInputStream::close(void)
  {
    if (input_file == NULL)
      { return; }
    if (filename.c_str()[0] != 0)
      {
    fclose(input_file);
    input_file = NULL;
      }
  }
  
  char FomaInputStream::stream_get() {
    return (char) fgetc(input_file); }

  short FomaInputStream::stream_get_short()
  {
    short i;
    assert(1 == fread(&i,sizeof(i),1,input_file));
    return i;
  }

  void FomaInputStream::stream_unget(char c) {
    ungetc ( (int)c, input_file ); }

  bool FomaInputStream::is_eof(void)
  {
    int c = getc(input_file);
    bool retval = (feof(input_file) != 0);
    ungetc(c, input_file);
    return retval;
  }
  
  bool FomaInputStream::is_bad(void)
  {
    return is_eof();
  }
  
  bool FomaInputStream::is_good(void)
  {
    return ! is_bad();
  };
  
  bool FomaInputStream::is_fst(void)
  {
    return is_fst(input_file);
  }
  
  bool FomaInputStream::is_fst(FILE * f)
  {
    if (f == NULL)
      { return false; }
    int c = getc(f);
    ungetc(c, f);
    return c == 31 || c == (int)'#';
  }
  
  bool FomaInputStream::is_fst(std::istream &s)
  {
    int c = s.peek();
    return s.good() && (c == 31 || c == (int)'#');
  }

  /* Skip the identifier string "FOMA_TYPE" */
  void FomaInputStream::skip_identifier_version_3_0(void)
  {
    char foma_identifier[10];
    int foma_id_count = fread(foma_identifier,10,1,input_file);
    if (foma_id_count != 1)
      {
        HFST_THROW(NotTransducerStreamException); }
    if (0 != strcmp(foma_identifier,"FOMA_TYPE"))
      {
        HFST_THROW(NotTransducerStreamException); }
  }
  
  void FomaInputStream::skip_hfst_header(void)
  {
    char hfst_header[6];
    int header_count = fread(hfst_header,6,1,input_file);
    if (header_count != 1)
      {
        HFST_THROW(NotTransducerStreamException); }
    try { skip_identifier_version_3_0(); }
    catch (const HfstException e)
      { throw e; }
  }

    void FomaInputStream::ignore(unsigned int n)
    {
      for (unsigned int i=0; i<n; i++)
    fgetc(input_file);
    }

  fsm * FomaInputStream::read_transducer()
  {
    if (is_eof())
      return NULL;
    fsm * t = FomaTransducer::read_net(input_file);
    if (t == NULL) {
      HFST_THROW(NotTransducerStreamException);
    }
    return t;
  };

  // ---------- FomaOutputStream functions ----------

  FomaOutputStream::FomaOutputStream(void)
  { ofile = stdout; }

    FomaOutputStream::FomaOutputStream(const std::string &str):
      filename(std::string(str))
  {
    if (filename != std::string()) {
      ofile = fopen(filename.c_str(), "wb");
      if (ofile == NULL) {
        HFST_THROW(StreamNotReadableException);
      }
    }
    else {
      ofile = stdout;
    }
  }

  void FomaOutputStream::close(void)
  {
    if (filename != std::string())
      { fclose(ofile); }
  }

  void FomaOutputStream::write(const char &c)
  {
    fputc(c,ofile);
  }
    
    void FomaOutputStream::write_transducer(fsm * transducer)
  {
    if (1 != FomaTransducer::write_net(transducer, ofile)) {
      HFST_THROW_MESSAGE
        (HfstFatalException,
         "an error happened when writing a foma transducer");
    }
  }

  
  fsm * FomaTransducer::create_empty_transducer(void)
  {
    fsm * retval = fsm_empty_set();
    return retval;
  }
  
  fsm * FomaTransducer::create_epsilon_transducer(void)
  {
    return fsm_empty_string();
  }
  
  void FomaTransducer::harmonize(fsm *net1, fsm *net2)
  {
    fsm_merge_sigma(net1, net2);
  }

  void FomaTransducer::delete_foma(fsm * net)
  {
    fsm_destroy(net);
  }

    fsm * FomaTransducer::define_transducer(const std::string &symbol)
  {
    /*if (symbol == hfst::internal_unknown) {
      return define_transducer(symbol, symbol);
      }*/
    fsm * retval = fsm_symbol(const_cast<char*>(symbol.c_str()));
    fsm_count(retval);
    return retval;
  }

  fsm * FomaTransducer::define_transducer
    (const std::string &isymbol, const std::string &osymbol)
  {
    // identity-to-identity gives wrong result if cross product is used
    if (isymbol == osymbol && isymbol == hfst::internal_identity) {
      return define_transducer(isymbol);
    }
    return fsm_cross_product( fsm_symbol(const_cast<char*>(isymbol.c_str())),
                              fsm_symbol(const_cast<char*>(osymbol.c_str())) );
    // should either argument be deleted?
  }
  
  fsm * FomaTransducer::define_transducer(const StringPairVector &spv)
  {
    if (spv.empty())
      return fsm_empty_string();
    
    int state_number=0;

    struct fsm_construct_handle *h;
    fsm *net;
    h = fsm_construct_init(strdup(std::string("").c_str()));
    
    for (StringPairVector::const_iterator it = spv.begin();
         it != spv.end(); it++)
      {
        char *in = strdup(it->first.c_str());
        char *out = strdup(it->second.c_str());
        fsm_construct_add_arc(h, state_number, state_number+1, in, out);
        state_number++;
      }
    
    fsm_construct_set_initial(h, 0);
    fsm_construct_set_final(h, state_number);

    net = fsm_construct_done(h);
    fsm_count(net);

    sigma_add_special (0, net->sigma);
    sigma_add_special (1, net->sigma);
    sigma_add_special (2, net->sigma);
    
    return net;
  }

  fsm * FomaTransducer::define_transducer(const StringPairSet &sps, bool cyclic)
  {
    if (sps.empty())
      return fsm_empty_string();
    
    int source=0;
    int target = (cyclic) ? 0 : 1;

    struct fsm_construct_handle *h;
    fsm *net;
    h = fsm_construct_init(strdup(std::string("").c_str()));
    
    for (StringPairSet::const_iterator it = sps.begin(); it != sps.end(); it++)
      {
    char *in = strdup(it->first.c_str());
    char *out = strdup(it->second.c_str());
    fsm_construct_add_arc(h, source, target, in, out);
      }
    
    fsm_construct_set_initial(h, source);
    fsm_construct_set_final(h, target);

    net = fsm_construct_done(h);
    fsm_count(net);
    
    return net;
  }

  fsm * FomaTransducer::define_transducer
    (const std::vector<StringPairSet> &spsv)
  {
    if (spsv.empty())
      return fsm_empty_string();
    
    int state_number=0;

    struct fsm_construct_handle *h;
    fsm *net;
    h = fsm_construct_init(strdup(std::string("").c_str()));
    
    for (std::vector<StringPairSet>::const_iterator it = spsv.begin();
         it != spsv.end(); it++)
      {
        for (StringPairSet::const_iterator it2 = (*it).begin();
             it2 != (*it).end(); it2++ ) {
          char *in = strdup(it2->first.c_str());
          char *out = strdup(it2->second.c_str());
          fsm_construct_add_arc(h, state_number, state_number+1, in, out);
        }
        state_number++;
      }
    
    fsm_construct_set_initial(h, 0);
    fsm_construct_set_final(h, state_number);

    net = fsm_construct_done(h);
    fsm_count(net);
    
    return net;
  }

  fsm * FomaTransducer::copy(fsm * t)
  {
    return fsm_copy(t);
  }
  
  fsm * FomaTransducer::determinize(fsm * t)
  {
    // returns the argument, so a copy is taken here
    return fsm_determinize(fsm_copy(t));
  }
  
  fsm * FomaTransducer::minimize(fsm * t)
  {
    // returns the argument, so a copy is taken here
    return fsm_minimize(fsm_copy(t));
  }
  
  fsm * FomaTransducer::remove_epsilons(fsm * t)
  {
    // returns the argument, so a copy is taken here
    return fsm_epsilon_remove(fsm_copy(t));
  }
  
  fsm * FomaTransducer::repeat_star(fsm * t)
  {
    return fsm_kleene_star(fsm_copy(t));
  }
  
  fsm * FomaTransducer::repeat_plus(fsm * t)
  {
    return fsm_kleene_plus(fsm_copy(t));
  }
  
  fsm * FomaTransducer::repeat_n(fsm * t, unsigned int n)
  {
    return fsm_concat_n(fsm_copy(t), n);
  }
  
  fsm * FomaTransducer::repeat_le_n(fsm * t, unsigned int n)
  {
    return fsm_concat_m_n(fsm_copy(t),0,n);
  }
  
  fsm * FomaTransducer::optionalize(fsm * t)
  {
    return fsm_optionality(fsm_copy(t));
  }
  
  fsm * FomaTransducer::invert(fsm * t)
  {
    return fsm_invert(fsm_copy(t));
  }
  
  fsm * FomaTransducer::reverse(fsm * t)
  {
    return fsm_reverse(fsm_copy(t));
  }
  
  fsm * FomaTransducer::extract_input_language(fsm * t)
  {
    // foma does not handle epsilon transducer properly..
    return fsm_upper(fsm_copy(t));
  }
  
  fsm * FomaTransducer::extract_output_language(fsm * t)
  {
    // foma does not handle epsilon transducer properly..
    return fsm_lower(fsm_copy(t));
  }
  
  fsm * FomaTransducer::substitute(fsm * t,String old_symbol,String new_symbol)
  {
    return fsm_substitute_symbol(t,
                                 strdup(old_symbol.c_str()),
                                 strdup(new_symbol.c_str()));
  }
  
    /* Conversion to HfstBasicTransducer is now used instead. */
  fsm * FomaTransducer::insert_freely(fsm * t, const StringPair &symbol_pair)
  {
    const char * epsilon = internal_epsilon.c_str();
    char * epsilon_marker = strdup("@_EPSILON_SYMBOL_MARKER_@");
    const char * identity = internal_identity.c_str();
    fsm * eps_marked =
      fsm_substitute_symbol(t, const_cast<char*>(epsilon),
                epsilon_marker);
    fsm * ins = fsm_kleene_star
      (
       fsm_union(
         fsm_symbol(const_cast<char*>(identity)),
         fsm_cross_product
         ( fsm_symbol(const_cast<char*>(epsilon)),
           fsm_symbol(const_cast<char*>(symbol_pair.second.c_str())
                  ))));
    fsm * comp = fsm_substitute_symbol
      ( fsm_compose(eps_marked, ins),
    const_cast<char*>(epsilon),
    const_cast<char*>(symbol_pair.first.c_str()));
  return fsm_substitute_symbol( comp,
                epsilon_marker,
                const_cast<char*>(epsilon));
  free(epsilon_marker);
  // marker should be removed from sigma..
  // (HfstBasicTransducer is now used instead)
  }
  
  fsm * FomaTransducer::compose
  (fsm * t1, fsm * t2)
  {
    return fsm_compose(fsm_copy(t1), fsm_copy(t2));
  }

  fsm * FomaTransducer::concatenate
  (fsm * t1, fsm * t2)
  {
    return fsm_concat(fsm_copy(t1), fsm_copy(t2));
  }

  fsm * FomaTransducer::disjunct
  (fsm * t1, fsm * t2)
  {
    return fsm_union(fsm_copy(t1), fsm_copy(t2));
  }

  fsm * FomaTransducer::intersect
  (fsm * t1, fsm * t2)
  {
    return fsm_intersect(fsm_copy(t1), fsm_copy(t2));
  }

  fsm * FomaTransducer::subtract
  (fsm * t1, fsm * t2)
  {
    return fsm_minus(fsm_copy(t1), fsm_copy(t2));
  }

  bool FomaTransducer::are_equivalent
  (fsm * t1, fsm * t2)
  {
    return fsm_isempty(fsm_union(fsm_minus(fsm_copy(t1),fsm_copy(t2)),
                                 fsm_minus(fsm_copy(t2),fsm_copy(t1))));
  }

  bool FomaTransducer::is_cyclic(fsm * t)
  {
    fsm_topsort(t);
    return !(t->is_loop_free);
  }
  
    unsigned int FomaTransducer::number_of_states(fsm * t)
    {
      unsigned int retval=0;
      int laststate = -1;
      for(int i=0; ((t->states)+i)->state_no != -1; i++)
        {
          if (((t->states)+i)->state_no != laststate)
            retval++;
          laststate = ((t->states)+i)->state_no;
        }
      return retval;
    }

    unsigned int FomaTransducer::number_of_arcs(fsm * t)
    {
      unsigned int retval=0;
      for(int i=0; ((t->states)+i)->state_no != -1; i++)
        {
          if (((t->states)+i)->in != -1)
            retval++;
        }
      return retval;
    }

  
  static bool extract_paths
  (fsm * t, int state,
   std::map<int,unsigned short> all_visitations,
   std::map<int, unsigned short> path_visitations,
   ExtractStringsCb& callback, int cycles,
   std::vector<hfst::FdState<int> >* fd_state_stack,
   bool filter_fd,
   StringPairVector &spv)
  {

    // Number of cycles exceeded
    if(cycles >= 0 && path_visitations[state] > cycles)
      return true;

    // Increment number of visitations
    all_visitations[state]++;
    path_visitations[state]++;
    
    if (spv.size() != 0)
      {
        //check finality
        bool final = false;
        for(int i=0; ((t->states)+i)->state_no != -1; i++)
          {
            fsm_state* s = (t->states)+i;
            if(s->state_no == state && s->final_state == 1)
              {
                final = true;
                break;
              }
          }
        
        hfst::HfstTwoLevelPath path(float(0), spv);
        hfst::ExtractStringsCb::RetVal ret = callback(path, final);
        if(!ret.continueSearch || !ret.continuePath)
          {
            path_visitations[state]--;
            return ret.continueSearch;
          }
      }
    

    // find and sort transitions
    std::vector<fsm_state*> sorted_arcs;
    for(int i=0; ((t->states)+i)->state_no != -1; i++)
    {
      fsm_state* s = (t->states)+i;
      if(s->state_no == state && s->target != -1)
      {
        size_t j;
        for(j=0; j<sorted_arcs.size(); j++)
          if (all_visitations[s->target]
              < all_visitations[sorted_arcs[j]->target])
            break;
        sorted_arcs.push_back(NULL);
        for( size_t k=sorted_arcs.size()-1; k>j; k-- )
          sorted_arcs[k] = sorted_arcs[k-1];
        sorted_arcs[j] = s;
      }
    }

    bool res = true;
    for(size_t i=0; i<sorted_arcs.size() && res == true; i++)
    {
      fsm_state* arc = sorted_arcs[i];
      
      bool added_fd_state = false;
      
      if (fd_state_stack) {
        if(fd_state_stack->back().get_table().get_operation(arc->in) != NULL) {
          fd_state_stack->push_back(fd_state_stack->back());
          if(fd_state_stack->back().apply_operation(arc->in))
            added_fd_state = true;
          else {
            fd_state_stack->pop_back();
            continue; // don't follow the transition
          }
        }
      }
      
      /* Handle spv here. Special symbols (flags, epsilons)
         are always inserted. */

      std::string istring("");
      std::string ostring("");
    
      //find the key in sigma
      char* c_in=NULL;
      for(struct sigma* sig=t->sigma; sig!=NULL&&sig->symbol!=NULL;
          sig=sig->next)
        { if(sig->number == arc->in) {
            c_in = sig->symbol;
            break; }
        }

      //find the key in sigma
      char* c_out=NULL;
      for(struct sigma* sig=t->sigma; sig!=NULL&&sig->symbol!=NULL;
          sig=sig->next) {
        if(sig->number == arc->out) {
          c_out = sig->symbol;
          break; }
      }

      if (!filter_fd || ( fd_state_stack != NULL &&
                          fd_state_stack->back().get_table().get_operation(arc->in)==NULL)) {
        assert(c_in != NULL);
        istring = strdup(c_in);
      }

      if (!filter_fd ||
          fd_state_stack->back().get_table().get_operation(arc->out)==NULL) {
        assert(c_out != NULL);
        ostring = strdup(c_out);
      }

      spv.push_back(StringPair(istring, ostring));

      res = extract_paths(t, arc->target, all_visitations, path_visitations,
                            callback, cycles,
                            fd_state_stack, filter_fd,
                            spv);

      spv.pop_back();

      if(added_fd_state)
        fd_state_stack->pop_back();
    }
    
    path_visitations[state]--;
    return res;
  }
  
  static const int BUFFER_START_SIZE = 64;
  
  void FomaTransducer::extract_paths
  (fsm * t, ExtractStringsCb& callback,
   int cycles, FdTable<int>* fd, bool filter_fd)
  {
    std::map<int, unsigned short> all_visitations;
    std::map<int, unsigned short> path_visitations;
    std::vector<hfst::FdState<int> >* fd_state_stack
      = (fd==NULL) ? NULL : new std::vector<hfst::FdState<int> >
      (1, hfst::FdState<int>(*fd));

    std::set<int> initial_states;
    
    StringPairVector spv;
    bool res = true;
    for (int i=0; ((t->states)+i)->state_no != -1 && res == true; i++) {
      if (((t->states)+i)->start_state == 1 &&
          (initial_states.find(((t->states)+i)->state_no)
           == initial_states.end()) ) {

        initial_states.insert(((t->states)+i)->state_no);
                
        res = hfst::implementations::extract_paths
          (t, ((t->states)+i)->state_no, all_visitations, path_visitations,
           callback, cycles, fd_state_stack,
           filter_fd, spv);
      }
    }
    // add epsilon path, if needed
    for (std::set<int>::const_iterator it = initial_states.begin();
         it != initial_states.end(); it++)
      {
        bool final_initial = false;
        for(int i=0; ((t->states)+i)->state_no != -1; i++)
          {
            fsm_state* s = (t->states)+i;
            if(s->state_no == *it && s->final_state == 1)
              {
                final_initial = true;
                break;
              }
          }
        if (final_initial) {
          StringPairVector empty_spv;
          HfstTwoLevelPath epsilon_path(float(0), empty_spv);
          callback(epsilon_path, true /* final*/);
        }
      }

  }

    void FomaTransducer::extract_random_paths
    (const fsm *t , HfstTwoLevelPaths &results, int max_num)
    {
      (void)t;
      (void)results;
      (void)max_num;
      HFST_THROW(FunctionNotImplementedException);
    }
  
  void FomaTransducer::insert_to_alphabet(fsm * t, const std::string &symbol)
  {
    sigma_add(strdup(symbol.c_str()), t->sigma);
  }

  void FomaTransducer::remove_from_alphabet
  (fsm * t, const std::string &symbol)
  {
    sigma_remove(strdup(symbol.c_str()), t->sigma);
  }

  StringSet FomaTransducer::get_alphabet(fsm *t)
  {
    StringSet alpha;
    for(struct sigma* p = t->sigma; p!=NULL; p=p->next)
    {
      if (p->symbol == NULL)
        break;
      alpha.insert(std::string(p->symbol));
    }
    // it seems that the specail symbols are not always included, but foma
    // is still aware of them..
    alpha.insert(internal_epsilon);
    alpha.insert(internal_unknown);
    alpha.insert(internal_identity);
    return alpha;
  }
    
  unsigned int FomaTransducer::get_symbol_number
  (fsm *t,
   const std::string &symbol)
  {
    if (symbol == internal_epsilon)
      return 0;
    if (symbol == internal_unknown)
      return 1;
    if (symbol == internal_identity)
      return 2;
    const char * c = symbol.c_str();
    for(struct sigma* p = t->sigma; p!=NULL; p=p->next)
      {
    if (p->symbol == NULL)
      break;
    if (strcmp(p->symbol, c) == 0)
      return (unsigned int)p->number;
      }
    HFST_THROW(SymbolNotFoundException);
  }

    unsigned int FomaTransducer::get_biggest_symbol_number(fsm * t)
    {
      unsigned int biggest_number=0;
      for(struct sigma* p = t->sigma; p!=NULL; p=p->next)
        {
          if (p->symbol == NULL)
            break;
          if (biggest_number < (unsigned int)p->number)
            biggest_number = (unsigned int)p->number;
        }
      // epsilon, unknown and identity are always included and
      // get_symbol_number always returns a value for them
      if (biggest_number < 2)
        return 2;
      return biggest_number;
    }

    StringVector FomaTransducer::get_symbol_vector
    (fsm * t)
    {
      unsigned int biggest_symbol_number = get_biggest_symbol_number(t);

      StringVector symbol_vector;
      symbol_vector.reserve(biggest_symbol_number+1);
      symbol_vector.resize(biggest_symbol_number+1,"");

      StringSet alphabet = get_alphabet(t);
      for (StringSet::const_iterator it = alphabet.begin(); it != alphabet.end(); it++)
        {
          unsigned int symbol_number = get_symbol_number(t, *it);
          symbol_vector.at(symbol_number) = *it;
        }
      return symbol_vector;
    }

    std::map<std::string, unsigned int> FomaTransducer::get_symbol_map
    (fsm * t)
    {
      StringSet alphabet = get_alphabet(t);
      std::map<std::string, unsigned int> symbol_map;
      for (StringSet::const_iterator it = alphabet.begin(); it != alphabet.end(); it++)
        {
          symbol_map[*it] = get_symbol_number(t, it->c_str());
        }
      return symbol_map;
    }



  FdTable<int>* FomaTransducer::get_flag_diacritics(fsm * t)
  {
    FdTable<int>* table = new FdTable<int>();
    for(struct sigma* p = t->sigma; p!=NULL; p=p->next)
    {
      if (p->symbol == NULL)
        break;
      if(FdOperation::is_diacritic(p->symbol))
        table->define_diacritic(p->number, p->symbol);
    }
    return table;
  }

#if GENERATE_LEXC_WRAPPER
    fsm * FomaTransducer::read_lexc(const std::string &filename, bool verbose)
  {
    char * filename_ = strdup(filename.c_str());
    char * lexcfile = file_to_mem(filename_);
    if (lexcfile == NULL)
      {
    std::string msg("Could not read file ");
    msg + filename;
    HFST_THROW_MESSAGE(StreamNotReadableException, msg);
      }
    free(filename_);
    int verbose_int = 0;
    if (verbose)
      verbose_int = 1;
    fsm * retval = fsm_lexc_parse_string(lexcfile, verbose_int);
    if (retval == NULL)
      {
    std::string msg("Not valid Lexc format in file ");
    msg + filename;
    HFST_THROW_MESSAGE(NotValidLexcFormatException, msg);
      }
    free(lexcfile);
    return retval;
  }
#endif

  fsm * FomaTransducer::eliminate_flags(fsm * t)
  {
    return flag_eliminate(t, NULL);
  }

  fsm * FomaTransducer::eliminate_flag(fsm * t, const std::string & flag)
  {
    char * flag_ = strdup(flag.c_str());
    fsm * retval = flag_eliminate(t, flag_);
    free(flag_);
    return retval;
  }

  void FomaTransducer::print_test(fsm * t)
  {
    net_print_att(t, stdout);
  }

    // HFST additions

    static int io_gets(FILE *infile, char *target);
    static inline int explode_line (char *buf, int *values);

    /* Read foma transducer . */
    fsm * FomaTransducer::read_net(FILE *infile) {
      
    const unsigned int READ_BUF_SIZE=4096;
    char buf[READ_BUF_SIZE];
    fsm *net;
    struct fsm_state * fsm_;
    
    char *new_symbol;
    int i, items, new_symbol_number, laststate, lineint[5], *cm;
    char last_final = 0;

    if (io_gets(infile, buf) == 0) {
        return NULL;
    }
    
    char* empty = strdup("");
    net = fsm_create(empty);
    free(empty);
    
    if (strcmp(buf, "##foma-net 1.0##") != 0) {
        printf("File format error foma!\n");
        return NULL;
    }
    io_gets(infile, buf);
    if (strcmp(buf, "##props##") != 0) {
        printf("File format error props!\n");
        return NULL;
    }
    /* Properties */
    io_gets(infile, buf);
    sscanf(buf, "%i %i %i %i %i " LONG_LONG_SPECIFIER " %i %i %i %i %i %i %s",
           &net->arity,
           &net->arccount,
           &net->statecount,
           &net->linecount,
           &net->finalcount,
           &net->pathcount,
           &net->is_deterministic,
           &net->is_pruned,
           &net->is_minimized,
           &net->is_epsilon_free,
           &net->is_loop_free,
           &net->is_completed,
           buf);
    // The following strcpy is commented out because we want to leave the
    // empty name we've written earlier and use HFST3's name scheme
    // for everything. NB: There's a limit of 40 chars for net->name.
    //strcpy(net->name, buf);
    io_gets(infile, buf);

    /* Sigma */
    if (strcmp(buf, "##sigma##") != 0) {
        printf("File format error sigma!\n");
        return NULL;
    }
    net->sigma = sigma_create();
    for (;;) {
      io_gets(infile, buf);
        if (buf[0] == '#') break;
        new_symbol = strstr(buf, " ");
        new_symbol[0] = '\0';
        new_symbol++;
        sscanf(buf,"%i", &new_symbol_number);
        sigma_add_number(net->sigma, new_symbol, new_symbol_number);
    }
    /* States */
    if (strcmp(buf, "##states##") != 0) {
        printf("File format error!\n");
        return NULL;
    }
    net->states = (fsm_state*) malloc(net->linecount*sizeof(struct fsm_state)); // error
    // DEBUG
    //fprintf(stderr, "Reserved space for %i state lines\n", net->linecount);
    fsm_ = net->states;
    laststate = -1;
    for (i=0; ;i++) {
      io_gets(infile, buf);
        if (buf[0] == '#') break;

        /* scanf is just too slow here */
        //items = sscanf(buf, "%i %i %i %i %i",&lineint[0],
        //&lineint[1], &lineint[2], &lineint[3], &lineint[4]);

        items = explode_line(buf, &lineint[0]);

        switch (items) {
        case 2:
            (fsm_+i)->state_no = laststate;
            (fsm_+i)->in = lineint[0];
            (fsm_+i)->out = lineint[0];
            (fsm_+i)->target = lineint[1];
            (fsm_+i)->final_state = last_final;
            break;
        case 3:
            (fsm_+i)->state_no = laststate;
            (fsm_+i)->in = lineint[0];
            (fsm_+i)->out = lineint[1];
            (fsm_+i)->target = lineint[2];
            (fsm_+i)->final_state = last_final;
            break;
        case 4:
            (fsm_+i)->state_no = lineint[0];
            (fsm_+i)->in = lineint[1];
            (fsm_+i)->out = lineint[1];
            (fsm_+i)->target = lineint[2];
            (fsm_+i)->final_state = lineint[3];
            laststate = lineint[0];
            last_final = lineint[3];
            break;
        case 5:
          (fsm_+i)->state_no = lineint[0]; // error
            (fsm_+i)->in = lineint[1];
            (fsm_+i)->out = lineint[2];
            (fsm_+i)->target = lineint[3];
            (fsm_+i)->final_state = lineint[4];
            laststate = lineint[0];
            last_final = lineint[4];
            break;
        default:
            printf("File format error\n");
            return NULL;
        }
        if (laststate > 0) {
            (fsm_+i)->start_state = 0;
        } else if (laststate == -1) {
            (fsm_+i)->start_state = -1;
        } else {
            (fsm_+i)->start_state = 1;
        }

    }
    if (strcmp(buf, "##cmatrix##") == 0) {
        cmatrix_init(net);
        cm = net->medlookup->confusion_matrix;
        for (;;) {
      io_gets(infile, buf);
            if (buf[0] == '#') break;
            sscanf(buf,"%i", &i);
            *cm = i;
            cm++;
        }
    }
    if (strcmp(buf, "##end##") != 0) {
        printf("File format error!\n");
        return NULL;
    }
    return(net);
}

static int io_gets(FILE *infile, char *target) {
    int i;
    int c = getc(infile);
    for (i = 0; c != '\n' && c != '\0'; i++) {
        *(target+i) = c;
        c = getc(infile);
    }
    *(target+i) = '\0';

    // Windows...
    if (*(target+i-1) == '\r')
      *(target+i-1) = '\0';

    if (c == '\0')
      ungetc(c, infile);
    return(i);
}

static inline int explode_line (char *buf, int *values) {

    int i, j, items;
    j = i = items = 0;
    for (;;) {
        for (i = j; *(buf+j) != ' ' && *(buf+j) != '\0'; j++) { }
        if (*(buf+j) == '\0') {
            *(values+items) = atoi(buf+i);
            items++;
            break;
        } else{
            *(buf+j) = '\0';
            *(values+items) = atoi(buf+i);
            items++;
            j++;
            i = j;
        }
    }
    return(items);
}

    int FomaTransducer::write_net(fsm * net, FILE * outfile) {

    struct sigma *sigma;
    struct fsm_state * fsm_;
    int i, maxsigma, laststate, *cm;

    // If this is not done, linecount can return a false value.
    fsm_count(net);

    /* Header */
    fprintf(outfile, "%s","##foma-net 1.0##\n");

    /* Properties */
    fprintf(outfile, "%s","##props##\n");
    fprintf(outfile, "%i %i %i %i %i " LONG_LONG_SPECIFIER " %i %i %i %i %i %i %s\n",
            net->arity,
            net->arccount,
            net->statecount,
            net->linecount,
            net->finalcount,
            net->pathcount,
            net->is_deterministic,
            net->is_pruned,
            net->is_minimized,
            net->is_epsilon_free,
            net->is_loop_free,
            net->is_completed,
            net->name);
    
    /* Sigma */
    fprintf(outfile, "%s","##sigma##\n");
    for (sigma = net->sigma; sigma != NULL && sigma->number != -1;
         sigma = sigma->next) {
        fprintf(outfile, "%i %s\n",sigma->number, sigma->symbol);
    }

    /* State array */
    laststate = -1;
    fsm_ = net->states;
    fprintf(outfile, "%s","##states##\n");

    for (fsm_ = net->states; fsm_->state_no !=-1; fsm_++) {
        if (fsm_->state_no != laststate) {
            if (fsm_->in != fsm_->out) {
                fprintf(outfile, "%i %i %i %i %i\n",
                        fsm_->state_no, fsm_->in, fsm_->out, fsm_->target,
                        fsm_->final_state);
            } else {
                fprintf(outfile, "%i %i %i %i\n",fsm_->state_no, fsm_->in,
                        fsm_->target, fsm_->final_state);
            }
        } else {
            if (fsm_->in != fsm_->out) {
                fprintf(outfile, "%i %i %i\n", fsm_->in, fsm_->out, fsm_->target);
            } else {
                fprintf(outfile, "%i %i\n", fsm_->in, fsm_->target);
            }
        }
        laststate = fsm_->state_no;
    }
    /* Sentinel for states */
    fprintf(outfile, "-1 -1 -1 -1 -1\n");

    /* Store confusion matrix */
    if (net->medlookup != NULL && net->medlookup->confusion_matrix != NULL) {

        fprintf(outfile, "%s","##cmatrix##\n");
        cm = net->medlookup->confusion_matrix;
        maxsigma = sigma_max(net->sigma)+1;
        fprintf(outfile, "maxsigma is: %i\n",maxsigma);
        for (i=0; i < maxsigma*maxsigma; i++) {
            fprintf(outfile, "%i\n", *(cm+i));
        }
    }

    /* End */
    fprintf(outfile, "%s","##end##\n");
    if (fflush(outfile) != 0) {
      HFST_THROW_MESSAGE
        (HfstFatalException,
         "an error happened when writing a foma transducer");
    }

    return(1);

    }

  } }

#else // MAIN_TEST was defined
#include <cstdlib>
#include <cassert>
#include <iostream>
using namespace hfst::implementations;

int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":";

    fsm * epsilon
      = FomaTransducer::define_transducer("@_EPSILON_SYMBOL_@");
    fsm * epsilon_i = FomaTransducer::extract_input_language(epsilon);
    fsm * epsilon_i_min = FomaTransducer::minimize(fsm_copy(epsilon_i));
    (void)epsilon_i_min;

    fsm * a = FomaTransducer::define_transducer("a");
    fsm * a2 = FomaTransducer::repeat_n(a, 2);
    (void)a2;
    
    std::cout << std::endl << "ok" << std::endl;
    return EXIT_SUCCESS;
}
#endif // MAIN_TEST
