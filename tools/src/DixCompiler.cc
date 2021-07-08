#include "DixCompiler.h"
#include "hfst-commandline.h"

namespace hfst {

DixCompiler::DixCompiler(std::string direction) {
  if (direction == "lr") {
    dir = LR;
  } else if (direction == "rl") {
    dir = RL;
  } else {
    error(EXIT_FAILURE, 0, "Unknown compilation direction '%s'", direction.c_str());
  }
  pars[""] = new HfstBasicTransducer();
}

HfstTransducer*
DixCompiler::getTransducer(hfst::ImplementationType impl) {
  HfstTransducer* ret = new HfstTransducer(*pars[""], impl);
  return &ret->minimize();
}

void
DixCompiler::parse(const char* filename) {
  xmlDocPtr doc;
  xmlNodePtr node;
  doc = xmlParseFile(filename);
  node = xmlDocGetRootElement(doc);
  if (node == NULL) {
    xmlFreeDoc(doc);
    error(EXIT_FAILURE, 0, "Libxml could not parse %s", filename);
  }
  if (!nameIs(node, "dictionary")) {
    xmlFreeDoc(doc);
    error(EXIT_FAILURE, 0, "Root element of %s is not <dictionary>", filename);
  }
  std::string typ = getAttr(node, "type");
  if (typ == "separable" || typ == "sequential") {
    mode = Separable;
  } else {
    mode = Standard;
  }
  for(xmlNode* child = node->children; child != NULL; child = child->next) {
    if (child->type != XML_ELEMENT_NODE) {
      continue;
    }
    if (nameIs(child, "sdefs")) {
      continue;
    } else if (nameIs(child, "alphabet")) {
      for (xmlNode* ch = child->children; ch != NULL; ch = ch->next) {
        if (ch->type == XML_TEXT_NODE) {
          std::string s = reinterpret_cast<const char*>(ch->content);
          for (auto& it : s) {
            alphabet.insert(it);
          }
        } else if (ch->type == XML_ELEMENT_NODE) {
          error(EXIT_FAILURE, 0, "Unexpected element %s on line %d",
                reinterpret_cast<const char*>(ch->name), ch->line);
        }
      }
    } else if (nameIs(child, "pardefs")) {
      for (xmlNode* par = child->children; par != NULL; par = par->next) {
        if (par->type != XML_ELEMENT_NODE) {
          continue;
        }
        if (nameIs(par, "pardef")) {
          readSection(par);
        } else {
          error(EXIT_FAILURE, 0, "Unexpected element %s on line %d",
                reinterpret_cast<const char*>(par->name), par->line);
        }
      }
    } else if (nameIs(child, "section")) {
      readSection(child);
    } else {
      xmlFreeDoc(doc);
      error(EXIT_FAILURE, 0, "Unexpected element %s on line %d",
              reinterpret_cast<const char*>(child->name), child->line);
    }
  }
  xmlFreeDoc(doc);
}

void
DixCompiler::readSection(xmlNode* section) {
  if (nameIs(section, "section")) {
    current_par_name = "";
    t = pars[current_par_name];
  } else {
    t = new HfstBasicTransducer();
    current_par_name = getAttr(section, "n");
    if (current_par_name == "") {
      error(EXIT_FAILURE, 0, "Unnamed pardef on line %d", section->line);
    }
    if (pars.find(current_par_name) != pars.end()) {
      error(EXIT_FAILURE, 0, "Redefinition of pardef %s on line %d",
              current_par_name.c_str(), section->line);
    }
  }
  for(xmlNode* ent = section->children; ent != NULL; ent = ent->next) {
    if (ent->type != XML_ELEMENT_NODE) {
      continue;
    }
    if (nameIs(ent, "e")) {
      readEntry(ent);
    } else {
      error(EXIT_FAILURE, 0, "Unexpected element %s on line %d",
              reinterpret_cast<const char*>(ent->name), ent->line);
    }
  }
  pars[current_par_name] = t;
}

void
DixCompiler::readEntry(xmlNode* ent) {
  std::string e_dir = getAttr(ent, "r");
  if ((e_dir == "LR" && dir == RL) || (e_dir == "RL" && dir == LR)) {
    return;
  }
  if (getAttr(ent, "i") == "yes") {
    return;
  }
  if (dir == RL) {
    std::string altval = getAttr(ent, "alt");
    if (altval != "" && altval != alt) {
      return;
    }
    std::string varval = getAttr(ent, "v");
    if (varval != "" && varval != var) {
      return;
    }
    std::string varl = getAttr(ent, "vl");
    if (varl != "" && varl != varleft) {
      return;
    }
  } else {
    std::string varr = getAttr(ent, "vr");
    if (varr != "" && varr != varright) {
      return;
    }
  }
  HfstState from = 0;
  HfstState to;
  for(xmlNode* elem = ent->children; elem != NULL; elem = elem->next) {
    if (elem->type != XML_ELEMENT_NODE) {
      continue;
    }
    if (nameIs(elem, "i")) {
      from = readInvariant(elem, from);
    } else if (nameIs(elem, "p")) {
      from = readPair(elem, from);
    } else if (nameIs(elem, "par")) {
      std::string name = getAttr(elem, "n");
      if (name == "") {
        error(EXIT_FAILURE, 0, "Missing paradigm name on line %d", elem->line);
      } else if (name == current_par_name) {
        error(EXIT_FAILURE, 0, "Paradigm refers to itself on line %d", elem->line);
      } else if (pars.find(name) == pars.end()) {
        error(EXIT_FAILURE, 0, "Reference to undefined paradigm %s on line %d",
                name.c_str(), elem->line);
      } else {
        to = t->add_state();
        t->insert_transducer(from, to, *pars[name]);
        from = to;
        // this could be done more efficiently by tracking where
        // we've already inserted initial and final paradigms
        // which is what lt-comp does
      }
    } else if (nameIs(elem, "re")) {
      std::string regex;
      for(xmlNode* seg = elem->children; seg != NULL; seg = seg->next) {
        if (seg->type == XML_TEXT_NODE) {
          regex += std::string(reinterpret_cast<const char*>(seg->content));
        } else if (seg->type == XML_ELEMENT_NODE) {
          error(EXIT_FAILURE, 0, "Unexpected element %s on line %d",
                reinterpret_cast<const char*>(seg->name), seg->line);
        }
      }
      RegexpCompiler comp(regex, elem->line, &alphabet);
      HfstBasicTransducer* r = comp.getTransducer();
      to = t->add_state();
      t->insert_transducer(from, to, *r);
      from = to;
    } else if (nameIs(elem, "ig")) {
      from = readInvariant(elem, from);
    } else {
      error(EXIT_FAILURE, 0, "Unexpected element %s on line %d",
              reinterpret_cast<const char*>(elem->name), elem->line);
    }
  }
  double weight = 0;
  std::string w = getAttr(ent, "w");
  if (w.size() == 0) {
    w = getAttr(ent, (dir == LR ? "wl" : "wr"));
  }
  if (w.size() > 0) {
    weight = stod(w);
  }
  t->set_final_weight(from, weight);
}

HfstState
DixCompiler::readInvariant(xmlNode* node, HfstState start) {
  std::vector<std::string> symbols;
  if (nameIs(node, "ig")) {
    symbols.push_back("#");
  }
  for(xmlNode* part = node->children; part != NULL; part = part->next) {
    readSegment(part, symbols);
  }
  HfstState from = start;
  HfstState to;
  for(auto sym : symbols) {
    to = t->add_state();
    HfstBasicTransition tr(to, sym, sym, 0);
    t->add_transition(from, tr);
    from = to;
    if (mode == Separable && (sym == "<ANY_CHAR>" || sym == "<ANY_TAG>")) {
      HfstBasicTransition loop(to, sym, sym, 0);
      t->add_transition(to, loop);
    }
  }
  return from;
}

HfstState
DixCompiler::readPair(xmlNode* node, HfstState start) {
  std::vector<std::string> left;
  std::vector<std::string> right;
  for(xmlNode* side = node->children; side != NULL; side = side->next) {
    if (side->type != XML_ELEMENT_NODE) {
      continue;
    }
    if (nameIs(side, "l")) {
      readContainer(side, left);
    } else if (nameIs(side, "r")) {
      readContainer(side, right);
    } else {
      error(EXIT_FAILURE, 0, "Unexpected element %s on line %d",
              reinterpret_cast<const char*>(side->name), side->line);
    }
  }
  while(left.size() < right.size()) {
    left.push_back("@_EPSILON_SYMBOL_@");
  }
  while(right.size() < left.size()) {
    right.push_back("@_EPSILON_SYMBOL_@");
  }
  HfstState from = start;
  HfstState to;
  for(size_t i = 0; i < left.size(); i++) {
    if (left[i] == "@_EPSILON_SYMBOL_@" && right[i] == "@_EPSILON_SYMBOL_@") {
      continue;
    }
    to = t->add_state();
    HfstBasicTransition tr(to, left[i], right[i], 0);
    t->add_transition(from, tr);
    from = to;
    if (mode == Separable &&
        (left[i] == "<ANY_CHAR>" || left[i] == "<ANY_TAG>" || left[i] == "@_EPSILON_SYMBOL_@") &&
        (right[i] == "<ANY_CHAR>" || right[i] == "<ANY_TAG>" || right[i] == "@_EPSILON_SYMBOL_@"))
    {
      HfstBasicTransition loop(to, left[i], right[i], 0);
      t->add_transition(to, loop);
      // it seems like if you have a:<ANY_TAG> <ANY_TAG>:a
      // it should realign to a:0 <ANY_TAG>:<ANY_TAG> :a
      // and loop, but for now we're just duplicating behavior of lsx-comp
    }
  }
  return from;
}

void
DixCompiler::readContainer(xmlNode* node, std::vector<std::string>& symbols) {
  for(xmlNode* elem = node->children; elem != NULL; elem = elem->next) {
    readSegment(elem, symbols);
  }
}

void
DixCompiler::readSegment(xmlNode* node, std::vector<std::string>& symbols) {
  if (node->type == XML_TEXT_NODE) {
    std::string text = std::string(reinterpret_cast<const char*>(node->content));
    for (auto c : text) {
      symbols.push_back(std::string(1, c));
    }
  } else if (node->type == XML_ELEMENT_NODE) {
    if (nameIs(node, "a")) {
      symbols.push_back("~");
    } else if (nameIs(node, "b")) {
      symbols.push_back(" ");
    } else if (nameIs(node, "g")) {
      symbols.push_back("#");
      readContainer(node, symbols);
    } else if (nameIs(node, "j")) {
      if (mode == Standard) {
        symbols.push_back("+");
      } else if (mode == Separable) {
        symbols.push_back("<$>");
      }
    } else if (nameIs(node, "m")) {
      if (keepboundaries) {
        symbols.push_back(">");
      }
    } else if (nameIs(node, "s")) {
      symbols.push_back("<" + getAttr(node, "n") + ">");
    } else if (nameIs(node, "t") && mode == Separable) {
      symbols.push_back("<ANY_TAG>");
    } else if (nameIs(node, "w") && mode == Separable) {
      symbols.push_back("<ANY_CHAR>");
    } else {
      error(EXIT_FAILURE, 0, "Unexpected element %s on line %d",
              reinterpret_cast<const char*>(node->name), node->line);
    }
  }
}

RegexpCompiler::RegexpCompiler(std::string& reg, int ln, std::set<char>* alpha) {
  regex = "(" + reg + ")";
  line = ln;
  alphabet = alpha;
  t = new HfstBasicTransducer();
  compiled = false;
  index = 0;
}

HfstState
RegexpCompiler::readSegment(HfstState from) {
  HfstState to = from;
  char c = regex[index];
  if (c == ')' || c == ']') {
    error(EXIT_FAILURE, 0,
          "Mismatched bracket in regular expression on line %d", line);
  } else if (c == '*' || c == '?' || c == '+') {
    error(EXIT_FAILURE, 0,
          "Unexpected quantifier in regular expression on line %d", line);
  } else if (c == '(') {
    to = t->add_state();
    HfstBasicTransition final_eps(to, "@_EPSILON_SYMBOL_@",
                                  "@_EPSILON_SYMBOL_@", 0);
    HfstState cur = from;
    index++;
    while (true) {
      if (index == regex.size()) {
        error(EXIT_FAILURE, 0,
              "Missing closing parenthesis in regular expression on line %d",
              line);
      }
      if (regex[index] == ')') {
        t->add_transition(cur, final_eps);
        break;
      } else if (regex[index] == '|') {
        t->add_transition(cur, final_eps);
        cur = from;
      } else {
        cur = readSegment(cur);
      }
    }
  } else if (c == '[') {
    index++;
    bool negate = false;
    if (regex[index] == '^') {
      negate = true;
      index++;
    }
    std::set<char> contents;
    char prev = 0;
    bool dash = false;
    for (;; index++) {
      if (index == regex.size()) {
        error(EXIT_FAILURE, 0,
              "Missing closing bracket in regular expression on line %d", line);
      }
      if (regex[index] == '\\') {
        index++;
        if (index == regex.size()) {
          error(EXIT_FAILURE, 0,
                "Trailing backslash in regular expression on line %d", line);
        }
        prev = regex[index];
        contents.insert(prev);
      } else if (regex[index] == '-') {
        if (prev == 0 || dash) {
          error(EXIT_FAILURE, 0,
                "Unexpected dash in regular expression on line %d", line);
        }
        dash = true;
      } else if (regex[index] == ']') {
        if (dash) {
          contents.insert('-');
        }
        break;
      } else {
        if (dash) {
          char cur = regex[index];
          if (cur <= prev) {
            error(EXIT_FAILURE, 0,
                  "Bad range in regular expression on line %d", line);
          }
          for (char i = prev+1; i < cur; i++) {
            contents.insert(i);
          }
          dash = false;
          prev = 0;
        } else {
          prev = regex[index];
          contents.insert(prev);
        }
      }
    }
    if (negate) {
      std::set<char> temp = *alphabet;
      temp.swap(contents);
      for (auto& it : temp) {
        contents.erase(it);
      }
    }
    if (contents.empty()) {
      error(EXIT_FAILURE, 0,
            "Empty character set in regular expression on line %d", line);
    }
    to = t->add_state();
    for (auto& it : contents) {
      HfstBasicTransition tr(to, std::string(1, it), std::string(1, it), 0);
      t->add_transition(from, tr);
    }
  } else {
    if (c == '\\') {
      c = regex[++index];
      if (index + 1 == regex.size()) {
        error(EXIT_FAILURE, 0,
              "Trailing backslash in regular expression on line %d", line);
      }
    }
    to = t->add_state();
    HfstBasicTransition tr(to, std::string(1, c), std::string(1, c), 0);
    t->add_transition(from, tr);
  }
  index++;
  if (index < regex.size()) {
    std::string eps = "@_EPSILON_SYMBOL_@";
    if (regex[index] == '+') {
      HfstBasicTransition tr1(to, eps, eps, 0);
      t->add_transition(from, tr1);
      HfstBasicTransition tr2(from, eps, eps, 0);
      t->add_transition(to, tr2);
      index++;
    } else if (regex[index] == '*') {
      HfstBasicTransition tr(to, eps, eps, 0);
      t->add_transition(from, tr);
      index++;
    } else if (regex[index] == '?') {
      HfstBasicTransition tr(from, eps, eps, 0);
      t->add_transition(to, tr);
      index++;
    }
  }
  return to;
}

void
RegexpCompiler::compile() {
  readSegment(0);
  if (index != regex.size()) {
    error(EXIT_FAILURE, 0,
          "Mismatched bracket in regular expression on line %d", line);
  } else {
    compiled = true;
  }
}
  
HfstBasicTransducer*
RegexpCompiler::getTransducer() {
  if (!compiled) {
    compile();
  }
  return t;
}

}
