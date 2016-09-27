// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include "LogWeightTransducer.h"
#include "HfstSymbolDefs.h"

#ifdef _MSC_VER
#include "back-ends/openfstwin/src/include/fst/fstlib.h"
#else
#include "back-ends/openfst/src/include/fst/fstlib.h"
#endif // _MSC_VER


#ifndef MAIN_TEST
namespace hfst { namespace implementations
{
  float log_seconds_in_harmonize=0;

    float LogWeightTransducer::get_profile_seconds() {
      return log_seconds_in_harmonize;
    }

  bool openfst_log_use_hopcroft=false;

  void openfst_log_set_hopcroft(bool value) {
    openfst_log_use_hopcroft=value;
  }

  void initialize_symbol_tables(LogFst *t);

  LogWeightInputStream::LogWeightInputStream(void):
    i_stream(),input_stream(cin)
  {}
  LogWeightInputStream::LogWeightInputStream(const std::string &filename_):
    filename(std::string(filename_)),i_stream(filename.c_str()),
    input_stream(i_stream)
  {}

  char LogWeightInputStream::stream_get() {
    return (char) input_stream.get(); }

  short LogWeightInputStream::stream_get_short() {
    short i;
    input_stream.read((char*)&i,sizeof(i));
    return i;
  }

  void LogWeightInputStream::stream_unget(char c) {
    input_stream.putback(c); }

  void LogWeightTransducer::remove_symbol_table(LogFst *t)
  {
    t->SetInputSymbols(NULL);
  }

  void LogWeightTransducer::insert_to_alphabet
    (LogFst *t, const std::string &symbol)
  {
    assert(t->InputSymbols() != NULL);
    fst::SymbolTable * st = t->InputSymbols()->Copy();
    st->AddSymbol(symbol);
    t->SetInputSymbols(st);
    delete st;
  }

  void LogWeightTransducer::remove_from_alphabet
  (LogFst *t, const std::string &symbol)
  {
    assert(t->InputSymbols() != NULL);
    fst::SymbolTable st(t->InputSymbols()->Name());
    for ( fst::SymbolTableIterator it
            = fst::SymbolTableIterator(*(t->InputSymbols()));
          ! it.Done(); it.Next() ) {
      if (it.Symbol() != symbol) {
    st.AddSymbol(it.Symbol(), it.Value());
      }
    }
    t->SetInputSymbols(&st);
  }

  StringSet LogWeightTransducer::get_alphabet(LogFst *t)
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

  unsigned int LogWeightTransducer::get_symbol_number
  (LogFst *t,
   const std::string &symbol)
  {
    assert(t->InputSymbols() != NULL);
    int64 i = t->InputSymbols()->Find(symbol);
    if (i < 0)
      HFST_THROW(SymbolNotFoundException);
    return (unsigned int)i;
  }

  /* Find the number-to-number mappings needed to be performed to t1
     so that it will follow the same symbol-to-number encoding as t2.
     @pre t2's symbol table must contain all symbols in t1's symbol table.
  */
  NumberNumberMap LogWeightTransducer::create_mapping(LogFst *t1, LogFst *t2)
  {
    NumberNumberMap km;
    // find the number-to-number mappings for transducer t1
    for ( fst::SymbolTableIterator it
            = fst::SymbolTableIterator(*(t1->InputSymbols()));
          ! it.Done(); it.Next() ) {
      km [ (unsigned int)it.Value() ]
        = (unsigned int) t2->InputSymbols()->Find( it.Symbol() );
    }
    return km;
  }

  /* Recode the symbol numbers in this transducer as indicated in KeyMap km. */
  void LogWeightTransducer::recode_symbol_numbers
    (LogFst *t, NumberNumberMap &km)
  {
    for (fst::StateIterator<LogFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        for (fst::MutableArcIterator<LogFst> aiter(t,s);
             !aiter.Done(); aiter.Next())
          {
            const LogArc &arc = aiter.Value();
            LogArc new_arc;
            new_arc.ilabel = km[arc.ilabel];
            new_arc.olabel = km[arc.olabel];
            new_arc.weight = arc.weight;
            new_arc.nextstate = arc.nextstate;
            aiter.SetValue(new_arc);
          }
      }
    return;
  }

  LogFst * LogWeightTransducer::set_final_weights(LogFst * t, float weight)
  {
    for (fst::StateIterator<LogFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if ( t->Final(s) != LogWeight::Zero() )
          t->SetFinal(s, weight);
      }
    return t;
  }

  LogFst * LogWeightTransducer::push_weights(LogFst * t, bool to_initial_state)
  {
    assert (t->InputSymbols() != NULL);
    LogFst * retval = new LogFst();
    if (to_initial_state)
      fst::Push<LogArc, REWEIGHT_TO_INITIAL>(*t, retval, fst::kPushWeights);
    else
      fst::Push<LogArc, REWEIGHT_TO_FINAL>(*t, retval, fst::kPushWeights);
    retval->SetInputSymbols(t->InputSymbols());
    return retval;
  }

  LogFst * LogWeightTransducer::transform_weights
    (LogFst * t,float (*func)(float f))
  {
    for (fst::StateIterator<LogFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if ( t->Final(s) != LogWeight::Zero() )
          t->SetFinal( s, func(t->Final(s).Value()) );
        for (fst::MutableArcIterator<LogFst> aiter(t,s);
             !aiter.Done(); aiter.Next())
          {
            const LogArc &arc = aiter.Value();
            LogArc new_arc;
            new_arc.ilabel = arc.ilabel;
            new_arc.olabel = arc.olabel;
            new_arc.weight = func(arc.weight.Value());
            new_arc.nextstate = arc.nextstate;
            aiter.SetValue(new_arc);
          }
      }
    return t;
  }

  void LogWeightTransducer::write_in_att_format(LogFst *t, FILE *ofile)
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
      
    for (fst::StateIterator<LogFst> siter(*t);
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
        for (fst::ArcIterator<LogFst> aiter(*t,s); !aiter.Done(); aiter.Next())
          {
            const LogArc &arc = aiter.Value();
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
        if (t->Final(s) != LogWeight::Zero())
          fprintf(ofile, "%i\t%f\n", origin, t->Final(s).Value());
        break;
        }
      }

    for (fst::StateIterator<LogFst> siter(*t);
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
          for (fst::ArcIterator<LogFst> aiter(*t,s);
               !aiter.Done(); aiter.Next())
            {
              const LogArc &arc = aiter.Value();
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
          if (t->Final(s) != LogWeight::Zero())
            fprintf(ofile, "%i\t%f\n", origin, t->Final(s).Value());
        }
      }
  }


  void LogWeightTransducer::write_in_att_format_number(LogFst *t, FILE *ofile)
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
      
    for (fst::StateIterator<LogFst> siter(*t);
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
        for (fst::ArcIterator<LogFst> aiter(*t,s); !aiter.Done(); aiter.Next())
          {
            const LogArc &arc = aiter.Value();
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
        if (t->Final(s) != LogWeight::Zero())
          fprintf(ofile, "%i\t%f\n", origin, t->Final(s).Value());
        break;
        }
      }

