// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

/* \file HfstApply.cc
   \brief HFST transducer functions that take several parameters are
   handled here.
 */

#include "HfstTransducer.h"

#ifndef MAIN_TEST

namespace hfst
{
  // Whether weights or other information are lost in the conversion.
  bool HfstTransducer::is_safe_conversion
  (ImplementationType original, ImplementationType converted) {
    if (original == converted)
      return true;
    if (original == TROPICAL_OPENFST_TYPE && converted == LOG_OPENFST_TYPE) {
      return false;
    }
    if (original == LOG_OPENFST_TYPE && converted == TROPICAL_OPENFST_TYPE) {
      return false;
    }
    if (original == TROPICAL_OPENFST_TYPE || original == LOG_OPENFST_TYPE) {
      if (converted == SFST_TYPE) {
        return false;
      }
      if (converted == FOMA_TYPE) {
        return false;
      }
      if (converted == XFSM_TYPE) {
        return false;
      }
    }
    /* Add here your library. */
    //#ifdef HAVE_MY_TRANSDUCER_LIBRARY
    //if (original == MY_TRANSDUCER_LIBRARY_TYPE) {
    //  // From my transducer to weighted transducer
    //  if ( converted == TROPICAL_OPENFST_TYPE ||
    //       converted == LOG_OPENFST_TYPE) {
    //  return true;  // if your library supports weights
    //  return false; // if your library does not support weights
    //  }
    //  // From my transducer to unweighted
    //  else {
    //  return true;  // if your library does not support weights
    //  return false; // if your library supports weights
    // }
    //}
    //#endif
    return true;
  }

