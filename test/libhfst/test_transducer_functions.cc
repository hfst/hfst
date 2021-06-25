/*
   Test file for HfstTransducer member functions:
   - compare
   - compose
   - convert
   - extract_paths and extract_paths_fd
   - insert_freely
   - is_cyclic
   - is_lookup_infinitely_ambiguous, lookup and lookup_fd
   - n_best
   - push_weights
   - set_final_weights
   - substitute
   - transform_weights
   
   Functions:
      concatenate
      determinize
      disjunct
      input_project
      intersect
      invert
      minimize
      optionalize
      output_project
      remove_epsilons
      repeat_n
      repeat_n_minus
      repeat_n_plus
      repeat_n_to_k
      repeat_plus
      repeat_star
      reverse
      subtract
   are tested in command line tool tests
*/

#include "HfstTransducer.h"
#include "auxiliary_functions.cc"

using namespace hfst;

using hfst::implementations::HfstBasicTransition;
using hfst::implementations::HfstBasicTransducer;

/* Used by the tests. */
bool compare_alphabets(const HfstTransducer &t1, const HfstTransducer &t2)
{
  StringSet alpha1 = t1.get_alphabet();
  StringSet alpha2 = t2.get_alphabet();
  bool retval = (alpha1 == alpha2);

  // DEBUG
  if (not retval)
    {
      for (const auto & it : alpha1) {
    std::cerr << it << std::endl;
      }
      std::cerr << "--" << std::endl;
      for (StringSet::const_iterator it = alpha2.begin();
       it != alpha1.end(); it++) {
    std::cerr << *it << std::endl;
      }
      std::cerr << "----" << std::endl;
    }
  
  return retval;
}

/* Used by function 'do_hfst_lookup_paths_contain'. */
bool compare_string_vectors(const StringVector &v1, const StringVector &v2,
                bool test_strings=false)
{
  /* Test the generated strings. */
  if (test_strings)
    {
      std::string v1_string;
      std::string v2_string;

      for (const auto & i : v1) {
    v1_string.append(i);
      }
      for (const auto & i : v2) {
    v2_string.append(i);
      }
      // fprintf(stderr, "####### comparing strings '%s' and '%s'...\n", v1_string.c_str(), v2_string.c_str() ); // debug
      return (v1_string.compare(v2_string) == 0);
    }

  /* Test the exact alignments. */
  if (v1.size() != v2.size())
    return false;
  for (unsigned int i=0; i<v1.size(); i++)
    {
      if (v1[i].compare(v2[i]) != 0)
    return false;
    }
  return true;
}

/* Does \a results contain \a expected_path. \a test_path_weight defines
   whether the weight of the path found in \a results must be equal to
   \a path_weight (a deviation of +/- 0.01 is allowed). */
bool do_hfst_lookup_paths_contain(const HfstOneLevelPaths &results,
                  const StringVector &expected_path,
                  float path_weight=0,
                  bool test_path_weight=false)
{
  bool found=false;
  float weight=0;
  for (const auto & result : results)
    {
      if (compare_string_vectors(result.second, expected_path, true))
    {
      found = true;
      weight = result.first;
    }
    }
  if (found == false) {
    // fprintf(stderr, "######## returning false...\n"); // debug
    return false;
  }
  if (not test_path_weight)
    return true;
  
  if (weight > (path_weight - 0.01) &&
      weight < (path_weight + 0.01))
    return true;
  fprintf(stderr, "FAIL: The path weight is %f, %f expected.\n",
      weight, path_weight);
  return false;
}

bool do_results_contain(const HfstTwoLevelPaths &paths,
            const std::string &istring,
            const std::string &ostring,
            float weight=0,
            bool test_path_weight=false)
{
  for (const auto & path : paths)
    {
      std::string path_istring;
      std::string path_ostring;
      for (StringPairVector::const_iterator IT = path.second.begin();
       IT != path.second.end(); IT++)
    {
      if (IT->first.compare("@_EPSILON_SYMBOL_@") != 0)
        path_istring.append(IT->first);
      if (IT->second.compare("@_EPSILON_SYMBOL_@") != 0)
        path_ostring.append(IT->second);
    }

      if (path_istring.compare(istring) == 0 &&
      path_ostring.compare(ostring) == 0)
    {
      if (not test_path_weight)
        return true;
      if (path.first > (weight - 0.01) &&
          path.first < (weight + 0.01))
        return true;
    }
    }
  return false;
}

