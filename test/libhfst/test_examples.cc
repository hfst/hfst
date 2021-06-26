/*
   Test file for ...
*/

#include "HfstTransducer.h"
#include "auxiliary_functions.cc"

using namespace hfst;

using hfst::implementations::HfstBasicTransition;
using hfst::implementations::HfstBasicTransducer;

bool function(const StringPair &sp, StringPairSet &sps)
{
  if (sp.second.compare(sp.first) != 0)
    return false;

  std::string isymbol = sp.first;
  std::string osymbol;

  if (sp.second.compare("a") == 0 ||
      sp.second.compare("o") == 0 ||
      sp.second.compare("u") == 0)
    osymbol = std::string("<back_wovel>");
  if (sp.second.compare("e") == 0 ||
      sp.second.compare("i") == 0)
    osymbol = std::string("<front_wovel>");

  sps.insert(StringPair(isymbol, osymbol));
  return true;
}


int main(int argc, char **argv)
{

  const unsigned int TYPES_SIZE=3;
  const ImplementationType types [] = {SFST_TYPE,
                       TROPICAL_OPENFST_TYPE,
                       /*LOG_OPENFST_TYPE,*/
                       FOMA_TYPE};

  HfstBasicTransducer tr1;
  tr1.add_state(1);
  tr1.set_final_weight(1, 0);
  tr1.add_transition
    (0, HfstBasicTransition(1, "@_UNKNOWN_SYMBOL_@", "foo", 0) );
  // tr1 is [ @_UNKNOWN_SYMBOL_@:foo ]
  
  HfstBasicTransducer tr2;
  tr2.add_state(1);
  tr2.add_state(2);
  tr2.set_final_weight(2, 0);
  tr2.add_transition
    (0, HfstBasicTransition(1, "@_IDENTITY_SYMBOL_@",
                "@_IDENTITY_SYMBOL_@", 0) );
  tr2.add_transition
    (1, HfstBasicTransition(2, "bar", "bar", 0) );
  // tr2 is [ [ @_IDENTITY_SYMBOL_@:@_IDENTITY_SYMBOL_@ ] [ bar:bar ] ]

  // The disjunction should be
  HfstBasicTransducer disj;
  disj.add_state(1);
  disj.add_state(2);
  disj.set_final_weight(2, 0);

  disj.add_transition
    (0, HfstBasicTransition(1, "@_IDENTITY_SYMBOL_@",
                "@_IDENTITY_SYMBOL_@", 0) );
  disj.add_transition
    (0, HfstBasicTransition(1, "foo", "foo", 0) );

  disj.add_transition
    (0, HfstBasicTransition(2, "@_UNKNOWN_SYMBOL_@", "foo", 0) );
  disj.add_transition
    (0, HfstBasicTransition(2, "bar", "foo", 0) );

  disj.add_transition
    (1, HfstBasicTransition(2, "bar", "bar", 0) );


  FILE * ofile = fopen("testfile.att", "wb");
  fprintf(ofile, "0 1 a b 0.4\n");
  fprintf(ofile, "1 c d\n");
  fclose(ofile);


  // For all transducer implementation types, perform the following tests: */
  for (auto type : types)
    {
      if (not HfstTransducer::is_implementation_type_available(type))
	continue;

      verbose_print("expanding unknowns", type);
      
      HfstTransducer Tr1(tr1, type);
      HfstTransducer Tr2(tr2, type);
      HfstTransducer Disj(disj, type);

      Tr1.disjunct(Tr2).minimize();
      // Tr1 is expanded to [ @_UNKNOWN_SYMBOL_@:foo | bar:foo ]
      // Tr2 is expanded to
      // [ [ @_IDENTITY_SYMBOL_@:@_IDENTITY_SYMBOL_@ | foo:foo ] [ bar:bar ] ]
 
      assert(Tr1.compare(Disj));


      verbose_print("testing NotValidAttFormatException", type);

      FILE * ifile = fopen("testfile.att", "rb");
      try {
	unsigned int linecount = 0;
	HfstTransducer t(ifile, type, "@_EPSILON_SYMBOL_@", linecount);
    assert(false);
      }
      catch (NotValidAttFormatException e)
    {
      assert(true);
    }
      fclose(ifile);
      

    }
      remove("testfile.att");

      if (HfstTransducer::is_implementation_type_available(FOMA_TYPE))
  {
    ImplementationType type=FOMA_TYPE;
    
    /* Create a simple lexicon transducer
       [[foo bar foo] | [foo bar baz]]. */
    
    HfstTokenizer tok;
    tok.add_multichar_symbol("foo");
    tok.add_multichar_symbol("bar");
    tok.add_multichar_symbol("baz");
    
    HfstTransducer words("foobarfoo", tok, type);
    HfstTransducer t("foobarbaz", tok, type);
    words.disjunct(t);
    
    
    /* Create a rule transducer that optionally replaces
       "bar" with "baz" between "foo" and "foo". */
    
    HfstTransducerPair context
      (HfstTransducer("foo", type),
       HfstTransducer("foo", type) );
    HfstTransducer mapping
      ("bar", "baz", type);
    bool optional=true;
    
    StringPairSet alphabet;
    alphabet.insert(StringPair("foo", "foo"));
    alphabet.insert(StringPair("bar", "bar"));
    alphabet.insert(StringPair("baz", "baz"));
    
    HfstTransducer rule = rules::replace_up
      (context, mapping, optional, alphabet);
    
    
    /* Apply the rule transducer to the lexicon. */
    words.compose(rule).minimize();
    
    
    /* Extract all string pairs from the result and print
       them to stdout. */
    
    HfstTwoLevelPaths results;
    
    try {
      words.extract_paths(results);
    }
    catch (TransducerIsCyclicException e)
      {
    /* This should not happen because transducer is not cyclic. */
    fprintf(stderr, "TEST FAILED\n");
    exit(1);
      }
    
    /* Go through all paths. */
    for (const auto & result : results)
      {
    /* Go through each path. */
    StringPairVector spv = result.second;
    std::string istring("");
    std::string ostring("");
    
    for (const auto & IT : spv)
      {
        istring.append(IT.first);
        ostring.append(IT.second);
      }
    /*fprintf(stdout, "%s : %s\n",
        istring.c_str(),
        ostring.c_str());*/
      }
  }


      if (HfstTransducer::is_implementation_type_available(SFST_TYPE))
  {
    ImplementationType type = SFST_TYPE;

    StringPairSet ruleset1;
    ruleset1.insert(StringPair("a","b"));
    ruleset1.insert(StringPair("c","d"));
    ruleset1.insert(StringPair("e","f"));
    HfstTransducer rule1(ruleset1, type);

    StringPairSet ruleset2;
    ruleset2.insert(StringPair("a","b"));
    ruleset2.insert(StringPair("g","g"));
    ruleset2.insert(StringPair("i","j"));
    HfstTransducer rule2(ruleset2, type);

    StringPairSet ruleset3;
    ruleset3.insert(StringPair("a","b"));
    ruleset3.insert(StringPair("k","l"));
    ruleset3.insert(StringPair("m","n"));
    HfstTransducer rule3(ruleset3, type);

    std::vector<HfstTransducer> rules;
    rules.push_back(rule1);
    rules.push_back(rule2);
    rules.push_back(rule3);

    HfstTransducer test("A", "a", type);
    test.compose_intersect(rules);
    //std::cerr << test;
  }


      if (HfstTransducer::is_implementation_type_available(SFST_TYPE))
  {
    HfstTransducer t("a", "a", SFST_TYPE);
    t.substitute(&function);
    HfstTransducer T("a", "<back_wovel>", SFST_TYPE);
    assert(t.compare(T));
  }

}