    for (fst::StateIterator<LogFst> siter(*t);
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
          for (fst::ArcIterator<LogFst> aiter(*t,s);
               !aiter.Done(); aiter.Next())
            {
              const LogArc &arc = aiter.Value();
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
          if (t->Final(s) != LogWeight::Zero())
            fprintf(ofile, "%i\t%f\n", origin, t->Final(s).Value());
        }
      }
  }


  void LogWeightTransducer::write_in_att_format(LogFst *t, std::ostream &os)
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
      
    for (fst::StateIterator<LogFst> siter(*t);
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
        for (fst::ArcIterator<LogFst> aiter(*t,s); !aiter.Done(); aiter.Next())
          {
            const LogArc &arc = aiter.Value();
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
        if (t->Final(s) != LogWeight::Zero())
          os << origin << "\t"
             << t->Final(s).Value() << "\n";
        break;
        }
      }

    for (fst::StateIterator<LogFst> siter(*t);
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
          for (fst::ArcIterator<LogFst> aiter(*t,s);
               !aiter.Done(); aiter.Next())
            {
              const LogArc &arc = aiter.Value();
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
          if (t->Final(s) != LogWeight::Zero())
            os << origin << "\t"
               << t->Final(s).Value() << "\n";
        }
      }
  }


  void LogWeightTransducer::write_in_att_format_number
    (LogFst *t, std::ostream &os)
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
      
    for (fst::StateIterator<LogFst> siter(*t);
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
        for (fst::ArcIterator<LogFst> aiter(*t,s); !aiter.Done(); aiter.Next())
          {
            const LogArc &arc = aiter.Value();
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
        if (t->Final(s) != LogWeight::Zero())
          os << origin << "\t"
             << t->Final(s).Value() << "\n";
        break;
        }
      }

    for (fst::StateIterator<LogFst> siter(*t);
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
          for (fst::ArcIterator<LogFst> aiter(*t,s);
               !aiter.Done(); aiter.Next())
            {
              const LogArc &arc = aiter.Value();
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
          if (t->Final(s) != LogWeight::Zero())
            os << origin << "\t"
               << t->Final(s).Value() << "\n";
        }
      }
  }
  

  // AT&T format is handled here ------------------------------

  /* A method used by function 'read_in_att_format'.
     Returns the state id of state number state_number and adds a new
     state to t if state_number is encountered for the first time and
     updates state_map accordingly. */
  StateId LogWeightTransducer::add_and_map_state
  (LogFst *t, int state_number, StateMap &state_map)
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
  /* Reads a description of a transducer in AT&T text format
     and returns a corresponding binary transducer.
     @note The initial state must be numbered as zero. */
  LogFst * LogWeightTransducer::read_in_att_format(FILE * ifile)
  {
    LogFst *t = new LogFst();
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
        //printf("read line: %s", line);
        char a1 [100]; char a2 [100]; char a3 [100];
        char a4 [100]; char a5 [100];
        int n = sscanf(line, "%s\t%s\t%s\t%s\t%s", a1, a2, a3, a4, a5);
        //printf("number of arguments: (%i)\n", n);

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
                      LogArc(input_number, output_number,
                             weight, target_state));
          }

        else  // line could not be parsed
          {
            //printf("ERROR: in AT&T file: line: \"%s\"\n", line);
            //throw NotValidAttFormatException();
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
     "?:?", "?:x" and "x:?"
     are expanded according to the StringSymbolSet 'unknown' that lists
     all symbols previously
     unknown to this transducer.
  */
  LogFst * LogWeightTransducer::expand_arcs(LogFst * t, StringSet &unknown,
                                            bool unknown_symbols_in_use)
  {

    LogFst * result = new LogFst();

    for (fst::StateIterator<LogFst> siter(*t);
         ! siter.Done(); siter.Next())
      result->AddState();

    // go through all states in this
    for (fst::StateIterator<LogFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        // create new state in result, if needed
        StateId s = siter.Value();
        StateId result_s=s;

        // make the new state initial, if needed
        if (t->Start() == s)
          result->SetStart(result_s);

        // make the new state final, if needed
        if (t->Final(s) != LogWeight::Zero())
          result->SetFinal(result_s, t->Final(s).Value());


        // go through all the arcs in this
        for (fst::ArcIterator<LogFst> aiter(*t,s); !aiter.Done(); aiter.Next())
          {
            const LogArc &arc = aiter.Value();

            // find the corresponding target state in result or,
            // if not found, create a new state
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
                    int64 inumber = is->Find(*it1);
                    for (StringSet::iterator it2 = unknown.begin();
                         it2 != unknown.end(); it2++)
                      {
                        int64 onumber = is->Find(*it2);
                        if (inumber != onumber)
                          result->AddArc(result_s,
                                         LogArc(inumber, onumber,
                                                arc.weight, result_nextstate));
                      }
                    result->AddArc(result_s,
                                   LogArc(inumber, 1, arc.weight,
                                          result_nextstate));
                    result->AddArc(result_s,
                                   LogArc(1, inumber, arc.weight,
                                          result_nextstate));
                  }
              }
            else if (arc.ilabel == 2 ||   // identity "?:?"
                     arc.olabel == 2 )
              {
                for (StringSet::iterator it = unknown.begin();
                     it != unknown.end(); it++)
                  {
                    int64 number = is->Find(*it);
                    result->AddArc(result_s,
                                   LogArc(number, number, arc.weight,
                                          result_nextstate));
                  }
              }
            else if (arc.ilabel == 1)  // "?:x"
              {
                for (StringSet::iterator it = unknown.begin();
                     it != unknown.end(); it++)
                  {
                    int64 number = is->Find(*it);
                    result->AddArc(result_s,
                                   LogArc(number, arc.olabel, arc.weight,
                                          result_nextstate));
                  }
              }
            else if (arc.olabel == 1)  // "x:?"
              {
                for (StringSet::iterator it = unknown.begin();
                     it != unknown.end(); it++)
                  {
                    int64 number = is->Find(*it);
                    result->AddArc(result_s,
                                   LogArc(arc.ilabel, number,
                                          arc.weight, result_nextstate));
                  }
              }
            }

            // the original transition is copied in all cases
            result->AddArc(result_s,
                           LogArc(arc.ilabel, arc.olabel,
                                  arc.weight, result_nextstate));
            
          }
      }

    //result->SetInputSymbols(t->InputSymbols());
    return result;
  }

  unsigned int LogWeightTransducer::number_of_states(const LogFst *t)
  {
    unsigned int retval=0;
    for (fst::StateIterator<LogFst> siter(*t);
         ! siter.Done(); siter.Next())
      retval++;
    return retval;
  }

  std::pair<LogFst*, LogFst*> LogWeightTransducer::harmonize
  (LogFst *t1, LogFst *t2, bool unknown_symbols_in_use)
  {
    clock_t startclock = clock();

    // 1. Calculate the set of unknown symbols for transducers t1 and t2.

    StringSet unknown_t1;    // symbols known to another but not this
    StringSet unknown_t2;    // and vice versa

    StringSet t1_symbols = get_alphabet(t1);
    StringSet t2_symbols = get_alphabet(t2);
    hfst::symbols::collect_unknown_sets(t1_symbols, unknown_t1,
                    t2_symbols, unknown_t2);
    
    // 2. Add new symbols from transducer t1 to the symbol table
    //    of transducer t2...

    SymbolTable * st2 = t2->InputSymbols()->Copy();
    for ( StringSet::const_iterator it = unknown_t2.begin();
          it != unknown_t2.end(); it++ ) {
        st2->AddSymbol(*it);
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
    
    LogFst *harmonized_t1;
    LogFst *harmonized_t2;

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

    clock_t endclock = clock();

    log_seconds_in_harmonize = log_seconds_in_harmonize +
      ( (float)(endclock - startclock) / CLOCKS_PER_SEC);

    return std::pair<LogFst*, LogFst*>(harmonized_t1, harmonized_t2);

  }



  /* Skip the identifier string "LOG_OFST_TYPE" */
  void LogWeightInputStream::skip_identifier_version_3_0(void)
  { input_stream.ignore(14); }

  void LogWeightInputStream::skip_hfst_header(void)
  {
    input_stream.ignore(6);
    //char c;
    //i_stream.get(c);
    //switch (c)
    //{
    //case 0:
    skip_identifier_version_3_0();
    //break;
    //default:
    //assert(false);
    //}
  }
  
  void LogWeightInputStream::close(void)
  {
    if (filename != string())
      { i_stream.close(); }
  }
  bool LogWeightInputStream::is_eof(void) const
  {
    return input_stream.peek() == EOF;
  }
  bool LogWeightInputStream::is_bad(void) const
  {
    if (filename == string())
      { return std::cin.bad(); }
    else
      { return input_stream.bad(); }
  }
  bool LogWeightInputStream::is_good(void) const
  {
    if(is_eof())
      return false;
    if (filename == string())
      { return std::cin.good(); }
    else
      { return input_stream.good(); }
  }
  
  bool LogWeightInputStream::is_fst(void) const
  {
    return is_fst(input_stream);
  }
  
  bool LogWeightInputStream::is_fst(FILE * f)
  {
    if (f == NULL)
      { return false; }
    int c = getc(f);
    ungetc(c, f);
    return c == 0xd6;
  }
  
  bool LogWeightInputStream::is_fst(istream &s)
  {
    return s.good() && (s.peek() == 0xd6);
  }

  bool LogWeightInputStream::operator() (void) const
  { return is_good(); }

  void LogWeightInputStream::ignore(unsigned int n)
  { input_stream.ignore(n); }

  LogFst * LogWeightInputStream::read_transducer()
  {
    if (is_eof())
      { //throw StreamIsClosedException();
        HFST_THROW(StreamIsClosedException); }
    LogFst * t;
    FstHeader header;
    try
      {
        if (filename == string())
          {
            header.Read(input_stream,"STDIN");
            t = static_cast<LogFst*>
              (LogFst::Read(input_stream,
                                  FstReadOptions("STDIN",
                                                 &header)));
          }
        else
          {
            header.Read(input_stream,filename);
            t = static_cast<LogFst*>
              (LogFst::Read(input_stream,
                                  FstReadOptions(filename,
                                                 &header)));
          }
        if (t == NULL)
          { //throw TransducerHasWrongTypeException();
            HFST_THROW(TransducerHasWrongTypeException); }
      }
    //catch (TransducerHasWrongTypeException e)
    catch (const HfstException e)
      { throw e; }

    try
      {
        return t;
      }
    //catch (HfstInterfaceException e)
    catch (const HfstException e)
      { throw e; }
  }

  /*
  LogWeightStateIterator::LogWeightStateIterator(LogFst * t):
    iterator(new StateIterator<LogFst>(*t))
  {}

  LogWeightStateIterator::~LogWeightStateIterator(void)
  { delete iterator; }

  void LogWeightStateIterator::next(void)
  {
    iterator->Next();
  }

  bool LogWeightStateIterator::done(void)
  {
    return iterator->Done();
  }

  LogWeightState LogWeightStateIterator::value(void)
  {
    return iterator->Value();
  }



  LogWeightTransition::LogWeightTransition(const LogArc &arc, LogFst *t):
    arc(arc), t(t)
  {}

  LogWeightTransition::~LogWeightTransition(void)
  {}

  std::string LogWeightTransition::get_input_symbol(void) const
  {
    return t->InputSymbols()->Find(arc.ilabel);
  }

  std::string LogWeightTransition::get_output_symbol(void) const
  {
    return t->InputSymbols()->Find(arc.olabel);
  }

  LogWeightState LogWeightTransition::get_target_state(void) const
  {
    return arc.nextstate;
  }

  LogWeight LogWeightTransition::get_weight(void) const
  {
    return arc.weight;
  }



  LogWeightTransitionIterator::LogWeightTransitionIterator
  (LogFst *t, StateId state):
    arc_iterator(new ArcIterator<LogFst>(*t, state)),
    t(t)
  {}

  LogWeightTransitionIterator::~LogWeightTransitionIterator(void)
  {}

  void LogWeightTransitionIterator::next()
  {
    arc_iterator->Next();
  }

  bool LogWeightTransitionIterator::done()
  {
    return arc_iterator->Done();
  }

  LogWeightTransition LogWeightTransitionIterator::value()
  {
    return LogWeightTransition(arc_iterator->Value(), this->t);
    }*/


  fst::SymbolTable LogWeightTransducer::create_symbol_table(std::string name) {
    fst::SymbolTable st(name);
    st.AddSymbol(internal_epsilon, 0);
    st.AddSymbol(internal_unknown, 1);
    st.AddSymbol(internal_identity, 2);
    return st;
  }
  
  void LogWeightTransducer::initialize_symbol_tables(LogFst *t) {
    SymbolTable st = create_symbol_table("");
    t->SetInputSymbols(&st);
  }

  LogFst * LogWeightTransducer::create_empty_transducer(void)
  {
    LogFst * t = new LogFst;
    initialize_symbol_tables(t);
    StateId s = t->AddState();
    t->SetStart(s);
    return t;
  }

  LogFst * LogWeightTransducer::create_epsilon_transducer(void)
  {
    LogFst * t = new LogFst;
    initialize_symbol_tables(t);
    StateId s = t->AddState();
    t->SetStart(s);
    t->SetFinal(s,0);
    return t;
  }

  void LogWeightTransducer::delete_transducer(LogFst * t)
  {
    delete t;
  }

  LogFst * LogWeightTransducer::define_transducer(unsigned int number)
  {
    LogFst * t = new LogFst;
    initialize_symbol_tables(t);
    StateId s1 = t->AddState();
    StateId s2 = t->AddState();
    t->SetStart(s1);
    t->SetFinal(s2,0);
    t->AddArc(s1,LogArc(number,number,0,s2));
    return t;
  }
  LogFst * LogWeightTransducer::define_transducer
    (unsigned int inumber, unsigned int onumber)
  {
    LogFst * t = new LogFst;
    initialize_symbol_tables(t);
    StateId s1 = t->AddState();
    StateId s2 = t->AddState();
    t->SetStart(s1);
    t->SetFinal(s2,0);
    t->AddArc(s1,LogArc(inumber,onumber,0,s2));
    return t;
  }


  LogFst * LogWeightTransducer::define_transducer(const std::string &symbol)
  {
    LogFst * t = new LogFst;
    SymbolTable st = create_symbol_table("");

    StateId s1 = t->AddState();
    StateId s2 = t->AddState();
    t->SetStart(s1);
    t->SetFinal(s2,0);
    t->AddArc(s1,LogArc(st.AddSymbol(symbol),
                        st.AddSymbol(symbol),0,s2));
    t->SetInputSymbols(&st);
    return t;
  }

  LogFst * LogWeightTransducer::define_transducer
    (const std::string &isymbol, const std::string &osymbol)
  {
    LogFst * t = new LogFst;
    SymbolTable st = create_symbol_table("");

    StateId s1 = t->AddState();
    StateId s2 = t->AddState();
    t->SetStart(s1);
    t->SetFinal(s2,0);
    t->AddArc(s1,LogArc(st.AddSymbol(isymbol),
                        st.AddSymbol(osymbol),0,s2));
    t->SetInputSymbols(&st);
    return t;
  }

  bool LogWeightTransducer::are_equivalent(LogFst *a, LogFst *b)
  {
    LogFst * mina = minimize(a);
    LogFst * minb = minimize(b);
    //write_in_att_format_number(a, stdout);
    //std::cerr << "--\n";
    //write_in_att_format_number(b, stdout);
    //std::cerr << "\n\n";
    EncodeMapper<LogArc> encode_mapper(0x0001,ENCODE);
    EncodeFst<LogArc> enca(*mina, &encode_mapper);
    EncodeFst<LogArc> encb(*minb, &encode_mapper);
    LogFst A(enca);
    LogFst B(encb);
    return Equivalent(A, B);
  }
  
  bool LogWeightTransducer::is_automaton(LogFst * t)
  {
    for (fst::StateIterator<LogFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        for (fst::ArcIterator<LogFst> aiter(*t,s);
             !aiter.Done(); aiter.Next())
          {
            const LogArc &arc = aiter.Value();
            if (arc.ilabel != arc.olabel)
              return false;
            if (arc.ilabel == 1) // ?:?
              return false;
          }
      }
    return true;
  }


  bool LogWeightTransducer::is_cyclic(LogFst * t)
  {
    return t->Properties(kCyclic, true) & kCyclic;
  }


  LogFst * LogWeightTransducer::define_transducer
  (const StringPairVector &spv)
  {
    LogFst * t = new LogFst;
    SymbolTable st = create_symbol_table("");

    StateId s1 = t->AddState();
    t->SetStart(s1);
    for (StringPairVector::const_iterator it = spv.begin();
         it != spv.end();
         ++it)
      {
        StateId s2 = t->AddState();
        t->AddArc(s1,LogArc(st.AddSymbol(it->first),
                            st.AddSymbol(it->second),0,s2));
        s1 = s2;
      }
    t->SetFinal(s1,0);
    t->SetInputSymbols(&st);
    return t;
  }

  LogFst * LogWeightTransducer::define_transducer
  (const StringPairSet &sps, bool cyclic)
  {
    LogFst * t = new LogFst;
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
          t->AddArc(s1,LogArc(st.AddSymbol(it->first),
                              st.AddSymbol(it->second),0,s2));
        }
    }
    t->SetFinal(s2,0);
    t->SetInputSymbols(&st);
    return t;
  }

  LogFst * LogWeightTransducer::define_transducer
  (const std::vector<StringPairSet> &spsv)
  {
    LogFst * t = new LogFst;
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
          t->AddArc(s1,LogArc(st.AddSymbol(it2->first),
                              st.AddSymbol(it2->second),0,s2));
        }
        s1 = s2;
      }
    t->SetFinal(s1,0);
    t->SetInputSymbols(&st);
    return t;
  }


  LogFst * LogWeightTransducer::define_transducer
  (const NumberPairVector &npv)
  {
    LogFst * t = new LogFst;

    StateId s1 = t->AddState();
    t->SetStart(s1);
    for (NumberPairVector::const_iterator it = npv.begin();
         it != npv.end();
         ++it)
      {
        StateId s2 = t->AddState();
        t->AddArc(s1,LogArc(it->first,it->second,0,s2));
        s1 = s2;
      }
    t->SetFinal(s1,0);
    return t;
  }

  LogFst * LogWeightTransducer::define_transducer
  (const NumberPairSet &nps, bool cyclic)
  {
    LogFst * t = new LogFst;

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
          t->AddArc(s1,LogArc(it->first,it->second,0,s2));
        }
    }
    t->SetFinal(s2,0);
    return t;
  }

  LogFst * LogWeightTransducer::define_transducer
  (const std::vector<NumberPairSet> &npsv)
  {
    LogFst * t = new LogFst;

    StateId s1 = t->AddState();
    t->SetStart(s1);
    for (std::vector<NumberPairSet>::const_iterator it = npsv.begin();
         it != npsv.end();
         ++it)
      {
        StateId s2 = t->AddState();
        for (NumberPairSet::const_iterator it2 = (*it).begin();
             it2 != (*it).end(); it2++ ) {
          t->AddArc(s1,LogArc(it2->first,it2->second,0,s2));
        }
        s1 = s2;
      }
    t->SetFinal(s1,0);
    return t;
  }


  LogFst *
  LogWeightTransducer::copy(LogFst * t)
  { return new LogFst(*t); }


  LogFst *
  LogWeightTransducer::determinize(LogFst * t)
  {
    RmEpsilon<LogArc>(t);
    EncodeMapper<LogArc> encode_mapper(kEncodeLabels|kEncodeWeights,ENCODE);
    Encode(t, &encode_mapper);
    LogFst * det = new LogFst();
    Determinize<LogArc>(*t, det);
    Decode(det, encode_mapper);
    return det;
  }


  LogFst * LogWeightTransducer::minimize
  (LogFst * t)
  {
    RmEpsilon<LogArc>(t);
    EncodeMapper<LogArc> encode_mapper(kEncodeLabels/*|kEncodeWeights*/,ENCODE);
    Encode(t, &encode_mapper);
    LogFst * det = new LogFst();
    Determinize<LogArc>(*t, det);
    Minimize<LogArc>(det);
    Decode(det, encode_mapper);
    return det;
  }


  void print_att_number(LogFst *t, FILE * ofile) {
    fprintf(ofile, "initial state: %i\n", t->Start());
    for (fst::StateIterator<LogFst> siter(*t);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        if ( t->Final(s) != LogWeight::Zero() )
          fprintf(ofile, "%i\t%f\n", s, t->Final(s).Value());
        for (fst::ArcIterator<LogFst> aiter(*t,s); !aiter.Done(); aiter.Next())
          {
            const LogArc &arc = aiter.Value();
            fprintf(ofile, "%i\t%i\t%i\t%i\t%f\n",
                    s, arc.nextstate, arc.ilabel, arc.olabel,
                    arc.weight.Value());
          }
      }
  }

  void LogWeightTransducer::test_minimize(void)
  {
    LogFst * t = new LogFst();
    StateId initial = t->AddState();
    t->SetStart(initial);
    StateId state = t->AddState();
    t->SetFinal(state, 0.5);
    t->AddArc(initial, LogArc(1,1,0.5,state));
    t->AddArc(state, LogArc(2,2,0.5,initial));
 
    // print t before minimize
    print_att_number(t, stderr);

    // epsilon removal
    RmEpsilonFst<LogArc> t_rm_eps(*t);

    // encode mapping
    EncodeMapper<LogArc> encode_mapper(0x0001,ENCODE);
    EncodeFst<LogArc> t_rm_eps_enc(t_rm_eps,
                                   &encode_mapper);

    // determinization
    DeterminizeFst<LogArc> t_DET(t_rm_eps_enc);

    // minimization
    DecodeFst<LogArc> dec(t_DET,
                          encode_mapper);
    LogFst t_det_std(t_DET);

    Minimize<LogArc>(&t_det_std);

    // print t after minimize
    print_att_number(&t_det_std, stderr);

    // decoding
    //Decode<LogArc>(&t_det, encode_mapper);  COMMENTED
  }

  /* For HfstMutableTransducer */

  StateId
  LogWeightTransducer::add_state(LogFst *t)
  {
    StateId s = t->AddState();
    if (s == 0)
      t->SetStart(s);
    return s;
  }

  void
  LogWeightTransducer::set_final_weight(LogFst *t, StateId s, float w)
  {
    t->SetFinal(s, w);
    return;
  }

  void
  LogWeightTransducer::add_transition
  (LogFst *t, StateId source, std::string &isymbol, std::string &osymbol,
   float w, StateId target)
  {
    SymbolTable *st = t->InputSymbols()->Copy();
    /*if (t->InputSymbols() != t->OutputSymbols()) {
      fprintf(stderr, "ERROR:  LogWeightTransducer::add_transition:"
      "  input and output symbols are not the same\n");
      throw ErrorException();
      }*/
    unsigned int ilabel = st->AddSymbol(isymbol);
    unsigned int olabel = st->AddSymbol(osymbol);
    t->AddArc(source, LogArc(ilabel, olabel, w, target));
    t->SetInputSymbols(st);
    delete st;
    return;
  }

  float
  LogWeightTransducer::get_final_weight(LogFst *t, StateId s)
  {
    return t->Final(s).Value();
  }

  float
  LogWeightTransducer::is_final(LogFst *t, StateId s)
  {
    return ( t->Final(s) != LogWeight::Zero() );
  }

  StateId
  LogWeightTransducer::get_initial_state(LogFst *t)
  {
    return t->Start();
  }

  LogFst *
  LogWeightTransducer::remove_epsilons(LogFst * t)
  { return new LogFst(RmEpsilonFst<LogArc>(*t)); }

  LogFst *
  LogWeightTransducer::n_best(LogFst * t, unsigned int n)
  {
    LogFst * n_best_fst = new LogFst();
    fst::ShortestPath(*t,n_best_fst,(size_t)n);
    return n_best_fst;
  }

  LogFst *
  LogWeightTransducer::repeat_star(LogFst * t)
  { return new LogFst(ClosureFst<LogArc>(*t,CLOSURE_STAR)); }

  LogFst *
  LogWeightTransducer::repeat_plus(LogFst * t)
  { return new LogFst(ClosureFst<LogArc>(*t,CLOSURE_PLUS)); }

  LogFst *
  LogWeightTransducer::repeat_n(LogFst * t, unsigned int n)
  {
    if (n == 0)
      { return create_epsilon_transducer(); }

    LogFst * repetition = create_epsilon_transducer();
    for (unsigned int i = 0; i < n; ++i)
      { Concat(repetition,*t); }
    repetition->SetInputSymbols(t->InputSymbols());
    return repetition;
  }

  LogFst *
  LogWeightTransducer::repeat_le_n(LogFst * t, unsigned int n)
  {
    if (n == 0)
      { return create_epsilon_transducer(); }

    LogFst * repetition = create_epsilon_transducer();
    for (unsigned int i = 0; i < n; ++i)
      {
        LogFst * optional_t = optionalize(t);
        Concat(repetition,*optional_t);
        delete optional_t;
      }
    repetition->SetInputSymbols(t->InputSymbols());
    return repetition;
  }

  LogFst *
  LogWeightTransducer::optionalize(LogFst * t)
  {
    LogFst * eps = create_epsilon_transducer();
    Union(eps,*t);
    eps->SetInputSymbols(t->InputSymbols());
    return eps;
  }


  LogFst *
  LogWeightTransducer::invert(LogFst * t)
  {
    LogFst * inverse = copy(t);
    Invert(inverse);
    inverse->SetInputSymbols(t->InputSymbols());
    return inverse;
  }

  /* Makes valgrind angry... */
  LogFst *
  LogWeightTransducer::reverse(LogFst * t)
  {
    LogFst * reversed = new LogFst;
    Reverse<LogArc,LogArc>(*t,reversed);
    reversed->SetInputSymbols(t->InputSymbols());
    return reversed;
  }

  LogFst * LogWeightTransducer::extract_input_language
  (LogFst * t)
  { LogFst * retval =  new LogFst(ProjectFst<LogArc>(*t,PROJECT_INPUT));
    retval->SetInputSymbols(t->InputSymbols());
    return retval; }

  LogFst * LogWeightTransducer::extract_output_language
  (LogFst * t)
  { LogFst * retval = new LogFst(ProjectFst<LogArc>(*t,PROJECT_OUTPUT));
    retval->SetInputSymbols(t->InputSymbols());
    return retval; }
  
  typedef std::pair<int,int> LabelPair;
  typedef std::vector<LabelPair> LabelPairVector;



  /*static LogFst * substitute(LogFst * t,
                                   StringPair old_symbol_pair,
                                   StringPair new_symbol_pair);
  static LogFst * substitute(LogFst * t,
                                   const StringPair old_symbol_pair,
                                   LogFst *transducer);*/

  LogFst * LogWeightTransducer::insert_freely
  (LogFst * t, const StringPair &symbol_pair)
  {
    SymbolTable * st = t->InputSymbols()->Copy();
    assert(st != NULL);
    for (fst::StateIterator<LogFst> siter(*t); !siter.Done(); siter.Next()) {
      StateId state_id = siter.Value();
      t->AddArc(state_id, fst::LogArc(st->AddSymbol(symbol_pair.first),
                                      st->AddSymbol(symbol_pair.second),
                                      0, state_id));
    }
    t->SetInputSymbols(st);
    delete st;
    return t;
    }

  /*static LogFst * insert_freely(LogFst * t, const NumberPair &number_pair)
  {
    for (fst::StateIterator<LogFst> siter(*t); !siter.Done(); siter.Next()) {
      StateId state_id = siter.Value();
      t->AddArc(state_id, fst::LogArc(number_pair.first, number_pair.second,
                                      0, state_id));
    }
    return t;
    }*/

  /*
  LogFst * LogWeightTransducer::substitute
  (LogFst *t, void (*func)(std::string &isymbol, std::string &osymbol) )
  {
    LogFst * tc = t->Copy();
    SymbolTable * st = tc->InputSymbols()->Copy();
    assert(st != NULL);

    for (fst::StateIterator<LogFst> siter(*tc);
         not siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        for (fst::MutableArcIterator<LogFst> aiter(tc,s);
        !aiter.Done(); aiter.Next())
          {
            const LogArc &arc = aiter.Value(); // current values
            LogArc new_arc;                    // new values
            
            std::string istring = st->Find(arc.ilabel);
            std::string ostring = st->Find(arc.olabel);
            func(istring,ostring);
            new_arc.ilabel = st->AddSymbol(istring);
            new_arc.olabel = st->AddSymbol(ostring);
            // copy weight and next state as such
            new_arc.weight = arc.weight.Value();
            new_arc.nextstate = arc.nextstate;
            aiter.SetValue(new_arc);
          }
      }
    tc->SetInputSymbols(st);
    delete st;
    return tc;
  }
  */

  LogFst * LogWeightTransducer::substitute
  (LogFst * t,unsigned int old_key,unsigned int new_key)
  {
    LabelPairVector v;
    v.push_back(LabelPair(old_key,new_key));
    RelabelFst<LogArc> t_subst(*t,v,v);
    return new LogFst(t_subst);
  }
  
  LogFst * LogWeightTransducer::substitute
  (LogFst * t,
   NumberPair old_key_pair,
   NumberPair new_key_pair)
  {
    EncodeMapper<LogArc> encode_mapper(0x0001,ENCODE);
    EncodeFst<LogArc> enc(*t,&encode_mapper);

    LogArc old_pair_code =
      encode_mapper(LogArc(old_key_pair.first,old_key_pair.second,0,0));
    LogArc new_pair_code =
      encode_mapper(LogArc(new_key_pair.first,new_key_pair.second,0,0));

    // First cast up, then cast down... For some reason dynamic_cast<LogFst*>
    // doesn't work although both EncodeFst<LogArc> and LogFst extend
    // Fst<LogArc>.
    // reinterpret_cast worked, but that is apparently unsafe...
    LogFst * subst =
      substitute(static_cast<LogFst*>(static_cast<Fst<LogArc>*>(&enc)),
                 static_cast<unsigned int>(old_pair_code.ilabel),
                 static_cast<unsigned int>(new_pair_code.ilabel));

    DecodeFst<LogArc> dec(*subst,encode_mapper);
    delete subst;
    return new LogFst(dec);
  }

  /* It is not certain whether the transition iterator goes through all
     the transitions that are added
     during the substitution. In that case, this function should build
     a new transducer instead of
     modifying the original one. */
  LogFst * LogWeightTransducer::substitute
  (LogFst *t,
   StringPair old_symbol_pair,
   StringPairSet new_symbol_pair_set)
  {
    LogFst * tc = t->Copy();
    fst::SymbolTable * st = tc->InputSymbols()->Copy();
    assert(st != NULL);
    for (fst::StateIterator<LogFst> siter(*tc);
         ! siter.Done(); siter.Next())
      {
        StateId s = siter.Value();
        for (fst::MutableArcIterator<LogFst> aiter(tc,s);
             !aiter.Done(); aiter.Next())
          {
            const LogArc &arc = aiter.Value();
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
                      LogArc new_arc;
                      new_arc.ilabel = st->AddSymbol(it->first);
                      new_arc.olabel = st->AddSymbol(it->second);
                      new_arc.weight = arc.weight.Value();
                      new_arc.nextstate = arc.nextstate;
                      aiter.SetValue(new_arc);
                      first_substitution=false; }
                    else
                      tc->AddArc(s, LogArc(st->AddSymbol(it->first),
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

  LogFst * LogWeightTransducer::substitute(LogFst *t,
                                                      std::string old_symbol,
                                                      std::string new_symbol)
  {
    assert(t->InputSymbols() != NULL);
    SymbolTable * st = t->InputSymbols()->Copy();
    LogFst * retval = substitute(t, st->AddSymbol(old_symbol),
                                 st->AddSymbol(new_symbol));
    retval->SetInputSymbols(st);
    delete st;
    return retval;
  }

  LogFst * LogWeightTransducer::substitute(LogFst *t,
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
    LogFst * retval = substitute(t, old_pair, new_pair);
    retval->SetInputSymbols(st);
    delete st;
    return retval;
  }

  LogFst * LogWeightTransducer::substitute(LogFst *t,
                                           const StringPair old_symbol_pair,
                                           LogFst *transducer)
  {

    assert(t->InputSymbols() != NULL);
    SymbolTable * st = t->InputSymbols()->Copy();

    int states = t->NumStates();
    for( int i = 0; i < states; ++i ) {

      for (fst::MutableArcIterator<LogFst> it(t,i);
           ! it.Done();
           it.Next()) {

        fst::LogArc arc = it.Value();

        // find arcs that must be replaced
        if ( arc.ilabel == st->AddSymbol(old_symbol_pair.first) &&
             arc.olabel == st->AddSymbol(old_symbol_pair.second) )
          {

          StateId destination_state = arc.nextstate;
          StateId start_state = t->AddState();

          // change the label of the arc to epsilon and point the arc
          // to a new state
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
          for (fst::StateIterator<LogFst> siter(*transducer);
               !siter.Done(); siter.Next()) {

            StateId tr_state_id = siter.Value();

            // final states in tr correspond in t to a non-final state which has
            // an epsilon transition to original destination state of arc that
            // is being replaced
            if ( is_final( transducer, tr_state_id ) )
              t->AddArc( tr_state_id + start_state,
                         fst::LogArc( 0,
                                      0,
                                      /* final weight is copied
                                         to the epsilon transition */
                                      transducer->Final(tr_state_id),
                                      destination_state
                                      )
                         );

            for (fst::ArcIterator<LogFst> aiter(*transducer, tr_state_id);
                 !aiter.Done(); aiter.Next()) {

              const fst::LogArc &tr_arc = aiter.Value();

              // adding arc from state 'tr_state_id+start_state'
              // to state 'tr_arc.nextstate'
              // copy arcs from tr to t
              t->AddArc( tr_state_id + start_state,
                         fst::LogArc( tr_arc.ilabel,
                                      tr_arc.olabel,
                                      tr_arc.weight,  /* weight remains
                                                         the same */
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

  LogFst * LogWeightTransducer::substitute(LogFst *t,
                                           const NumberPair old_number_pair,
                                           LogFst *transducer)
  {
    int states = t->NumStates();
    for( int i = 0; i < states; ++i ) {

      for (fst::MutableArcIterator<LogFst> it(t,i);
           ! it.Done();
           it.Next()) {

        fst::LogArc arc = it.Value();

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
          for (fst::StateIterator<LogFst> siter(*transducer);
               !siter.Done(); siter.Next()) {

            StateId tr_state_id = siter.Value();

            // final states in tr correspond in t to a non-final state which has
            // an epsilon transition to original destination state of arc that
            // is being replaced
            if ( is_final( transducer, tr_state_id ) )
              t->AddArc( tr_state_id + start_state,
                         fst::LogArc( 0,
                                      0,
                                      // final weight is copied
                                      // to the epsilon transition
                                      transducer->Final(tr_state_id),
                                      destination_state
                                      )
                         );

            for (fst::ArcIterator<LogFst> aiter(*transducer, tr_state_id);
                 !aiter.Done(); aiter.Next()) {

              const fst::LogArc &tr_arc = aiter.Value();

              // adding arc from state 'tr_state_id+start_state'
              // to state 'tr_arc.nextstate'
              // copy arcs from tr to t
              t->AddArc( tr_state_id + start_state,
                         fst::LogArc( tr_arc.ilabel,
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

  LogFst * LogWeightTransducer::compose(LogFst * t1,
                         LogFst * t2)
  {
    StringSet foo;
    // a copy of t2 is created so that its symbol table check sum is
    // the same as t1's
    // (else OpenFst complains about non-matching check sums... )
    LogFst * t2_ = expand_arcs(t2, foo, false);
    t2_->SetInputSymbols(t1->InputSymbols());
    t1->SetOutputSymbols(t1->InputSymbols());
    
    ArcSort(t1, OLabelCompare<LogArc>());
    ArcSort(t2_, ILabelCompare<LogArc>());

    LogFst *result = new LogFst();
    Compose(*t1, *t2_, result);
    delete t2_;

    result->SetInputSymbols(t1->InputSymbols());
    return result;
  }

  LogFst * LogWeightTransducer::concatenate(LogFst * t1,
                                                       LogFst * t2)
  {
    LogFst * result = new LogFst(*t1);
    Concat(result,*t2);
    result->SetInputSymbols(t1->InputSymbols());
    return result;
  }

  LogFst * LogWeightTransducer::disjunct(LogFst * t1,
                                                    LogFst * t2)
  {
    LogFst * result = new LogFst(*t1);
    Union(result,*t2);
    result->SetInputSymbols(t1->InputSymbols());
    return result;
  }

  LogFst * LogWeightTransducer::disjunct
  (LogFst * t, const StringPairVector &spv)
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
        for(fst::ArcIterator<LogFst> it(*t,s); !it.Done(); it.Next())
          {
            const LogArc& a = it.Value();
            if (a.ilabel == (int)inumber && a.olabel == (int)onumber) {
              transition_found=true;
              s = a.nextstate;
              break;
            }
          }

        if (! transition_found) {
          StateId new_state = t->AddState();
          t->AddArc(s, LogArc(inumber,onumber,0,new_state));
          s = new_state;
        }
      }

    t->SetFinal(s,0);
    
    t->SetInputSymbols(st);
    return t;
  }

  LogFst * LogWeightTransducer::disjunct
  (LogFst * t, const NumberPairVector &npv)
  {
    StateId s = t->Start();

    for (NumberPairVector::const_iterator it = npv.begin();
         it != npv.end(); it++)
      {
        unsigned int inumber = it->first;
        unsigned int onumber = it->second;

        bool transition_found=false;
        for(fst::ArcIterator<LogFst> it(*t,s); !it.Done(); it.Next())
          {
            const LogArc& a = it.Value();
            if (a.ilabel == (int)inumber && a.olabel == (int)onumber) {
              transition_found=true;
              s = a.nextstate;
              break;
            }
          }

        if (! transition_found) {
          StateId new_state = t->AddState();
          t->AddArc(s, LogArc(inumber,onumber,0,new_state));
          s = new_state;
        }
      }

    t->SetFinal(s,0);
    return t;
  }

  LogFst * LogWeightTransducer::intersect(LogFst * t1,
                           LogFst * t2)
  {
    if (t1->OutputSymbols() == NULL)
      t1->SetOutputSymbols(t1->InputSymbols());
    if (t2->OutputSymbols() == NULL)
      t2->SetOutputSymbols(t2->InputSymbols());

    ArcSort(t1, OLabelCompare<LogArc>());
    ArcSort(t2, ILabelCompare<LogArc>());

    RmEpsilon(t1);
    RmEpsilon(t2);

    EncodeMapper<LogArc> encoder(0x0001,ENCODE);
    EncodeFst<LogArc> enc1(*t1, &encoder);
    EncodeFst<LogArc> enc2(*t2, &encoder);
    DeterminizeFst<LogArc> det1(enc1);
    DeterminizeFst<LogArc> det2(enc2);

    IntersectFst<LogArc> intersect(det1,det2);
    LogFst *foo = new LogFst(intersect);
    DecodeFst<LogArc> decode(*foo, encoder);
    delete foo;
    LogFst *result = new LogFst(decode);
    result->SetInputSymbols(t1->InputSymbols());
    return result;
  }

  LogFst * LogWeightTransducer::subtract(LogFst * t1,
                          LogFst * t2)
  {
    bool DEBUG=false;

    if (DEBUG) printf("Log subtract...\n");

    if (t1->OutputSymbols() == NULL)
      t1->SetOutputSymbols(t1->InputSymbols());
    if (t2->OutputSymbols() == NULL)
      t2->SetOutputSymbols(t2->InputSymbols());

    ArcSort(t1, OLabelCompare<LogArc>());
    ArcSort(t2, ILabelCompare<LogArc>());

    //RmEpsilonFst<LogArc> rm1(*t1);
    //RmEpsilonFst<LogArc> rm2(*t2);

    RmEpsilon(t1);
    RmEpsilon(t2);

    if (DEBUG) printf("  ..epsilons removed\n");

    EncodeMapper<LogArc> encoder(0x0003,ENCODE); // t2 must be unweighted
    //EncodeFst<LogArc> enc1(rm1, &encoder);
    //EncodeFst<LogArc> enc2(rm2, &encoder);
    EncodeFst<LogArc> enc1(*t1, &encoder);
    EncodeFst<LogArc> enc2(*t2, &encoder);
    DeterminizeFst<LogArc> det1(enc1);
    DeterminizeFst<LogArc> det2(enc2);

    if (DEBUG) printf("  ..determinized\n");

    LogFst *difference = new LogFst();
    Difference(det1, det2, difference);
    DecodeFst<LogArc> subtract(*difference, encoder);
    delete difference;

    if (DEBUG) printf("  ..subtracted\n");

    //DifferenceFst<LogArc> subtract(enc1,enc2);
    LogFst *result = new LogFst(subtract);
    result->SetInputSymbols(t1->InputSymbols());
    return result;
  }

  LogFst * LogWeightTransducer::set_weight(LogFst * t,float f)
  {
    LogFst * t_copy = new LogFst(*t);
    for (fst::StateIterator<LogFst> iter(*t);
         ! iter.Done(); iter.Next())
      {
        if (t_copy->Final(iter.Value()) != fst::LogWeight::Zero())
          { t_copy->SetFinal(iter.Value(),f); }
      }
    return t_copy;
  }


  // ----- TRIE FUNCTIONS BEGINS -----

  int LogWeightTransducer::has_arc(LogFst &t,
              LogArc::StateId sourcestate,
              LogArc::Label ilabel,
              LogArc::Label olabel)
  {
    for (ArcIterator<LogFst> aiter(t,sourcestate);
         !aiter.Done();
         aiter.Next())
      {
        if ((aiter.Value().ilabel == ilabel) &&
            (aiter.Value().olabel == olabel))
          { return aiter.Position(); }
      }

    return -1;
  }

  void LogWeightTransducer::disjunct_as_tries(LogFst &t1,
                         StateId t1_state,
                         const LogFst * t2,
                         StateId t2_state)
  {
    if (t2->Final(t2_state) != LogWeight::Zero())
      {
        t1.SetFinal(t1_state,
                    Plus(t1.Final(t1_state),
                         t2->Final(t2_state)));
      }
    for (ArcIterator<LogFst> aiter(*t2,t2_state);
         !aiter.Done();
         aiter.Next())
      {
        const LogArc &arc = aiter.Value();
        int arc_index = has_arc(t1,t1_state,arc.ilabel,arc.olabel);
        if (arc_index == -1)
          {
            LogArc::StateId new_state = t1.AddState();
            t1.AddArc(t1_state,LogArc(arc.ilabel,
                                      arc.olabel,
                                      arc.weight,
                                      new_state));
            add_sub_trie(t1,new_state,t2,arc.nextstate);
          }
        else
          {
            MutableArcIterator<LogFst> ajter(&t1,t1_state);
            ajter.Seek(arc_index);
            disjunct_as_tries(t1,ajter.Value().nextstate,t2,arc.nextstate);
          }
      }
  }
  
  void LogWeightTransducer::add_sub_trie(LogFst &t1,
                  StateId t1_state,
                    const LogFst * t2,
                    StateId t2_state)
  {
    if (t2->Final(t2_state) != LogWeight::Zero())
      {
        t1.SetFinal(t1_state,
                    Plus(t1.Final(t1_state),
                         t2->Final(t2_state)));
      }
    for (ArcIterator<LogFst> aiter(*t2,t2_state);
         !aiter.Done();
         aiter.Next())
      {
        const LogArc &arc = aiter.Value();
        LogArc::StateId new_state = t1.AddState();
        t1.AddArc(t1_state,LogArc(arc.ilabel,
                                  arc.olabel,
                                  arc.weight,
                                  new_state));
        add_sub_trie(t1,new_state,t2,arc.nextstate);
      }
  }

  // ----- TRIE FUNCTIONS END -----
  

  static bool extract_paths
  (LogFst * t, LogArc::StateId s,
   std::map<StateId,unsigned short> all_visitations,
   std::map<StateId, unsigned short> path_visitations,
   /*std::vector<char>& lbuffer, int lpos, std::vector<char>& ubuffer,
     int upos,*/ float weight_sum,
   hfst::ExtractStringsCb& callback, int cycles,
   std::vector<hfst::FdState<int64> >* fd_state_stack,
   bool filter_fd, /*bool include_spv,*/ StringPairVector &spv)
  {
    if(cycles >= 0 && path_visitations[s] > cycles)
      return true;
    all_visitations[s]++;
    path_visitations[s]++;
    
    /*
    if(lpos > 0 && upos > 0)
    {
      lbuffer[lpos]=0;
      ubuffer[upos]=0;
      bool final = t->Final(s) != LogWeight::Zero();
      hfst::WeightedPath<float> path
        (&lbuffer[0],&ubuffer[0],weight_sum+(final?t->Final(s).Value():0));
      if (include_spv) {
        path.spv = spv;
        path.is_spv_in_use = true;
      }
      hfst::ExtractStringsCb::RetVal ret = callback(path, final);
      if(!ret.continueSearch || !ret.continuePath)
      {
        path_visitations[s]--;
        return ret.continueSearch;
      }
    }
    */

    if (spv.size() != 0)
      {
        bool final = t->Final(s) != LogWeight::Zero();
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
    vector<const LogArc*> arcs;
    for(fst::ArcIterator<LogFst> it(*t,s); !it.Done(); it.Next())
    {
      const LogArc& a = it.Value();
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
      const LogArc &arc = *(arcs[i]);
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

      /*
      int lp=lpos;
      int up=upos;
      
      if (arc.ilabel != 0 &&
          (!filter_fd || fd_state_stack->back().get_table().
           get_operation(arc.ilabel)==NULL))
      {
        std::string str = t->InputSymbols()->Find(arc.ilabel);
        if(lpos+str.length() >= lbuffer.size())
          lbuffer.resize(lbuffer.size()*2, 0);
        strcpy(&lbuffer[lpos], str.c_str());
        lp += str.length();
      }
      if (arc.olabel != 0 &&
          (!filter_fd || fd_state_stack->back().get_table().
           get_operation(arc.olabel)==NULL))
      {
        std::string str = t->InputSymbols()->Find(arc.olabel);
        if(upos+str.length() > ubuffer.size())
          ubuffer.resize(ubuffer.size()*2, 0);
        strcpy(&ubuffer[upos], str.c_str());
        up += str.length();
      }
      */
      
      /* Handle spv here. Special symbols (flags, epsilons) are
         always inserted. */

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
         /*lbuffer,lp, ubuffer,up,*/ weight_sum+arc.weight.Value(), callback,
         cycles, fd_state_stack, filter_fd,
         /*include_spv,*/ spv);
     
      spv.pop_back();
 
      if(added_fd_state)
        fd_state_stack->pop_back();
    }
    
    path_visitations[s]--;
    return res;
  }
  
  static const int BUFFER_START_SIZE = 64;
  
  void LogWeightTransducer::extract_paths
  (LogFst * t, hfst::ExtractStringsCb& callback,
   int cycles, FdTable<int64>* fd, bool filter_fd/*, bool include_spv*/)
  {
    if (t->Start() == -1)
      return;
    
    //vector<char> lbuffer(BUFFER_START_SIZE, 0);
    //vector<char> ubuffer(BUFFER_START_SIZE, 0);
    map<StateId, unsigned short> all_visitations;
    map<StateId, unsigned short> path_visitations;
    std::vector<hfst::FdState<int64> >* fd_state_stack = (fd==NULL) ? NULL :
      new std::vector<hfst::FdState<int64> >(1, hfst::FdState<int64>(*fd));
    
    StringPairVector spv;
    hfst::implementations::extract_paths
      (t,t->Start(),all_visitations,path_visitations,
       /*lbuffer,0,ubuffer,0,*/0.0f,callback,cycles,fd_state_stack,filter_fd,
       /*include_spv,*/ spv);
  }

  void LogWeightTransducer::extract_random_paths
  (const LogFst *t, HfstTwoLevelPaths &results, int max_num)
  {
    (void)t;
    (void)results;
    (void)max_num;
    HFST_THROW(FunctionNotImplementedException);
  }
  
  FdTable<int64>* LogWeightTransducer::get_flag_diacritics(LogFst * t)
  {
    FdTable<int64>* table = new FdTable<int64>();
    const fst::SymbolTable* symbols = t->InputSymbols();
    for(fst::SymbolTableIterator it=fst::SymbolTableIterator(*symbols);
        !it.Done(); it.Next())
    {
      if(FdOperation::is_diacritic(it.Symbol()))
        table->define_diacritic(it.Value(), it.Symbol());
    }
    return table;
  }

  void LogWeightTransducer::represent_empty_transducer_as_having_one_state
  (LogFst *t)
  {
    if (t->Start() == fst::kNoStateId || t->NumStates() == 0) {
      delete t;
      t = create_empty_transducer();
    }
    return;
  }



  LogWeightOutputStream::LogWeightOutputStream(void):
    filename(std::string()), output_stream(std::cout)
  {
    if (!output_stream)
      fprintf(stderr, "LogWeightOutputStream: ERROR: failbit set (3).\n");
  }

  LogWeightOutputStream::LogWeightOutputStream(const std::string &str):
    filename(std::string(str)),o_stream(str.c_str(),std::ios::out),
    output_stream(o_stream)
  {}

  void LogWeightOutputStream::write(const char &c)
  {
    output_stream.put(char(c));
  }

  void LogWeightOutputStream::write_transducer(LogFst * transducer)
  {
    if (!output_stream)
      fprintf(stderr, "LogWeightOutputStream: ERROR: failbit set (1).\n");
    /* When writing a transducer, both input and output symbol tables are
       included. */
    fst::SymbolTable output_st(*(transducer->InputSymbols()));
    transducer->SetOutputSymbols(&output_st);
    transducer->Write(output_stream,FstWriteOptions()); }

  void LogWeightOutputStream::close(void)
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
    LogWeightTransducer ofst;
    LogFst * t = ofst.create_empty_transducer();
    delete t;
    t = ofst.create_epsilon_transducer();
    delete t;
    std::cout << std::endl << "ok" << std::endl;
    return EXIT_SUCCESS;
}
#endif // MAIN_TEST
