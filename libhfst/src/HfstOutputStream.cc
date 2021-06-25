// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include <string>

using std::string;

#include "HfstTransducer.h"
#include "HfstOutputStream.h"

#ifndef MAIN_TEST

namespace hfst
{
  HfstOutputStream::HfstOutputStream(ImplementationType type, bool hfst_format):
    type(type), hfst_format(hfst_format), is_open(false)
  {
    if (! HfstTransducer::is_lean_implementation_type_available(type)) {
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);
    }

    switch(type)
      {
#if HAVE_SFST || HAVE_LEAN_SFST
      case SFST_TYPE:
        implementation.sfst =
          new hfst::implementations::SfstOutputStream();
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst =
          new hfst::implementations::TropicalWeightOutputStream(hfst_format);
        break;
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        implementation.log_ofst =
          new hfst::implementations::LogWeightOutputStream();
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        implementation.foma =
          new hfst::implementations::FomaOutputStream();
        break;
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        implementation.xfsm =
          new hfst::implementations::XfsmOutputStream(); // throws error, not implemented
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        implementation.my_transducer_library =
          new hfst::implementations::MyTransducerLibraryOutputStream(hfst_format);
        break;
#endif
      case HFST_OL_TYPE:
        implementation.hfst_ol =
          new hfst::implementations::HfstOlOutputStream(false);
        break;
      case HFST_OLW_TYPE:
        implementation.hfst_ol =
          new hfst::implementations::HfstOlOutputStream(true);
        break;
      default:
        HFST_THROW(SpecifiedTypeRequiredException);
        break;
      }
    this->is_open=true;
  }

  // FIXME: HfstOutputStream takes a string parameter,
  //        HfstInputStream a const char*
  HfstOutputStream::HfstOutputStream
  (const std::string &filename,ImplementationType type, bool hfst_format_):
    type(type), hfst_format(hfst_format_), is_open(false)
  {
    if (! HfstTransducer::is_lean_implementation_type_available(type)) {
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);
    }

    switch(type)
      {
#if HAVE_SFST || HAVE_LEAN_SFST
      case SFST_TYPE:
        implementation.sfst =
          new hfst::implementations::SfstOutputStream(filename);
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        // FIXME: this should be done in TropicalWeight layer
        if (filename.compare("") == 0)
          implementation.tropical_ofst =
            new hfst::implementations::TropicalWeightOutputStream(hfst_format);
        else
          implementation.tropical_ofst =
            new hfst::implementations::TropicalWeightOutputStream
            (filename, hfst_format);
        break;
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        implementation.log_ofst =
          new hfst::implementations::LogWeightOutputStream(filename);
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        implementation.foma =
          new hfst::implementations::FomaOutputStream(filename);
        break;
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        /* XFSM api only offers a function that reads transducers that takes a filename argument.
           That is why we don't write an HFST header. */
        hfst_format = false;
        implementation.xfsm =
          new hfst::implementations::XfsmOutputStream(filename);
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        implementation.my_transducer_library =
          new hfst::implementations::MyTransducerLibraryOutputStream(filename, hfst_format);
        break;
#endif
      case HFST_OL_TYPE:
        implementation.hfst_ol =
          new hfst::implementations::HfstOlOutputStream(filename, false);
        break;
      case HFST_OLW_TYPE:
        implementation.hfst_ol =
          new hfst::implementations::HfstOlOutputStream(filename.c_str(), true);
        break;
      default:
        HFST_THROW(SpecifiedTypeRequiredException);
        break;
      }
    this->is_open=true;
  }
  
  HfstOutputStream::~HfstOutputStream(void)
  {
    switch (type)
      {
#if HAVE_SFST || HAVE_LEAN_SFST
      case SFST_TYPE:
        delete implementation.sfst;
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        delete implementation.tropical_ofst;
        break;
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        delete implementation.log_ofst;
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        delete implementation.foma;
        break;
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        delete implementation.xfsm;
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        delete implementation.my_transducer_library;
        break;
#endif
      case HFST_OL_TYPE:
      case HFST_OLW_TYPE:
        delete implementation.hfst_ol;
        break;
      default:
        assert(false);
      }
  }

  void HfstOutputStream::append(std::vector<char> &str, const std::string &s)
  {
    for (char c : s)
      str.push_back(c);
    str.push_back('\0');
  }

  void HfstOutputStream::write(const std::string &s)
  {
    for (char c : s)
      write(c);
  }

  void HfstOutputStream::write(const std::vector<char> &s)
  {
    for (char c : s)
      write(c);
  }

  void HfstOutputStream::write(const char &c)
  {
    switch(type)
      {
#if HAVE_SFST || HAVE_LEAN_SFST
      case SFST_TYPE:
        implementation.sfst->write(c);
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst->write(c);
        break;
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        implementation.log_ofst->write(c);
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        implementation.foma->write(c);
        break;
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        throw "operation XfsmOutputStream::write(const char &c) not supported";
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        implementation.my_transducer_library->write(c);
        break;
#endif
        // we always have HFST_OL, right?
      case HFST_OL_TYPE:
      case HFST_OLW_TYPE:
        implementation.hfst_ol->write(c);
        break;
        
      default:
        assert(false);
      }
  }

  void HfstOutputStream::append_hfst_header_data(std::vector<char> &header)
  {
    append(header, "version");
    append(header, "3.3");
    append(header, "type");

    std::string type_value;

    switch(type)
      {
#if HAVE_SFST || HAVE_LEAN_SFST
      case SFST_TYPE:
        type_value=std::string("SFST");
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        type_value=std::string("TROPICAL_OPENFST");
        break;
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        type_value=std::string("LOG_OPENFST");
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        type_value=std::string("FOMA");
        break;
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        type_value=std::string("XFSM");
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        type_value=std::string("MY_TRANSDUCER_LIBRARY");
        break;
#endif
      case HFST_OL_TYPE:
        type_value=std::string("HFST_OL");
        break;
      case HFST_OLW_TYPE:
        type_value=std::string("HFST_OLW");
        break;
      default:
        assert(false);
      }

    append(header, type_value);
  }

