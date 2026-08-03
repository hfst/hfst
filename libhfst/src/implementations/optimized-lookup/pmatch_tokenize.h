// Copyright (c) 2016-2019 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.
#ifndef _HFST_OL_TRANSDUCER_PMATCH_TOKENIZE_H_
#define _HFST_OL_TRANSDUCER_PMATCH_TOKENIZE_H_

#include <iostream>
#include <iomanip>
#include <iterator>

#include "pmatch.h"

namespace hfst_ol_tokenize {

enum OutputFormat {
    tokenize,
    space_separated,
    xerox,
    cg,
    finnpos,
    giellacg,
    conllu,
    visl,
    jsonl,
};

struct TokenizeSettings {
    OutputFormat output_format = tokenize;
    int max_weight_classes = std::numeric_limits<int>::max();
    bool dedupe = false;
    bool print_weights = false;
    bool print_all = false;
    double time_cutoff = 0.0;
    float weight_cutoff = -1.0;
    bool verbose = true;
    float beam = -1.0;
    bool tokenize_multichar = false;
    bool hack_uncompose = false;
};

void print_nonmatching_sequence(std::string const & str, std::ostream & outstream, const TokenizeSettings& s);

void match_and_print(hfst_ol::PmatchContainer & container,
                     std::ostream & outstream,
                     const std::string & input_text,
                     const TokenizeSettings& s);

void process_input(hfst_ol::PmatchContainer & container,
                   std::istream& instream,
                   std::ostream& outstream,
                   const TokenizeSettings& s);

}

inline std::size_t find_first_not_of_def(const std::string & str, char c, std::size_t def) {
    auto ret = str.find_first_not_of(c);
    if(ret == std::string::npos) {
        return def;
    }
    else {
        return ret;
    }
}

inline std::size_t find_last_not_of_def(const std::string & str, char c, std::size_t def) {
    auto ret = str.find_last_not_of(c);
    if(ret == std::string::npos) {
        return def;
    }
    else {
        return ret;
    }
}

#endif //_HFST_OL_TRANSDUCER_PMATCH_TOKENIZE_H_
