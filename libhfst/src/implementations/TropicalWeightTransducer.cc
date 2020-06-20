// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include "TropicalWeightTransducer.h"
#include "HfstSymbolDefs.h"
#include "HfstLookupFlagDiacritics.h"
#include "HfstBasicTransducer.h"
#include "ConvertTransducerFormat.h"

#ifdef _MSC_VER
#include "back-ends/openfstwin/src/include/fst/fstlib.h"
#else
#include "back-ends/openfst/src/include/fst/fstlib.h"
#endif // _MSC_VER

#if defined(USE_FOMA_EPSILON_REMOVAL) && defined(HAVE_FOMA)
#include "FomaTransducer.h"
#endif

#ifdef PROFILE_OPENFST
#include <ctime>
#endif

#ifndef MAIN_TEST

#define CHECK_EPSILON_CYCLES(x, y) { hfst::implementations::HfstBasicTransducer * fsm = hfst::implementations::ConversionFunctions::tropical_ofst_to_hfst_basic_transducer( x ); if (fsm->has_negative_epsilon_cycles()) { if (warning_stream != NULL) { *warning_stream << y << ": warning: transducer has epsilon cycles with a negative weight" << std::endl; } } delete fsm; }

namespace hfst {
  bool get_encode_weights();

  namespace implementations {

    // add this to the beginning of code block to be profiled
    //#ifdef PROFILE_OPENFST
    //clock_t startclock = clock();
    //#endif

    // and this to the end of the block
    //#ifdef PROFILE_OPENFST
    //clock_t endclock = clock();
    //tropical_seconds = tropical_seconds +
    //      ( (float)(endclock - startclock) / CLOCKS_PER_SEC);
    //#endif

  float tropical_seconds=0;
  float TropicalWeightTransducer::get_profile_seconds() {
    return tropical_seconds;
  }

    std::ostream * TropicalWeightTransducer::warning_stream = NULL;

    std::ostream * TropicalWeightTransducer::get_warning_stream()
    {
      return warning_stream;
    }

    void TropicalWeightTransducer::set_warning_stream(std::ostream * os)
    {
      warning_stream = os;
    }
 
    // This function can be moved to its own file if TropicalWeightTransducer.o
    // yields a 'File too big' error.
    StdVectorFst * TropicalWeightTransducer::push_labels
    (StdVectorFst * t, bool to_initial_state)
    {
      assert (t->InputSymbols() != NULL);

      CHECK_EPSILON_CYCLES(t, "push_labels");
      
      fst::StdVectorFst * retval = new fst::StdVectorFst();
      if (to_initial_state)
        fst::Push<StdArc, REWEIGHT_TO_INITIAL>(*t, retval, fst::kPushLabels);
      else
        fst::Push<StdArc, REWEIGHT_TO_FINAL>(*t, retval, fst::kPushLabels);
      retval->SetInputSymbols(t->InputSymbols());
      return retval;
    }

    // This function can be moved to its own file if TropicalWeightTransducer.o
    // yields a 'File too big' error.
    StdVectorFst * TropicalWeightTransducer::push_weights
    (StdVectorFst * t, bool to_initial_state)
    {
      assert (t->InputSymbols() != NULL);

      CHECK_EPSILON_CYCLES(t, "push_weights");
      
      fst::StdVectorFst * retval = new fst::StdVectorFst();
      if (to_initial_state)
        fst::Push<StdArc, REWEIGHT_TO_INITIAL>(*t, retval, fst::kPushWeights);
      else
        fst::Push<StdArc, REWEIGHT_TO_FINAL>(*t, retval, fst::kPushWeights);
      retval->SetInputSymbols(t->InputSymbols());
      return retval;
    }

    void TropicalWeightTransducer::add_to_weights(StdVectorFst * t, float w)
    {
      for (fst::StateIterator<StdVectorFst> siter(*t);
           ! siter.Done(); siter.Next())
        {
          StateId s = siter.Value();
          for (fst::MutableArcIterator<fst::StdVectorFst> aiter(t,s);
               !aiter.Done(); aiter.Next())
            {
              const StdArc &arc = aiter.Value();

              StdArc new_arc;
              new_arc.ilabel = arc.ilabel;
              new_arc.olabel = arc.olabel;
              new_arc.nextstate = arc.nextstate;
              new_arc.weight = arc.weight.Value() + w;

              aiter.SetValue(new_arc);
            }
          if (t->Final(s) != TropicalWeight::Zero())
            {
              float old_weight = t->Final(s).Value();
              t->SetFinal(s, old_weight + w);
            }
        }
    }

    float TropicalWeightTransducer::get_smallest_weight(StdVectorFst * t)
    {
      // in case of an empty transducer, infinity is returned
      // (empty in the sense of having no transitions or final states)
      float retval = std::numeric_limits<float>::infinity();
      for (fst::StateIterator<StdVectorFst> siter(*t);
           ! siter.Done(); siter.Next())
        {
          StateId s = siter.Value();
          for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
               !aiter.Done(); aiter.Next())
            {
              const StdArc &arc = aiter.Value();
              float w = arc.weight.Value();
              if (w < retval)
                retval = w;
            }
          if (t->Final(s) != TropicalWeight::Zero())
            {
              float w = t->Final(s).Value();
              if (w < retval)
                retval = w;
            }
        }
      return retval;
    }

