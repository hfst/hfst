// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.
#include "HfstBasicTransducer.h"

#ifndef MAIN_TEST

namespace hfst {
  namespace implementations {


  }
}

#else // MAIN_TEST was defined
#include <iostream>

int main(int argc, char * argv[])
{
  using namespace hfst::implementations;
  std::cout << "Unit tests for " __FILE__ ":" << std::endl;

    return EXIT_SUCCESS;

    HfstBasicTransducer g1;
    g1.add_state(1);
    g1.add_transition(0, HfstBasicTransition(1, "a", "a", 0.3));
    g1.add_transition(0, HfstBasicTransition(1, "b", "b", 0.1));
    //g1.add_transition(0, HfstBasicTransition(1, "@_IDENTITY_SYMBOL_@", "@_IDENTITY_SYMBOL_@", 0.1));
    g1.add_transition(0, HfstBasicTransition(1, "c", "c", 0.1));
    g1.add_transition(0, HfstBasicTransition(1, "d", "d", 0.1));
    g1.set_final_weight(1, 0.5);

    //g1.write_in_att_format(stderr);
    //std::cerr << "--" << std::endl;

    HfstBasicTransducer subst1;
    subst1.add_state(1);
    subst1.add_transition(0, HfstBasicTransition(1, "A", "A", 0.2));
    subst1.set_final_weight(1, 0.5);

    HfstBasicTransducer subst2;
    subst2.add_state(1);
    subst2.add_transition(0, HfstBasicTransition(1, "B", "B", 0.2));
    subst2.set_final_weight(1, 0.5);

    std::map<std::string, HfstBasicTransducer> subst_map;
    subst_map["a"] = subst1;
    subst_map["b"] = subst2;
    g1.substitute(subst_map, false);
    //std::pair<std::string, std::string> p = std::pair<std::string, std::string>("a", "a");
    //g1.substitute(p, subst);

    g1.remove_transition(0, HfstBasicTransition(1, "d", "d", 0.1));

    g1.write_in_att_format(stdout);
    std::cout << std::endl << "ok" << std::endl;
    return EXIT_SUCCESS;
}

#endif // MAIN_TEST
