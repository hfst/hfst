// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include <cstdio>

#include "HfstTransducer.h"
using hfst::HfstTransducer;
using hfst::HfstBasicTransducer;
using hfst::implementations::HfstState;

namespace hfst {

#if defined(_MSC_VER) && _MSC_VER < 1900

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

  __inline int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
  {
    int count = -1;

    if (size != 0)
      count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
      count = _vscprintf(format, ap);

    return count;
  }

  __inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
  {
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
  }

#endif

void
print_dot(FILE* out, HfstTransducer& t)
  {
    //fprintf(out, "// This graph generated with hfst-fst2txt\n");
    if (t.get_name() != "")
      {
        fprintf(out, "digraph \"%s\" {\n", t.get_name().c_str());
      }
    else
      {
        fprintf(out, "digraph H {\n");
      }
    fprintf(out, "charset = UTF8;\n");
    fprintf(out, "rankdir = LR;\n");
    fprintf(out, "node [shape=circle,style=filled,fillcolor=yellow]\n");
    HfstBasicTransducer* mutt = new HfstBasicTransducer(t);
    HfstState s = 0;
    // for some reason, dot works nicer if I first have all nodes, then arcs
    for (HfstBasicTransducer::const_iterator state = mutt->begin();
         state != mutt->end();
         ++state)
      {
        if (mutt->is_final_state(s))
          {
            if (mutt->get_final_weight(s) > 0)
              {
                fprintf(out, "q%d [shape=doublecircle,"
                       "label=\"q%d/\\n%.2f\"] \n",
                        s, s, mutt->get_final_weight(s));
              }
            else
              {
                fprintf(out, "q%d [shape=doublecircle,"
                       "label=\"q%d\"] \n",
                        s, s);
              }
          }
        else
          {
            fprintf(out, "q%d [label=\"q%d\"] \n",
                    s, s);
          }
        ++s;
      } // each state
    s = 0;
    for (HfstBasicTransducer::const_iterator state = mutt->begin();
         state != mutt->end();
         ++state)
      {
        std::map<HfstState, std::string> target_labels;
        for (HfstBasicTransducer::HfstTransitions::const_iterator arc =
             state->begin();
             arc != state->end();
             ++arc)
          {
            std::string old_label = target_labels[arc->get_target_state()];
            std::string first = arc->get_input_symbol();
            std::string second = arc->get_output_symbol();
            if (first == hfst::internal_epsilon)
              {
                first = std::string("00");
              }
            else if (first == hfst::internal_identity)
              {
                first = std::string("??");
              }
            else if (first == hfst::internal_unknown)
              {
                first = std::string("?1");
              }
            if (second == hfst::internal_epsilon)
              {
                second = std::string("00");
              }
            else if (second == hfst::internal_identity)
              {
                second = std::string("??");
              }
            else if (second == hfst::internal_unknown)
              {
                second = std::string("?2");
              }
#define DOT_MAX_LABEL_SIZE 64
            char* l = static_cast<char*>(malloc(sizeof(char) *
                                                DOT_MAX_LABEL_SIZE));
            if (first == second)
              {
                if (arc->get_weight() > 0)
                  {
                    if (old_label != "")
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                             "%s, %s/%.2f", old_label.c_str(),
                             first.c_str(),
                             arc->get_weight()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException, "sprinting dot arc label");
                          }
                      }
                    else
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                                     "%s/%.2f", first.c_str(),
                                     arc->get_weight()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException, "sprinting dot arc label");
                          }
                      }  // if old label empty
                  } // if weighted
                else
                  {
                    if (old_label != "")
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                             "%s, %s", old_label.c_str(),
                             first.c_str()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException, "sprinting dot arc label");
                          }
                      }
                    else
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                                     "%s", first.c_str()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException,
                                               "sprinting dot arc label");
                          }
                      } // if old label empty
                  } // if weighted
              } // if id pair
            else
              {
                if (arc->get_weight() > 0)
                  {
                    if (old_label != "")
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                                     "%s, %s:%s/%.2f", old_label.c_str(),
                                    first.c_str(), second.c_str(),
                                    arc->get_weight()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException,
                                  "sprinting dot arc label");
                          }
                      }
                    else
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                                     "%s:%s/%.2f",
                                    first.c_str(), second.c_str(),
                                    arc->get_weight()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException,
                                  "sprinting dot arc label");
                          }
                      }  // old label empty
                  } // if weighted
                else
                  {
                    if (old_label != "")
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                                     "%s, %s:%s", old_label.c_str(),
                                    first.c_str(), second.c_str()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException,
                                  "sprinting dot arc label");
                          }
                      }
                    else
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                                     "%s:%s",
                                    first.c_str(), second.c_str()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException,
                                  "sprinting dot arc label");
                          }
                      } // if old label empty
                  } // if weighted
              } // if id pair
            string sl(l);
            replace_all(sl, "\"", "\\\"");
            target_labels[arc->get_target_state()] = sl;
            free(l);
          } // each arc
        for (std::map<HfstState,std::string>::const_iterator tl = target_labels.begin();
             tl != target_labels.end();
             ++tl)
          {
            fprintf(out, "q%d -> q%d ", s, tl->first);
            fprintf(out, "[label=\"%s \"];\n", tl->second.c_str());
          }
        ++s;
      } // each state
    fprintf(out, "}\n");
  }

