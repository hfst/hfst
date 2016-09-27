// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include "HfstTropicalTransducerTransitionData.h"
#include <string>
#include <map>
#include <set>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <vector>
#include "../HfstExceptionDefs.h"

#include "../hfstdll.h"

namespace hfst {

  namespace implementations {

      HfstTropicalTransducerTransitionData::SymbolType HfstTropicalTransducerTransitionData::get_epsilon()
      {
        return SymbolType("@_EPSILON_SYMBOL_@");
      }

      HfstTropicalTransducerTransitionData::SymbolType HfstTropicalTransducerTransitionData::get_unknown()
      {
        return SymbolType("@_UNKNOWN_SYMBOL_@");
      }

      HfstTropicalTransducerTransitionData::SymbolType HfstTropicalTransducerTransitionData::get_identity()
      {
        return SymbolType("@_IDENTITY_SYMBOL_@");
      }

    HfstTropicalTransducerTransitionData::Number2SymbolVector
    HfstTropicalTransducerTransitionData::number2symbol_map;
    Number2SymbolVectorInitializer
    dummy1(HfstTropicalTransducerTransitionData::number2symbol_map);
    HfstTropicalTransducerTransitionData::Symbol2NumberMap
    HfstTropicalTransducerTransitionData::symbol2number_map;
    Symbol2NumberMapInitializer
    dummy2(HfstTropicalTransducerTransitionData::symbol2number_map);
    unsigned int HfstTropicalTransducerTransitionData::max_number=2;

    unsigned int HfstTropicalTransducerTransitionData::get_max_number() {
      return max_number;
    }

    std::vector<unsigned int> HfstTropicalTransducerTransitionData::get_harmonization_vector
        (const std::vector<HfstTropicalTransducerTransitionData::SymbolType> &symbols)
      {
        std::vector<unsigned int> harmv;
        harmv.reserve(symbols.size());
        harmv.resize(symbols.size(), 0);
        for (unsigned int i=0; i<symbols.size(); i++)
          {
            if (symbols.at(i) != "")
              harmv.at(i) = get_number(symbols.at(i));
          }
        return harmv;
      }

      std::vector<unsigned int> HfstTropicalTransducerTransitionData::get_reverse_harmonization_vector
        (const std::map<HfstTropicalTransducerTransitionData::SymbolType, unsigned int> &symbols)
      {
        std::vector<unsigned int> harmv;
        harmv.reserve(max_number+1);
        harmv.resize(max_number+1, 0);
        for (unsigned int i=0; i<harmv.size(); i++)
          {
            std::map<SymbolType, unsigned int>::const_iterator it
              = symbols.find(get_symbol(i));
            if (it != symbols.end())
              harmv.at(i) = it->second;
          }
        return harmv;
      }

      const std::string & HfstTropicalTransducerTransitionData::get_symbol(unsigned int number)
      {
        if (number >= number2symbol_map.size()) {
          std::string message("HfstTropicalTransducerTransitionData: "
                              "number ");
          std::ostringstream oss;
          oss << number;
          message.append(oss.str());
          message.append(" is not mapped to any symbol");
          HFST_THROW_MESSAGE
            (HfstFatalException, message);
        }
        return number2symbol_map[number];
      }

      unsigned int HfstTropicalTransducerTransitionData::get_number(const std::string &symbol)
      {
        if(symbol == "") { // FAIL
          Symbol2NumberMap::iterator it = symbol2number_map.find(symbol);
          if (it == symbol2number_map.end()) {
            std::cerr << "ERROR: No number for the empty symbol\n"
                      << std::endl;
          }
          else {
            std::cerr << "ERROR: The empty symbol corresdponds to number "
                      << it->second << std::endl;
          }
          assert(false);
        }

        Symbol2NumberMap::iterator it = symbol2number_map.find(symbol);
        if (it == symbol2number_map.end())
          {
            max_number++;
            symbol2number_map[symbol] = max_number;
            number2symbol_map.push_back(symbol);
            return max_number;
          }
        return it->second;
      }


    void HfstTropicalTransducerTransitionData::print_transition_data()
      {
    fprintf(stderr, "%i:%i %f\n",
        input_number, output_number, weight);
      }

    HfstTropicalTransducerTransitionData::HfstTropicalTransducerTransitionData():
      input_number(0), output_number(0), weight(0) {}

    HfstTropicalTransducerTransitionData::HfstTropicalTransducerTransitionData
        (const HfstTropicalTransducerTransitionData &data) {
        input_number = data.input_number;
        output_number = data.output_number;
        weight = data.weight;
      }

