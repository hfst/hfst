// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

//! @FIXME: The structure of this class and its functions is disorganised.

#include <string>

using std::string;

#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "implementations/ConvertTransducerFormat.h"

using hfst::implementations::HfstBasicTransducer;
using hfst::implementations::ConversionFunctions;

#ifndef MAIN_TEST

namespace hfst
{
  void debug_error(const char *msg)
  {
#if PRINT_DEBUG_MESSAGES
    fprintf(stderr, "%s\n", msg);
#else
    (void)msg;
#endif
  }

  void HfstInputStream::ignore(unsigned int n)
  {
    switch (type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        this->implementation.sfst->ignore(n);
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        this->implementation.tropical_ofst->ignore(n);
        break;
#if HAVE_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        this->implementation.log_ofst->ignore(n);
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        this->implementation.foma->ignore(n);
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        this->implementation.my_transducer_library->ignore(n);
        break;
#endif
      case HFST_OL_TYPE:
      case HFST_OLW_TYPE:
          this->implementation.hfst_ol->ignore(n);
          break;

      default:
        assert(false);
        break;
      }
  }

  char &HfstInputStream::stream_get(char &c)
  {
    if (input_stream != NULL)
      return c = input_stream->get();
    switch (type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        return c = this->implementation.sfst->stream_get();
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        return c = this->implementation.tropical_ofst->stream_get();
        break;
#if HAVE_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        return c = this->implementation.log_ofst->stream_get();
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        return c = this->implementation.foma->stream_get();
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        return c = this->implementation.my_transducer_library->stream_get();
        break;
#endif
      case HFST_OL_TYPE:
      case HFST_OLW_TYPE:
          return c = this->implementation.hfst_ol->stream_get();
          break;

      default:
        assert(false);
        break;
      }
    HFST_THROW(HfstFatalException); // make compiler happy
  }

  short &HfstInputStream::stream_get(short &i)
  {
    if (input_stream != NULL)
      {
        input_stream->read((char*)&i,sizeof(i));
        return i;
      }
    switch (type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        return i = this->implementation.sfst->stream_get_short();
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        return i = this->implementation.tropical_ofst->stream_get_short();
        break;
#if HAVE_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        return i = this->implementation.log_ofst->stream_get_short();
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        return i = this->implementation.foma->stream_get_short();
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        return i =
          this->implementation.my_transducer_library->stream_get_short();
        break;
#endif
      case HFST_OL_TYPE:
      case HFST_OLW_TYPE:
          return i = this->implementation.hfst_ol->stream_get_short();
          break;

      default:
        assert(false);
        break;
      }
    HFST_THROW(HfstFatalException); // make compiler happy
  }

  unsigned short &HfstInputStream::stream_get(unsigned short &i)
  {
      char byte_1 = stream_get();
      char byte_2 = stream_get();
      return  i = (unsigned short)
          (((unsigned char) byte_2) << 8) + ((unsigned char) byte_1);
  }

  char HfstInputStream::stream_get()
  {
    if (input_stream != NULL)
      return (char) input_stream->get();
    switch (type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        return this->implementation.sfst->stream_get();
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        return this->implementation.tropical_ofst->stream_get();
        break;
#if HAVE_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        return this->implementation.log_ofst->stream_get();
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        return this->implementation.foma->stream_get();
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        return this->implementation.my_transducer_library->stream_get();
        break;
#endif
      case HFST_OL_TYPE:
      case HFST_OLW_TYPE:
          return this->implementation.hfst_ol->stream_get();
          break;

      default:
        assert(false);
        break;
      }
    HFST_THROW(HfstFatalException); // make compiler happy
  }

  void HfstInputStream::stream_unget(char c)
  {
    if (input_stream != NULL) {
      input_stream->putback(c);
      return;
    }
    switch (type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        this->implementation.sfst->stream_unget(c);
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        this->implementation.tropical_ofst->stream_unget(c);
        break;
#if HAVE_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        this->implementation.log_ofst->stream_unget(c);
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        this->implementation.foma->stream_unget(c);
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        this->implementation.my_transducer_library->stream_unget(c);
        break;
#endif
      case HFST_OL_TYPE:
      case HFST_OLW_TYPE:
          return this->implementation.hfst_ol->stream_unget(c);
          break;
      default:
        assert(false);
        break;
      }
  }