void
print_dot(std::ostream & out, HfstTransducer& t)
  {
    out.precision(2);

    //out << "// This graph generated with hfst-fst2txt" << std::endl;
    if (t.get_name() != "")
      {
        out << "digraph \"" << t.get_name() << "\" {" << std::endl;
      }
    else
      {
        out << "digraph H {" << std::endl;
      }
    out << "charset = UTF8;" << std::endl;
    out << "rankdir = LR;" << std::endl;
    out << "node [shape=circle,style=filled,fillcolor=yellow]" << std::endl;
    HfstBasicTransducer* mutt = new HfstBasicTransducer(t);
    HfstState s = 0;
    // for some reason, dot works nicer if I first have all nodes, then arcs
    for (HfstBasicTransducer::const_iterator state = mutt->begin();
         state != mutt->end();
         ++state)
      {
        if (mutt->is_final_state(s))
          {
            if (mutt->get_final_weight(s) > 0)
              {
                out << "q" << s << " [shape=doublecircle," <<
                  "label=\"q" << s << "/\\n" << mutt->get_final_weight(s) << "\"] " << std::endl;
              }
            else
              {
                out << "q" << s << " [shape=doublecircle,"
                  "label=\"q" << s << " \"] " << std::endl;
              }
          }
        else
          {
            out << "q" << s << " [label=\"q" << s << "\"] " << std::endl;
          }
        ++s;
      } // each state
    s = 0;
    for (HfstBasicTransducer::const_iterator state = mutt->begin();
         state != mutt->end();
         ++state)
      {
        std::map<HfstState, std::string> target_labels;
        for (HfstBasicTransducer::HfstTransitions::const_iterator arc =
             state->begin();
             arc != state->end();
             ++arc)
          {
            std::string old_label = target_labels[arc->get_target_state()];
            std::string first = arc->get_input_symbol();
            std::string second = arc->get_output_symbol();
            if (first == hfst::internal_epsilon)
              {
                first = std::string("00");
              }
            else if (first == hfst::internal_identity)
              {
                first = std::string("??");
              }
            else if (first == hfst::internal_unknown)
              {
                first = std::string("?1");
              }
            if (second == hfst::internal_epsilon)
              {
                second = std::string("00");
              }
            else if (second == hfst::internal_identity)
              {
                second = std::string("??");
              }
            else if (second == hfst::internal_unknown)
              {
                second = std::string("?2");
              }
#define DOT_MAX_LABEL_SIZE 64
            char* l = static_cast<char*>(malloc(sizeof(char) *
                                                DOT_MAX_LABEL_SIZE));
            if (first == second)
              {
                if (arc->get_weight() > 0)
                  {
                    if (old_label != "")
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                             "%s, %s/%.2f", old_label.c_str(),
                             first.c_str(),
                             arc->get_weight()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException, "sprinting dot arc label");
                          }
                      }
                    else
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                                     "%s/%.2f", first.c_str(),
                                     arc->get_weight()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException, "sprinting dot arc label");
                          }
                      }  // if old label empty
                  } // if weighted
                else
                  {
                    if (old_label != "")
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                             "%s, %s", old_label.c_str(),
                             first.c_str()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException, "sprinting dot arc label");
                          }
                      }
                    else
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                                     "%s", first.c_str()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException,
                                               "sprinting dot arc label");
                          }
                      } // if old label empty
                  } // if weighted
              } // if id pair
            else
              {
                if (arc->get_weight() > 0)
                  {
                    if (old_label != "")
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                                     "%s, %s:%s/%.2f", old_label.c_str(),
                                    first.c_str(), second.c_str(),
                                    arc->get_weight()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException,
                                  "sprinting dot arc label");
                          }
                      }
                    else
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                                     "%s:%s/%.2f",
                                    first.c_str(), second.c_str(),
                                    arc->get_weight()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException,
                                  "sprinting dot arc label");
                          }
                      }  // old label empty
                  } // if weighted
                else
                  {
                    if (old_label != "")
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                                     "%s, %s:%s", old_label.c_str(),
                                    first.c_str(), second.c_str()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException,
                                  "sprinting dot arc label");
                          }
                      }
                    else
                      {
                        if (snprintf(l, DOT_MAX_LABEL_SIZE,
                                     "%s:%s",
                                    first.c_str(), second.c_str()) < 0)
                          {
                            HFST_THROW_MESSAGE(HfstException,
                                  "sprinting dot arc label");
                          }
                      } // if old label empty
                  } // if weighted
              } // if id pair
            string sl(l);
            replace_all(sl, "\"", "\\\"");
            target_labels[arc->get_target_state()] = sl;
            free(l);
          } // each arc
        for (std::map<HfstState,std::string>::const_iterator tl = target_labels.begin();
             tl != target_labels.end();
             ++tl)
          {
            out << "q" << s << " -> q" << tl->first << " ";
            out << "[label=\"" << tl->second << " \"];" << std::endl;
          }
        ++s;
      } // each state
  out << "}" << std::endl;
  }

}