/*void print_weighted_paths(const WeightedPaths<float>::Set &paths)
{
  for (WeightedPaths<float>::Set::const_iterator it = paths.begin();
       it != paths.end(); it++)
    {
      fprintf(stderr, "%s:%s\t%f\n",
          it->istring.c_str(), it->ostring.c_str(), it->weight);
    }
    }*/

void print_string_vector(const StringVector &sv)
{
  for (const auto & it : sv)
    {
      fprintf(stderr, "\"%s\" ", it.c_str());
    }
}

void print_lookup_path(const HfstOneLevelPath &path)
{
  print_string_vector(path.second);
  fprintf(stderr, "\t%f", path.first);
}

void print_lookup_paths(const HfstOneLevelPaths &paths)
{
  for (const auto & path : paths)
    {
      print_lookup_path(path);
      fprintf(stderr, "\n");
    }
}

/* Used in testing function 'transform_weights'. */
float modify_weights(float f)
{
  return f/2;
}

/* Used in testing function 'substitute'. */
bool modify_transitions(const StringPair &sp, StringPairSet &sps)
{
  if (sp.first.compare(sp.second) == 0) {
    sps.insert(StringPair("<ID>", "<ID>"));
    return true;
  }
  return false;
}


int main(int argc, char **argv)
{

  const unsigned int TYPES_SIZE=3;
  const ImplementationType types [] = {SFST_TYPE,
                       TROPICAL_OPENFST_TYPE,
                       /*LOG_OPENFST_TYPE,*/
                                       FOMA_TYPE};

  /* For all transducer implementation types, perform the following tests: */
  for (unsigned int i=0; i<TYPES_SIZE; i++)
    {

      if (not HfstTransducer::is_implementation_type_available(types[i]))
        continue;

      try {

      /* Function compare. */
      {
    verbose_print("function compare", types[i]);

    HfstTransducer t1("foo", "bar", types[i]);
    HfstTransducer t2("foo", "@_EPSILON_SYMBOL_@", types[i]);
    HfstTransducer t3("@_EPSILON_SYMBOL_@", "bar", types[i]);
    t2.concatenate(t3);
    t2.minimize();
    /* Alignments must be the same. */
    assert(not t1.compare(t2));

    HfstTransducer t4("foo", "bar", types[i]);
    HfstTransducer t5("@_EPSILON_SYMBOL_@", types[i]);
    t4.concatenate(t5);
    /* One transducer is minimal, another is not. */
    assert(t1.compare(t4));

    /* Weights. */
    if (types[i] == TROPICAL_OPENFST_TYPE ||
        types[i] == LOG_OPENFST_TYPE)
      {
        HfstTransducer t6("foo", "bar", types[i]);
        t6.set_final_weights(0.3);
        HfstTransducer t7("foo", "bar", types[i]);
        t7.set_final_weights(0.1);

        /* Weights differ. */
        assert(not t6.compare(t7));

        HfstTransducer t8("@_EPSILON_SYMBOL_@", types[i]);
        t8.set_final_weights(0.2);
        t7.concatenate(t8);
        /* Weights are the same on each path. */
        assert(t6.compare(t7));
      }
      }


      /* Function compose. */
      {
    verbose_print("function compose", types[i]);

    HfstTransducer t1("foo", "bar", types[i]);
    t1.set_final_weights(2);
    HfstTransducer t2("bar", "baz", types[i]);
    t2.set_final_weights(3);
    HfstTransducer t3("foo", "baz", types[i]);
    t3.set_final_weights(5);
    t1.compose(t2);
    assert(t1.compare(t3));
      }

      /* Function shuffle. */
      {
        verbose_print("function shuffle", types[i]);
        
        hfst::HfstTokenizer tok;
        HfstTransducer t1("abc", "abc", tok, types[i]);
        HfstTransducer t1_ = t1;
        HfstTransducer t2("cde", "cde", tok, types[i]);
        t1.shuffle(t2);

        HfstTransducer t3("abc", "abC", tok, types[i]);
        try {
          t3.shuffle(t2);
          assert(false);
        }
        catch (const TransducersAreNotAutomataException & e)
          {
            assert(true);
          }
      }


      /* Function convert. */
      {
    verbose_print("function convert", types[i]);

    HfstTransducer t1("foo", "bar", types[i]);
    HfstTransducer t2("foo", "bar", types[i]);
    /* Go through all implementation formats
       and get back to the original one.*/
    for (unsigned int j=0; j<=TYPES_SIZE; j++)
      {
        unsigned int index = (i+j)%TYPES_SIZE;
        if (HfstTransducer::is_implementation_type_available(types[index]))
          t1.convert(types[index]);
        assert(compare_alphabets(t1, t2));
      }
    assert(t1.compare(t2));
    assert(compare_alphabets(t1, t2));
      }


      /* Functions extract_paths and extract_paths_fd. */
      {
    verbose_print("function extract_paths(_fd)", types[i]);

    using hfst::StringPair;
    using hfst::StringPairSet;
    using hfst::HfstTokenizer;
    //using hfst::WeightedPaths;

    /* Create a transducer that contains animals. */
    HfstTokenizer tok;
    HfstTransducer cat("cat", "cats", tok, types[i]);
    cat.set_final_weights(3);
    HfstTransducer dog("dog", "dogs", tok, types[i]);
    dog.set_final_weights(2.5);
    HfstTransducer mouse("mouse", "mice",  tok, types[i]);
    mouse.set_final_weights(1.7);
    HfstTransducer animals(types[i]);
    animals.disjunct(cat);
    animals.disjunct(dog);
    animals.disjunct(mouse);
    animals.minimize();

    /* What we expect to get from the animal transducer. */
    StringPairSet expected_results;
    expected_results.insert(StringPair("cat","cats"));
    expected_results.insert(StringPair("dog","dogs"));
    expected_results.insert(StringPair("mouse","mice"));

    HfstTwoLevelPaths results;
    animals.extract_paths(results,
                3, /* max_num */
                0  /* cycles */ );

    /* Test that results are as expected. */
    assert(results.size() == 3);
    for (HfstTwoLevelPaths::const_iterator it = results.begin();
         it != results.end(); it++)
      {
        std::string istring;
        std::string ostring;
        for(const auto & IT : it->second)
          {
        if (IT.first.compare("@_EPSILON_SYMBOL_@") != 0)
          istring.append(IT.first);
        if (IT.second.compare("@_EPSILON_SYMBOL_@") != 0)
          ostring.append(IT.second);
          }
        StringPair sp(istring, ostring);

        assert(expected_results.find(sp) != expected_results.end());
        /* Test weights. */
        if (types[i] == TROPICAL_OPENFST_TYPE ||
        types[i] == LOG_OPENFST_TYPE)
          {
        /* Rounding can affect precision. */
        if (istring.compare("cat") == 0)
          assert(it->first > 2.99 && it->first < 3.01);
        else if (istring.compare("dog") == 0)
          assert(it->first > 2.49 && it->first < 2.51);
        else if (istring.compare("mouse") == 0)
          assert(it->first > 1.69 && it->first < 1.71);
        else
          assert(false);
          }
      }
    
    /* More tests... */

    
    /* Functions is_lookup_infinitely_ambiguous, lookup and lookup_fd. */
    verbose_print("functions is_lookup_infinitely_ambiguous "
              "and lookup(_fd)", types[i]);
    
    /* add an animal with two possible plural forms */
    // if type is LOG_OPENFST_TYPE:
    // FATAL: EncodeMapper: Weight-encoded arc has non-trivial weight
    if (types[i] != LOG_OPENFST_TYPE)
      {
        HfstTransducer hippopotamus1("hippopotamus", "hippopotami",
                     tok, types[i]);
        hippopotamus1.set_final_weights(1.2);
        HfstTransducer hippopotamus2("hippopotamus", "hippopotamuses",
                     tok, types[i]);
        hippopotamus2.set_final_weights(1.4);
        animals.disjunct(hippopotamus1);
        animals.disjunct(hippopotamus2);
        animals.minimize();
      }

    /* convert to optimized lookup format */
    HfstTransducer animals_ol(animals);

    // debug
    // std::cerr << "####### animals_ol:" << std::endl;
    // std::cerr << animals_ol << std::endl;

    if (types[i] == TROPICAL_OPENFST_TYPE ||
        types[i] == LOG_OPENFST_TYPE) {
      animals_ol.convert(HFST_OLW_TYPE); }
    else {
      animals_ol.convert(HFST_OL_TYPE); }

    /* no limit to the number of lookup results */
    ssize_t limit=-1;

    /* strings to lookup */
    StringVector lookup_cat = tok.tokenize_one_level("cat");
    StringVector lookup_dog = tok.tokenize_one_level("dog");
    StringVector lookup_mouse = tok.tokenize_one_level("mouse");
    StringVector lookup_hippopotamus
      = tok.tokenize_one_level("hippopotamus");

    /* where results of lookup are stored */
    HfstOneLevelPaths * results_cat;
    HfstOneLevelPaths * results_dog;
    HfstOneLevelPaths * results_mouse;
    HfstOneLevelPaths * results_hippopotamus;

    /* check that lookups are not infinitely ambiguous */
    assert(not animals_ol.is_lookup_infinitely_ambiguous(lookup_cat));
    assert(not animals_ol.is_lookup_infinitely_ambiguous(lookup_dog));
    assert(not animals_ol.is_lookup_infinitely_ambiguous(lookup_mouse));
    assert(not animals_ol.is_lookup_infinitely_ambiguous
           (lookup_hippopotamus));

    // todo: more is_lookup_infinitely_ambiguous tests...

    /* perform lookups */
    results_cat = animals_ol.lookup(lookup_cat, limit);
    results_dog = animals_ol.lookup(lookup_dog, limit);
    results_mouse = animals_ol.lookup(lookup_mouse, limit);
    results_hippopotamus = animals_ol.lookup(lookup_hippopotamus, limit);

    /* check that the number of results is correct */
    assert(results_cat->size() == 1);
    assert(results_dog->size() == 1);
    assert(results_mouse->size() == 1);

    if (types[i] != LOG_OPENFST_TYPE)
      assert(results_hippopotamus->size() == 2);

    bool test_weight=false;
    if (types[i] == TROPICAL_OPENFST_TYPE ||
        types[i] == LOG_OPENFST_TYPE) {
      test_weight=true; }

    /* check that the results are correct */
    StringVector expected_path = tok.tokenize_one_level("cats");

    assert(do_hfst_lookup_paths_contain
         (*results_cat, expected_path, 3, test_weight));

    expected_path = tok.tokenize_one_level("dogs");
    assert(do_hfst_lookup_paths_contain
           (*results_dog, expected_path, 2.5, test_weight));

    expected_path = tok.tokenize_one_level("mice");
    assert(do_hfst_lookup_paths_contain
        (*results_mouse, expected_path, 1.7, test_weight));

    expected_path = tok.tokenize_one_level("hippopotami");
    if (types[i] != LOG_OPENFST_TYPE)
      assert(do_hfst_lookup_paths_contain
         (*results_hippopotamus, expected_path, 1.2, test_weight));
    
    expected_path = tok.tokenize_one_level("hippopotamuses");
    if (types[i] != LOG_OPENFST_TYPE)
      assert(do_hfst_lookup_paths_contain
         (*results_hippopotamus, expected_path, 1.4, test_weight));


    // if type is LOG_OPENFST_TYPE:
    // FATAL: SingleShortestPath: Weight needs to have the path property
    // and be right distributive: log
    if (types[i] != LOG_OPENFST_TYPE)
      {

        /* Function n_best. */
        verbose_print("function n_best", types[i]);
        
        bool weighted=false;
        if (types[i] == TROPICAL_OPENFST_TYPE ||
        types[i] == LOG_OPENFST_TYPE)
          weighted=true;

        HfstTransducer animals1(animals);
        animals1.n_best(1);
        HfstTwoLevelPaths results1;
        animals1.extract_paths(results1);
        assert(results1.size() == 1);
        if (weighted) {
          assert(do_results_contain
             (results1, "hippopotamus", "hippopotami", 1.2, true));
        }

        HfstTransducer animals2(animals);
        animals2.n_best(2);
        HfstTwoLevelPaths results2;
        animals2.extract_paths(results2);
        assert(results2.size() == 2);
        if (weighted) {
          assert(do_results_contain
             (results2, "hippopotamus", "hippopotami", 1.2, true)
             &&
             do_results_contain
             (results2, "hippopotamus", "hippopotamuses", 1.4, true));
        }

        HfstTransducer animals3(animals);
        animals3.n_best(3);
        HfstTwoLevelPaths results3;
        animals3.extract_paths(results3);
        assert(results3.size() == 3);
        if (weighted) {
          assert(do_results_contain
             (results3, "hippopotamus", "hippopotami", 1.2, true)
             &&
             do_results_contain
             (results3, "hippopotamus", "hippopotamuses", 1.4, true)
             &&
             do_results_contain
             (results3, "mouse", "mice", 1.7, true));
        }

        HfstTransducer animals4(animals);
        animals4.n_best(4);
        HfstTwoLevelPaths results4;
        animals4.extract_paths(results4);
        assert(results4.size() == 4);
        if (weighted) {
          assert(do_results_contain
             (results4, "hippopotamus", "hippopotami", 1.2, true)
             &&
             do_results_contain
             (results4, "hippopotamus", "hippopotamuses", 1.4, true)
             &&
             do_results_contain
             (results4, "mouse", "mice", 1.7, true)
             &&
             do_results_contain
             (results4, "dog", "dogs", 2.5, true));
        }

        HfstTransducer animals5(animals);
        animals5.n_best(5);
        HfstTwoLevelPaths results5;
        animals5.extract_paths(results5);
        assert(results5.size() == 5);
        if (weighted) {
          assert(do_results_contain
             (results5, "hippopotamus", "hippopotami", 1.2, true)
             &&
             do_results_contain
             (results5, "hippopotamus", "hippopotamuses", 1.4, true)
             &&
             do_results_contain
             (results5, "mouse", "mice", 1.7, true)
             &&
             do_results_contain
             (results5, "dog", "dogs", 2.5, true)
             &&
             do_results_contain
             (results5, "cat", "cats", 3.0, true));
        }

      }
    
    delete results_cat;
    delete results_dog;
    delete results_mouse;
    delete results_hippopotamus;

      }


      /* Functions insert_freely. */
      {
    verbose_print("functions insert_freely", types[i]);

    HfstTransducer t1("a", "b", types[i]);
    t1.insert_freely(StringPair("c", "d"));

    HfstTransducer t2("a", "b", types[i]);
    HfstTransducer tr("c", "d", types[i]);
    t2.insert_freely(tr); // FAILS HERE
    assert(t1.compare(t2));
    
    HfstTransducer cd_star("c", "d", types[i]);
    cd_star.repeat_star();
    HfstTransducer ab("a", "b", types[i]);
    HfstTransducer test(types[i]);
    test = cd_star;
    test.concatenate(ab);
    test.concatenate(cd_star);

    assert(t1.compare(test));
    assert(t2.compare(test));

    HfstTransducer unk2unk
      ("@_UNKNOWN_SYMBOL_@", "@_UNKNOWN_SYMBOL_@", types[i]);
    unk2unk.insert_freely(StringPair("c", "d"));
    HfstTransducer dc("d", "c", types[i]);

    std::cerr << unk2unk << "--\n" << dc << std::endl;

    HfstTransducer empty(types[i]);
    assert(not unk2unk.intersect(dc).compare(empty));

    HfstTransducer unk2unk_
      ("@_UNKNOWN_SYMBOL_@", "@_UNKNOWN_SYMBOL_@", types[i]);
    HfstTransducer cd_("c", "d", types[i]);
    unk2unk_.insert_freely(cd_);

    HfstTransducer dc_("d", "c", types[i]);
    HfstTransducer empty_(types[i]);
    assert(not unk2unk_.intersect(dc_).compare(empty));
      }


      /* Function is_cyclic. */
      {
    verbose_print("function is_cyclic", types[i]);

    HfstTransducer t1("a", "b", types[i]);
    assert(not t1.is_cyclic());
    t1.repeat_star();
    assert(t1.is_cyclic());
      }
      }
      catch (const HfstException & e)
        {
          std::cerr << e() << std::endl;
        }

      /* Function push_weights. */
      {
    if (types[i] == TROPICAL_OPENFST_TYPE)
      {
        verbose_print("function push_weights", types[i]);

        /* Create an HFST basic transducer [a:b] with transition
           weight 0.3 and final weight 0.5. */
        HfstBasicTransducer t;
        t.add_state(1);
        t.add_transition(0, HfstBasicTransition(1, "a", "b", 0.3));
        t.set_final_weight(1, 0.5);
        
        /* Convert to tropical OpenFst format and push weights
           toward final and initial states. */
        HfstTransducer T_final(t, TROPICAL_OPENFST_TYPE);
        T_final.push_weights(TO_FINAL_STATE);
        HfstTransducer T_initial(t, TROPICAL_OPENFST_TYPE);
        T_initial.push_weights(TO_INITIAL_STATE);
        
        /* Convert back to HFST basic transducer. */
        HfstBasicTransducer t_final(T_final);
        HfstBasicTransducer t_initial(T_initial);
        
        /* Test the final weight. */
        try {
          /* Rounding can affect the precision. */
          assert(0.79 < t_final.get_final_weight(1) &&
             t_final.get_final_weight(1) < 0.81);
        }
        /* If the state does not exist or is not final */
        //catch (hfst::exceptions::HfstArgumentException e) {
        catch (const HfstException e) {
          assert(false);
        }

        /* Test the transition weight. */
        try {
          hfst::implementations::HfstBasicTransitions transitions = t_initial[0];
          assert(transitions.size() == 1);
          float weight = transitions.begin()->get_weight();
          /* Rounding can affect the precision. */
          assert(0.79 < weight &&
             weight < 0.81);
        }
        /* If the state does not exist or is not final */
        //catch (hfst::exceptions::HfstArgumentException e) {
        catch (const HfstException e) {
          assert(false);
        }
      }
    
      }


      /* Functions set_final_weights and transform_weights. */
      {
    if (types[i] == TROPICAL_OPENFST_TYPE ||
        types[i] == LOG_OPENFST_TYPE)
      {
        verbose_print("functions set_final_weights and "
              "transform_weights", types[i]);
        
        /* Create an HFST basic transducer [a:b] with transition
           weight 0.3 and final weight 0.5. */
        HfstBasicTransducer t;
        t.add_state(1);
        t.add_transition(0, HfstBasicTransition(1, "a", "b", 0.3));
        t.set_final_weight(1, 0.5);

        /* Modify weights. */
        HfstTransducer T(t, types[i]);
        T.set_final_weights(0.2);
        T.transform_weights(&modify_weights);
        T.push_weights(TO_FINAL_STATE);

        /* Convert back to HFST basic transducer and test the weight. */
        HfstBasicTransducer tc(T);
        try {
          assert(0.24 < tc.get_final_weight(1) &&
             tc.get_final_weight(1) < 0.26);
        }
        /* If the state does not exist or is not final */
        //catch (hfst::exceptions::HfstArgumentException e) {
        catch (const HfstException e) {
          assert(false);
        }

      }
      }


      /* Functions substitute. */
      {
    verbose_print("functions substitute", types[i]);

    HfstTokenizer tok;
    tok.add_multichar_symbol("<eps>");
    HfstTransducer t("cat", "cats", tok, types[i]);

    /* String with String */
    HfstTransducer t1(t);
    t1.substitute("c", "C", true, false);
    t1.substitute("t", "T", false, true);
    t1.substitute("@_EPSILON_SYMBOL_@", "<eps>");
    t1.substitute("a", "A");
    t1.substitute("T", "T");      // special
    t1.substitute("foo", "bar");  // cases
    HfstTransducer t1_("CAt<eps>", "cATs", tok, types[i]);
    assert(t1.compare(t1_));

    /* StringPair with StringPair */
    HfstTransducer t2(t);
    t2.substitute(StringPair("c","c"), StringPair("C","c"));
    t2.substitute(StringPair("C","c"), StringPair("H","h"));
    t2.substitute(StringPair("a","a"), StringPair("a","a"));      // special
    t2.substitute(StringPair("foo","bar"), StringPair("f","b"));  // cases
    HfstTransducer t2_("Hat", "hats", tok, types[i]);
    assert(t2.compare(t2_));

    /* StringPair with StringPairSet */
    HfstTransducer t3(t);
    StringPairSet sps;
    sps.insert(StringPair("c","c"));
    sps.insert(StringPair("C","C"));
    sps.insert(StringPair("h","h"));
    sps.insert(StringPair("H","H"));
    t3.substitute(StringPair("c","c"), sps);
    HfstTransducer t3_("cat", "cats", tok, types[i]);
    HfstTransducer t3_1("Cat", "Cats", tok, types[i]);
    HfstTransducer t3_2("hat", "hats", tok, types[i]);
    HfstTransducer t3_3("Hat", "Hats", tok, types[i]);
    t3_.disjunct(t3_1);
    t3_.disjunct(t3_2);
    t3_.disjunct(t3_3);
    t3_.minimize();
    assert(t3.compare(t3_));

    /* StringPair with HfstTransducer */
    HfstTransducer t4(t);
    HfstTransducer subs("ch", tok, types[i]);
    t4.substitute(StringPair("c","c"), subs);
    HfstTransducer t4_("chat", "chats", tok, types[i]);
    assert(t4.compare(t4_));

    /* Substitute with function */
    HfstTransducer t5(t);
    t5.substitute(&modify_transitions);
    tok.add_multichar_symbol("<ID>");
    HfstTransducer t5_("<ID><ID><ID>", "<ID><ID><ID>s", tok, types[i]);
    assert(t5.compare(t5_));

    /* Perform multiple string-to-string substitutions. */
    HfstTransducer t6(t);
    HfstSymbolSubstitutions subs_symbol;
    subs_symbol["c"] = "C";
    subs_symbol["a"] = "A";
    subs_symbol["t"] = "T";
    subs_symbol["s"] = "S";
    t6.substitute(subs_symbol);
    HfstTransducer t6_("CAT", "CATS", tok, types[i]);
    assert(t6.compare(t6_));

    /* Perform multiple string pair-to-string pair substitutions. */
    HfstTransducer t7(t);
    HfstSymbolPairSubstitutions subs_pair;
    subs_pair[StringPair("a", "a")] = StringPair("A", "a");
    subs_pair[StringPair("s", "s")] = StringPair("S", "S");
    subs_pair[StringPair("t", "t")] = StringPair("t", "T");
    t7.substitute(subs_pair);
    HfstTransducer t7_("cAt", "caTs", tok, types[i]);
    assert(t7.compare(t7_));

      }

      {
    verbose_print("alphabets", types[i]);

    HfstTransducer a2unk("a", "@_UNKNOWN_SYMBOL_@", types[i]);
    assert(a2unk.get_alphabet().size() == 4);
    a2unk.insert_to_alphabet("FOO");
    assert(a2unk.get_alphabet().size() == 5);
    a2unk.remove_from_alphabet("FOO");
    assert(a2unk.get_alphabet().size() == 4);
    StringSet alpha = a2unk.get_alphabet();
    assert(alpha.find("FOO") == alpha.end());
      }

      /* Test that binary operations do not change the transducer argument. */

      {
    verbose_print("binary operations", types[i]);

    HfstTransducer id2id(internal_identity, internal_identity, types[i]);
    HfstTransducer a2b("a", "b", types[i]);
    
    {
      HfstTransducer a2b_copy(a2b);
      HfstTransducer id2id_copy(id2id);
      HfstTransducer id2id_copy2(id2id);
      
      a2b_copy.concatenate(id2id_copy);
      assert(id2id_copy.compare(id2id_copy2));
      assert(id2id_copy.get_alphabet() == id2id_copy2.get_alphabet());
    }

    {
      HfstTransducer a2b_copy(a2b);
      HfstTransducer id2id_copy(id2id);
      HfstTransducer id2id_copy2(id2id);
      
      a2b_copy.disjunct(id2id_copy);
      assert(id2id_copy.compare(id2id_copy2));
      assert(id2id_copy.get_alphabet() == id2id_copy2.get_alphabet());
    }

    {
      HfstTransducer a2b_copy(a2b);
      HfstTransducer id2id_copy(id2id);
      HfstTransducer id2id_copy2(id2id);
      
      a2b_copy.intersect(id2id_copy);
      assert(id2id_copy.compare(id2id_copy2));
      assert(id2id_copy.get_alphabet() == id2id_copy2.get_alphabet());
    }

    {
      HfstTransducer a2b_copy(a2b);
      HfstTransducer id2id_copy(id2id);
      HfstTransducer id2id_copy2(id2id);
      
      a2b_copy.subtract(id2id_copy);
      assert(id2id_copy.compare(id2id_copy2));
      assert(id2id_copy.get_alphabet() == id2id_copy2.get_alphabet());
    }

    {
      HfstTransducer a2b_copy(a2b);
      HfstTransducer id2id_copy(id2id);
      HfstTransducer id2id_copy2(id2id);
      
      a2b_copy.compose(id2id_copy);
      assert(id2id_copy.compare(id2id_copy2));
      assert(id2id_copy.get_alphabet() == id2id_copy2.get_alphabet());
    }

    {
      HfstTransducer a2b_copy(a2b);
      HfstTransducer id2id_copy(id2id);
      HfstTransducer id2id_copy2(id2id);
      
      a2b_copy.insert_freely(id2id_copy);
      assert(id2id_copy.compare(id2id_copy2));
      assert(id2id_copy.get_alphabet() == id2id_copy2.get_alphabet());
    }

    /* Test that binary functions work when the argument and the calling
       object are the same. */
    {
      HfstTransducer foo("foo", types[i]);
      foo.concatenate(foo);
      HfstTransducer foofoo("foo", types[i]);
      HfstTransducer foo2("foo", types[i]);
      foofoo.concatenate(foo2);
      assert(foo.compare(foofoo));
    }
    
    
      }
    }

  // A special case..

  if (HfstTransducer::is_implementation_type_available(SFST_TYPE) &&
      HfstTransducer::is_implementation_type_available(TROPICAL_OPENFST_TYPE) &&
      HfstTransducer::is_implementation_type_available(FOMA_TYPE))
  {
    HfstTransducer t_sfst("c", "c", SFST_TYPE);
    t_sfst.convert(TROPICAL_OPENFST_TYPE);
    HfstTransducer t_foma("a", "b", FOMA_TYPE);
    t_foma.substitute(StringPair("a", "b"), StringPair("b", "a"));
    StringSet alphabet = t_foma.get_alphabet();
    assert(alphabet.find("c") == alphabet.end());
  }

}

