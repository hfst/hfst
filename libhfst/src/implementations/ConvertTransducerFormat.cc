// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#if HAVE_SFST
#include "back-ends/sfst/fst.h"
#endif // HAVE_SFST

#if HAVE_FOMA
#ifndef _FOMALIB_H_
#define _FOMALIB_H_
#include "back-ends/foma/fomalib.h"
#endif // _FOMALIB_H_
#endif // HAVE_FOMA

#include "ConvertTransducerFormat.h"
#include "optimized-lookup/convert.h"
#include "HfstBasicTransducer.h"
#include "HfstTransducer.h"

#ifndef MAIN_TEST
namespace hfst { namespace implementations
{

  ConversionFunctions::String2NumberMap
    ConversionFunctions::string_to_number_map;
  StringVector ConversionFunctions::number_to_string_vector;

  StringVectorInitializer
  dummy3(ConversionFunctions::number_to_string_vector);
  String2NumberMapInitializer
  dummy4(ConversionFunctions::string_to_number_map);

  std::string ConversionFunctions::get_string(unsigned int number)
  {
    if (number >= number_to_string_vector.size()) {
      return std::string(""); } // number not found
    return number_to_string_vector[number];
  }
  
  unsigned int ConversionFunctions::get_number(const std::string &str)
  {
    String2NumberMap::iterator it =
      string_to_number_map.find(str);
    if (it == string_to_number_map.end()) { // string not found
      number_to_string_vector.push_back(str);
      unsigned int new_index = number_to_string_vector.size()-1;
      string_to_number_map[str] = new_index;
      return new_index;
    }
    return it->second;
  }
  
  ConversionFunctions::NumberVector
    ConversionFunctions::get_harmonization_vector
      (const StringVector &coding_vector)
  {
    NumberVector retval;
    retval.reserve(coding_vector.size());
    for (StringVector::const_iterator it = coding_vector.begin();
     it != coding_vector.end(); it++) {
      if (*it != "")
    retval.push_back(get_number(*it));
      else // a gap in indexing
    retval.push_back(0);
    }
    return retval;
  }


  HfstBasicTransducer * ConversionFunctions::
  hfst_transducer_to_hfst_basic_transducer
  (const hfst::HfstTransducer &t) {
#if HAVE_SFST
    if (t.type == SFST_TYPE) {
      HfstBasicTransducer * retval = sfst_to_hfst_basic_transducer(t.implementation.sfst);
      retval->name = t.get_name();
      return retval;
    }
#endif // HAVE_SFST

#if HAVE_OPENFST
    if (t.type == TROPICAL_OPENFST_TYPE) {
      HfstBasicTransducer * retval = tropical_ofst_to_hfst_basic_transducer
        (t.implementation.tropical_ofst);
      retval->name = t.get_name();
      return retval;
    }
#if HAVE_OPENFST_LOG
    if (t.type == LOG_OPENFST_TYPE) {
      HfstBasicTransducer * retval = log_ofst_to_hfst_basic_transducer(t.implementation.log_ofst);
      retval->name = t.get_name();
      return retval;
    }
#endif
#endif // HAVE_OPENFST
    
#if HAVE_FOMA
    if (t.type == FOMA_TYPE) {
      HfstBasicTransducer * retval = foma_to_hfst_basic_transducer(t.implementation.foma);
      retval->name = t.get_name();
      return retval;
    }
#endif // HAVE_FOMA

#if HAVE_XFSM
    if (t.type == XFSM_TYPE) {
      HfstBasicTransducer * retval = xfsm_to_hfst_basic_transducer(t.implementation.xfsm);
      retval->name = t.get_name();
      return retval;
    }
#endif // HAVE_FOMA

    
    /* Add here your implementation. */
    //#if HAVE_MY_TRANSDUCER_LIBRARY
    //    if (t.type == MY_TRANSDUCER_LIBRARY_TYPE)
    //      return my_transducer_library_transducer_to_hfst_basic_transducer
    //        (t.implementation.my_transducer_library);
    //#endif // HAVE_MY_TRANSDUCER_LIBRARY

    if (t.type == HFST_OL_TYPE || t.type == HFST_OLW_TYPE)
      {
      HfstBasicTransducer * retval = hfst_ol_to_hfst_basic_transducer(t.implementation.hfst_ol);
      retval->name = t.get_name();
      return retval;
      }

    HFST_THROW(FunctionNotImplementedException);
  }


  /* Add here your conversion functions or write them to a separate file. */
  //#if HAVE_MY_TRANSDUCER_LIBRARY
  //
  //HfstBasicTransducer * ConversionFunctions::
  //my_transducer_library_transducer_to_hfst_basic_transducer
  //  (my_namespace::MyFst * t) {
  //(void)t;
  //HFST_THROW(FunctionNotImplementedException);
  //}

  //my_namespace::MyFst * ConversionFunctions::
  //hfst_basic_transducer_to_my_transducer_library_transducer
  //  (const HfstBasicTransducer * t) {
  //(void)t;
  //HFST_THROW(FunctionNotImplementedException);
  //}
  //#endif // HAVE_MY_TRANSDUCER_LIBRARY


} }

#else // MAIN_TEST was defined
#include <cstdlib>
#include <cassert>

#include "../HfstTransducer.h"

using namespace hfst;
using namespace hfst::implementations;

int main(void)
  {
    std::cout << "Unit tests for " __FILE__ ":";

    hfst::ImplementationType types [3] = {hfst::SFST_TYPE, hfst::FOMA_TYPE, hfst::TROPICAL_OPENFST_TYPE};

    for (unsigned int i=0; i < 3; i++)
      {
        if (hfst::HfstTransducer::is_implementation_type_available(types[i]))
          {
            HfstTokenizer tok;
            HfstTransducer fsm1("cat", "dog", tok, types[i]);
            fsm1.set_final_weights(4);
            
            HfstBasicTransducer * fsm1_converted =
              ConversionFunctions::hfst_transducer_to_hfst_basic_transducer(fsm1);
            
            HfstTransducer fsm1_converted_twice(*fsm1_converted, types[i]);
            
            if (! fsm1.compare(fsm1_converted_twice))
              return 1;
          }
      }
    
    return 0;
  }
#endif // MAIN_TEST