#if HAVE_SFST || HAVE_LEAN_SFST
void
HfstOutputStream::append_implementation_specific_header_data(std::vector<char>&
                                                             header,
                                                             HfstTransducer&
                                                             transducer)
#else
void
HfstOutputStream::append_implementation_specific_header_data(std::vector<char>&,
                                                             HfstTransducer&)
#endif
  {
#if HAVE_SFST || HAVE_LEAN_SFST
    switch(type)
      {
      case SFST_TYPE:
        implementation.sfst->append_implementation_specific_header_data
          (header, transducer.implementation.sfst);
        break;
      default:
        break;
      }
#endif
  }

  HfstOutputStream &HfstOutputStream::flush()
  {
    if (! this->is_open) {
      HFST_THROW(StreamIsClosedException); }
#if HAVE_XFSM
    if (type == XFSM_TYPE)
      {
        implementation.xfsm->flush();
      }
#endif
    return *this;
  }

  HfstOutputStream &HfstOutputStream::redirect (HfstTransducer &transducer)
  {
    return this->operator<<(transducer);
  }

  HfstOutputStream &HfstOutputStream::operator<< (HfstTransducer &transducer)
  {
    if (! this->is_open) {
      HFST_THROW(StreamIsClosedException);
    }
      

    if (type != transducer.type)
      {
        HFST_THROW_MESSAGE(TransducerTypeMismatchException,
                           "operator<<: "
                           "HfstOutputStream and HfstTransducer do not "
                           "have the same type");
      }

    /* Write the HFST header. The header has the following structure:
       
       - the first four chars identify an HFST header:  "HFST"
       - the fifth char is a separator:                 "\0"
       - the sixth and seventh char tell the length of the rest of the header
         (beginning after the eighth char)
       - the eighth char is a separator and is not counted
         to the header length: "\0"
       - the rest of the header consists of pairs of attributes and their values
         that are each separated by a char "\0"

       An example:

       "HFST\0"
       "\0\x1c\0"
       "version\0"  "3.0\0"
       "type\0"     "FOMA\0"
       "name\0"     "\0"
       
       This is the header of a version 3.0 HFST transducer whose implementation
       type is foma and whose name is not defined, i.e. is the empty string "".
       The two bytes "\0\x1c" that form the length field tell that the length of
       the rest of the header (i.e. the sequence of bytes
       "version\03.0\0type\0FOMA\0name\0\0") is 0 * 256 + 28 * 1 = 28 bytes.

       HFST version 3.0 header must contain at least the attributes 'version',
       'type' and 'name' and their values. Implementation-specific attributes
       can follow after these obligatory attributes.

       Note: in XFSM format, we never write the HFST header. hfst_format is always
       false if the stream is of XFSM format.
     */
    if (hfst_format) {

      const int MAX_HEADER_LENGTH=65535;

      // collect the header data here
      std::vector<char> header;
      append_hfst_header_data(header); // attributes "version" and "type"
      for (std::map<string,string>::const_iterator prop =
               transducer.props.begin();
           prop != transducer.props.end();
           ++prop)
        {
          if ((prop->first == "type") || (prop->first == "version"))
            {
              // special hanling above
              continue;
            }
          append(header, prop->first);
          append(header, prop->second);
        }

      append_implementation_specific_header_data(header, transducer);

      // write the field that identifies the header as an HFST header
      write("HFST");
      write('\0');

      // write header length using two bytes
      int header_length = (int)header.size();
      if (header_length > MAX_HEADER_LENGTH) {
        //fprintf(stderr, "ERROR: transducer header is too long\n");
        //exit(1);
        HFST_THROW_MESSAGE(HfstFatalException, "transducer header is too long");
      }

      char first_byte = *((char*)(&header_length));
      char second_byte = *((char*)(&header_length)+1);
      write(first_byte);
      write(second_byte);
      write('\0');

      // write the rest of the header
      write(header);
    } // if (hfst_format)

    switch (type)
      {
#if HAVE_SFST || HAVE_LEAN_SFST
      case SFST_TYPE:
        implementation.sfst->write_transducer
          (transducer.implementation.sfst);
        return *this;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst->write_transducer
          (transducer.implementation.tropical_ofst);
        return *this;
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        implementation.log_ofst->write_transducer
          (transducer.implementation.log_ofst);
        return *this;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        implementation.foma->write_transducer
          (transducer.implementation.foma);
        return *this;
#endif
#if HAVE_XFSM
        /* This stores the transducer in a list that is written only when flush() is called. */
      case XFSM_TYPE:
        implementation.xfsm->write_transducer
          (transducer.implementation.xfsm);
        return *this;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        implementation.my_transducer_library->write_transducer
          (transducer.implementation.my_transducer_library);
#endif
      case HFST_OL_TYPE:
      case HFST_OLW_TYPE:
        implementation.hfst_ol->write_transducer
          (transducer.implementation.hfst_ol);
        return *this;
      default:
        assert(false);
        return *this;
      }
  }

  void HfstOutputStream::close(void) {
    switch (type)
      {
#if HAVE_SFST || HAVE_LEAN_SFST
      case SFST_TYPE:
        implementation.sfst->close();
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst->close();
        break;
#if HAVE_OPENFST_LOG || HAVE_LEAN_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        implementation.log_ofst->close();
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        implementation.foma->close();
        break;
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        implementation.xfsm->close();
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        implementation.my_transducer_library->close();
        break;
#endif
      case HFST_OL_TYPE:
      case HFST_OLW_TYPE:
        implementation.hfst_ol->close();
        break;
      default:
        assert(false);
      }
    this->is_open=false;
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