  HfstTransducer &HfstTransducer::apply(
#if HAVE_SFST
 SFST::Transducer * (*sfst_funct)(SFST::Transducer *),
#endif
#if HAVE_OPENFST
 fst::StdVectorFst * (*tropical_ofst_funct)(fst::StdVectorFst *),
#if HAVE_OPENFST_LOG
 hfst::implementations::LogFst *
 (*log_ofst_funct)(hfst::implementations::LogFst *),
#endif
#endif
#if HAVE_FOMA
 struct fsm * (*foma_funct)(struct fsm *),
#endif
#if HAVE_XFSM
 NETptr (*xfsm_funct)(NETptr),
#endif
 /* Add your library. */
 //#if HAVE_MY_TRANSDUCER_LIBRARY
 //my_namespace::MyFst * (*my_transducer_library_funct)(my_namespace::MyFst *),
 //#endif
  bool foo )
    {
      (void)foo;
    switch(this->type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        {
          SFST::Transducer * sfst_temp =
            sfst_funct(implementation.sfst);
          sfst_interface.delete_transducer(implementation.sfst);
          implementation.sfst = sfst_temp;
          break;
        }
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        {
          fst::StdVectorFst * tropical_ofst_temp =
            tropical_ofst_funct(implementation.tropical_ofst);
          tropical_ofst_interface.delete_transducer(implementation.tropical_ofst);
          implementation.tropical_ofst = tropical_ofst_temp;
          break;
        }
#if HAVE_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        {
          hfst::implementations::LogFst * log_ofst_temp =
            log_ofst_funct(implementation.log_ofst);
          log_ofst_interface.delete_transducer(implementation.log_ofst);
          implementation.log_ofst = log_ofst_temp;
          break;
        }
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        {
      fsm * foma_temp =
            foma_funct(implementation.foma);
      this->foma_interface.delete_foma(implementation.foma);
          implementation.foma = foma_temp;
          break;
        }
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        {
          (void)xfsm_funct(implementation.xfsm);
          break;
        }
#endif
        /* Add your library here. */
        //#if HAVE_MY_TRANSDUCER_LIBRARY
        //case MY_TRANSDUCER_LIBRARY_TYPE:
        //{
        //  my_namespace::MyFst * my_fst_temp =
        //  my_transducer_library_funct(implementation.my_transducer_library);
        //delete implementation.my_transducer_library;
        //implementation.my_transducer_library = my_fst_temp;
        //break;
        //}
        //#endif
        case ERROR_TYPE:
        default:
          HFST_THROW(TransducerHasWrongTypeException);
      }
    return *this;
  }

  HfstTransducer &HfstTransducer::apply
  (
#if HAVE_SFST
SFST::Transducer * (*sfst_funct)(SFST::Transducer *, unsigned int n),
#endif
#if HAVE_OPENFST
   fst::StdVectorFst * (*tropical_ofst_funct)
(fst::StdVectorFst *, unsigned int n),
#if HAVE_OPENFST_LOG
   hfst::implementations::LogFst * (*log_ofst_funct)
   (hfst::implementations::LogFst *, unsigned int n),
#endif
#endif
#if HAVE_FOMA
   fsm * (*foma_funct)(fsm *, unsigned int n),
#endif
#if HAVE_XFSM
NETptr (*xfsm_funct)(NETptr, unsigned int n),
#endif
/* Add your library here. */
//#if HAVE_MY_TRANSDUCER_LIBRARY
//   my_namespace::MyFst *
//     (*my_transducer_library_funct)(my_namespace::MyFst *,int n),
//#endif
   unsigned int n )
  {
    switch(this->type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        {
          SFST::Transducer * sfst_temp =
            sfst_funct(implementation.sfst,n);
          sfst_interface.delete_transducer(implementation.sfst);
          implementation.sfst = sfst_temp;
          break;
        }
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        {
          fst::StdVectorFst * tropical_ofst_temp =
            tropical_ofst_funct(implementation.tropical_ofst,n);
          tropical_ofst_interface.delete_transducer(implementation.tropical_ofst);
          implementation.tropical_ofst = tropical_ofst_temp;
          break;
        }
#if HAVE_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        {
          hfst::implementations::LogFst * log_ofst_temp =
            log_ofst_funct(implementation.log_ofst,n);
          log_ofst_interface.delete_transducer(implementation.log_ofst);
          implementation.log_ofst = log_ofst_temp;
          break;
        }
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        {
      fsm * foma_temp =
            foma_funct(implementation.foma,n);
      this->foma_interface.delete_foma(implementation.foma);
          implementation.foma = foma_temp;
          break;
    }
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        {
      NETptr xfsm_temp =
            xfsm_funct(implementation.xfsm,n);
      delete implementation.xfsm;
          implementation.xfsm = xfsm_temp;
          break;
    }
#endif
        /* Add your library here. */
        //#if HAVE_MY_TRANSDUCER_LIBRARY
        //case MY_TRANSDUCER_LIBRARY_TYPE:
        //{
        //my_namespace::MyFst * my_fst_temp =
        //  my_transducer_library_funct(implementation.my_transducer_library,n);
        //delete (implementation.my_transducer_library);
        //implementation.my_transducer_library = my_fst_temp;
        //break;
        //}
        //#endif
        case ERROR_TYPE:
        default:
          HFST_THROW(TransducerHasWrongTypeException);
      }
    return *this;
  }

  HfstTransducer &HfstTransducer::apply
  (
#if HAVE_SFST
   SFST::Transducer * (*sfst_funct)(SFST::Transducer *, String, String),
#endif
#if HAVE_OPENFST
   fst::StdVectorFst * (*tropical_ofst_funct)(fst::StdVectorFst *,String,
                                              String),
#if HAVE_OPENFST_LOG
   hfst::implementations::LogFst * (*log_ofst_funct)
   (hfst::implementations::LogFst *,String, String),
#endif
#endif
#if HAVE_FOMA
   fsm * (*foma_funct)(fsm *, String, String),
#endif
#if HAVE_XFSM
   NETptr (*xfsm_funct)(NETptr, String, String),
#endif
   /* Add your library here. */
   //#if HAVE_MY_TRANSDUCER_LIBRARY
   //my_namespace::MyFst *
   // (*my_transducer_library_funct)(my_namespace::MyFst *, String, String),
   //#endif
   String s1, String s2)
  {
    switch(this->type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        {
          SFST::Transducer * sfst_temp =
            sfst_funct(implementation.sfst,s1,s2);
          sfst_interface.delete_transducer(implementation.sfst);
          implementation.sfst = sfst_temp;
          break;
        }
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        {
          fst::StdVectorFst * tropical_ofst_temp =
            tropical_ofst_funct(implementation.tropical_ofst,s1,s2);
          tropical_ofst_interface.delete_transducer(implementation.tropical_ofst);
          implementation.tropical_ofst = tropical_ofst_temp;
          break;
        }
#if HAVE_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        {
          hfst::implementations::LogFst * log_ofst_temp =
            log_ofst_funct(implementation.log_ofst,s1,s2);
          log_ofst_interface.delete_transducer(implementation.log_ofst);
          implementation.log_ofst = log_ofst_temp;
          break;
        }
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        {
      fsm * foma_temp =
            foma_funct(implementation.foma,s1,s2);
      this->foma_interface.delete_foma(implementation.foma);
          implementation.foma = foma_temp;
      break;
        }
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        {
      NETptr xfsm_temp =
            xfsm_funct(implementation.xfsm,s1,s2);
          delete implementation.xfsm;
          implementation.xfsm = xfsm_temp;
      break;
        }
#endif
        /* Add your library here. */
        //#if HAVE_MY_TRANSDUCER_LIBRARY
        //case MY_TRANSDUCER_LIBRARY_TYPE:
        //{
        //my_namespace::MyFst * my_fst_temp =
        //  my_transducer_library_funct
        //    (implementation.my_transducer_library,s1,s2);
        //delete(implementation.my_transducer_library);
        //implementation.my_transducer_library = my_fst_temp;
        //break;
        //}
        //#endif
        case ERROR_TYPE:
        default:
          HFST_THROW(TransducerHasWrongTypeException);
      }
    return *this;
  }


  HfstTransducer &HfstTransducer::apply(
#if HAVE_SFST
   SFST::Transducer * (*sfst_funct)(SFST::Transducer *,
                                    SFST::Transducer *),
#endif
#if HAVE_OPENFST
   fst::StdVectorFst * (*tropical_ofst_funct)(fst::StdVectorFst *,
                                              fst::StdVectorFst *),
#if HAVE_OPENFST_LOG
   hfst::implementations::LogFst * (*log_ofst_funct)
   (hfst::implementations::LogFst *,hfst::implementations::LogFst *),
#endif
#endif
#if HAVE_FOMA
   fsm * (*foma_funct)(fsm *,
                                    fsm *),
#endif
#if HAVE_XFSM
   NETptr (*xfsm_funct)(NETptr, NETptr),
#endif
   /* Add your library here. */
   //#if HAVE_MY_TRANSDUCER_LIBRARY
   //my_namespace::MyFst * (*my_transducer_library_funct)(my_namespace::MyFst *,
   //                                    my_namespace::MyFst *),
   //#endif
   HfstTransducer &another_tr, bool harmonize)
  {
    if (this->type != another_tr.type)
      HFST_THROW(TransducerTypeMismatchException);

    HfstTransducer another(another_tr);

    /* prevent harmonization, if needed */
    if (! harmonize)
      {
        this->insert_missing_symbols_to_alphabet_from(another);
        another.insert_missing_symbols_to_alphabet_from(*this);
      }

    /* special symbols are never harmonized */
    this->insert_missing_symbols_to_alphabet_from(another, true);
    another.insert_missing_symbols_to_alphabet_from(*this, true);
    HfstTransducer * another_ =
      this->harmonize_(another);
    if (another_ == NULL) // foma
      { another_ = new HfstTransducer(another); }

    switch (this->type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        {
          SFST::Transducer * sfst_temp =
            sfst_funct(implementation.sfst,
               another_->implementation.sfst);
          sfst_interface.delete_transducer(implementation.sfst);
          implementation.sfst = sfst_temp;
          break;
        }
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        {
          fst::StdVectorFst * tropical_ofst_temp =
            tropical_ofst_funct(this->implementation.tropical_ofst,
                                another_->implementation.tropical_ofst);
          tropical_ofst_interface.delete_transducer(implementation.tropical_ofst);
          implementation.tropical_ofst = tropical_ofst_temp;
          break;
        }
#if HAVE_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        {
          hfst::implementations::LogFst * log_ofst_temp =
            log_ofst_funct(implementation.log_ofst,
                           another_->implementation.log_ofst);
          log_ofst_interface.delete_transducer(implementation.log_ofst);
          implementation.log_ofst = log_ofst_temp;
          break;
        }
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        {
          fsm * foma_temp =
            foma_funct(implementation.foma,another_->implementation.foma);
          this->foma_interface.delete_foma(implementation.foma);
          implementation.foma = foma_temp;
          break;
        }
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        {
          NETptr xfsm_temp =
            xfsm_funct(implementation.xfsm,another_->implementation.xfsm);
          //delete implementation.xfsm;
          implementation.xfsm = xfsm_temp;
          break;
        }
#endif
        /* Add your library here. */
        //#if HAVE_MY_TRANSDUCER_LIBRARY
        //case MY_TRANSDUCER_LIBRARY_TYPE:
        //{
        //  my_namespace::MyFst * my_fst_temp =
        //    my_transducer_library_funct
        //      (implementation.my_transducer_library,
        //       another.implementation.my_transducer_library);
        //  delete implementation.my_transducer_library;
        //  implementation.my_transducer_library = my_fst_temp;
        //  break;
        //}
        //#endif
        case ERROR_TYPE:
        default:
          HFST_THROW(TransducerHasWrongTypeException);
      }
    delete another_;

    return *this;
  }
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
