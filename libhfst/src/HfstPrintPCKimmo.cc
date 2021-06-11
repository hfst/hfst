// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include "HfstTransducer.h"
using hfst::HfstTransducer;
using hfst::HfstBasicTransducer;
using hfst::implementations::HfstState;

namespace hfst {

void
print_pckimmo(FILE* out, HfstTransducer& t)
  {
    HfstBasicTransducer mutt {t};
    HfstState s = 0;
    HfstState last = 0;
    std::set<std::pair<std::string,std::string> > pairs;
    for (const auto & state: mutt)
      {
        for (const auto & arc : state)
          {
            const std::string first = arc.get_input_symbol();
            const std::string second = arc.get_output_symbol();
            pairs.emplace(first, second);
          }
        ++last;
      }
    // width of the first column
    unsigned int numwidth = 0;
    for (unsigned int i = 1; i < last; i *= 10, numwidth++)
      {}
    // first line is input symbols per pair
    // (left corner is digit width + 2)
    fprintf(out, "%*s  ", numwidth, " ");
    for (const auto & p: pairs)
      {
        if (p.first == hfst::internal_epsilon)
          {
            fprintf(out, "%.*s ", numwidth, "0");
          }
        else if (p.first == hfst::internal_unknown)
          {
            fprintf(out, "%.*s ", numwidth, "@");
          }
        else
          {
            fprintf(out, "%.*s ", numwidth, p.first.c_str());
          }
      }
    // second line is output symbols per pair
    fprintf(out, "\n");
    // (left corner is digit width + 2)
    fprintf(out, "%*s  ", numwidth, " ");
    for (const auto & p: pairs)
      {
        if (p.second == hfst::internal_epsilon)
          {
            fprintf(out, "%.*s ", numwidth, "0");
          }
        else if (p.second == hfst::internal_unknown)
          {
            fprintf(out, "%.*s ", numwidth, "@");
          }
        else
          {
            fprintf(out, "%.*s ", numwidth, p.second.c_str());
          }

      }
    // the transition table per state
    fprintf(out, "\n");
    for (const auto & state : mutt)
      {
        if (mutt.is_final_state(s))
          {
            fprintf(out, "%.*d. ", numwidth, s + 1);
          }
        else
          {
            fprintf(out, "%.*d: ", numwidth, s + 1);
          }
        // map everything to sink state 0 first
        std::map<std::pair<std::string,std::string>,HfstState> transitions;
        for(const auto & p : pairs)
          {
            transitions[p] = -1;
          }
        for (const auto & arc : state)
          {
            const std::string first = arc.get_input_symbol();
            const std::string second = arc.get_output_symbol();
            transitions[std::make_pair(first,second)] = arc.get_target_state();
          }
        for(const auto & trans : transitions)
          {
            fprintf(out, "%.*d ", numwidth, trans.second + 1);
          }
        fprintf(out, "\n");
        ++s;
      } // for each state
  }

}