    bool TropicalWeightTransducer::has_weights(const StdVectorFst * t)
    {
      for (fst::StateIterator<StdVectorFst> siter(*t);
           ! siter.Done(); siter.Next())
        {
          StateId s = siter.Value();
          for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
               !aiter.Done(); aiter.Next())
            {
              const StdArc &arc = aiter.Value();
              if (arc.weight.Value() != 0)
		{
		  return true;
		}
            }
          if (t->Final(s) != TropicalWeight::Zero())
            {
              if (t->Final(s).Value() != 0)
		{
		  return true;
		}
            }
        }
      return false;
    }

    // This function can be moved to its own file if TropicalWeightTransducer.o
    // yields a 'File too big' error.
    StdVectorFst * TropicalWeightTransducer::minimize(StdVectorFst * t)
    {

      CHECK_EPSILON_CYCLES(t, "minimize");

#if defined(USE_FOMA_EPSILON_REMOVAL) && defined(HAVE_FOMA)
      if (!has_weights(t))
      	{
	  hfst::implementations::HfstBasicTransducer * basic1
	    = hfst::implementations::ConversionFunctions::tropical_ofst_to_hfst_basic_transducer(t);
	  struct fsm * fst1 = hfst::implementations::ConversionFunctions::hfst_basic_transducer_to_foma(basic1);
	  struct fsm * fst2 = hfst::implementations::FomaTransducer::remove_epsilons(fst1);
	  hfst::implementations::HfstBasicTransducer * basic2
	    = hfst::implementations::ConversionFunctions::foma_to_hfst_basic_transducer(fst2);
	  delete t;
	  t = hfst::implementations::ConversionFunctions::hfst_basic_transducer_to_tropical_ofst(basic2);
	}
      else
	{
	  RmEpsilon<StdArc>(t);
	}
#else
      RmEpsilon<StdArc>(t);
#endif

      float w = get_smallest_weight(t);
      if (w < 0)
        {
          add_to_weights(t, -w);
        }

      EncodeMapper<StdArc> encode_mapper
        (hfst::get_encode_weights() ? (kEncodeLabels|kEncodeWeights) : (kEncodeLabels), ENCODE);
      Encode(t, &encode_mapper);
      StdVectorFst * det = new StdVectorFst();

      Determinize<StdArc>(*t, det);
      Minimize<StdArc>(det);
      Decode(det, encode_mapper);

      if (w < 0)
	{
	  add_to_weights(det, w);
	}

      return det;
    }

    void print_att_number(StdVectorFst *t, FILE * ofile);

  bool openfst_tropical_use_hopcroft=false;
  void openfst_tropical_set_hopcroft(bool value) {
    openfst_tropical_use_hopcroft=value;
  }

  void initialize_symbol_tables(StdVectorFst *t);

  TropicalWeightInputStream::TropicalWeightInputStream(void):
    i_stream(),input_stream(cin)
  {}

  TropicalWeightInputStream::TropicalWeightInputStream
    (const std::string &filename_):
      filename(std::string(filename_)),
      i_stream(filename.c_str(), std::ios::in | std::ios::binary),
      input_stream(i_stream)
  {}

  TropicalWeightInputStream::TropicalWeightInputStream(std::istream &is):
    input_stream(is)
  {}

  char TropicalWeightInputStream::stream_get() {
    return (char) input_stream.get(); }

  short TropicalWeightInputStream::stream_get_short() {
    short i;
    input_stream.read((char*)&i,sizeof(i));
    return i;
  }

  void TropicalWeightInputStream::stream_unget(char c) {
    input_stream.putback(c); }

  void TropicalWeightTransducer::remove_symbol_table(StdVectorFst *t)
  {
    t->SetInputSymbols(NULL);
  }

  void TropicalWeightTransducer::insert_to_alphabet
    (StdVectorFst *t, const std::string &symbol)
  {
    assert(t->InputSymbols() != NULL);
    fst::SymbolTable * st = t->InputSymbols()->Copy();
    st->AddSymbol(symbol);
    t->SetInputSymbols(st);
    delete st;
  }

  void TropicalWeightTransducer::remove_from_alphabet
  (StdVectorFst *t, const std::string &symbol)
  {
    assert(t->InputSymbols() != NULL);
    fst::SymbolTable st(t->InputSymbols()->Name());
    for ( fst::SymbolTableIterator it
            = fst::SymbolTableIterator(*(t->InputSymbols()));
           !it.Done(); it.Next() ) {
      if (it.Symbol() != symbol) {
    st.AddSymbol(it.Symbol(), it.Value());
      }
    }
    t->SetInputSymbols(&st);
  }

  void TropicalWeightTransducer::get_initial_input_symbols
  (StdVectorFst *t, StateId s, std::set<StateId> & visited_states, StringSet & symbols)
  {
    visited_states.insert(s);
    for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
         !aiter.Done(); aiter.Next())
      {
        const StdArc &arc = aiter.Value();
        assert(t->InputSymbols() != NULL);
        std::string sym = t->InputSymbols()->Find(arc.ilabel);
        assert(sym != "");

        if ((!FdOperation::is_diacritic(sym)) && arc.ilabel != 0 )
          {
            symbols.insert(sym);
          }
	else
        {
	    if (visited_states.find(arc.nextstate) == visited_states.end())
            {
		get_initial_input_symbols(t, arc.nextstate, visited_states, symbols);
            }
        }
      }
  }
  
  StringSet TropicalWeightTransducer::get_initial_input_symbols(StdVectorFst *t)
  {
    assert(t->InputSymbols() != NULL);
    StringSet symbols;
    StateId s = t->Start();
    if (s + 1 == 0) {
        // This can apparently happen with empty transducers
        // causing a segfault
        return symbols;
    }
    std::set<StateId> visited_states;
    get_initial_input_symbols(t, s, visited_states, symbols);
    return symbols;
  }

    // ???
  void TropicalWeightTransducer::get_first_input_symbols
  (StdVectorFst *t, StateId s, std::set<StateId> & visited_states, StringSet & symbols)
  {
    visited_states.insert(s);
    for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
         !aiter.Done(); aiter.Next())
      {
        const StdArc &arc = aiter.Value();
        assert(t->InputSymbols() != NULL);
        std::string sym = t->InputSymbols()->Find(arc.ilabel);
        assert(sym != "");

        if ((!FdOperation::is_diacritic(sym)) && arc.ilabel != 0 )
          {
            symbols.insert(t->InputSymbols()->Find(arc.ilabel));
          }
        if (visited_states.find(arc.nextstate) == visited_states.end())
          {
            get_first_input_symbols(t, arc.nextstate, visited_states, symbols);
          }
      }
  }

    // ???
  StringSet TropicalWeightTransducer::get_first_input_symbols(StdVectorFst *t)
  {
    assert(t->InputSymbols() != NULL);
    StringSet symbols;
    StateId s = t->Start();
    std::set<StateId> visited_states;
    get_first_input_symbols(t, s, visited_states, symbols);
    return symbols;
  }

  StringSet TropicalWeightTransducer::get_alphabet(StdVectorFst *t)
  {
    assert(t->InputSymbols() != NULL);
    StringSet s;
    for ( fst::SymbolTableIterator it
            = fst::SymbolTableIterator(*(t->InputSymbols()));
          ! it.Done(); it.Next() ) {
      s.insert( std::string(it.Symbol()) );
    }
    return s;
  }

  unsigned int TropicalWeightTransducer::get_symbol_number
  (StdVectorFst *t,
   const std::string &symbol)
  {
    assert(t->InputSymbols() != NULL);
    int64 i = t->InputSymbols()->Find(symbol);
    if (i < 0)
      HFST_THROW(SymbolNotFoundException);
    return (unsigned int)i;
  }

  unsigned int TropicalWeightTransducer::get_biggest_symbol_number
  (StdVectorFst *t)
  {
    unsigned int biggest_number=0;
    for ( fst::SymbolTableIterator it
            = fst::SymbolTableIterator(*(t->InputSymbols()));
          ! it.Done(); it.Next() ) {
      if (it.Value() > biggest_number)
        biggest_number = it.Value();
    }
    return biggest_number;
  }

  StringVector TropicalWeightTransducer::get_symbol_vector
  (StdVectorFst *t)
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

  /* Find the number-to-number mappings needed to be performed to t1
     so that it will follow the same symbol-to-number encoding as t2.
     @pre t2's symbol table must contain all symbols in t1's symbol table.
  */
  NumberNumberMap TropicalWeightTransducer::create_mapping
    (fst::StdVectorFst *t1, fst::StdVectorFst *t2)
  {
    NumberNumberMap km;
    // find the number-to-number mappings for transducer t1
    for ( fst::SymbolTableIterator it
            = fst::SymbolTableIterator(*(t1->InputSymbols()));
          ! it.Done(); it.Next() ) {
      km [ (unsigned int)it.Value() ]
        = (unsigned int) t2->InputSymbols()->Find( it.Symbol() );
      
      assert(it.Value() >= 0);
      assert(! (t2->InputSymbols()->Find( it.Symbol() ) < 0));
    }
    
    return km;
  }

  /* Recode the symbol numbers in this transducer as indicated in KeyMap km. */
  void TropicalWeightTransducer::recode_symbol_numbers
    (StdVectorFst *t, NumberNumberMap &km)
  {
    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        for (fst::MutableArcIterator<StdVectorFst> aiter(t,s);
             !aiter.Done(); aiter.Next())
          {
            const StdArc &arc = aiter.Value();
            StdArc new_arc;
            new_arc.ilabel = km[arc.ilabel];
            new_arc.olabel = km[arc.olabel];

        assert(new_arc.ilabel >= 0);
        assert(new_arc.olabel >= 0);

            new_arc.weight = arc.weight;
            new_arc.nextstate = arc.nextstate;
            aiter.SetValue(new_arc);
          }
      }
    return;
  }

  StdVectorFst * TropicalWeightTransducer::set_final_weights
  (StdVectorFst * t, float weight, bool increment)
  {
    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if ( t->Final(s) != TropicalWeight::Zero() )
          {
            if (increment)
              {
                float old_weight = t->Final(s).Value();
                t->SetFinal(s, weight+old_weight);
              }
            else
              {
                t->SetFinal(s, weight);
              }
          }
      }
    return t;
  }

  // Moved to TropicalWeightTransducerPushWeights.cc
  /*  StdVectorFst * TropicalWeightTransducer::push_weights
    (StdVectorFst * t, bool to_initial_state)
  {
    assert (t->InputSymbols() != NULL);
    fst::StdVectorFst * retval = new fst::StdVectorFst();
    if (to_initial_state)
      fst::Push<StdArc, REWEIGHT_TO_INITIAL>(*t, retval, fst::kPushWeights);
    else
      fst::Push<StdArc, REWEIGHT_TO_FINAL>(*t, retval, fst::kPushWeights);
    retval->SetInputSymbols(t->InputSymbols());
    return retval;
    }*/

  StdVectorFst * TropicalWeightTransducer::transform_weights
    (StdVectorFst * t,float (*func)(float f))
  {
    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if ( t->Final(s) != TropicalWeight::Zero() )
          t->SetFinal( s, func(t->Final(s).Value()) );
        for (fst::MutableArcIterator<StdVectorFst> aiter(t,s);
             !aiter.Done(); aiter.Next())
          {
            const StdArc &arc = aiter.Value();
            StdArc new_arc;
            new_arc.ilabel = arc.ilabel;
            new_arc.olabel = arc.olabel;
            new_arc.weight = func(arc.weight.Value());
            new_arc.nextstate = arc.nextstate;
            aiter.SetValue(new_arc);
          }
      }
    return t;
  }

  void TropicalWeightTransducer::write_in_att_format
    (StdVectorFst *t, FILE *ofile)
  {

    const SymbolTable *sym = t->InputSymbols();
    assert(sym != NULL);

    // this takes care that initial state is always printed as number zero
    // and state number zero (if it is not initial) is printed as another number
    // (basically as the number of the initial state in that case, i.e.
    // the numbers of initial state and state number zero are swapped)
    StateId zero_print=0;
    StateId initial_state = t->Start();
    if (initial_state != 0) {
      zero_print = initial_state;
    }
      
    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if (s == initial_state) {
        int origin;  // how origin state is printed, see the first comment
        if (s == 0)
          origin = zero_print;
        else if (s == initial_state)
          origin = 0;
        else
          origin = (int)s;
        for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
             !aiter.Done(); aiter.Next())
          {
            const StdArc &arc = aiter.Value();
            int target;  // how target state is printed, see the first comment
            if (arc.nextstate == 0)
              target = zero_print;
            else if (arc.nextstate == initial_state)
              target = 0;
            else
              target = (int)arc.nextstate;
            fprintf(ofile, "%i\t%i\t%s\t%s\t%f\n", origin, target,
                    sym->Find(arc.ilabel).c_str(),
                    sym->Find(arc.olabel).c_str(),
                    arc.weight.Value());
          }
        if (t->Final(s) != TropicalWeight::Zero())
          fprintf(ofile, "%i\t%f\n", origin, t->Final(s).Value());
        break;
        }
      }

    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if (s != initial_state) {
          int origin;  // how origin state is printed, see the first comment
          if (s == 0)
            origin = zero_print;
          else if (s == initial_state)
            origin = 0;
          else
            origin = (int)s;
          for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
               !aiter.Done(); aiter.Next())
            {
              const StdArc &arc = aiter.Value();
              int target;  // how target state is printed, see the first comment
              if (arc.nextstate == 0)
                target = zero_print;
              else if (arc.nextstate == initial_state)
                target = 0;
              else
                target = (int)arc.nextstate;
              fprintf(ofile, "%i\t%i\t%s\t%s\t%f\n", origin, target,
                      sym->Find(arc.ilabel).c_str(),
                      sym->Find(arc.olabel).c_str(),
                      arc.weight.Value());
            }
          if (t->Final(s) != TropicalWeight::Zero())
            fprintf(ofile, "%i\t%f\n", origin, t->Final(s).Value());
        }
      }
  }


  void TropicalWeightTransducer::write_in_att_format_number
    (StdVectorFst *t, FILE *ofile)
  {

    // this takes care that initial state is always printed as number zero
    // and state number zero (if it is not initial) is printed as another number
    // (basically as the number of the initial state in that case, i.e.
    // the numbers of initial state and state number zero are swapped)
    StateId zero_print=0;
    StateId initial_state = t->Start();
    if (initial_state != 0) {
      zero_print = initial_state;
    }
      
    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if (s == initial_state) {
        int origin;  // how origin state is printed, see the first comment
        if (s == 0)
          origin = zero_print;
        else if (s == initial_state)
          origin = 0;
        else
          origin = (int)s;
        for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
             !aiter.Done(); aiter.Next())
          {
            const StdArc &arc = aiter.Value();
            int target;  // how target state is printed, see the first comment
            if (arc.nextstate == 0)
              target = zero_print;
            else if (arc.nextstate == initial_state)
              target = 0;
            else
              target = (int)arc.nextstate;
            fprintf(ofile, "%i\t%i\t\\%i\t\\%i\t%f\n", origin, target,
                    arc.ilabel, arc.olabel,
                    arc.weight.Value());
          }
        if (t->Final(s) != TropicalWeight::Zero())
          fprintf(ofile, "%i\t%f\n", origin, t->Final(s).Value());
        break;
        }
      }

    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if (s != initial_state) {
          int origin;  // how origin state is printed, see the first comment
          if (s == 0)
            origin = zero_print;
          else if (s == initial_state)
            origin = 0;
          else
            origin = (int)s;
          for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
               !aiter.Done(); aiter.Next())
            {
              const StdArc &arc = aiter.Value();
              int target;  // how target state is printed, see the first comment
              if (arc.nextstate == 0)
                target = zero_print;
              else if (arc.nextstate == initial_state)
                target = 0;
              else
                target = (int)arc.nextstate;
              fprintf(ofile, "%i\t%i\t\\%i\t\\%i\t%f\n", origin, target,
                      arc.ilabel, arc.olabel,
                      arc.weight.Value());
            }
          if (t->Final(s) != TropicalWeight::Zero())
            fprintf(ofile, "%i\t%f\n", origin, t->Final(s).Value());
        }
      }
  }


  void TropicalWeightTransducer::write_in_att_format
    (StdVectorFst *t, std::ostream &os)
  {

    const SymbolTable *sym = t->InputSymbols();
    assert(sym != NULL);

    // this takes care that initial state is always printed as number zero
    // and state number zero (if it is not initial) is printed as another number
    // (basically as the number of the initial state in that case, i.e.
    // the numbers of initial state and state number zero are swapped)
    StateId zero_print=0;
    StateId initial_state = t->Start();
    if (initial_state != 0) {
      zero_print = initial_state;
    }
      
    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if (s == initial_state) {
        int origin;  // how origin state is printed, see the first comment
        if (s == 0)
          origin = zero_print;
        else if (s == initial_state)
          origin = 0;
        else
          origin = (int)s;
        for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
             !aiter.Done(); aiter.Next())
          {
            const StdArc &arc = aiter.Value();
            int target;  // how target state is printed, see the first comment
            if (arc.nextstate == 0)
              target = zero_print;
            else if (arc.nextstate == initial_state)
              target = 0;
            else
              target = (int)arc.nextstate;
            os << origin << "\t"
               << target << "\t"
               << sym->Find(arc.ilabel).c_str() << "\t"
               << sym->Find(arc.olabel).c_str() << "\t"
               << arc.weight.Value() << "\n";
          }
        if (t->Final(s) != TropicalWeight::Zero())
          os << origin << "\t"
             << t->Final(s).Value() << "\n";
        break;
        }
      }

    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if (s != initial_state) {
          int origin;  // how origin state is printed, see the first comment
          if (s == 0)
            origin = zero_print;
          else if (s == initial_state)
            origin = 0;
          else
            origin = (int)s;
          for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
               !aiter.Done(); aiter.Next())
            {
              const StdArc &arc = aiter.Value();
              int target;  // how target state is printed, see the first comment
              if (arc.nextstate == 0)
                target = zero_print;
              else if (arc.nextstate == initial_state)
                target = 0;
              else
                target = (int)arc.nextstate;
              os << origin << "\t"
                 << target << "\t"
                 << sym->Find(arc.ilabel).c_str() << "\t"
                 << sym->Find(arc.olabel).c_str() << "\t"
                 << arc.weight.Value() << "\n";
            }
          if (t->Final(s) != TropicalWeight::Zero())
            os << origin << "\t"
               << t->Final(s).Value() << "\n";
        }
      }
  }


  void TropicalWeightTransducer::write_in_att_format_number
    (StdVectorFst *t, std::ostream &os)
  {
    // this takes care that initial state is always printed as number zero
    // and state number zero (if it is not initial) is printed as another number
    // (basically as the number of the initial state in that case, i.e.
    // the numbers of initial state and state number zero are swapped)
    StateId zero_print=0;
    StateId initial_state = t->Start();
    if (initial_state != 0) {
      zero_print = initial_state;
    }
      
    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if (s == initial_state) {
        int origin;  // how origin state is printed, see the first comment
        if (s == 0)
          origin = zero_print;
        else if (s == initial_state)
          origin = 0;
        else
          origin = (int)s;
        for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
             !aiter.Done(); aiter.Next())
          {
            const StdArc &arc = aiter.Value();
            int target;  // how target state is printed, see the first comment
            if (arc.nextstate == 0)
              target = zero_print;
            else if (arc.nextstate == initial_state)
              target = 0;
            else
              target = (int)arc.nextstate;
            os << origin << "\t"
               << target << "\t"
               << "\\" << arc.ilabel << "\t"
               << "\\" << arc.olabel << "\t"
               << arc.weight.Value() << "\n";
          }
        if (t->Final(s) != TropicalWeight::Zero())
          os << origin << "\t"
             << t->Final(s).Value() << "\n";
        break;
        }
      }

    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if (s != initial_state) {
          int origin;  // how origin state is printed, see the first comment
          if (s == 0)
            origin = zero_print;
          else if (s == initial_state)
            origin = 0;
          else
            origin = (int)s;
          for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
               !aiter.Done(); aiter.Next())
            {
              const StdArc &arc = aiter.Value();
              int target;  // how target state is printed, see the first comment
              if (arc.nextstate == 0)
                target = zero_print;
              else if (arc.nextstate == initial_state)
                target = 0;
              else
                target = (int)arc.nextstate;
              os << origin << "\t"
                 << target << "\t"
                 << "\\" << arc.ilabel << "\t"
                 << "\\" << arc.olabel << "\t"
                 << arc.weight.Value() << "\n";
            }
          if (t->Final(s) != TropicalWeight::Zero())
            os << origin << "\t"
               << t->Final(s).Value() << "\n";
        }
      }
  }
  

  // AT&T format is handled here ------------------------------

  /* Maps state numbers in AT&T text format to state ids used by
     OpenFst transducers. */
  typedef std::map<int, StateId> StateMap;

  /* A method used by function 'read_in_att_format'.
     Returns the state id of state number state_number and adds a new
     state to t if state_number is encountered for the first time and
     updates state_map accordingly. */
  StateId TropicalWeightTransducer::
  add_and_map_state(StdVectorFst *t, int state_number,
                    StateMap &state_map)
  {
    StateMap::iterator it = state_map.find(state_number);
    if (it == state_map.end()) {
      StateId retval  = t->AddState();
      state_map.insert( std::pair<int, StateId>(state_number, retval) );
      return retval;
    }
    else
      return it->second;
  }

  // FIXME?: atof and atoi
  /* Reads a description of a transducer in AT&T text format and returns
     a corresponding binary transducer.
     @note The initial state must be numbered as zero. */
  StdVectorFst * TropicalWeightTransducer::read_in_att_format(FILE * ifile)
  {
    StdVectorFst *t = new StdVectorFst();
    SymbolTable st = create_symbol_table("");

    char line [255];
    StateMap state_map;

    // Add initial state that is numbered as zero.
    StateId initial_state = add_and_map_state(t, 0, state_map);
    t->SetStart(initial_state);


    while ( fgets(line, 255, ifile) != NULL )
      {
        if (*line == '-') // transducer separator
          return t;

        char a1 [100]; char a2 [100]; char a3 [100];
        char a4 [100]; char a5 [100];
        int n = sscanf(line, "%s\t%s\t%s\t%s\t%s", a1, a2, a3, a4, a5);

        // set value of weight
        float weight = 0;
        if (n == 2)
          weight = atof(a2);
        if (n == 5)
          weight = atof(a5);

        if (n == 1 || n == 2)  // final state line
          {
            int final_number = atoi(a1);
            StateId final_state = add_and_map_state(t, final_number, state_map);
            t->SetFinal(final_state, weight);
          }

        else if (n == 4 || n == 5)  // transition line
          {
            int origin_number = atoi(a1);
            int target_number = atoi(a2);
            StateId origin_state
              = add_and_map_state(t, origin_number, state_map);
            StateId target_state
              = add_and_map_state(t, target_number, state_map);

            int input_number = st.AddSymbol(std::string(a3));
            int output_number = st.AddSymbol(std::string(a4));

            t->AddArc(origin_state,
                      StdArc(input_number, output_number,
                             weight, target_state));
          }

        else  // line could not be parsed
          {
            std::string message(line);
            HFST_THROW_MESSAGE
              (NotValidAttFormatException,
               message);
          }

      }

    t->SetInputSymbols(&st);
    return t;
  }

  /*
     Create a copy of this transducer where all transitions of type
     "?:?", "?:x" and "x:?" are expanded according to the StringSymbolSet
     'unknown' that lists all symbols previously unknown to this transducer.
  */
  StdVectorFst * TropicalWeightTransducer::expand_arcs
  (StdVectorFst * t, StringSet &unknown,
   bool unknown_symbols_in_use)
  {

    StdVectorFst * result = new StdVectorFst();

    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      result->AddState();

    // go through all states in this
    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        // create new state in result, if needed
        StateId s = siter.Value();
        StateId result_s=s;

        // make the new state initial, if needed
        if (t->Start() == s)
          result->SetStart(result_s);

        // make the new state final, if needed
        if (t->Final(s) != TropicalWeight::Zero())
          result->SetFinal(result_s, t->Final(s).Value());


        // go through all the arcs in this
        for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
             !aiter.Done(); aiter.Next())
          {
            const StdArc &arc = aiter.Value();

            // find the corresponding target state in result or, if not found,
            // create a new state
            StateId result_nextstate=arc.nextstate;

            // expand the transitions, if needed

            if (unknown_symbols_in_use) {

            const fst::SymbolTable *is = t->InputSymbols();

            if ( arc.ilabel == 1 &&       // cross-product "?:?"
                 arc.olabel == 1 )
              {
                for (StringSet::iterator it1 = unknown.begin();
                     it1 != unknown.end(); it1++)
                  {
            if (! FdOperation::is_diacritic(*it1)) {
              
              int64 inumber = is->Find(*it1);
              for (StringSet::iterator it2 = unknown.begin();
               it2 != unknown.end(); it2++)
            {
              if (! FdOperation::is_diacritic(*it2)) {
                int64 onumber = is->Find(*it2);
                if (inumber != onumber)
                  result->AddArc(result_s,
                         StdArc(inumber, onumber,
                            arc.weight,
                            result_nextstate));
              }
            }
              result->AddArc(result_s,
                     StdArc(inumber, 1, arc.weight,
                        result_nextstate));
              result->AddArc(result_s,
                     StdArc(1, inumber, arc.weight,
                        result_nextstate));
            }
          }
              }
            else if (arc.ilabel == 2 ||   // identity "?:?"
                     arc.olabel == 2 )
              {
                for (StringSet::iterator it = unknown.begin();
                     it != unknown.end(); it++)
                  {
            if (! FdOperation::is_diacritic(*it)) {
              int64 number = is->Find(*it);
              result->AddArc(result_s,
                     StdArc(number, number,
                        arc.weight, result_nextstate));
            }
                  }
              }
            else if (arc.ilabel == 1)  // "?:x"
              {
                for (StringSet::iterator it = unknown.begin();
                     it != unknown.end(); it++)
                  {
            if (! FdOperation::is_diacritic(*it)) {
              int64 number = is->Find(*it);
              result->AddArc(result_s,
                     StdArc(number, arc.olabel,
                        arc.weight, result_nextstate));
            }
                  }
              }
            else if (arc.olabel == 1)  // "x:?"
              {
                for (StringSet::iterator it = unknown.begin();
                     it != unknown.end(); it++)
                  {
            if (! FdOperation::is_diacritic(*it)) {
              int64 number = is->Find(*it);
              result->AddArc(result_s,
                     StdArc(arc.ilabel, number,
                        arc.weight, result_nextstate));
            }
          }
              }
            }

            // the original transition is copied in all cases
            result->AddArc(result_s, StdArc(arc.ilabel, arc.olabel,
                                            arc.weight, result_nextstate));
          }
      }

    return result;
  }


    /*  static StdVectorFst * copy_fst(const StdVectorFst * t)
  {
    StdVectorFst * result = new StdVectorFst();

    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      result->AddState();
    
    // go through all states in this
    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        // create new state in result, if needed
        StateId s = siter.Value();
        StateId result_s=s;

        // make the new state initial, if needed
        if (t->Start() == s)
          result->SetStart(result_s);

        // make the new state final, if needed
        if (t->Final(s) != TropicalWeight::Zero())
          result->SetFinal(result_s, t->Final(s).Value());
        

        // go through all the arcs in this
        for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
             !aiter.Done(); aiter.Next())
          {
            const StdArc &arc = aiter.Value();

            // find the corresponding target state in result or, if not found,
            // create a new state
            StateId result_nextstate=arc.nextstate;
            
            // copy the transition
            result->AddArc(result_s, StdArc(arc.ilabel, arc.olabel,
                                            arc.weight, result_nextstate));
          }
      }
    return result;
    }*/

  unsigned int TropicalWeightTransducer::number_of_states(const StdVectorFst *t)
  {
    unsigned int retval=0;
    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      retval++;
    return retval;
  }

  unsigned int TropicalWeightTransducer::number_of_arcs(const StdVectorFst *t)
  {
    unsigned int retval=0;
    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        for (fst::ArcIterator<StdVectorFst> aiter(*t,siter.Value());
             !aiter.Done(); aiter.Next())
          retval++;
      }
    return retval;
  }

  std::pair<StdVectorFst*, StdVectorFst*> TropicalWeightTransducer::harmonize
  (StdVectorFst *t1, StdVectorFst *t2, bool unknown_symbols_in_use)
  {
    bool DEBUG=false;

    // 1. Calculate the set of unknown symbols for transducers t1 and t2.

    StringSet unknown_t1;    // symbols known to another but not this
    StringSet unknown_t2;    // and vice versa

    StringSet t1_symbols = get_alphabet(t1);
    StringSet t2_symbols = get_alphabet(t2);
    hfst::symbols::collect_unknown_sets(t1_symbols, unknown_t1,
                    t2_symbols, unknown_t2);
    
    if (DEBUG)
      {
    fprintf(stderr, "New symbols for t1: ");
    for (StringSet::const_iterator it = unknown_t1.begin();
         it != unknown_t1.end(); it++)
      fprintf(stderr, "'%s', ", it->c_str());
    fprintf(stderr, "\n");
    fprintf(stderr, "New symbols for t2: ");
    for (StringSet::const_iterator it = unknown_t2.begin();
         it != unknown_t2.end(); it++)
      fprintf(stderr, "'%s', ", it->c_str());
    fprintf(stderr, "\n");
      }


    // 2. Add new symbols from transducer t1 to the symbol table of
    //    transducer t2...

    SymbolTable * st2 = t2->InputSymbols()->Copy();
    for ( StringSet::const_iterator it = unknown_t2.begin();
          it != unknown_t2.end(); it++ ) {
      if(st2->AddSymbol(*it) < 3) {
        std::cerr << "ERROR: string " << *it << " got strange number" << std::endl;
        assert(false);
      }
    }
    t2->SetInputSymbols(st2);
    
    // ...calculate the number mappings needed in harmonization...
    NumberNumberMap km = create_mapping(t1, t2);

    // ... replace the symbol table of t1 with a copy of t2's symbol table
    t1->SetInputSymbols(st2);
    delete st2;

    // ...and recode the symbol numbers of transducer t1 so that
    //    it follows the new symbol table.
    recode_symbol_numbers(t1, km);

    // 3. Calculate the set of symbol pairs to which a non-identity "?:?"
    //    transition is expanded for both transducers.
    
    fst::StdVectorFst *harmonized_t1;
    fst::StdVectorFst *harmonized_t2;

    if (! unknown_symbols_in_use)
      harmonized_t1 = t1;
    else {
      harmonized_t1 = expand_arcs(t1, unknown_t1, unknown_symbols_in_use);
      harmonized_t1->SetInputSymbols(t1->InputSymbols());
    }

    if (! unknown_symbols_in_use)
      harmonized_t2 = t2;
    else {
      harmonized_t2 = expand_arcs(t2, unknown_t2, unknown_symbols_in_use);
      harmonized_t2->SetInputSymbols(t2->InputSymbols());
    }

    return std::pair<StdVectorFst*, StdVectorFst*>
      (harmonized_t1, harmonized_t2);

  }



  /* Skip the identifier string "TROPICAL_OFST_TYPE" */
  void TropicalWeightInputStream::skip_identifier_version_3_0(void)
  { input_stream.ignore(19); }

  void TropicalWeightInputStream::skip_hfst_header(void)
  {
    input_stream.ignore(6);
    skip_identifier_version_3_0();
  }
  
  void TropicalWeightInputStream::close(void)
  {
    if (filename != string())
      { i_stream.close(); }
  }
  bool TropicalWeightInputStream::is_eof(void) const
  {
    return input_stream.peek() == EOF;
  }
  bool TropicalWeightInputStream::is_bad(void) const
  {
    if (filename == string())
      { return std::cin.bad(); }
    else
      { return input_stream.bad(); }
  }
  bool TropicalWeightInputStream::is_good(void) const
  {
    if(is_eof())
      return false;
    if (filename == string())
      { return std::cin.good(); }
    else
      { return input_stream.good(); }
  }
  
  bool TropicalWeightInputStream::is_fst(void) const
  {
    return is_fst(input_stream);
  }
  
  bool TropicalWeightInputStream::is_fst(FILE * f)
  {
    if (f == NULL)
      { return false; }
    int c = getc(f);
    ungetc(c, f);
    return c == 0xd6;
  }
  
  bool TropicalWeightInputStream::is_fst(istream &s)
  {
    return s.good() && (s.peek() == 0xd6);
  }

  bool TropicalWeightInputStream::operator() (void) const
  { return is_good(); }

  void TropicalWeightInputStream::ignore(unsigned int n)
  { input_stream.ignore(n); }

  StdVectorFst * TropicalWeightInputStream::read_transducer()
  {
    if (is_eof())
      {
        HFST_THROW(StreamIsClosedException); }
    StdVectorFst * t;
    FstHeader header;
    try
      {
        if (filename == string())
          {
            header.Read(input_stream,"STDIN");
            t = static_cast<StdVectorFst*>
              (StdVectorFst::Read(input_stream,
                                  FstReadOptions("STDIN",
                                                 &header)));
          }
        else
          {
            header.Read(input_stream,filename);
            t = static_cast<StdVectorFst*>
              (StdVectorFst::Read(input_stream,
                                  FstReadOptions(filename,
                                                 &header)));
          }
        if (t == NULL)
          {
            HFST_THROW(TransducerHasWrongTypeException); }
      }

    catch (const HfstException e )
      { throw e; }

    try
      {
        return t;
      }

    catch (const HfstException e )
      { throw e; }
  }


    /*TropicalWeightStateIterator::TropicalWeightStateIterator(StdVectorFst * t):
    iterator(new StateIterator<StdVectorFst>(*t))
  {}

  TropicalWeightStateIterator::~TropicalWeightStateIterator(void)
  { delete iterator; }

  void TropicalWeightStateIterator::next(void)
  {
    iterator->Next();
  }

  bool TropicalWeightStateIterator::done(void)
  {
    return iterator->Done();
  }

  TropicalWeightState TropicalWeightStateIterator::value(void)
  {
    return iterator->Value();
  }



  TropicalWeightTransition::TropicalWeightTransition
  (const StdArc &arc, StdVectorFst *t):
    arc(arc), t(t)
  {}

  TropicalWeightTransition::~TropicalWeightTransition(void)
  {}

  std::string TropicalWeightTransition::get_input_symbol(void) const
  {
    return t->InputSymbols()->Find(arc.ilabel);
  }

  std::string TropicalWeightTransition::get_output_symbol(void) const
  {
    return t->InputSymbols()->Find(arc.olabel);
  }

  TropicalWeightState TropicalWeightTransition::get_target_state(void) const
  {
    return arc.nextstate;
  }

  TropicalWeight TropicalWeightTransition::get_weight(void) const
  {
    return arc.weight;
  }

  TropicalWeightTransitionIterator::TropicalWeightTransitionIterator
  (StdVectorFst *t, StateId state):
    arc_iterator(new ArcIterator<StdVectorFst>(*t, state)),
    t(t)
  {}

  TropicalWeightTransitionIterator::~TropicalWeightTransitionIterator(void)
  {}

  void TropicalWeightTransitionIterator::next()
  {
    arc_iterator->Next();
  }

  bool TropicalWeightTransitionIterator::done()
  {
    return arc_iterator->Done();
  }

  TropicalWeightTransition TropicalWeightTransitionIterator::value()
  {
    return TropicalWeightTransition(arc_iterator->Value(), this->t);
    }*/


  fst::SymbolTable TropicalWeightTransducer::create_symbol_table
  (std::string name) {
    fst::SymbolTable st(name);
    st.AddSymbol(internal_epsilon, 0);
    st.AddSymbol(internal_unknown, 1);
    st.AddSymbol(internal_identity, 2);
    return st;
  }
  
  void TropicalWeightTransducer::initialize_symbol_tables(StdVectorFst *t) {
    SymbolTable st = create_symbol_table("");
    t->SetInputSymbols(&st);
  }

  StdVectorFst * TropicalWeightTransducer::create_empty_transducer(void)
  {
    StdVectorFst * t = new StdVectorFst;
    initialize_symbol_tables(t);
    StateId s = t->AddState();
    t->SetStart(s);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::create_epsilon_transducer(void)
  {
    StdVectorFst * t = new StdVectorFst;
    initialize_symbol_tables(t);
    StateId s = t->AddState();
    t->SetStart(s);
    t->SetFinal(s,0);
    return t;
  }

  void TropicalWeightTransducer::delete_transducer(StdVectorFst * t)
  {
    delete t;
  }

  StdVectorFst * TropicalWeightTransducer::define_transducer
  (unsigned int number)
  {
    StdVectorFst * t = new StdVectorFst;
    initialize_symbol_tables(t);
    StateId s1 = t->AddState();
    StateId s2 = t->AddState();
    t->SetStart(s1);
    t->SetFinal(s2,0);
    t->AddArc(s1,StdArc(number,number,0,s2));
    return t;
  }
  StdVectorFst * TropicalWeightTransducer::define_transducer
    (unsigned int inumber, unsigned int onumber)
  {
    StdVectorFst * t = new StdVectorFst;
    initialize_symbol_tables(t);
    StateId s1 = t->AddState();
    StateId s2 = t->AddState();
    t->SetStart(s1);
    t->SetFinal(s2,0);
    t->AddArc(s1,StdArc(inumber,onumber,0,s2));
    return t;
  }


  StdVectorFst * TropicalWeightTransducer::define_transducer
  (const std::string &symbol)
  {

    assert(! (symbol == ""));

    StdVectorFst * t = new StdVectorFst;
    SymbolTable st = create_symbol_table("");

    StateId s1 = t->AddState();
    StateId s2 = t->AddState();
    t->SetStart(s1);
    t->SetFinal(s2,0);
    t->AddArc(s1,StdArc(st.AddSymbol(symbol),
                        st.AddSymbol(symbol),0,s2));
    t->SetInputSymbols(&st);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::define_transducer
    (const std::string &isymbol, const std::string &osymbol)
  {

    assert(! (isymbol == ""));
    assert(! (osymbol == ""));

    StdVectorFst * t = new StdVectorFst;
    SymbolTable st = create_symbol_table("");

    StateId s1 = t->AddState();
    StateId s2 = t->AddState();
    t->SetStart(s1);
    t->SetFinal(s2,0);
    t->AddArc(s1,StdArc(st.AddSymbol(isymbol),
                        st.AddSymbol(osymbol),0,s2));
    t->SetInputSymbols(&st);
    return t;
  }

  bool TropicalWeightTransducer::are_equivalent
  (StdVectorFst *a_, StdVectorFst *b_)
  {
    StdVectorFst * a = copy(a_);
    StdVectorFst * b = copy(b_);

    CHECK_EPSILON_CYCLES(a, "are_equivalent");
    CHECK_EPSILON_CYCLES(b, "are_equivalent");

    RmEpsilon<StdArc>(a);
    RmEpsilon<StdArc>(b);

    EncodeMapper<StdArc> encode_mapper
      (hfst::get_encode_weights() ? (kEncodeLabels|kEncodeWeights) : (kEncodeLabels), ENCODE);

    Encode(a, &encode_mapper);
    Encode(b, &encode_mapper);

    StdVectorFst * deta = new StdVectorFst();
    StdVectorFst * detb = new StdVectorFst();

    Determinize<StdArc>(*a, deta);
    Determinize<StdArc>(*b, detb);

    delete a;
    delete b;

    bool retval = Equivalent(*deta, *detb);

    delete deta;
    delete detb;

    return retval;
  }
  
  bool TropicalWeightTransducer::is_automaton(StdVectorFst * t)
  {
    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
             !aiter.Done(); aiter.Next())
          {
            const StdArc &arc = aiter.Value();
            if (arc.ilabel != arc.olabel)
              return false;
            if (arc.ilabel == 1) // ?:?
              return false;
          }
      }
    return true;
  }

  bool TropicalWeightTransducer::is_cyclic(StdVectorFst * t)
  {
    return t->Properties(kCyclic, true) & kCyclic;
  }


  StdVectorFst * TropicalWeightTransducer::define_transducer
  (const StringPairVector &spv)
  {
    StdVectorFst * t = new StdVectorFst;
    SymbolTable st = create_symbol_table("");

    StateId s1 = t->AddState();
    t->SetStart(s1);
    for (StringPairVector::const_iterator it = spv.begin();
         it != spv.end();
         ++it)
      {
        StateId s2 = t->AddState();

    assert(! (it->first == ""));
    assert(! (it->second == ""));

        t->AddArc(s1,StdArc(st.AddSymbol(it->first),
                            st.AddSymbol(it->second),0,s2));
        s1 = s2;
      }
    t->SetFinal(s1,0);
    t->SetInputSymbols(&st);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::define_transducer
  (const StringPairSet &sps, bool cyclic)
  {
    StdVectorFst * t = new StdVectorFst;
    SymbolTable st = create_symbol_table("");

    StateId s1 = t->AddState();  // start state
    t->SetStart(s1);
    StateId s2=s1;               // final state

    if (! sps.empty()) {
      if (! cyclic)
        s2 = t->AddState();
      for (StringPairSet::const_iterator it = sps.begin();
           it != sps.end();
           ++it)
        {
      assert(! (it->first == ""));
      assert(! (it->second == ""));

          t->AddArc(s1,StdArc(st.AddSymbol(it->first),
                              st.AddSymbol(it->second),0,s2));
        }
    }
    t->SetFinal(s2,0);
    t->SetInputSymbols(&st);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::define_transducer
  (const std::vector<StringPairSet> &spsv)
  {
    StdVectorFst * t = new StdVectorFst;
    SymbolTable st = create_symbol_table("");

    StateId s1 = t->AddState();
    t->SetStart(s1);
    for (std::vector<StringPairSet>::const_iterator it = spsv.begin();
         it != spsv.end();
         ++it)
      {
        StateId s2 = t->AddState();
        for (StringPairSet::const_iterator it2 = (*it).begin();
             it2 != (*it).end(); it2++ ) {

      assert(! (it2->first == ""));
      assert(! (it2->second == ""));

          t->AddArc(s1,StdArc(st.AddSymbol(it2->first),
                              st.AddSymbol(it2->second),0,s2));
        }
        s1 = s2;
      }
    t->SetFinal(s1,0);
    t->SetInputSymbols(&st);
    return t;
  }


  StdVectorFst * TropicalWeightTransducer::define_transducer
  (const NumberPairVector &npv)
  {
    StdVectorFst * t = new StdVectorFst;

    StateId s1 = t->AddState();
    t->SetStart(s1);
    for (NumberPairVector::const_iterator it = npv.begin();
         it != npv.end();
         ++it)
      {
        StateId s2 = t->AddState();
        t->AddArc(s1,StdArc(it->first,it->second,0,s2));
        s1 = s2;
      }
    t->SetFinal(s1,0);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::define_transducer
  (const NumberPairSet &nps, bool cyclic)
  {
    StdVectorFst * t = new StdVectorFst;

    StateId s1 = t->AddState();  // start state
    t->SetStart(s1);
    StateId s2=s1;               // final state

    if (! nps.empty()) {
      if (! cyclic)
        s2 = t->AddState();
      for (NumberPairSet::const_iterator it = nps.begin();
           it != nps.end();
           ++it)
        {
          t->AddArc(s1,StdArc(it->first,it->second,0,s2));
        }
    }
    t->SetFinal(s2,0);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::define_transducer
  (const std::vector<NumberPairSet> &npsv)
  {
    StdVectorFst * t = new StdVectorFst;

    StateId s1 = t->AddState();
    t->SetStart(s1);
    for (std::vector<NumberPairSet>::const_iterator it = npsv.begin();
         it != npsv.end();
         ++it)
      {
        StateId s2 = t->AddState();
        for (NumberPairSet::const_iterator it2 = (*it).begin();
             it2 != (*it).end(); it2++ ) {
          t->AddArc(s1,StdArc(it2->first,it2->second,0,s2));
        }
        s1 = s2;
      }
    t->SetFinal(s1,0);
    return t;
  }


  StdVectorFst *
  TropicalWeightTransducer::copy(StdVectorFst * t)
  { return new StdVectorFst(*t); }


  StdVectorFst *
  TropicalWeightTransducer::determinize(StdVectorFst * t)
  {

    CHECK_EPSILON_CYCLES(t, "determinize");

    RmEpsilon<StdArc>(t);

    float w = get_smallest_weight(t);
    if (w < 0)
      {
        add_to_weights(t, -w);
      }

    EncodeMapper<StdArc> encode_mapper
      (hfst::get_encode_weights() ? (kEncodeLabels|kEncodeWeights) : (kEncodeLabels), ENCODE);
    Encode(t, &encode_mapper);
    StdVectorFst * det = new StdVectorFst();
    Determinize<StdArc>(*t, det);
    Decode(det, encode_mapper);

    if (w < 0)
      {
        add_to_weights(det, w);
      }

    return det;
  }


  // Moved to TropicalWeightTransducerMinimize.cc
  /*  StdVectorFst * TropicalWeightTransducer::minimize(StdVectorFst * t)
  {
    RmEpsilon<StdArc>(t);

    EncodeMapper<StdArc> encode_mapper
      (hfst::get_encode_weights() ? (kEncodeLabels|kEncodeWeights) : (kEncodeLabels), ENCODE);
    Encode(t, &encode_mapper);
    StdVectorFst * det = new StdVectorFst();

    Determinize<StdArc>(*t, det);
    Minimize<StdArc>(det);
    Decode(det, encode_mapper);
    return det;
    }*/


  void TropicalWeightTransducer::print_alphabet(const StdVectorFst *t)
  {
    for(fst::SymbolTableIterator it=fst::SymbolTableIterator
      (*t->InputSymbols());
        !it.Done(); it.Next())
      {
    fprintf(stderr, "'%s', ", it.Symbol().c_str());
      }
    fprintf(stderr, "\n");
  }

  void print_att_number(StdVectorFst *t, FILE * ofile) {
    fprintf(ofile, "initial state: %i\n", t->Start());
    for (fst::StateIterator<StdVectorFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if ( t->Final(s) != TropicalWeight::Zero() )
          fprintf(ofile, "%i\t%f\n", s, t->Final(s).Value());
        for (fst::ArcIterator<StdVectorFst> aiter(*t,s);
             !aiter.Done(); aiter.Next())
          {
            const StdArc &arc = aiter.Value();
            fprintf(ofile, "%i\t%i\t%i\t%i\t%f\n",
                    s, arc.nextstate, arc.ilabel, arc.olabel,
                    arc.weight.Value());
          }
      }
  }

  /*  void TropicalWeightTransducer::test_minimize(void)
  {
    StdVectorFst * t = new StdVectorFst();
    StateId initial = t->AddState();
    t->SetStart(initial);
    StateId state = t->AddState();
    t->SetFinal(state, 0.5);
    t->AddArc(initial, StdArc(1,1,0.5,state));
    t->AddArc(state, StdArc(2,2,0.5,initial));
 
    // print t before minimize
    print_att_number(t, stderr);

    // epsilon removal
    RmEpsilonFst<StdArc> t_rm_eps(*t);

    // encode mapping
    EncodeMapper<StdArc> encode_mapper(0x0001,ENCODE);
    EncodeFst<StdArc> t_rm_eps_enc(t_rm_eps,
                                   &encode_mapper);

    // determinization
    DeterminizeFst<StdArc> t_DET(t_rm_eps_enc);

    // minimization
    DecodeFst<StdArc> dec(t_DET,
                          encode_mapper);
    StdVectorFst t_det_std(t_DET);

    Minimize<StdArc>(&t_det_std);

    // print t after minimize
    print_att_number(&t_det_std, stderr);

    }*/

  /* For HfstMutableTransducer */

  StateId
  TropicalWeightTransducer::add_state(StdVectorFst *t)
  {
    StateId s = t->AddState();
    if (s == 0)
      t->SetStart(s);
    return s;
  }

  void
  TropicalWeightTransducer::set_final_weight
  (StdVectorFst *t, StateId s, float w)
  {
    t->SetFinal(s, w);
    return;
  }

  void
  TropicalWeightTransducer::add_transition
  (StdVectorFst *t, StateId source, std::string &isymbol,
   std::string &osymbol, float w, StateId target)
  {
    SymbolTable *st = t->InputSymbols()->Copy();
    unsigned int ilabel = st->AddSymbol(isymbol);
    unsigned int olabel = st->AddSymbol(osymbol);
    t->AddArc(source, StdArc(ilabel, olabel, w, target));
    t->SetInputSymbols(st);
    delete st;
    return;
  }

  float
  TropicalWeightTransducer::get_final_weight(StdVectorFst *t, StateId s)
  {
    return t->Final(s).Value();
  }

  float
  TropicalWeightTransducer::is_final(StdVectorFst *t, StateId s)
  {
    return ( t->Final(s) != TropicalWeight::Zero() );
  }

  StateId
  TropicalWeightTransducer::get_initial_state(StdVectorFst *t)
  {
    return t->Start();
  }

  StdVectorFst *
  TropicalWeightTransducer::remove_epsilons(StdVectorFst * t)
  {
    CHECK_EPSILON_CYCLES(t, "remove_epsilons");
    return new StdVectorFst(RmEpsilonFst<StdArc>(*t)); }

  StdVectorFst *
  TropicalWeightTransducer::prune(StdVectorFst * t)
  { StdVectorFst * retval = new StdVectorFst();
    fst::Prune(*t, retval, TropicalWeight::One());
    return retval;
  }

  StdVectorFst *
  TropicalWeightTransducer::n_best(StdVectorFst * t, unsigned int n)
  {
    CHECK_EPSILON_CYCLES(t, "n_best");

    StdVectorFst * n_best_fst = new StdVectorFst();
    StdVectorFst * scaled = t->Copy();
    RmEpsilon(scaled);
    float w = get_smallest_weight(scaled);
    if (w < 0)
      {
        add_to_weights(scaled, -w);
      }
    try
      {
        fst::ShortestPath(*scaled,n_best_fst,(size_t)n);
      }
    catch (const std::bad_alloc & e)
      {
        (void)e;
        delete n_best_fst; delete scaled;
        HFST_THROW_MESSAGE(HfstFatalException, "TropicalWeightTransducer::nbest runs out of memory");
      }
    RmEpsilon(n_best_fst);
    if (w < 0)
      {
        add_to_weights(n_best_fst, w);
      }
    delete scaled;
    return n_best_fst;
  }

  StdVectorFst *
  TropicalWeightTransducer::repeat_star(StdVectorFst * t)
  { return new StdVectorFst(ClosureFst<StdArc>(*t,CLOSURE_STAR)); }

  StdVectorFst *
  TropicalWeightTransducer::repeat_plus(StdVectorFst * t)
  { return new StdVectorFst(ClosureFst<StdArc>(*t,CLOSURE_PLUS)); }

  StdVectorFst *
  TropicalWeightTransducer::repeat_n(StdVectorFst * t, unsigned int n)
  {
    if (n == 0)
      { return create_epsilon_transducer(); }

    StdVectorFst * repetition = create_epsilon_transducer();
    repetition->SetInputSymbols(t->InputSymbols());
    for (unsigned int i = 0; i < n; ++i)
      { Concat(repetition,*t); }
    return repetition;
  }

  StdVectorFst *
  TropicalWeightTransducer::repeat_le_n(StdVectorFst * t, unsigned int n)
  {
    if (n == 0)
      { return create_epsilon_transducer(); }

    StdVectorFst * repetition = create_epsilon_transducer();
    repetition->SetInputSymbols(t->InputSymbols());

    for (unsigned int i = 0; i < n; ++i)
      {
        StdVectorFst * optional_t = optionalize(t);
        optional_t->SetInputSymbols(t->InputSymbols());
        Concat(repetition,*optional_t);
        delete optional_t;
      }
    return repetition;
  }

  StdVectorFst *
  TropicalWeightTransducer::optionalize(StdVectorFst * t)
  {
    StdVectorFst * eps = create_epsilon_transducer();
    eps->SetInputSymbols(t->InputSymbols());
    eps->SetOutputSymbols(t->OutputSymbols());
    Union(eps,*t);
    return eps;
  }


  StdVectorFst *
  TropicalWeightTransducer::invert(StdVectorFst * t)
  {
    StdVectorFst * inverse = copy(t);
    Invert(inverse);
    inverse->SetInputSymbols(t->InputSymbols());
    return inverse;
  }

  /* Makes valgrind angry... */
  StdVectorFst *
  TropicalWeightTransducer::reverse(StdVectorFst * t)
  {
    StdVectorFst * reversed = new StdVectorFst;
    Reverse<StdArc,StdArc>(*t,reversed);
    reversed->SetInputSymbols(t->InputSymbols());
    return reversed;
  }

  StdVectorFst * TropicalWeightTransducer::extract_input_language
  (StdVectorFst * t)
  { StdVectorFst * proj = new StdVectorFst(ProjectFst<StdArc>
                                             (*t,PROJECT_INPUT));
    // substitute unknown with identity
    StdVectorFst * retval = substitute(proj, 1, 2);
    delete proj;
    retval->SetInputSymbols(t->InputSymbols());
    return retval;
  }

  StdVectorFst * TropicalWeightTransducer::extract_output_language
  (StdVectorFst * t)
  { StdVectorFst * proj = new StdVectorFst(ProjectFst<StdArc>
                                             (*t,PROJECT_OUTPUT));
    // substitute unknown with identity
    StdVectorFst * retval = substitute(proj, 1, 2);
    delete proj;
    retval->SetInputSymbols(t->InputSymbols());
    return retval;
  }
  
  typedef std::pair<int,int> LabelPair;
  typedef std::vector<LabelPair> LabelPairVector;

  StdVectorFst * TropicalWeightTransducer::insert_freely
  (StdVectorFst * t, const StringPair &symbol_pair)
  {
    SymbolTable * st = t->InputSymbols()->Copy();
    assert(st != NULL);
    for (fst::StateIterator<fst::StdFst> siter(*t);
         !siter.Done(); siter.Next()) {
      StateId state_id = siter.Value();
      t->AddArc(state_id,
                fst::StdArc(st->AddSymbol(symbol_pair.first),
                            st->AddSymbol(symbol_pair.second), 0, state_id));
    }
    t->SetInputSymbols(st);
    delete st;
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::substitute
  (StdVectorFst * t,unsigned int old_key,unsigned int new_key)
  {
    LabelPairVector v;
    v.push_back(LabelPair(old_key,new_key));
    RelabelFst<StdArc> t_subst(*t,v,v);
    return new StdVectorFst(t_subst);
  }
  
  void TropicalWeightTransducer::set_symbol_table
  (StdVectorFst * t,
   std::vector<std::pair<unsigned short, std::string> > symbol_mappings)
  {
    SymbolTable st = create_symbol_table("");
    for (unsigned int i=0; i<symbol_mappings.size(); i++)
      {
        st.AddSymbol(symbol_mappings[i].second,
                     symbol_mappings[i].first);
      }
    t->SetInputSymbols(&st);
  }


  StdVectorFst * TropicalWeightTransducer::substitute
  (StdVectorFst * t,
   NumberPair old_key_pair,
   NumberPair new_key_pair)
  {
    EncodeMapper<StdArc> encode_mapper(0x0001,ENCODE);
    EncodeFst<StdArc> enc(*t,&encode_mapper);

    StdArc old_pair_code =
      encode_mapper(StdArc(old_key_pair.first,old_key_pair.second,0,0));
    StdArc new_pair_code =
      encode_mapper(StdArc(new_key_pair.first,new_key_pair.second,0,0));

    // First cast up, then cast down...
    // For some reason dynamic_cast<StdVectorFst*>
    // doesn't work although both EncodeFst<StdArc> and StdVectorFst
    // extend Fst<StdArc>.
    // reinterpret_cast worked, but that is apparently unsafe...
    StdVectorFst * subst =
      substitute(static_cast<StdVectorFst*>(static_cast<Fst<StdArc>*>(&enc)),
                 static_cast<unsigned int>(old_pair_code.ilabel),
                 static_cast<unsigned int>(new_pair_code.ilabel));

    DecodeFst<StdArc> dec(*subst,encode_mapper);
    delete subst;
    return new StdVectorFst(dec);
  }

  StdVectorFst * TropicalWeightTransducer::substitute
    (StdVectorFst *t,
     StringPair old_symbol_pair,
     StringPairSet new_symbol_pair_set)
  {
    fst::StdVectorFst * tc = t->Copy();
    fst::SymbolTable * st = tc->InputSymbols()->Copy();
    assert(st != NULL);
    for (fst::StateIterator<fst::StdVectorFst> siter(*tc);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        for (fst::MutableArcIterator<StdVectorFst> aiter(tc,s);
             !aiter.Done(); aiter.Next())
          {
            const StdArc &arc = aiter.Value();
            if ( strcmp( st->Find(arc.ilabel).c_str(),
                         old_symbol_pair.first.c_str() ) == 0 &&
                 strcmp( st->Find(arc.olabel).c_str(),
                         old_symbol_pair.second.c_str() ) == 0 )
              {
                bool first_substitution=true;
                for (StringPairSet::iterator it = new_symbol_pair_set.begin();
                     it != new_symbol_pair_set.end(); it++)
                  {
                    if (first_substitution) {
                      StdArc new_arc;
                      new_arc.ilabel = st->AddSymbol(it->first);
                      new_arc.olabel = st->AddSymbol(it->second);
                      new_arc.weight = arc.weight.Value();
                      new_arc.nextstate = arc.nextstate;
                      aiter.SetValue(new_arc);
                      first_substitution=false; }
                    else
                      tc->AddArc(s, StdArc(st->AddSymbol(it->first),
                                           st->AddSymbol(it->second),
                                           arc.weight.Value(),
                                           arc.nextstate));
                  }
              }
          }
      }
    tc->SetInputSymbols(st);
    delete st;
    return tc;
  }

  StdVectorFst * TropicalWeightTransducer::substitute(StdVectorFst *t,
                                                      std::string old_symbol,
                                                      std::string new_symbol)
  {
    assert(t->InputSymbols() != NULL);
    SymbolTable * st = t->InputSymbols()->Copy();
    StdVectorFst * retval
      = substitute(t, st->AddSymbol(old_symbol), st->AddSymbol(new_symbol));
    retval->SetInputSymbols(st);
    delete st;
    return retval;
  }

  StdVectorFst * TropicalWeightTransducer::substitute
  (StdVectorFst *t,
   StringPair old_symbol_pair,
   StringPair new_symbol_pair)
  {
    assert(t->InputSymbols() != NULL);
    SymbolTable * st = t->InputSymbols()->Copy();
    // TODO: cl.exe: conversion from 'int64' to 'unsigned int'
    pair<unsigned int, unsigned int> old_pair
      ((unsigned int)st->AddSymbol(old_symbol_pair.first),
       (unsigned int)st->AddSymbol(old_symbol_pair.second));
    pair<unsigned int, unsigned int> new_pair
      ((unsigned int)st->AddSymbol(new_symbol_pair.first),
       (unsigned int)st->AddSymbol(new_symbol_pair.second));
    StdVectorFst * retval = substitute(t, old_pair, new_pair);
    retval->SetInputSymbols(st);
    delete st;
    return retval;
  }

  StdVectorFst * TropicalWeightTransducer::substitute
  (StdVectorFst *t,
   const StringPair old_symbol_pair,
   StdVectorFst *transducer)
  {

    assert(t->InputSymbols() != NULL);
    SymbolTable * st = t->InputSymbols()->Copy();

    int states = t->NumStates();
    for( int i = 0; i < states; ++i ) {

      for (fst::MutableArcIterator<fst::StdVectorFst> it(t,i);
           ! it.Done();
           it.Next()) {

        fst::StdArc arc = it.Value();

        // find arcs that must be replaced
        if ( arc.ilabel == st->AddSymbol(old_symbol_pair.first) &&
             arc.olabel == st->AddSymbol(old_symbol_pair.second) )
          {

          StateId destination_state = arc.nextstate;
          StateId start_state = t->AddState();

          // change the label of the arc to epsilon and point
          // the arc to a new state
          arc.ilabel = 0;
          arc.olabel = 0;
          arc.nextstate = start_state;
          // weight remains the same
          it.SetValue(arc);


          // add rest of the states to transducer t
          int states_to_add = transducer->NumStates();
          for (int j=1; j<states_to_add; j++)
            t->AddState();


          // go through all states and arcs in replace transducer tr
          for (fst::StateIterator<fst::StdFst> siter(*transducer);
               !siter.Done(); siter.Next()) {

            StateId tr_state_id = siter.Value();

            // final states in tr correspond in t to a non-final state which has
            // an epsilon transition to original destination state of arc that
            // is being replaced
            if ( is_final( transducer, tr_state_id ) )
              t->AddArc( tr_state_id + start_state,
                         fst::StdArc( 0,
                                      0,
                                      // final weight is copied to
                                      // the epsilon transition
                                      transducer->Final(tr_state_id),
                                      destination_state
                                      )
                         );

            for (fst::ArcIterator<fst::StdFst> aiter(*transducer, tr_state_id);
                 !aiter.Done(); aiter.Next()) {

              const fst::StdArc &tr_arc = aiter.Value();

              // adding arc from state 'tr_state_id+start_state'
              // to state 'tr_arc.nextstate'
              // copy arcs from tr to t
              t->AddArc( tr_state_id + start_state,
                         fst::StdArc( tr_arc.ilabel,
                                      tr_arc.olabel,
                                      // weight remains the same
                                      tr_arc.weight,
                                      tr_arc.nextstate + start_state
                                      )
                         );

            }
          }
        }
      }
    }

    t->SetInputSymbols(st);
    delete st;
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::substitute
  (StdVectorFst *t,
   const NumberPair old_number_pair,
   StdVectorFst *transducer)
  {

    int states = t->NumStates();
    for( unsigned int i = 0; i < (unsigned int)states; ++i ) {

      for (fst::MutableArcIterator<fst::StdVectorFst> it(t,i);
           ! it.Done();
           it.Next()) {

        fst::StdArc arc = it.Value();

        // find arcs that must be replaced
        if ( arc.ilabel == (int)old_number_pair.first &&
             arc.olabel == (int)old_number_pair.second )
          {

          StateId destination_state = arc.nextstate;
          StateId start_state = t->AddState();

          // change the label of the arc to epsilon and point
          // the arc to a new state
          arc.ilabel = 0;
          arc.olabel = 0;
          arc.nextstate = start_state;
          // weight remains the same
          it.SetValue(arc);


          // add rest of the states to transducer t
          int states_to_add = transducer->NumStates();
          for (int j=1; j<states_to_add; j++)
            t->AddState();


          // go through all states and arcs in replace transducer tr
          for (fst::StateIterator<fst::StdFst> siter(*transducer);
               !siter.Done(); siter.Next()) {

            StateId tr_state_id = siter.Value();

            // final states in tr correspond in t to a non-final state which has
            // an epsilon transition to original destination state of arc that
            // is being replaced
            if ( is_final( transducer, tr_state_id ) )
              t->AddArc( tr_state_id + start_state,
                         fst::StdArc( 0,
                                      0,
                                      // final weight is copied to
                                      // the epsilon transition
                                      transducer->Final(tr_state_id),
                                      destination_state
                                      )
                         );

            for (fst::ArcIterator<fst::StdFst> aiter(*transducer, tr_state_id);
                 !aiter.Done(); aiter.Next()) {

              const fst::StdArc &tr_arc = aiter.Value();

              // adding arc from state 'tr_state_id+start_state'
              // to state 'tr_arc.nextstate'
              // copy arcs from tr to t
              t->AddArc( tr_state_id + start_state,
                         fst::StdArc( tr_arc.ilabel,
                                      tr_arc.olabel,
                                      // weight remains the same
                                      tr_arc.weight,
                                      tr_arc.nextstate + start_state
                                      )
                         );
              
            }
          }
        }
      }
    }

    return t;
  }

  StdVectorFst * TropicalWeightTransducer::compose(StdVectorFst * t1,
                         StdVectorFst * t2)
  {
    StringSet foo;
    // a copy of t2 is created so that its symbol table check sum
    // is the same as t1's
    // (else OpenFst complains about non-matching check sums... )
    StdVectorFst * t2_ = expand_arcs(t2, foo, false);
    t1->SetOutputSymbols(t1->InputSymbols());
    t2_->SetInputSymbols(t1->OutputSymbols());

    ArcSort(t1, StdOLabelCompare());
    ArcSort(t2_, StdILabelCompare());

    StdVectorFst *result = new StdVectorFst();
    Compose(*t1, *t2_, result);
    delete t2_;

    t1->SetOutputSymbols(NULL);

    result->SetInputSymbols(t1->InputSymbols());
    return result;
  }

  StdVectorFst * TropicalWeightTransducer::concatenate(StdVectorFst * t1,
                                                       StdVectorFst * t2)
  {
    StdVectorFst * result = new StdVectorFst(*t1);
    result->SetInputSymbols(t1->InputSymbols());
    Concat(result,*t2);
    return result;
  }

  StdVectorFst * TropicalWeightTransducer::disjunct(StdVectorFst * t1,
                                                    StdVectorFst * t2)
  {
    StdVectorFst * result = new StdVectorFst(*t1);
    result->SetInputSymbols(t1->InputSymbols());
    Union(result,*t2);
    return result;
  }

  StdVectorFst * TropicalWeightTransducer::disjunct
  (StdVectorFst * t, const StringPairVector &spv)
  {
    SymbolTable * st = t->InputSymbols()->Copy();
    assert(st != NULL);

    StateId s = t->Start();

    for (StringPairVector::const_iterator it = spv.begin();
         it != spv.end(); it++)
      {
        unsigned int inumber = st->AddSymbol(it->first.c_str());
        unsigned int onumber = st->AddSymbol(it->second.c_str());

        bool transition_found=false;
        for(fst::ArcIterator<StdVectorFst> it(*t,s); !it.Done(); it.Next())
          {
            const StdArc& a = it.Value();
            if (a.ilabel == (int)inumber && a.olabel == (int)onumber) {
              transition_found=true;
              s = a.nextstate;
              break;
            }
          }

        if (! transition_found) {
          StateId new_state = t->AddState();
          t->AddArc(s, StdArc(inumber,onumber,0,new_state));
          s = new_state;
        }
      }

    t->SetFinal(s,0);
    
    t->SetInputSymbols(st);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::disjunct
  (StdVectorFst * t, const NumberPairVector &npv)
  {
    StateId s = t->Start();

    for (NumberPairVector::const_iterator it = npv.begin();
         it != npv.end(); it++)
      {
        unsigned int inumber = it->first;
        unsigned int onumber = it->second;

        bool transition_found=false;
        for(fst::ArcIterator<StdVectorFst> it(*t,s); !it.Done(); it.Next())
          {
            const StdArc& a = it.Value();
            if (a.ilabel == (int)inumber && a.olabel == (int)onumber) {
              transition_found=true;
              s = a.nextstate;
              break;
            }
          }

        if (! transition_found) {
          StateId new_state = t->AddState();
          t->AddArc(s, StdArc(inumber,onumber,0,new_state));
          s = new_state;
        }
      }

    t->SetFinal(s,0);
    return t;
  }

  StdVectorFst * TropicalWeightTransducer::intersect(StdVectorFst * t1,
                                                     StdVectorFst * t2)
  {
    CHECK_EPSILON_CYCLES(t1, "intersect");
    CHECK_EPSILON_CYCLES(t2, "intersect");

    RmEpsilon(t1);
    RmEpsilon(t2);

    ArcSort(t1, OLabelCompare<StdArc>());
    ArcSort(t2, ILabelCompare<StdArc>());

    // weights must not be encoded, else e.g. [a:b::1] & [a:b::2] will be empty
    EncodeMapper<StdArc> encoder(0x0001,ENCODE);

    Encode<StdArc>(t1, &encoder);
    Encode<StdArc>(t2, &encoder);

    ArcSort(t1, OLabelCompare<StdArc>());
    ArcSort(t2, ILabelCompare<StdArc>());

    IntersectFst<StdArc> intersect(*t1, *t2);

    StdVectorFst *foo = new StdVectorFst(intersect);
    DecodeFst<StdArc> decode(*foo, encoder);
    delete foo;
    StdVectorFst *result = new StdVectorFst(decode);

    t1->SetOutputSymbols(NULL);
    t2->SetOutputSymbols(NULL);

    return result;
  }

  StdVectorFst * TropicalWeightTransducer::subtract(StdVectorFst * t1,
                                                    StdVectorFst * t2)
  {
    bool DEBUG=false;

    if (DEBUG) printf("Tropical subtract...\n");

    if (t1->OutputSymbols() == NULL)
      t1->SetOutputSymbols(t1->InputSymbols());
    if (t2->OutputSymbols() == NULL)
      t2->SetOutputSymbols(t2->InputSymbols());

    CHECK_EPSILON_CYCLES(t1, "subtract");
    CHECK_EPSILON_CYCLES(t2, "subtract");

    RmEpsilon(t1);
    RmEpsilon(t2);

    ArcSort(t1, OLabelCompare<StdArc>());
    ArcSort(t2, ILabelCompare<StdArc>());

    if (DEBUG) printf("  ..epsilons removed\n");

    // Remove weights from t2, is this really needed?
    StdVectorFst *t2_ = copy(t2);

    for (fst::StateIterator<StdVectorFst> siter(*t2_);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        for (fst::MutableArcIterator<StdVectorFst> aiter(t2_,s);
             !aiter.Done(); aiter.Next())
          {
            const StdArc &arc = aiter.Value();
            StdArc new_arc;
            new_arc.ilabel = arc.ilabel;
            new_arc.olabel = arc.olabel;
            new_arc.weight = 0;
            new_arc.nextstate = arc.nextstate;
            aiter.SetValue(new_arc);
      }
    if (t2_->Final(s) != fst::TropicalWeight::Zero())
          { t2_->SetFinal(s,0); }
      }

    EncodeMapper<StdArc> encoder(kEncodeLabels, ENCODE);
    Encode<StdArc> (t1, &encoder);
    Encode<StdArc> (t2_, &encoder);

    ArcSort(t1, OLabelCompare<StdArc>());
    ArcSort(t2_, ILabelCompare<StdArc>());

    StdVectorFst * det2 = new StdVectorFst();
    Determinize<StdArc>(*t2_, det2);
    delete t2_;

    if (DEBUG) printf("  ..determinized\n");

    StdVectorFst *difference = new StdVectorFst();
    Difference(*t1, *det2, difference);
    delete det2;
    DecodeFst<StdArc> subtract(*difference, encoder);
    delete difference;

    if (DEBUG) printf("  ..subtracted\n");

    t1->SetOutputSymbols(NULL);
    t2->SetOutputSymbols(NULL);

    StdVectorFst *result = new StdVectorFst(subtract);

    return result;
  }

  StdVectorFst * TropicalWeightTransducer::set_weight(StdVectorFst * t,float f)
  {
    StdVectorFst * t_copy = new StdVectorFst(*t);
    for (fst::StateIterator<StdVectorFst> iter(*t);
         ! iter.Done(); iter.Next())
      {
        if (t_copy->Final(iter.Value()) != fst::TropicalWeight::Zero())
          { t_copy->SetFinal(iter.Value(),f); }
      }
    return t_copy;
  }
  

  // ----- TRIE FUNCTIONS BEGINS -----

  int TropicalWeightTransducer::has_arc(fst::StdVectorFst &t,
              StdArc::StateId sourcestate,
              StdArc::Label ilabel,
              StdArc::Label olabel)
  {
    for (fst::ArcIterator<StdVectorFst> aiter(t,sourcestate);
         !aiter.Done();
         aiter.Next())
      {
        if ((aiter.Value().ilabel == ilabel) &&
            (aiter.Value().olabel == olabel))
          { return aiter.Position(); }
      }

    return -1;
  }

  fst::StdVectorFst * TropicalWeightTransducer::disjunct_as_tries(fst::StdVectorFst * t1,
                                                                  const fst::StdVectorFst * t2)
  {
    disjunct_as_tries(*(t1), t1->Start(), t2, t2->Start());
    return t1;
  }

  void TropicalWeightTransducer::disjunct_as_tries(fst::StdVectorFst &t1,
                         StateId t1_state,
                         const fst::StdVectorFst * t2,
                         StateId t2_state)
  {
    if (t2->Final(t2_state) != fst::TropicalWeight::Zero())
      {
        t1.SetFinal(t1_state,
                    Plus(t1.Final(t1_state),
                         t2->Final(t2_state)));
      }
    for (fst::ArcIterator<StdVectorFst> aiter(*t2,t2_state);
         !aiter.Done();
         aiter.Next())
      {
        const StdArc &arc = aiter.Value();
        int arc_index = has_arc(t1,t1_state,arc.ilabel,arc.olabel);
        if (arc_index == -1)
          {
            StdArc::StateId new_state = t1.AddState();
            t1.AddArc(t1_state,StdArc(arc.ilabel,
                                      arc.olabel,
                                      arc.weight,
                                      new_state));
            add_sub_trie(t1,new_state,t2,arc.nextstate);
          }
        else
          {
            MutableArcIterator<fst::StdVectorFst> ajter(&t1,t1_state);
            ajter.Seek(arc_index);
            disjunct_as_tries(t1,ajter.Value().nextstate,t2,arc.nextstate);
          }
      }
  }
  
  void TropicalWeightTransducer::add_sub_trie(fst::StdVectorFst &t1,
                  StateId t1_state,
                  const fst::StdVectorFst * t2,
                  StateId t2_state)
  {
    if (t2->Final(t2_state) != fst::TropicalWeight::Zero())
      {
        t1.SetFinal(t1_state,
                    Plus(t1.Final(t1_state),
                         t2->Final(t2_state)));
      }
    for (fst::ArcIterator<fst::StdVectorFst> aiter(*t2,t2_state);
         !aiter.Done();
         aiter.Next())
      {
        const StdArc &arc = aiter.Value();
        StdArc::StateId new_state = t1.AddState();
        t1.AddArc(t1_state,fst::StdArc(arc.ilabel,
                                  arc.olabel,
                                  arc.weight,
                                  new_state));
        add_sub_trie(t1,new_state,t2,arc.nextstate);
      }
  }

  // ----- TRIE FUNCTIONS END -----

  static bool extract_paths
  (StdVectorFst * t, StdArc::StateId s,
   std::map<StateId,unsigned short> all_visitations,
   std::map<StateId, unsigned short> path_visitations,
   float weight_sum,
   hfst::ExtractStringsCb& callback, int cycles,
   std::vector<hfst::FdState<int64> >* fd_state_stack,
   bool filter_fd,
   StringPairVector &spv)
  {
    if(cycles >= 0 && path_visitations[s] > cycles)
      return true;
    all_visitations[s]++;
    path_visitations[s]++;


    if (spv.size() != 0)
      {
        bool final = t->Final(s) != TropicalWeight::Zero();
        hfst::HfstTwoLevelPath path
          (weight_sum+(final?t->Final(s).Value():0), spv);
        hfst::ExtractStringsCb::RetVal ret = callback(path, final);
        if(!ret.continueSearch || !ret.continuePath)
          {
            path_visitations[s]--;
            return ret.continueSearch;
          }
      }
    
    // sort arcs by number of visitations
    vector<const StdArc*> arcs;
    for(fst::ArcIterator<StdVectorFst> it(*t,s); !it.Done(); it.Next())
    {
      const StdArc& a = it.Value();
      size_t i;
      for( i=0; i<arcs.size(); i++ )
        if (all_visitations[a.nextstate] < all_visitations[arcs[i]->nextstate])
          break;
      arcs.push_back(NULL);
      for( size_t k=arcs.size()-1; k>i; k-- )
        arcs[k] = arcs[k-1];
      arcs[i] = &a;
    }
    
    bool res = true;
    for( size_t i=0; i<arcs.size() && res == true; i++ )
    {
      const StdArc &arc = *(arcs[i]);
      bool added_fd_state = false;
    
      if (fd_state_stack) {
        if(fd_state_stack->back().get_table().get_operation(arc.ilabel)
           != NULL) {
          fd_state_stack->push_back(fd_state_stack->back());
          if(fd_state_stack->back().apply_operation(arc.ilabel))
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

      if (!filter_fd ||
          fd_state_stack->back().get_table().get_operation(arc.ilabel)==NULL)
        istring = t->InputSymbols()->Find(arc.ilabel);

      if (!filter_fd ||
          fd_state_stack->back().get_table().get_operation(arc.olabel)==NULL)
        ostring = t->InputSymbols()->Find(arc.olabel);

      spv.push_back(StringPair(istring, ostring));


      res = extract_paths
        (t, arc.nextstate, all_visitations, path_visitations,
         weight_sum+arc.weight.Value(),
         callback, cycles, fd_state_stack, filter_fd,
         spv);
      
      spv.pop_back();

      if(added_fd_state)
        fd_state_stack->pop_back();
    }
    
    path_visitations[s]--;
    return res;
  }
  
  static const int BUFFER_START_SIZE = 64;
  
  void TropicalWeightTransducer::extract_paths
  (StdVectorFst * t, hfst::ExtractStringsCb& callback,
   int cycles, FdTable<int64>* fd, bool filter_fd)
  {
    if (t->Start() == -1)
      return;
    
    map<StateId, unsigned short> all_visitations;
    map<StateId, unsigned short> path_visitations;
    std::vector<hfst::FdState<int64> >* fd_state_stack
      = (fd==NULL) ? NULL : new std::vector<hfst::FdState<int64> >
      (1, hfst::FdState<int64>(*fd));
    
    StringPairVector spv;
    hfst::implementations::extract_paths
      (t,t->Start(),all_visitations,path_visitations,
       0.0f,callback,cycles,fd_state_stack,filter_fd,
       spv);

    // add epsilon path, if needed
    if (t->Start() != -1 && t->Final(t->Start()) != TropicalWeight::Zero()) {
      StringPairVector empty_spv;
      HfstTwoLevelPath epsilon_path(t->Final(t->Start()).Value(), empty_spv);
      callback(epsilon_path, true /* final*/);
    }
    if (fd_state_stack != NULL)
      { delete fd_state_stack; }
  }

  static bool is_minimal_and_empty(StdVectorFst *t)
  {
    int start_state = t->Start();
    if (start_state < 0)
      return true;
    
    for (fst::ArcIterator<StdVectorFst> aiter(*t,start_state);
         !aiter.Done(); aiter.Next())
      {
        return false;
      }
    return true;
  }

  /* Get a random path from transducer \a t. */
  static HfstTwoLevelPath random_path_(StdVectorFst *t) {

    /* If the transducer is empty, return. */
    if (is_minimal_and_empty(t)) {
      throw "transducer is empty";
    }
    
    HfstTwoLevelPath path;
    path.first=0; // zero weight
    StateId current_state = t->Start();

    bool is_epsilon_path_accepted =
      (t->Final(t->Start()) != TropicalWeight::Zero());

    /* If we cannot proceed, all elements in \a path whose index is smaller
       that \a last_index constitute the longest path that is recognized by
       transducer \a t so far. */
    int last_index=0;

    unsigned int num_states = t->NumStates();

    /* Whether a state has been visited. */
    std::vector<int> visited;
    visited.reserve(num_states);

    /* Whether the state is marked as broken, i.e. we cannot proceed from
       that state. These arrays are used for giving more probability for
       shorter paths if \a t is cyclic. */
    std::vector<int> broken;
    broken.reserve(num_states);

    for (unsigned int i=0; i < num_states; i++ ) {
      visited.push_back(0);
      broken.push_back(0);
    }

    while (1) {

      visited[ current_state ] = 1;
      
      vector<fst::StdArc> t_transitions;

      for (fst::ArcIterator<StdVectorFst> aiter(*t,current_state);
       !aiter.Done(); aiter.Next())
    {
      t_transitions.push_back(aiter.Value());
    }
      
      /* If we cannot proceed, return the longest path so far. */
      if (t_transitions.empty() || broken[current_state]) {
        for (int i=(int)path.second.size()-1; i>=last_index; i--) {
          path.second.pop_back();
        }
        if (!is_epsilon_path_accepted && path.second.size() == 0)
          throw "cannot extract random path";
        return path;
      }

      /* Go through all transitions in a random order.
     (If \a t is pruned, only one transition is proceeded.) */
      while ( ! t_transitions.empty() ) {
    unsigned int index = rand() % t_transitions.size();
    fst::StdArc arc = t_transitions.at(index);
    t_transitions.erase(t_transitions.begin()+index);
    
    StateId t_target = arc.nextstate;

    path.second.push_back
      (StringPair
       (t->InputSymbols()->Find(arc.ilabel),
        t->InputSymbols()->Find(arc.olabel)));
    path.first = path.first + arc.weight.Value();

    /* If the target state is final, */
    if ( t->Final(t_target) != TropicalWeight::Zero() ) {
      if ( (rand() % 4) == 0 ) {  // randomly return the path so far,
        path.first = path.first + t->Final(t_target).Value();
        if (!is_epsilon_path_accepted && path.second.size() == 0)
          throw "cannot extract random path";
        return path;
      } // or continue.
      last_index = (int)path.second.size();
    }

    /* Give more probability for shorter paths. */
    if ( broken[ t_target ] == 0 ) {
      if ( visited[ t_target ] == 1 )
        if ( (rand() % 4) == 0 )
          broken[ t_target ] = 1;
    }
    
    if ( visited[ t_target ] == 1 ) {
      if ( (rand() % 4) == 0 )
        broken[ t_target ] = 1;
    }

    /* Proceed to the target state. */
    current_state = t_target;
    break;
      }
    }
    if (!is_epsilon_path_accepted && path.second.size() == 0)
      throw "cannot extract random path";
    return path;
  };

  /* Try to extract a random path from \a t at most \a max_times times. */
  static HfstTwoLevelPath random_path(StdVectorFst *t, unsigned int max_times=5)
  {
    HfstTwoLevelPath path;

    while (max_times > 0)
      {
        try
          {
            --max_times;
            path = random_path_(t);
            return path;
          }
        catch (const char * msg)
          {
            if (strcmp("transducer is empty", msg) == 0)
              {
                throw msg;
              }
            else if (strcmp("cannot extract random path", msg) == 0)
              {
                continue;
              }
            else
              {
                throw "cannot extract random path";
              }
          }
      }
    throw "cannot extract random path";
  }

  void TropicalWeightTransducer::extract_random_paths_fd
  (StdVectorFst *t, HfstTwoLevelPaths &results, int max_num, bool filter_fd)
  {
    FlagDiacriticTable fdt;
    StringSet alpha = get_alphabet(t);
    for (StringSet::const_iterator it = alpha.begin();
         it != alpha.end(); it++)
      {
        fdt.insert_symbol(*it);
      }

    HfstTwoLevelPaths fd_results;
    // We filter flags efter extracting paths, so we request five times
    // more paths than wanted.
    extract_random_paths(t, fd_results, (5 * max_num));

    for (HfstTwoLevelPaths::const_iterator it = fd_results.begin();
         (it != fd_results.end() && max_num > 0); it++)
      {
        HfstTwoLevelPath path = *it;
        StringVector sv = hfst::symbols::to_string_vector(path);
        
        if (fdt.is_valid_string(sv))
          {
            if (filter_fd)
              {
                path = hfst::symbols::remove_flags(path);
              }
            results.insert(path);
            --max_num;
          }
      }

    return;
  }


  void TropicalWeightTransducer::extract_random_paths
  (StdVectorFst *t, HfstTwoLevelPaths &results, int max_num)
  {
    srand((unsigned int)(time(0)));

    while (max_num > 0)
      {
        /* Try to extract one path at most 5 times. */
        HfstTwoLevelPath path;
        try
          {
            --max_num;
            path = random_path(t, 5);
          }
        catch (const char * msg)
          {
            if(strcmp("cannot extract random path", msg) == 0)
              {
                continue; // one trial used, keep on trying
              }
            return; // not even possible to extract paths
          }
        
        /* If we extract the same path again, try at most 5 times
           to extract another one. */
        unsigned int i = max_num;
        while ( (results.find(path) != results.end()) &&
                i > 0)
          {
            try
              {
                --i;
                path = random_path(t, 5);
              }
            catch (const char * msg) { (void)msg; } // keep on trying
          }

        /* Insert the path (another or the same). */
        results.insert(path);
      }
    
    return;
  }

  FdTable<int64>* TropicalWeightTransducer::get_flag_diacritics
  (StdVectorFst * t)
  {
    FdTable<int64>* table = new FdTable<int64>();
    const fst::SymbolTable* symbols = t->InputSymbols();
    for(fst::SymbolTableIterator it=fst::SymbolTableIterator(*symbols);
        !it.Done(); it.Next())
    {
      if(FdOperation::is_diacritic(it.Symbol()))
        {
          table->define_diacritic(it.Value(), it.Symbol());
        }
    }
    return table;
  }

  void TropicalWeightTransducer::
  represent_empty_transducer_as_having_one_state(StdVectorFst *t)
  {
    if (t->Start() == fst::kNoStateId || t->NumStates() == 0) {
      delete t;
      t = create_empty_transducer();
    }
    return;
  }



  TropicalWeightOutputStream::TropicalWeightOutputStream(bool hfst_format):
    filename(std::string()), output_stream(std::cout), hfst_format(hfst_format)
  {
    if (!output_stream)
      fprintf(stderr, "TropicalWeightOutputStream: ERROR: failbit set (3).\n");
  }

  TropicalWeightOutputStream::TropicalWeightOutputStream
  (const std::string &str, bool hfst_format):
    filename(std::string(str)),o_stream(str.c_str(),std::ios::out | std::ios::binary),
    output_stream(o_stream),hfst_format(hfst_format)
  {}

  void TropicalWeightOutputStream::write(const char &c)
  {
    output_stream.put(char(c));
  }

  void TropicalWeightOutputStream::write_transducer(StdVectorFst * transducer)
  {
    if (!output_stream)
      fprintf(stderr, "TropicalWeightOutputStream: ERROR: failbit set (1).\n");
    /* When writing a transducer in the backend format,
       both input and output symbol tables are included. */

    if (transducer->InputSymbols() == NULL) {
      fprintf(stderr, "### Missing Input Symbol Table when writing! ###\n");
    }

    fst::SymbolTable *output_st=NULL;
    if (! hfst_format) {
      output_st = new fst::SymbolTable(*(transducer->InputSymbols()));
      transducer->SetOutputSymbols(output_st);
    }
    transducer->Write(output_stream,FstWriteOptions());
    if (output_st != NULL)
      delete output_st;
  }


  void TropicalWeightOutputStream::close(void)
  {
    if (filename != string())
      { o_stream.close(); }
  }
  }
}

#else // MAIN_TEST was defined
#include <cassert>
#include <cstdlib>
#include <iostream>

using namespace hfst::implementations;

int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":";
  TropicalWeightTransducer ofst;
  StdVectorFst * t = ofst.create_empty_transducer();
  delete t;
  t = ofst.create_epsilon_transducer();
  delete t;
  std::cout << std::endl << "ok" << std::endl;
  return EXIT_SUCCESS;
}
#endif // MAIN_TEST