  char HfstInputStream::stream_peek()
  {
    char c = stream_get();
    stream_unget(c);
    return c;
  }

  std::string HfstInputStream::stream_getstring()
  {
    std::string retval("");
    while (true) {
      char c = stream_get();
      if (stream_eof()) {
        HFST_THROW(EndOfStreamException);
      }
      if (c == '\0')
        break;
      retval.append(1,c);
    }
    return retval;
  }

  bool HfstInputStream::stream_eof()
  {
    if (input_stream != 0)
      return input_stream->eof();
    return is_eof();
  }

  bool HfstInputStream::set_implementation_specific_header_data
  (StringPairVector& /* data */, unsigned int /* index*/)
  {
    switch (type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        //return this->implementation.sfst->
        //set_implementation_specific_header_data(data, index);
        break;
#endif
      default:
        break;
      }
    return false;
  }

  void HfstInputStream::read_transducer(HfstTransducer &t)
  {
    if (type != XFSM_TYPE)
      {
        if (input_stream != NULL) { // first transducer in the stream
          input_stream = NULL;
          if (stream_eof())
            HFST_THROW(EndOfStreamException);
          // if header bytes have been read from a file, skip these bytes
          if (strcmp(filename.c_str(), "") != 0)
            ignore(bytes_to_skip);
        }
        else {
          if (stream_eof())
            HFST_THROW(EndOfStreamException);
          ImplementationType current_type  = this->get_type();
          ImplementationType stype = this->stream_fst_type();
          if (stype != current_type) {
            HFST_THROW_MESSAGE(TransducerTypeMismatchException,
                               "HfstInputStream contains HfstTransducers "
                               "whose type is not the same");
          }
        }
      }

    switch (type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        {
        t.implementation.sfst =
          this->implementation.sfst->read_transducer();

          /* If we were reading an SFST transducer with no HFST header,
             it is possible that epsilon is coded differently than
             "@_EPSILON_SYMBOL_@" and/or that numbers 1 and 2 are reserved
             for other use than "@_UNKNOWN_SYMBOL_@" or
             "@_IDENTITY_SYMBOL_@". */
          if (not has_hfst_header)
            {
              HfstBasicTransducer * net =
                ConversionFunctions::
                  sfst_to_hfst_basic_transducer
                (t.implementation.sfst);
              hfst::implementations::SfstTransducer::delete_transducer(t.implementation.sfst);
              t.implementation.sfst =
                ConversionFunctions::
                  hfst_basic_transducer_to_sfst(net);
              delete net;
            }
        break;
        }
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        {
          t.implementation.tropical_ofst =
            this->implementation.tropical_ofst->read_transducer();

          /* If we were reading an OpenFst transducer with no HFST header,
             it is possible that it has separate input and output symbol tables
             and/or that it has no input or output table or neither
             and/or that epsilon is coded differently than "@_EPSILON_SYMBOL_@"
             and/or that numbers 1 and 2 are reserved for other use than
             "@_UNKNOWN_SYMBOL_@" or "@_IDENTITY_SYMBOL_@". */
          if (! has_hfst_header)
            {
              HfstBasicTransducer * net =
                ConversionFunctions::
                  tropical_ofst_to_hfst_basic_transducer
                (t.implementation.tropical_ofst, false);
              t.tropical_ofst_interface.delete_transducer(t.implementation.tropical_ofst);
              t.implementation.tropical_ofst =
                ConversionFunctions::
                  hfst_basic_transducer_to_tropical_ofst(net);
              delete net;
            }

          // A special case: HFST version 2 transducer
          if (hfst_version_2_weighted_transducer) // an SFST alphabet follows
            {
              stream_get(); // UTF8
              
              // read the symbol mapping
              unsigned short n=0;
              n = n + (unsigned short)stream_get() * 1;
              n = n + (unsigned short)stream_get() * 256;
              

              // special symbol-to-number mappings
              std::vector<std::pair<unsigned short, std::string> >
                special_cases;
              // normal symbol-to-number mappings
              std::vector<std::pair<unsigned short, std::string> >
                symbol_mappings;

              unsigned short max_number=0;

              for( unsigned i=0; i<n; i++) {

                max_number++;

                unsigned short symbol_number=0;
                symbol_number = symbol_number +
                  (unsigned short)stream_get() * 1;
                symbol_number = symbol_number +
                  (unsigned short)stream_get() * 256;
                
                std::string symbol_string("");
                char c = stream_get();
                while (c != '\0') {
                  symbol_string = symbol_string + std::string(1,c);
                  c = stream_get();
                }

                // epsilon, unknown and identity numbers must be handled
                //separately
                if (symbol_number == 0 || symbol_number == 1 ||
                    symbol_number == 2) {
                  special_cases.push_back
                    (std::pair<unsigned short, std::string>
                     (symbol_number,symbol_string));
                }
                else {
                  symbol_mappings.push_back
                    (std::pair<unsigned short, std::string>
                     (symbol_number, symbol_string));
                }
              }

              max_number--;

              // handle special symbol cases
              for (unsigned int i=0; i<special_cases.size(); i++) {
                if (special_cases[i].first == 0) {
                }
                else {

                  //special_cases[i].first, max_number+1);
                  fst::StdVectorFst * tmp =
                    t.tropical_ofst_interface.substitute
                    (t.implementation.tropical_ofst,
                     special_cases[i].first,
                     (unsigned short)++max_number);
                  t.implementation.tropical_ofst = tmp;

                  symbol_mappings.push_back
                    (std::pair<unsigned short, std::string>
                     (max_number, special_cases[i].second));

                }
              }

              t.tropical_ofst_interface.set_symbol_table
                (t.implementation.tropical_ofst, symbol_mappings);

              // skip the character pairs
              unsigned short to_skip=0;
              to_skip = to_skip + (unsigned short)stream_get() * 1;
              to_skip = to_skip + (unsigned short)stream_get() * 256;
              unsigned int to_skip_ = 4 * (unsigned int)to_skip;


              for( unsigned int i=0; i<to_skip_; i++)
                stream_get();

            }
        break;
        }
#if HAVE_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        {
        t.implementation.log_ofst =
          this->implementation.log_ofst->read_transducer();

          /* If we were reading an OpenFst transducer with no HFST header,
             it is possible that it has separate input and output symbol tables
             and/or that it has no input or output table or neither
             and/or that epsilon is coded differently than "@_EPSILON_SYMBOL_@"
             and/or that numbers 1 and 2 are reserved for other use than
             "@_UNKNOWN_SYMBOL_@" or "@_IDENTITY_SYMBOL_@". */
          if (! has_hfst_header)
            {
              HfstBasicTransducer * net =
                ConversionFunctions::
                  log_ofst_to_hfst_basic_transducer
                (t.implementation.log_ofst, false);
              t.log_ofst_interface.delete_transducer(t.implementation.log_ofst);
              t.implementation.log_ofst =
                ConversionFunctions::
                  hfst_basic_transducer_to_log_ofst(net);
              delete net;
            }

        if (hfst_version_2_weighted_transducer) // this should not happen
          {
            //fprintf(stderr, "ERROR: not transducer stream\n");
            //exit(1);
            HFST_THROW_MESSAGE(HfstFatalException, "not transducer stream");
          }
        break;
        }
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        t.implementation.foma =
          this->implementation.foma->read_transducer();
        break;
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        t.implementation.xfsm =
          this->implementation.xfsm->read_transducer();
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        t.implementation.my_transducer_library =
          this->implementation.my_transducer_library->read_transducer();
        break;
#endif
      case HFST_OL_TYPE:
      case HFST_OLW_TYPE:
        t.implementation.hfst_ol =
          this->implementation.hfst_ol->read_transducer(false);
        if(t.get_type() != type) // weights need to be added or removed
        { t.convert(type); }
        break;
      case ERROR_TYPE:
      default:
        debug_error("#1");
        HFST_THROW(NotTransducerStreamException);
        break;
  }

