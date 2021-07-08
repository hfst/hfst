#ifndef GUARD_DixCompiler_h
#define GUARD_DixCompiler_h

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <cstdio>
#include <map>
#include <vector>

//#if HAVE_LIBXML_TREE_H
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
//#endif

namespace hfst { class HfstTransducer; }
#include "XreCompiler.h"
#include "../implementations/HfstBasicTransducer.h"
#include "HfstTransducer.h"

using hfst::implementations::HfstState;

namespace hfst {

class RegexpCompiler {
  private:
    HfstBasicTransducer* t;
    std::string regex;
    std::set<char>* alphabet;
    size_t index;
    bool compiled;
    int line;

    void compile();
    HfstState readSegment(HfstState from);
  public:
    RegexpCompiler(std::string& reg, int ln, std::set<char>* alpha);
    HfstBasicTransducer* getTransducer();
};

enum DixType {
  Standard,
  Separable
};

enum Direction { LR, RL };

class DixCompiler {
  private:
    std::ostream * error_stream;

    Direction dir;
    DixType mode;
    std::set<char> alphabet;
    std::string current_par_name;
    std::map<std::string, HfstBasicTransducer*> pars;
    HfstBasicTransducer* t;

    std::string alt;
    std::string var;
    std::string varleft;
    std::string varright;
    bool keepboundaries;

    inline static bool const nameIs(const xmlNodePtr node, const char* name) {
      return xmlStrcmp(node->name, reinterpret_cast<const xmlChar*>(name)) == 0;
    };
    inline static std::string const getAttr(const xmlNodePtr node, const char* attr) {
      std::string ret;
      xmlChar* val = xmlGetProp(node, reinterpret_cast<const xmlChar*>(attr));
      if (val != NULL) {
        ret = reinterpret_cast<const char*>(val);
      }
      return ret;
    };

    void readSection(xmlNode* section);
    void readEntry(xmlNode* ent);
    HfstState readInvariant(xmlNode* node, HfstState start);
    HfstState readPair(xmlNode* node, HfstState start);
    void readContainer(xmlNode* node, std::vector<std::string>& symbols);
    void readSegment(xmlNode* node, std::vector<std::string>& symbols);

  public:
    DixCompiler(std::string direction);
    void parse(const char* filename);
    HfstTransducer* getTransducer(hfst::ImplementationType impl);
    void setalt(char* val) { alt = val; }
    void setvar(char* val) { var = val; }
    void setvarleft(char* val) { varleft = val; }
    void setvarright(char* val) { varright = val; }
    void setkeepboundaries(bool val) { keepboundaries = val; }
};

}

#endif
