/*
  Test file for flag diacritics.
*/

#include "HfstTransducer.h"
#include "auxiliary_functions.cc"

using namespace hfst;
using hfst::implementations::HfstState;
using hfst::implementations::HfstBasicTransducer;
using hfst::implementations::HfstBasicTransition;

int main(int argc, char **argv)
{

  HfstBasicTransducer t;
  HfstState s1 = t.add_state();
  HfstState s2 = t.add_state();
  HfstState s3 = t.add_state();
  HfstState s4 = t.add_state();
  HfstState s5 = t.add_state();
  HfstState s6 = t.add_state();
  t.set_final_weight(s6, 0);
  
  std::string fd1("@U.FEATURE.FOO@");
  std::string fd2("@U.FEATURE.BAR@");
  
  t.add_transition(0, HfstBasicTransition(s1, fd1, fd1, 0));
  t.add_transition(0, HfstBasicTransition(s2, "a", "a", 0));
  t.add_transition(s1, HfstBasicTransition(s3, "b", "b", 0));
  t.add_transition(s2, HfstBasicTransition(s3, fd2, fd2, 0));
  t.add_transition(s3, HfstBasicTransition(s4, "c", "c", 0));
  t.add_transition(s3, HfstBasicTransition(s5, "d", "d", 0));
  t.add_transition(s4, HfstBasicTransition(s6, fd2, fd2, 0));
  t.add_transition(s5, HfstBasicTransition(s6, fd1, fd1, 0));


  const unsigned int TYPES_SIZE=5;
  const ImplementationType types [] = {SFST_TYPE,
                       TROPICAL_OPENFST_TYPE,
                       /*LOG_OPENFST_TYPE,*/
                       FOMA_TYPE,
                       HFST_OL_TYPE,
                       HFST_OLW_TYPE};


  /* For all transducer implementation types, perform the following tests: */
  for (unsigned int i=0; i<TYPES_SIZE-2; i++)
    // FIXME: infinite loop in HFST_OL_TYPE
    {
      if (not HfstTransducer::is_implementation_type_available(types[i]))
	continue;

      if (types[i] != LOG_OPENFST_TYPE) {

      verbose_print("Identitites with flags", types[i]);

      HfstTransducer id("@_IDENTITY_SYMBOL_@", types[i]);
      id.repeat_star();
      HfstTransducer ab_flag("a", "b", types[i]);
      HfstTransducer flag("@U.F.A@", types[i]);
      ab_flag.disjunct(flag);

      ab_flag.concatenate(id);
      id.minimize();

      HfstTransducer a_tr("a", types[i]);
      HfstTransducer b_tr("b", types[i]);
      HfstTransducer abid("@_IDENTITY_SYMBOL_@", types[i]);
      abid.disjunct(a_tr);
      abid.disjunct(b_tr);
      abid.repeat_star();
      abid.minimize();

      assert(abid.compare(id));
      }

      verbose_print("Unification flags", types[i]);
      
      HfstTransducer tr(t, types[i]);
      HfstTwoLevelPaths results;

      tr.extract_paths_fd(results);

      assert(results.size() == 2);

      std::set<StringPair> result_strings;
      for (const auto & result : results)
    {
      std::string istring;
      std::string ostring;
      for (StringPairVector::const_iterator IT = result.second.begin();
           IT != result.second.end(); IT++)
        {
          istring.append(IT->first);
          ostring.append(IT->second);
        }
      result_strings.insert(StringPair(istring, ostring));
    }
      
      assert(result_strings.find(StringPair("ac","ac"))
         != result_strings.end());
      assert(result_strings.find(StringPair("bd","bd"))
         != result_strings.end());
      

      // TODO: More tests...

    }
}