    if (type != XFSM_TYPE)
      {
        t.set_name(name);
        for (std::map<string,string>::const_iterator prop = props.begin();
             prop != props.end();
             ++prop)
          {
            t.set_property(prop->first, prop->second);
          }
      }

}

  HfstInputStream::TransducerType HfstInputStream::guess_fst_type
  (int &bytes_read)
  {

    bytes_read=0;

    char c = stream_peek();

    switch(c)
      {
      case (char)0xd6: // OpenFst
        {
          char chars_read[26];
          for (unsigned int i=0; i<26; i++) {
            chars_read[i]=(char)stream_get();
            if ( stream_eof() ) {
              HFST_THROW(EndOfStreamException);
            }
          }
          for (int i=25; i>=0; i--)
            stream_unget(chars_read[i]);

          if (chars_read[18]=='s') // standard
            return OPENFST_TROPICAL_;
          else if (chars_read[18]=='l') // log
            return OPENFST_LOG_;
          else {
            HFST_THROW(NotTransducerStreamException);
          }
          break;
        }
      case '#':  // foma
        return FOMA_;
        break;
      case (char)0x1f:  // native foma (gz magic number is 1F 8B 08)
        {
          char c0 = (char)stream_get();
          if (stream_eof()) { HFST_THROW(EndOfStreamException); }
          char c1 = (char)stream_get();
          if (stream_eof()) { HFST_THROW(EndOfStreamException); }
          char c2 = (char)stream_get();
          if (stream_eof()) { HFST_THROW(EndOfStreamException); }
          stream_unget(c2);
          stream_unget(c1);
          stream_unget(c0);
          if (c0 == (char)0x1f && c1 == (char)0x8b && c2 == (char)0x08)
            HFST_THROW(FileIsInGZFormatException);
          else
            HFST_THROW(NotTransducerStreamException);
        }
      case 'a':  // SFST
        return SFST_;
        break;
#ifdef HAVE_MY_TRANSDUCER_LIBRARY
        /* We suppose that the first char in a MyFst is 'm'. */
      case 'm':  // MyFst
        return MY_TRANSDUCER_LIBRARY_;
        break;
#endif
      case 'P':
        {
          has_hfst_header=false;
          // extract HFST version 2 header
          (void)stream_get();
          (void)stream_get();
          (void)stream_get();
          (void)stream_get();
          bytes_read=4;
          char c5=stream_get();
          if (c5 == 'A')
            return HFST_VERSION_2_UNWEIGHTED_WITHOUT_ALPHABET;
          if (c5 == 'a') {
            stream_unget(c5);
            return HFST_VERSION_2_UNWEIGHTED;
          }
          else {
                    debug_error("#3");
              HFST_THROW(NotTransducerStreamException);
          }
          break;
        }
      case 'A':
        {
          has_hfst_header=true;
          (void)stream_get();
          bytes_read=1;
          char c2 = stream_peek();
          if (c2 == 'a')
            return HFST_VERSION_2_UNWEIGHTED_WITHOUT_ALPHABET;
          if (c2 == (char)0xd6)
            return HFST_VERSION_2_WEIGHTED;
          else
            return ERROR_TYPE_;
          break;
        }
      case (char)0:
        {
          return XFSM_;
          break;
        }
      default:
        return ERROR_TYPE_;
      }
    return ERROR_TYPE_;
  }

  void HfstInputStream::process_header_data
  (StringPairVector &header_data, bool )
  {
    if (header_data.size() < 2) {
      HFST_THROW_MESSAGE(TransducerHeaderException,
                         "Hfst header has too few attributes");
    }

    // (1) first pair "version", "3.0"
    if ( ! ( ( strcmp("version", header_data[0].first.c_str()) == 0 ) &&
               ( ( strcmp("3.0", header_data[0].second.c_str()) == 0 ) ||
                 ( strcmp("3.3", header_data[0].second.c_str()) == 0 ) ) ) ){
      HFST_THROW_MESSAGE(TransducerHeaderException,
                         "Hfst header: transducer version not recognised");
    }

    // (2) second pair "type", (valid type field)
    if ( ! ( strcmp("type", header_data[1].first.c_str()) == 0 ) ) {
      HFST_THROW_MESSAGE(TransducerHeaderException,
                         "Hfst header: transducer type not given");
    }

    if (strcmp("SFST", header_data[1].second.c_str()) == 0 )
      type = SFST_TYPE;
    else if (strcmp("FOMA", header_data[1].second.c_str()) == 0 )
      type = FOMA_TYPE;
    else if (strcmp("TROPICAL_OPENFST", header_data[1].second.c_str()) == 0 ||
             strcmp("TROPICAL_OFST", header_data[1].second.c_str()) == 0)
      type = TROPICAL_OPENFST_TYPE;
    else if (strcmp("LOG_OPENFST", header_data[1].second.c_str()) == 0 ||
             strcmp("LOG_OFST", header_data[1].second.c_str()) == 0 )
      type = LOG_OPENFST_TYPE;
#if HAVE_MY_TRANSDUCER_LIBRARY
    else if (strcmp("MY_TRANSDUCER_LIBRARY", header_data[1].second.c_str())
             == 0 )
      type = MY_TRANSDUCER_LIBRARY_TYPE;
#endif
    else if (strcmp("HFST_OL", header_data[1].second.c_str()) == 0 )
      type = HFST_OL_TYPE;
    else if (strcmp("HFST_OLW", header_data[1].second.c_str()) == 0 )
      type = HFST_OLW_TYPE;
    else {
      HFST_THROW_MESSAGE(TransducerHeaderException,
                         "Hfst header: transducer type not recognised");
    }

    if (header_data.size() == 2)
      return;

    // (3) third optional pair "name", (string)
    if (header_data[2].first.compare("name") == 0) {
      name = header_data[2].second;
    }
    for (StringPairVector::const_iterator prop = header_data.begin();
         prop != header_data.end();
         ++prop)
      {
        props[prop->first] = prop->second;
      }
  }
    

  /* Try to read a hfst header. If successful, return true and the number
     of bytes read. If not, return false and 0. Throw a
     NotTransducerStreamException if the header cannot
     be parsed after a field "HFST3" or "HFST".
     Throw a TransducerHeaderException if the header data cannot be parsed. */
  bool HfstInputStream::read_hfst_header(int &bytes_read)
  {
    char c =stream_peek();

    if (c != 'H') {
      bytes_read=0;
      return false;
    }
    int header_bytes=0;
    // try to read an HFST version 3.0 header
    if (read_library_header(header_bytes))
      {
      int size_bytes=0;
      int header_size = get_header_size(size_bytes); // throws error
      StringPairVector header_info =
        get_header_data(header_size);
      process_header_data(header_info, false);           // throws error

      bytes_read = header_bytes + size_bytes + header_size;
      return true;
      }
    header_bytes=0;
    // try to read a pre-release HFST version 3.0 header
    if (read_library_header_old(header_bytes))
      {
      int type_bytes=0;
      type = get_fst_type_old(type_bytes); // throws error
      if (type == ERROR_TYPE) {
        HFST_THROW(NotTransducerStreamException);
      }
      bytes_read = header_bytes + type_bytes;

      return true;
      }
    return false;
  }

  ImplementationType HfstInputStream::get_fst_type_old(int &bytes_read)
  {
    std::string fst_type = stream_getstring();
    if (stream_eof()) {
      debug_error("#5");
      //HFST_THROW(NotTransducerStreamException);
      HFST_THROW(EndOfStreamException);
    }
    if (fst_type.compare("SFST_TYPE") == 0)
      { bytes_read=10; return SFST_TYPE; }
    if (fst_type.compare("FOMA_TYPE") == 0)
      { bytes_read=10; return FOMA_TYPE; }
    if (fst_type.compare("TROPICAL_OPENFST_TYPE") == 0)
      { bytes_read=19; return TROPICAL_OPENFST_TYPE; }
    if (fst_type.compare("LOG_OPENFST_TYPE") == 0)
      { bytes_read=14; return LOG_OPENFST_TYPE; }
    if (fst_type.compare("HFST_OL_TYPE") == 0)
      { bytes_read=13; return HFST_OL_TYPE; }
    if (fst_type.compare("HFST_OLW_TYPE") == 0)
      { bytes_read=14; return HFST_OLW_TYPE; }
    return ERROR_TYPE;
  }

  bool HfstInputStream::read_library_header_old(int &bytes_read)
  {
    const char *id = "HFST3";
    
    for (int i=0; i<6; i++) {
      char c = stream_get();
      if(c != id[i]) { /* No match */
        stream_unget(c);
        if(i > 0) {
          for(int j=i-1; j>=0; j--) {
            stream_unget(id[j]);
          }
        }
        bytes_read=0;
        return false;
      }
    }
    bytes_read=6;
    return true;
  }

  bool HfstInputStream::read_library_header(int &bytes_read)
  {
    const char *id = "HFST";
    
    for (int i=0; i<5; i++) {
      char c = stream_get();
      if(c != id[i]) { /* No match */
        //fprintf(stderr, "No match: %c != %c\n", c, id[i]);
        stream_unget(c);
        if(i > 0) {
          for(int j=i-1; j>=0; j--) {
            stream_unget(id[j]);
          }
        }
        bytes_read=0;
        return false;
      }
    }
    bytes_read=5;
    return true;
  }

  int HfstInputStream::get_header_size(int &bytes_read)
  {
    unsigned short header_size = 0;
    stream_get(header_size);
    char c = stream_get();
    if (c != 0) {
      debug_error("#6");
      HFST_THROW_MESSAGE(NotTransducerStreamException,
                         "HFST header: header size could not be read");
    }
    bytes_read=3;

    return header_size;
  }

  StringPairVector HfstInputStream::get_header_data(int header_size)
  {
    StringPairVector retval;
    int bytes_read=0;

    while(true)
      {
        std::string str1 = stream_getstring();
        std::string str2 = stream_getstring();

        bytes_read = bytes_read + (int)str1.length() + (int)str2.length() + 2;

        if (bytes_read > header_size) {
          debug_error("#7");

          HFST_THROW_MESSAGE(NotTransducerStreamException,
                             "HFST header: FATAL: more bytes read than "
                             "the header contains");
        }
        if (stream_eof()) {
          debug_error("#8");
          HFST_THROW_MESSAGE(NotTransducerStreamException,
                             "HFST header: FATAL: stream ended"
                             " before the header could be read");
        }

        retval.push_back(std::pair<std::string, std::string>
                         (str1, str2) );
        if (bytes_read == header_size)
          break;
      }
    return retval;
  }
  

  /* The implementation type of the first transducer in the stream. */
  ImplementationType HfstInputStream::stream_fst_type()
  {
    int bytes_read=0;

    // whether the stream contains an HFST version 3.0 transducer
    if (read_hfst_header(bytes_read)) {
      has_hfst_header=true;
      bytes_to_skip=bytes_read;
      return type;
    }

    // whether the stream contains an HFST version <3.0 transducer
    // or a native SFST, OpenFst, foma or xfsm transducer
    TransducerType transducer_type = guess_fst_type(bytes_read);
    bytes_to_skip=bytes_read;

    switch (transducer_type)
      {
      case HFST_VERSION_2_WEIGHTED:
        hfst_version_2_weighted_transducer=true;
        return TROPICAL_OPENFST_TYPE;
        break;
      case HFST_VERSION_2_UNWEIGHTED_WITHOUT_ALPHABET:
        fprintf(stderr, "ERROR: version 2 HFST transducer with no alphabet "
                " cannot be processed\n"
                "Add an alphabet with HFST version 2 tool hfst-symbols\n" );
        return ERROR_TYPE;
        break;
      case HFST_VERSION_2_UNWEIGHTED:
        return SFST_TYPE;
        break;
      case OPENFST_TROPICAL_:
        return TROPICAL_OPENFST_TYPE;
        break;
      case OPENFST_LOG_:
        return LOG_OPENFST_TYPE;
        break;
      case SFST_:
        return SFST_TYPE;
        break;
      case FOMA_:
        return FOMA_TYPE;
        break;
      case XFSM_:
        return XFSM_TYPE;
        break;
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_:
        return MY_TRANSDUCER_LIBRARY_TYPE;
        break;
#endif
      case ERROR_TYPE_:
      default:
        return ERROR_TYPE;
      }
  }

  /* Open a transducer stream to stdout.
     The implementation type of the stream is defined by
     the type of the first transducer in the stream. */
  HfstInputStream::HfstInputStream(void):
    bytes_to_skip(0), filename(std::string()), has_hfst_header(false),
    hfst_version_2_weighted_transducer(false)
  {
    input_stream = &std::cin;
    if (stream_eof())
      HFST_THROW(EndOfStreamException);
    type = stream_fst_type();

    if ( ! HfstTransducer::is_implementation_type_available(type)) {
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);
    }

    switch (type)
    {
#if HAVE_SFST
    case SFST_TYPE:
      implementation.sfst = new hfst::implementations::SfstInputStream;
      break;
#endif
#if HAVE_OPENFST
    case TROPICAL_OPENFST_TYPE:
      implementation.tropical_ofst =
        new hfst::implementations::TropicalWeightInputStream;
      break;
#if HAVE_OPENFST_LOG
    case LOG_OPENFST_TYPE:
      implementation.log_ofst =
        new hfst::implementations::LogWeightInputStream;
      break;
#endif
#endif
#if HAVE_FOMA
    case FOMA_TYPE:
      implementation.foma = new hfst::implementations::FomaInputStream;
      break;
#endif
#if HAVE_XFSM
    case XFSM_TYPE:
      // Will throw an error.
      implementation.xfsm = new hfst::implementations::XfsmInputStream;
      break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
    case MY_TRANSDUCER_LIBRARY_TYPE:
      implementation.my_transducer_library
        = new hfst::implementations::MyTransducerLibraryInputStream;
      break;
#endif
    case HFST_OL_TYPE:
      implementation.hfst_ol =
        new hfst::implementations::HfstOlInputStream(false);
      break;
    case HFST_OLW_TYPE:
      implementation.hfst_ol =
        new hfst::implementations::HfstOlInputStream(true);
      break;
    default:
      debug_error("#9");
      HFST_THROW_MESSAGE(NotTransducerStreamException,
                         "transducer type not recognised");
    }
  }

  // FIXME: HfstOutputStream takes a string parameter,
  //        HfstInputStream a const char*
  HfstInputStream::HfstInputStream(const std::string &filename):
    bytes_to_skip(0), filename(std::string(filename)), has_hfst_header(false),
    hfst_version_2_weighted_transducer(false)
  {
    if (strcmp("",filename.c_str()) != 0) {
      std::ifstream ifs(filename.c_str());
      if (ifs.fail())
        HFST_THROW_MESSAGE(NotTransducerStreamException,
                           "file could not be opened");
      input_stream = &ifs;
      if (stream_eof())
        HFST_THROW(EndOfStreamException);
      type = stream_fst_type();
    }
    else {
      input_stream = &std::cin;
      if (stream_eof())
        HFST_THROW(EndOfStreamException);
      type = stream_fst_type();
    }
    
    if ( ! HfstTransducer::is_implementation_type_available(type)) {
      throw ImplementationTypeNotAvailableException("ImplementationTypeNotAvailableException", __FILE__, __LINE__, type);
    }
    
    switch (type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        implementation.sfst
          = new hfst::implementations::SfstInputStream(filename);
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        if (strcmp(filename.c_str(),"") == 0) {
          // FIXME: this should be done in TropicalWeight layer
          implementation.tropical_ofst =
            new hfst::implementations::TropicalWeightInputStream();
        }
        else
          implementation.tropical_ofst =
            new hfst::implementations::TropicalWeightInputStream(filename);
        break;
#if HAVE_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        implementation.log_ofst =
          new hfst::implementations::LogWeightInputStream(filename);
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        implementation.foma
          = new hfst::implementations::FomaInputStream(filename);
        break;
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        implementation.xfsm
          = new hfst::implementations::XfsmInputStream(filename);
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        implementation.my_transducer_library
          = new hfst::implementations::MyTransducerLibraryInputStream(filename);
        break;
#endif
      case HFST_OL_TYPE:
        implementation.hfst_ol
          = new hfst::implementations::HfstOlInputStream(filename, false);
        break;
      case HFST_OLW_TYPE:
        implementation.hfst_ol
          = new hfst::implementations::HfstOlInputStream(filename, true);
        break;
      default:
        debug_error("#10");
        
        HFST_THROW_MESSAGE(NotTransducerStreamException,
                           "transducer type not recognised");
      }
  }
  
  HfstInputStream::~HfstInputStream(void)
  {
    switch (type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        delete implementation.sfst;
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        delete implementation.tropical_ofst;
        break;
#if HAVE_OPENFST_LOG
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
      case ERROR_TYPE:
      default:
        debug_error("#11");
        HFST_THROW(NotTransducerStreamException);
      }
  }

  void HfstInputStream::close(void)
  {
    switch (type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        implementation.sfst->close();
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        implementation.tropical_ofst->close();
        break;
#if HAVE_OPENFST_LOG
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
  }

  bool HfstInputStream::is_eof(void)
  {
    switch (type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        return implementation.sfst->is_eof();
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        return implementation.tropical_ofst->is_eof();
        break;
#if HAVE_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        return implementation.log_ofst->is_eof();
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        return implementation.foma->is_eof();
        break;
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        return implementation.xfsm->is_eof();
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        return implementation.my_transducer_library->is_eof();
        break;
#endif
      case HFST_OL_TYPE:
      case HFST_OLW_TYPE:
        return implementation.hfst_ol->is_eof();
      default:
        assert(false);
        return false;
      }
  }

  bool HfstInputStream::is_bad(void)
  {
    switch (type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        return implementation.sfst->is_bad();
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        return implementation.tropical_ofst->is_bad();
        break;
#if HAVE_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        return implementation.log_ofst->is_bad();
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        return implementation.foma->is_bad();
        break;
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        return implementation.xfsm->is_bad();
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        return implementation.my_transducer_library->is_bad();
        break;
#endif
      case HFST_OL_TYPE:
      case HFST_OLW_TYPE:
        return implementation.hfst_ol->is_bad();
        break;
      default:
        assert(false);
        return false;
      }
  }

  bool HfstInputStream::is_good(void)
  {
    switch (type)
      {
#if HAVE_SFST
      case SFST_TYPE:
        return implementation.sfst->is_good();
        break;
#endif
#if HAVE_OPENFST
      case TROPICAL_OPENFST_TYPE:
        return implementation.tropical_ofst->is_good();
        break;
#if HAVE_OPENFST_LOG
      case LOG_OPENFST_TYPE:
        return implementation.log_ofst->is_good();
        break;
#endif
#endif
#if HAVE_FOMA
      case FOMA_TYPE:
        return implementation.foma->is_good();
        break;
#endif
#if HAVE_XFSM
      case XFSM_TYPE:
        return implementation.xfsm->is_good();
        break;
#endif
#if HAVE_MY_TRANSDUCER_LIBRARY
      case MY_TRANSDUCER_LIBRARY_TYPE:
        return implementation.my_transducer_library->is_good();
        break;
#endif
      case HFST_OL_TYPE:
      case HFST_OLW_TYPE:
        return implementation.hfst_ol->is_good();
        break;
      default:
        assert(false);
        return false;
      }
  }
  
  ImplementationType HfstInputStream::get_type(void) const
  {
    return type;
  }

  bool HfstInputStream::is_hfst_header_included(void) const
  {
    return has_hfst_header;
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