    HfstTropicalTransducerTransitionData::HfstTropicalTransducerTransitionData(HfstTropicalTransducerTransitionData::SymbolType isymbol,
                                                                               HfstTropicalTransducerTransitionData::SymbolType osymbol,
                                                                               HfstTropicalTransducerTransitionData::WeightType weight) {
      if (isymbol == "" || osymbol == "")
        HFST_THROW_MESSAGE
          (EmptyStringException,
           "HfstTropicalTransducerTransitionData"
           "(SymbolType, SymbolType, WeightType)");

      input_number = get_number(isymbol);
      output_number = get_number(osymbol);
      this->weight = weight;
    }

    HfstTropicalTransducerTransitionData::HfstTropicalTransducerTransitionData
    (unsigned int inumber,
     unsigned int onumber,
     WeightType weight) {
      input_number = inumber;
      output_number = onumber;
      this->weight = weight;
    }

    const HfstTropicalTransducerTransitionData::SymbolType & HfstTropicalTransducerTransitionData::get_input_symbol() const {
      return get_symbol(input_number);
    }

    const HfstTropicalTransducerTransitionData::SymbolType & HfstTropicalTransducerTransitionData::get_output_symbol() const {
      return get_symbol(output_number);
    }

    unsigned int HfstTropicalTransducerTransitionData::get_input_number() const {
      return input_number;
    }

    unsigned int HfstTropicalTransducerTransitionData::get_output_number() const {
      return output_number;
    }

    HfstTropicalTransducerTransitionData::WeightType HfstTropicalTransducerTransitionData::get_weight() const {
      return weight;
    }

    void HfstTropicalTransducerTransitionData::set_weight(WeightType w) {
      weight = w;
    }


    bool HfstTropicalTransducerTransitionData::is_epsilon(const SymbolType &symbol) {
      return (symbol.compare("@_EPSILON_SYMBOL_@") == 0);
    }
    bool HfstTropicalTransducerTransitionData::is_unknown(const SymbolType &symbol) {
      return (symbol.compare("@_UNKNOWN_SYMBOL_@") == 0);
    }
    bool HfstTropicalTransducerTransitionData::is_identity(const SymbolType &symbol) {
      return (symbol.compare("@_IDENTITY_SYMBOL_@") == 0);
    }
    bool HfstTropicalTransducerTransitionData::is_valid_symbol(const SymbolType &symbol) {
      if (symbol == "")
        return false;
      return true;
    }

    HfstTropicalTransducerTransitionData::SymbolType HfstTropicalTransducerTransitionData::get_marker(const SymbolTypeSet &sts) {
      (void)sts;
      return SymbolType("@_MARKER_SYMBOL_@");
    }

    bool HfstTropicalTransducerTransitionData::operator<(const HfstTropicalTransducerTransitionData &another)
      const {
      if (input_number < another.input_number )
        return true;
      if (input_number > another.input_number)
          return false;
      if (output_number < another.output_number)
        return true;
      if (output_number > another.output_number)
        return false;
      return (weight < another.weight);
    }

    bool HfstTropicalTransducerTransitionData::less_than_ignore_weight(const HfstTropicalTransducerTransitionData &another)
      const {
      if (input_number < another.input_number )
        return true;
      if (input_number > another.input_number)
        return false;
      if (output_number < another.output_number)
        return true;
      if (output_number > another.output_number)
        return false;
      return false;
    }

    void HfstTropicalTransducerTransitionData::operator=(const HfstTropicalTransducerTransitionData &another)
    {
      input_number = another.input_number;
      output_number = another.output_number;
      weight = another.weight;
    }

    Number2SymbolVectorInitializer::Number2SymbolVectorInitializer
    (HfstTropicalTransducerTransitionData::Number2SymbolVector &vect) {
      vect.push_back(std::string("@_EPSILON_SYMBOL_@"));
      vect.push_back(std::string("@_UNKNOWN_SYMBOL_@"));
      vect.push_back(std::string("@_IDENTITY_SYMBOL_@"));
    }

    Symbol2NumberMapInitializer::Symbol2NumberMapInitializer
    (HfstTropicalTransducerTransitionData::Symbol2NumberMap &map) {
      map["@_EPSILON_SYMBOL_@"] = 0;
      map["@_UNKNOWN_SYMBOL_@"] = 1;
      map["@_IDENTITY_SYMBOL_@"] = 2;
    }

  } // namespace implementations

} // namespace hfst
