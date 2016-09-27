// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.
#include "HfstTransducer.h"

#ifndef MAIN_TEST

namespace hfst
{
  namespace rules
  {

    HfstTransducer replace( HfstTransducer &t,
                            ReplaceType repl_type,
                            bool optional,
                            StringPairSet &alphabet )
    {

      ImplementationType type = t.get_type();

      HfstTransducer t_proj(t);
      if (repl_type == REPL_UP)
        t_proj.input_project();
      else if (repl_type == REPL_DOWN)
        t_proj.output_project();
      else {
        //fprintf(stderr, "ERROR: replace: Impossible replace type\n");
        //exit(1);
        HFST_THROW_MESSAGE(HfstFatalException, "impossible replace type");
      }

      HfstTransducer pi_star(alphabet, type, true);

      // tc = ( .* t_proj .* )
      HfstTransducer tc(pi_star);
      tc.concatenate(t_proj);
      tc.concatenate(pi_star);

      // tc_neg = ! ( .* t_proj .* )
      HfstTransducer tc_neg(pi_star);
      tc_neg.subtract(tc);

      // retval = ( tc_neg t )* tc_neg
      HfstTransducer retval(tc_neg);
      retval.concatenate(t);
      retval.repeat_star();
      retval.concatenate(tc_neg);

      if (optional)
        retval.disjunct(pi_star);

      return retval;
    }

    HfstTransducer replace_transducer(HfstTransducer &t,
                                      std::string lm, std::string rm,
                                      ReplaceType repl_type,
                                      StringPairSet &alphabet)
    {

      t.optimize();

      ImplementationType type = t.get_type();

      // tm = ( L (L >> (R >> t)) R )
      HfstTransducer tc(t);
      tc.insert_freely(StringPair(rm,rm));
      tc.insert_freely(StringPair(lm,lm));
      HfstTransducer tm(lm, type);
      HfstTransducer rmtr(rm,type);
      tm.concatenate(tc);
      tm.concatenate(rmtr);

      tm.optimize();
      
      HfstTransducer retval = replace(tm, repl_type, false, alphabet);

      retval.optimize();
      return retval;
    }



    HfstTransducer replace_context(HfstTransducer &t,
                                   std::string m1, std::string m2,
                                   StringPairSet &alphabet)
    {
      // ct = .* ( m1 >> ( m2 >> t ))  ||  !(.* m1)

      // m1 >> ( m2 >> t )
      HfstTransducer t_copy(t);
      t_copy.insert_freely(StringPair(m1,m1));
      t_copy.insert_freely(StringPair(m2,m2));

      HfstTransducer pi_star(alphabet, t.get_type(), true);
 
      // arg1 = .* ( m1 >> ( m2 >> t ))
      HfstTransducer arg1(pi_star);

      arg1.concatenate(t_copy);

      // arg2 = !(.* m1)
      HfstTransducer m1_tr(m1, t.get_type());
      HfstTransducer tmp(pi_star);
      tmp.concatenate(m1_tr);
      HfstTransducer arg2(pi_star);
      arg2.subtract(tmp);

      // ct = .* ( m1 >> ( m2 >> t ))  ||  !(.* m1)
      HfstTransducer ct = arg1.compose(arg2);

      // mt = m2* m1 .*
      HfstTransducer mt(m2, t.get_type());
      mt.repeat_star();
      mt.concatenate(m1_tr);
      mt.concatenate(pi_star);


      // !( (!ct mt) | (ct !mt) )

      // ct !mt
      HfstTransducer tmp2(pi_star);
      tmp2.subtract(mt);
      HfstTransducer ct_neg_mt(ct);
      ct_neg_mt.concatenate(tmp2);


      // !ct mt
      HfstTransducer neg_ct_mt(pi_star);
      neg_ct_mt.subtract(ct);
      neg_ct_mt.concatenate(mt);


      // disjunction
      HfstTransducer disj = neg_ct_mt.disjunct(ct_neg_mt);


      // negation
      HfstTransducer retval(pi_star);
      retval.subtract(disj);

      retval.optimize();
      return retval;
    }



    /* identical to  ![ .* l [a:. & !a:b] r .* ]  */
    HfstTransducer two_level_if(HfstTransducerPair &context,
                                StringPairSet &mappings,
                                StringPairSet &alphabet)
    {
      if (context.first.get_type() != context.second.get_type()) {
        HFST_THROW_MESSAGE(TransducerTypeMismatchException,
                           "rules::two_level_if");
      }
      ImplementationType type = context.first.get_type();

      assert(context.second.get_type() != ERROR_TYPE);
      assert(context.second.get_type() != ERROR_TYPE);

      // calculate [ a:. ]
      StringPairSet input_to_any;
      for (StringPairSet::iterator it = mappings.begin();
           it != mappings.end(); it++)
        {
          for (StringPairSet::iterator alpha_it = alphabet.begin();
               alpha_it != alphabet.end(); alpha_it++)
            {
              if (alpha_it->first == it->first)
                {
                  input_to_any.insert(StringPair
                                      (alpha_it->first, alpha_it->second));
                }
            }
        }

      // center == [ a:. ]
      HfstTransducer center(input_to_any, type);

      // calculate [ .* - a:b ]
      HfstTransducer neg_mappings(alphabet, type, true);
      //neg_mappings.repeat_star();

      HfstTransducer mappings_tr(mappings, type);
      neg_mappings.subtract(mappings_tr);

      // center == [ a:. & !a:b ]
      center.intersect(neg_mappings);

      // left context == [ .* l ]
      HfstTransducer left_context(alphabet, type, true);
      left_context.concatenate(context.first);

      // right_context == [ r .* ]
      HfstTransducer right_context(context.second);
      HfstTransducer universal(alphabet, type, true);
      right_context.concatenate(universal);

      HfstTransducer inside(left_context.concatenate(center).
                            concatenate(right_context));

      HfstTransducer retval(universal.subtract(inside));
      return retval;
    }


    // equivalent to !(!(.* l) a:b .* | .* a:b !(r .*))
    HfstTransducer two_level_only_if(HfstTransducerPair &context,
                                     StringPairSet &mappings,
                                     StringPairSet &alphabet)
    {
      if (context.first.get_type() != context.second.get_type()) {
        HFST_THROW_MESSAGE(TransducerTypeMismatchException,
                           "rules::two_level_only_if");
      }
      ImplementationType type = context.first.get_type();

      assert(context.second.get_type() != ERROR_TYPE);
      assert(context.second.get_type() != ERROR_TYPE);

      // center = a:b
      HfstTransducer center(mappings, type);

      // left_neg = !(.* l)
      HfstTransducer left(alphabet, type, true);
      left.concatenate(context.first);
      HfstTransducer left_neg(alphabet, type, true);
      left_neg.subtract(left);

      // right_neg = !(r .*)
      HfstTransducer universal(alphabet, type, true);
      HfstTransducer right(context.second);
      right.concatenate(universal);
      HfstTransducer right_neg(alphabet, type, true);
      right_neg.subtract(right);

      // left_neg + center + universal  |  universal + center + right_neg
      HfstTransducer rule(left_neg);
      rule.concatenate(center);
      rule.concatenate(universal);
      HfstTransducer rule_right(universal);
      rule_right.concatenate(center);
      rule_right.concatenate(right_neg);
      rule.disjunct(rule_right);

      HfstTransducer rule_neg(alphabet, type, true);
      rule_neg.subtract(rule);

      return rule_neg;
    }

    HfstTransducer two_level_if_and_only_if(HfstTransducerPair &context,
                                            StringPairSet &mappings,
                                            StringPairSet &alphabet)
    {
      HfstTransducer if_rule = two_level_if(context, mappings, alphabet);
      HfstTransducer only_if_rule = two_level_only_if
        (context, mappings, alphabet);
      return if_rule.intersect(only_if_rule);
    }

    HfstTransducer replace_in_context(HfstTransducerPair &context,
                                      ReplaceType repl_type,
                                      HfstTransducer &t,
                                      bool optional,
                                      StringPairSet &alphabet)
    {

      // test that all transducers have the same type
      if (context.first.get_type() != context.second.get_type() ||
          context.first.get_type() != t.get_type() ) {
          HFST_THROW_MESSAGE(TransducerTypeMismatchException,
                             "rules::replace_in_context");
      }
      ImplementationType type = t.get_type();

      // test that both context transducers are automata
      // this could be done more efficiently...
      HfstTransducer t1_proj(context.first);
      t1_proj.input_project();
      HfstTransducer t2_proj(context.second);
      t2_proj.input_project();

      if ( ! t1_proj.compare(context.first) ||
           ! t2_proj.compare(context.second) ) {
        HFST_THROW(ContextTransducersAreNotAutomataException); }
      
      std::string leftm("@_LEFT_MARKER_@");
      std::string rightm("@_RIGHT_MARKER_@");
      std::string epsilon(internal_epsilon);

      // HfstTransducer pi(alphabet, type);

      // Create the insert boundary transducer (.|<>:<L>|<>:<R>)*
      StringPairSet pi1 = alphabet;
      pi1.insert(StringPair(internal_epsilon, leftm));
      pi1.insert(StringPair(internal_epsilon, rightm));
      HfstTransducer ibt(pi1, type, true);

      // Create the remove boundary transducer (.|<L>:<>|<R>:<>)*
      StringPairSet pi2 = alphabet;
      pi2.insert(StringPair(leftm, internal_epsilon));
      pi2.insert(StringPair(rightm, internal_epsilon));
      HfstTransducer rbt(pi2, type, true);

      // Add the markers to the alphabet
      alphabet.insert(StringPair(leftm,leftm));
      alphabet.insert(StringPair(rightm,rightm));

      HfstTransducer pi_star(alphabet,type,true);

      // Create the constrain boundary transducer !(.*<L><R>.*)
      HfstTransducer leftm_to_leftm(leftm, leftm, type);
      HfstTransducer rightm_to_rightm(rightm, rightm, type);
      HfstTransducer tmp(pi_star);
      tmp.concatenate(leftm_to_leftm);
      tmp.concatenate(rightm_to_rightm);
      tmp.concatenate(pi_star);
      HfstTransducer cbt(pi_star);
      cbt.subtract(tmp);
      cbt.optimize();

      // left context transducer .* (<R> >> (<L> >> LEFT_CONTEXT)) || !(.*<L>)
      HfstTransducer lct = replace_context
        (context.first, leftm, rightm, alphabet);

      lct.optimize();

      // right context transducer:
      // reversion( (<R> >> (<L> >> reversion(RIGHT_CONTEXT))) .* || !(<R>.*) )
      HfstTransducer right_rev(context.second);

      right_rev.reverse();
      right_rev.optimize();

      HfstTransducer rct = replace_context(right_rev, rightm, leftm, alphabet);
      rct.reverse();
      rct.optimize();

      // unconditional replace transducer
      HfstTransducer rt(type);
      if (repl_type == REPL_UP || repl_type == REPL_RIGHT ||
          repl_type == REPL_LEFT || repl_type == REPL_DOWN_KARTTUNEN)
        rt = replace_transducer( t, leftm, rightm, REPL_UP, alphabet );
      else
        rt = replace_transducer( t, leftm, rightm, REPL_DOWN, alphabet );
      rt.optimize();

      // build the conditional replacement transducer
      HfstTransducer result(ibt);

      result.compose(cbt);
      result.optimize(); // added
      
      if (repl_type == REPL_UP || repl_type == REPL_RIGHT)
        result.compose(rct);

      if (repl_type == REPL_UP || repl_type == REPL_LEFT)
        result.compose(lct);
      
      result.optimize();  // ADDED

      result.compose(rt);
      
      if (repl_type == REPL_DOWN || repl_type == REPL_RIGHT ||
      repl_type == REPL_DOWN_KARTTUNEN)
        result.compose(lct);

      if (repl_type == REPL_DOWN || repl_type == REPL_LEFT ||
      repl_type == REPL_DOWN_KARTTUNEN)
        result.compose(rct);
      
      result.optimize();  // ADDED

      result.compose(rbt);

      // Remove the markers from the alphabet
      alphabet.erase(StringPair(leftm,leftm));
      alphabet.erase(StringPair(rightm,rightm));
      
      if (optional) {
        HfstTransducer pi_star_(alphabet, type, true);
        result.disjunct(pi_star_);
      }

      result.optimize();
      return result;
    }


    HfstTransducer replace_up(HfstTransducerPair &context,
                              HfstTransducer &mapping,
                              bool optional,
                              StringPairSet &alphabet)
    {
      return replace_in_context(context, REPL_UP, mapping, optional, alphabet);
    }

    HfstTransducer replace_down(HfstTransducerPair &context,
                HfstTransducer &mapping,
                bool optional,
                StringPairSet &alphabet)
    {
      return replace_in_context
        (context, REPL_DOWN, mapping, optional, alphabet);
    }

    HfstTransducer replace_down_karttunen(HfstTransducerPair &context,
                      HfstTransducer &mapping,
                      bool optional,
                      StringPairSet &alphabet)
    {
      return replace_in_context
        (context, REPL_DOWN_KARTTUNEN, mapping, optional, alphabet);
    }

    HfstTransducer replace_right(HfstTransducerPair &context,
                                 HfstTransducer &mapping,
                                 bool optional,
                                 StringPairSet &alphabet)
    {
      return replace_in_context
        (context, REPL_RIGHT, mapping, optional, alphabet);
    }

    HfstTransducer replace_left(HfstTransducerPair &context,
                                HfstTransducer &mapping,
                                bool optional,
                                StringPairSet &alphabet)
    {
      return replace_in_context
        (context, REPL_LEFT, mapping, optional, alphabet);
    }

    HfstTransducer replace_up(HfstTransducer &mapping,
                              bool optional,
                              StringPairSet &alphabet)
    {
      return replace(mapping, REPL_UP, optional, alphabet);
    }

    HfstTransducer replace_down(HfstTransducer &mapping,
                                bool optional,
                                StringPairSet &alphabet)
    {
      return replace(mapping, REPL_DOWN, optional, alphabet);
    }

    // Left arrow replace up without context
    HfstTransducer left_replace_up(HfstTransducer &mapping,
                                   bool optional,
                                   StringPairSet &alphabet)
    {
      
      if ( optional )
        {
          return replace_up(mapping, 1, alphabet).invert();
        }
      else
        {
          return replace_up(mapping, 0, alphabet).invert();
        }
    }

    // Left arrow replace up
    HfstTransducer left_replace_up (HfstTransducerPair &context,
                                    HfstTransducer &mapping,
                                    bool optional,
                                    StringPairSet &alphabet)
    {
      if ( optional )
        {
          return replace_up(context, mapping, 1, alphabet).invert();
        }
      else
        {
          return replace_up(context, mapping, 0, alphabet).invert();
        }
    }

    // Left arrow replace down (XFST's version)
    HfstTransducer left_replace_down_karttunen( HfstTransducerPair &context,
                                                HfstTransducer &mapping,
                                                bool optional,
                                                StringPairSet &alphabet)
    {
      if ( optional )
        {
          return replace_down_karttunen(context, mapping, 1, alphabet).invert();
        }
      else
        {
          return replace_down_karttunen(context, mapping, 0, alphabet).invert();
        }
    }

    // Left arrow replace down (SFST's version)
    HfstTransducer left_replace_down (  HfstTransducerPair &context,
                                        HfstTransducer &mapping,
                                        bool optional,
                                        StringPairSet &alphabet)
    {
      if ( optional )
        {
          return replace_down(context, mapping, 1, alphabet).invert();
        }
      else
        {
          return replace_down(context, mapping, 0, alphabet).invert();
        }
    }

    // Left arrow replace left
    HfstTransducer left_replace_left ( HfstTransducerPair &context,
                                       HfstTransducer &mapping,
                                       bool optional,
                                       StringPairSet &alphabet)
    {
      if ( optional )
        {
          return replace_left(context, mapping, 1, alphabet).invert();
        }
      else
        {
          return replace_left(context, mapping, 0, alphabet).invert();
        }
    }

    // Left arrow replace right
    HfstTransducer left_replace_right ( HfstTransducerPair &context,
                                        HfstTransducer &mapping,
                                        bool optional,
                                        StringPairSet &alphabet)
    {
      if ( optional )
        {
          return replace_right(context, mapping, 1, alphabet).invert();
        }
      else
        {
          return replace_right(context, mapping, 0, alphabet).invert();
        }
    }

    HfstTransducer restriction(HfstTransducerPairVector &contexts,
                               HfstTransducer &mapping,
                               StringPairSet &alphabet,
                               TwolType twol_type,
                               int direction )
    {
      // Make sure that contexts contains at least one transducer pair and that
      // all transducers in the set have the same type.
      ImplementationType type=ERROR_TYPE;
      bool type_defined=false;
      for (HfstTransducerPairVector::const_iterator it = contexts.begin();
           it != contexts.end(); it++)
        {
          if (! type_defined) {
            type = it->first.get_type();
            type_defined=true;
          }
          else {
            if (type != it->first.get_type()) {
              HFST_THROW_MESSAGE(TransducerTypeMismatchException,
                                 "rules::restriction");
            }
          }
          if (type != it->second.get_type()) {
            HFST_THROW_MESSAGE(TransducerTypeMismatchException,
                               "rules::restriction");
      }
         }
      if (! type_defined) {
    HFST_THROW_MESSAGE(EmptySetOfContextsException, "rules::restriction");
      }

      std::string marker("@_MARKER_@");
      HfstTransducer mt(marker, type);
      HfstTransducer pi_star(alphabet, type, true);
      
      // center transducer
      HfstTransducer l1(internal_epsilon, type);
      l1.concatenate(pi_star);
      l1.concatenate(mt);
      l1.concatenate(mapping);
      l1.concatenate(mt);
      l1.concatenate(pi_star);

      HfstTransducer tmp(type);
      if (direction == 0)
        tmp = pi_star;
      else if (direction == 1)
        tmp = mapping.input_project().compose(pi_star);
      else {
        tmp = pi_star;
        tmp.compose(mapping.output_project());
      }

      // context transducer
      // pi_star + left[i] + mt + tmp + mt + + right[i] + pi_star
      HfstTransducer l2(type);
      for (HfstTransducerPairVector::const_iterator it = contexts.begin();
           it != contexts.end(); it++)
        {
          HfstTransducer ct(internal_epsilon, type);
          ct.concatenate(pi_star);
          ct.concatenate(it->first);
          ct.concatenate(mt);
          ct.concatenate(tmp);
          ct.concatenate(mt);
          ct.concatenate(it->second);
          ct.concatenate(pi_star);
          l2.disjunct(ct);
        }

      HfstTransducer result(type);
      
      if (twol_type == twol_right) {
        // TheAlphabet - ( l1 - l2 ).substitute(marker,epsilon)
        HfstTransducer retval(alphabet, type, true);
        HfstTransducer tmp1(l1);
        tmp1.subtract(l2);
        tmp1.substitute(marker,internal_epsilon);
        retval.subtract(tmp1);
        return retval;
      }
      else if (twol_type == twol_left) {
        // TheAlphabet - ( l2 - l1 ).substitute(marker,epsilon)
        HfstTransducer retval(alphabet, type, true);
        HfstTransducer tmp1(l2);
        tmp1.subtract(l1);
        tmp1.substitute(marker, internal_epsilon);
        retval.subtract(tmp1);
        return retval;
      }
      else if (twol_type == twol_both) {
        // TheAlphabet - ( l1 - l2 ).substitute(marker,epsilon)
        // TheAlphabet - ( l2 - l1 ).substitute(marker,epsilon)
        // intersect
        HfstTransducer retval1(alphabet, type, true);
        HfstTransducer tmp1(l1);
        tmp1.subtract(l2);
        tmp1.substitute(marker, internal_epsilon);
        retval1.subtract(tmp1);

        HfstTransducer retval2(alphabet, type, true);
        HfstTransducer tmp2(l2);
        tmp2.subtract(l1);
        tmp2.substitute(marker, internal_epsilon);
        retval2.subtract(tmp2);

        return retval1.intersect(retval2);
      }
      else {
        assert(false);
        return HfstTransducer(type); // make compiler happy
          }
    }

    HfstTransducer restriction(HfstTransducerPairVector &contexts,
                               HfstTransducer &mapping,
                               StringPairSet &alphabet) {
      return restriction(contexts, mapping, alphabet, twol_right, 0);
    }

    HfstTransducer coercion(HfstTransducerPairVector &contexts,
                            HfstTransducer &mapping,
                            StringPairSet &alphabet) {
      return restriction(contexts, mapping, alphabet, twol_left, 0);
    }

    HfstTransducer restriction_and_coercion(HfstTransducerPairVector &contexts,
                                            HfstTransducer &mapping,
                                            StringPairSet &alphabet) {
      return restriction(contexts, mapping, alphabet, twol_both, 0);
    }


    HfstTransducer surface_restriction(HfstTransducerPairVector &contexts,
                                       HfstTransducer &mapping,
                                       StringPairSet &alphabet) {
      return restriction(contexts, mapping, alphabet, twol_right, 1);
    }

    HfstTransducer surface_coercion(HfstTransducerPairVector &contexts,
                                    HfstTransducer &mapping,
                                    StringPairSet &alphabet) {
      return restriction(contexts, mapping, alphabet, twol_left, 1);
    }

    HfstTransducer surface_restriction_and_coercion
    (HfstTransducerPairVector &contexts,
     HfstTransducer &mapping,
     StringPairSet &alphabet) {
      return restriction(contexts, mapping, alphabet, twol_both, 1);
    }


    HfstTransducer deep_restriction(HfstTransducerPairVector &contexts,
                                    HfstTransducer &mapping,
                                    StringPairSet &alphabet) {
      return restriction(contexts, mapping, alphabet, twol_right, -1);
    }

    HfstTransducer deep_coercion(HfstTransducerPairVector &contexts,
                                 HfstTransducer &mapping,
                                 StringPairSet &alphabet) {
      return restriction(contexts, mapping, alphabet, twol_left, -1);
    }

    HfstTransducer deep_restriction_and_coercion
    (HfstTransducerPairVector &contexts,
     HfstTransducer &mapping,
     StringPairSet &alphabet) {
      return restriction(contexts, mapping, alphabet, twol_both, -1);
    }

  }

}

#else // MAIN_TEST was defined

#include <iostream>
using namespace hfst;
using namespace implementations;
using namespace hfst::rules;


/*
 * Right arrow basic tests (including replace_up without context)
 */
void right_arrow_test1( ImplementationType type )
{
        HfstTokenizer TOK;
        TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

        // Set mapping
        HfstTransducer mapping("ca", "d", TOK, type);

        // Set context
        HfstTransducer left("ca", TOK, type);
        HfstTransducer right("c", type);
        HfstTransducerPair context = HfstTransducerPair( left, right );

        // Set alphabet
        StringPair a1 = StringPair("a", "a");
        StringPair a2 = StringPair("c", "c");
        StringPair a3 = StringPair("d", "d");
        StringPairSet alphabet;
        alphabet.insert(a1);
        alphabet.insert(a2);
        alphabet.insert(a3);

        // Input stream
        HfstTransducer input1("cacacac", TOK, type);
        HfstTransducer input2("cac", TOK, type);

        /* RIGHT ARROW RULE
         *
         * To test right arrow rule, it's needed to compose input transducer with the rule transducer
         * Input_transducer .o. right_arrow_rule_tr
         */

        HfstTransducer replaceUp        = replace_up(context, mapping, 0, alphabet);
        HfstTransducer replaceDown      = replace_down_karttunen(context, mapping, 0, alphabet);
        HfstTransducer replaceLeft      = replace_left(context, mapping, 0, alphabet);
        HfstTransducer replaceRight = replace_right(context, mapping, 0, alphabet);

        HfstTransducer replaceUpWithoutContext  = replace_up(mapping, 0, alphabet);

        HfstTransducer replaceUpOptional        = replace_up(context, mapping, 1, alphabet);
        HfstTransducer replaceDownOptional      = replace_down_karttunen(context, mapping, 1, alphabet);
        HfstTransducer replaceLeftOptional      = replace_left(context, mapping, 1, alphabet);
        HfstTransducer replaceRightOptional = replace_right(context, mapping, 1, alphabet);

        HfstTransducer replaceUpWithoutContextOptional  = replace_up(mapping, 1, alphabet);

        // Results for right arrow rule

        // ca -> d || ca_c  ( input: cacacac )
        HfstTransducer result_right1("cacacac", "cad@_EPSILON_SYMBOL_@d@_EPSILON_SYMBOL_@c", TOK, type);
        // ca (->) d || ca_c  ( input: cacacac )
        HfstTransducer result_right1_optional(result_right1);
        result_right1_optional.disjunct(input1);
        // ca -> d (Without context)( input: cacacac )
        HfstTransducer result_right1WithoutContext("cacacac", "d@_EPSILON_SYMBOL_@d@_EPSILON_SYMBOL_@d@_EPSILON_SYMBOL_@c", TOK, type);
        // ca (->) d (Without context, optional)( input: cacacac )
        HfstTransducer result_right1WithoutContextOptional("cac", "d@_EPSILON_SYMBOL_@c", TOK, type);
        result_right1WithoutContextOptional.disjunct(input2);
        // ca -> d \\ ca_c  ( input: cacacac )
        HfstTransducer result_right2("cacacac", "cacad@_EPSILON_SYMBOL_@c", TOK, type);
        // ca (->) d \\ ca_c  ( input: cacacac )
        HfstTransducer result_right2_optional(result_right2);
        result_right2_optional.disjunct(input1);
        // ca -> d // ca_c  ( input: cacacac )
        HfstTransducer result_right3("cacacac", "cad@_EPSILON_SYMBOL_@cac", TOK, type);
        // ca (->) d // ca_c  ( input: cacacac )
        HfstTransducer result_right3_optional(result_right3);
        result_right3_optional.disjunct(input1);
        // ca -> d \/ ca_c  ( input: cacacac )
        HfstTransducer result_right4(type);
        result_right4.disjunct(result_right2).minimize().disjunct(result_right3).minimize();
        // ca (->) d \/ ca_c  ( input: cacacac )
        HfstTransducer result_right4_optional(result_right4);
        result_right4_optional.disjunct(input1).minimize();

        // RIGHT ARROW RULE
        // -> || Replace up
        HfstTransducer tmp = input1;
        tmp.compose(replaceUp).minimize();
        assert ( tmp.compare( result_right1 ) );
        // (->) || Replace up optional
        tmp = input1;
        tmp.compose(replaceUpOptional).minimize();
        assert ( tmp.compare( result_right1_optional ) );
        // -> Replace up without context
        tmp = input1;
        tmp.compose(replaceUpWithoutContext).minimize();
        assert ( tmp.compare( result_right1WithoutContext ) );
        // (->) Replace up without context Optional
        tmp = input2;
        tmp.compose(replaceUpWithoutContextOptional).minimize();
        assert ( tmp.compare( result_right1WithoutContextOptional ) );
        // -> \\ Replace left
        tmp = input1;
        tmp.compose(replaceLeft).minimize();
        assert ( tmp.compare( result_right2 ) );
        // (->) \\ Replace left optional
        tmp = input1;
        tmp.compose(replaceLeftOptional).minimize();
        assert ( tmp.compare( result_right2_optional ) );
        // -> // Replace right
        tmp = input1;
        tmp.compose(replaceRight).minimize();
        assert ( tmp.compare( result_right3 ) );
        // (->) // Replace right optional
        tmp = input1;
        tmp.compose(replaceRightOptional).minimize();
        assert ( tmp.compare( result_right3_optional ) );
        // -> \/ Replace down
        tmp = input1;
        tmp.compose(replaceDown).minimize();
        assert ( tmp.compare( result_right4 ) );
        // (->) \/ Replace down optional
        tmp = input1;
        tmp.compose(replaceDownOptional).minimize();
        assert ( tmp.compare( result_right4_optional ) );

}
/*
 * Both contexts are epsilon transducers
 */
void right_arrow_test2( ImplementationType type )
{
        HfstTokenizer TOK;
        TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

        // Both contexts are epsilon transducers

        // Set context
        HfstTransducer leftEpsilon("@_EPSILON_SYMBOL_@", type);
        HfstTransducer rightEpsilon("@_EPSILON_SYMBOL_@", type);

        HfstTransducerPair contextEpsilon = HfstTransducerPair( leftEpsilon, rightEpsilon );

        // Set mapping
        HfstTransducer mapping("a", "d", TOK, type);

        // Set alphabet
        StringPair a1 = StringPair("a", "a");
        StringPair a2 = StringPair("c", "c");
        StringPair a3 = StringPair("d", "d");
        StringPairSet alphabet;
        alphabet.insert(a1);
        alphabet.insert(a2);
        alphabet.insert(a3);

        // Input stream
        HfstTransducer input1("caadaaa", TOK, type);

        HfstTransducer replaceUp        = replace_up(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceDown      = replace_down_karttunen(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceLeft      = replace_left(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceRight = replace_right(contextEpsilon, mapping, 0, alphabet);

        HfstTransducer replaceUpOptional        = replace_up(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceDownOptional      = replace_down_karttunen(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceLeftOptional      = replace_left(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceRightOptional = replace_right(contextEpsilon, mapping, 1, alphabet);

        // results
        HfstTransducer result1("caadaaa", "cdddddd", TOK, type);
        HfstTransducer result1Optional(type);
        result1Optional.disjunct(result1).minimize().disjunct(input1).minimize();

        // -> ||
        HfstTransducer tmp = input1;
        tmp.compose(replaceUp).minimize();
        assert ( tmp.compare( result1 ) );
        // (->) ||
        tmp = input1;
        tmp.compose(replaceUpOptional).minimize();
        assert ( tmp.compare( result1Optional ) );
        // -> \\  Replace left
        tmp = input1;
        tmp.compose(replaceLeft).minimize();
        assert ( tmp.compare( result1 ) );
        // (->) \\  Replace left optional
        tmp = input1;
        tmp.compose(replaceLeftOptional).minimize();
        assert ( tmp.compare( result1Optional ) );
        // -> //
        tmp = input1;
        tmp.compose(replaceRight).minimize();
        assert ( tmp.compare( result1 ) );
        // (->) //
        tmp = input1;
        tmp.compose(replaceRightOptional).minimize();
        assert ( tmp.compare( result1Optional ) );
        // -> \/
        tmp = input1;
        tmp.compose(replaceDown).minimize();
        assert ( tmp.compare( result1 ) );
        // (->) \/
        tmp = input1;
        tmp.compose(replaceDownOptional).minimize();
        assert ( tmp.compare( result1Optional ) );

}
/*
 *  Left context is epsilon transducer
 */
void right_arrow_test3( ImplementationType type )
{
        HfstTokenizer TOK;
        TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

        // Set context
        HfstTransducer leftEpsilon("@_EPSILON_SYMBOL_@", type);
        HfstTransducer right("d", type);
        HfstTransducerPair contextEpsilon = HfstTransducerPair( leftEpsilon, right );

        // Set mapping
        HfstTransducer mapping("a", "d", TOK, type);

        // Set alphabet
        StringPair a1 = StringPair("a", "a");
        StringPair a2 = StringPair("c", "c");
        StringPair a3 = StringPair("d", "d");
        StringPairSet alphabet;
        alphabet.insert(a1);
        alphabet.insert(a2);
        alphabet.insert(a3);

        // Input stream
        HfstTransducer input1("caadaaa", TOK, type);

        // Create replace transducers
        HfstTransducer replaceUp        = replace_up(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceDown      = replace_down_karttunen(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceLeft      = replace_left(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceRight = replace_right(contextEpsilon, mapping, 0, alphabet);

        HfstTransducer replaceUpOptional        = replace_up(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceDownOptional      = replace_down_karttunen(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceLeftOptional      = replace_left(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceRightOptional = replace_right(contextEpsilon, mapping, 1, alphabet);

        // Results
        HfstTransducer result1("caadaaa", "caddaaa", TOK, type);
        HfstTransducer result1Optional(type);
        result1Optional.disjunct(result1).minimize().disjunct(input1).minimize();

        HfstTransducer result2("caadaaa", "cdddaaa", TOK, type);
        HfstTransducer result2Optional(type);
        result2Optional.disjunct(result2).minimize().disjunct(input1).minimize();

        // -> ||
        HfstTransducer tmp = input1;
        tmp.compose(replaceUp).minimize();
        assert ( tmp.compare( result1 ) );
        // (->) ||
        tmp = input1;
        tmp.compose(replaceUpOptional).minimize();
        assert ( tmp.compare( result1Optional ) );
        // -> \\  Replace left
        tmp = input1;
        tmp.compose(replaceLeft).minimize();
        assert ( tmp.compare( result2 ) );
        // (->) \\  Replace left optional
        tmp = input1;
        tmp.compose(replaceLeftOptional).minimize();
        assert ( tmp.compare( result2Optional ) );
        // -> //
        tmp = input1;
        tmp.compose(replaceRight).minimize();
        assert ( tmp.compare( result1 ) );
        // (->) //
        tmp = input1;
        tmp.compose(replaceRightOptional).minimize();
        assert ( tmp.compare( result1Optional ) );
        // -> \/
        tmp = input1;
        tmp.compose(replaceDown).minimize();
        assert ( tmp.compare( result2 ) );
        // (->) \/
        tmp = input1;
        tmp.compose(replaceDownOptional).minimize();
        assert ( tmp.compare( result2Optional ) );
}

/*
 * Right context is epsilon trasnducer
 */
void right_arrow_test4( ImplementationType type )
{
        HfstTokenizer TOK;
        TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

        // Set context
        HfstTransducer left("d", type);
        HfstTransducer rightEpsilon("@_EPSILON_SYMBOL_@", type);
        HfstTransducerPair contextEpsilon = HfstTransducerPair( left, rightEpsilon );

        // Set mapping
        HfstTransducer mapping("a", "d", TOK, type);

        // Set alphabet
        StringPair a1 = StringPair("a", "a");
        StringPair a2 = StringPair("c", "c");
        StringPair a3 = StringPair("d", "d");
        StringPairSet alphabet;
        alphabet.insert(a1);
        alphabet.insert(a2);
        alphabet.insert(a3);

        // Inpust stream
        HfstTransducer input1("caadaaa", TOK, type);

        // Replace transducers
        HfstTransducer replaceUp        = replace_up(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceDown      = replace_down_karttunen(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceLeft      = replace_left(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceRight = replace_right(contextEpsilon, mapping, 0, alphabet);

        HfstTransducer replaceUpOptional        = replace_up(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceDownOptional      = replace_down_karttunen(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceLeftOptional      = replace_left(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceRightOptional = replace_right(contextEpsilon, mapping, 1, alphabet);

        // Results
        HfstTransducer result1("caadaaa", "caaddaa", TOK, type);
        HfstTransducer result1Optional(type);
        result1Optional.disjunct(result1).minimize().disjunct(input1).minimize();

        HfstTransducer result2("caadaaa", "caadddd", TOK, type);
        HfstTransducer result2Optional(type);
        result2Optional.disjunct(result2).minimize().disjunct(input1).minimize();

        // -> ||
        HfstTransducer tmp = input1;
        tmp.compose(replaceUp).minimize();
        assert ( tmp.compare( result1 ) );
        // (->) ||
        tmp = input1;
        tmp.compose(replaceUpOptional).minimize();
        assert ( tmp.compare( result1Optional ) );
        // -> \\  Replace left
        tmp = input1;
        tmp.compose(replaceLeft).minimize();
        assert ( tmp.compare( result1 ) );
        // (->) \\  Replace left optional
        tmp = input1;
        tmp.compose(replaceLeftOptional).minimize();
        assert ( tmp.compare( result1Optional ) );
        // -> //
        tmp = input1;
        tmp.compose(replaceRight).minimize();
        assert ( tmp.compare( result2 ) );
        // (->) //
        tmp = input1;
        tmp.compose(replaceRightOptional).minimize();
        assert ( tmp.compare( result2Optional ) );
        // -> \/  Replace down
        tmp = input1;
        tmp.compose(replaceDown).minimize();
        assert ( tmp.compare( result2 ) );
        // (->) \/  Replace down optional
        tmp = input1;
        tmp.compose(replaceDownOptional).minimize();
        assert ( tmp.compare( result2Optional ) );

}
/*
 * Left arrow basic tests (including replace_up without context)
 */
void left_arrow_test1( ImplementationType type )
{
        HfstTokenizer TOK;
        TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

        // Set mapping
        HfstTransducer mapping("ca", "d", TOK, type);

        // Set context
        HfstTransducer left("ca", TOK, type);
        HfstTransducer right("c", type);
        HfstTransducerPair context = HfstTransducerPair( left, right );

        // Set alphabet
        StringPair a1 = StringPair("a", "a");
        StringPair a2 = StringPair("c", "c");
        StringPair a3 = StringPair("d", "d");
        StringPairSet alphabet;
        alphabet.insert(a1);
        alphabet.insert(a2);
        alphabet.insert(a3);

        // Input stream
        HfstTransducer input1("cacacac", TOK, type);
        HfstTransducer input2("cac", TOK, type);

        // Results:
        // d <- ca || ca_c  ( input: cacacac )
        HfstTransducer result_left1("cad@_EPSILON_SYMBOL_@d@_EPSILON_SYMBOL_@c", "cacacac", TOK, type);
        // d <- ca Without context ( input: cacacac )
        HfstTransducer result_left1_WithoutContext("d@_EPSILON_SYMBOL_@d@_EPSILON_SYMBOL_@d@_EPSILON_SYMBOL_@c", "cacacac", TOK, type);
        // ca (<-) d || ca_c  ( input: cacacac )
        HfstTransducer result_left1_optional(result_left1);
        result_left1_optional.disjunct(input1);
        // ca (<-) d Without context ( input: cacacac )
        HfstTransducer result_left1_WithoutContext_optional("d@_EPSILON_SYMBOL_@c", "cac", TOK, type);
        result_left1_WithoutContext_optional.disjunct(input2);

        // d <- ca \\ ca_c  ( input: cacacac )
        HfstTransducer result_left2("cacad@_EPSILON_SYMBOL_@c", "cacacac",  TOK, type);
        // ca (<-) d \\ ca_c  ( input: cacacac )
        HfstTransducer result_left2_optional(result_left2);
        result_left2_optional.disjunct(input1);
        // d <- ca // ca_c  ( input: cacacac )
        HfstTransducer result_left3("cad@_EPSILON_SYMBOL_@cac", "cacacac", TOK, type);
        // ca (<-) d // ca_c  ( input: cacacac )
        HfstTransducer result_left3_optional(result_left3);
        result_left3_optional.disjunct(input1);
        // d <- ca \/ ca_c  ( input: cacacac )
        HfstTransducer result_left4(type);
        result_left4.disjunct(result_left2).minimize().disjunct(result_left3).minimize();
        // d (<-) ca \/ ca_c  ( input: cacacac )
        HfstTransducer result_left4_optional(result_left4);
        result_left4_optional.disjunct(input1);

        /* LEFT ARROW RULE
         *
         * To test left arrow rule, it's needed to compose the rule transducer with the input one
         * Left_arrow_rule_tr .o. input_transducer
         */
        HfstTransducer larrow_replaceUp         = left_replace_up(context, mapping, 0, alphabet);
        HfstTransducer larrow_replaceDown       = left_replace_down_karttunen(context, mapping, 0, alphabet);
        HfstTransducer larrow_replaceLeft       = left_replace_left(context, mapping, 0, alphabet);
        HfstTransducer larrow_replaceRight      = left_replace_right(context, mapping, 0, alphabet);

        HfstTransducer larrow_replaceUpWithoutContext   = left_replace_up(mapping, 0, alphabet);

        HfstTransducer larrow_replaceUpOptional         = left_replace_up(context, mapping, 1, alphabet);
        HfstTransducer larrow_replaceDownOptional       = left_replace_down_karttunen(context, mapping, 1, alphabet);
        HfstTransducer larrow_replaceLeftOptional       = left_replace_left(context, mapping, 1, alphabet);
        HfstTransducer larrow_replaceRightOptional      = left_replace_right(context, mapping, 1, alphabet);

        HfstTransducer larrow_replaceUpWithoutContextOptional   = left_replace_up(mapping, 1, alphabet);

        // To test left arrow rule, it's needed to compose the rule transducer with the input one
        // Left_arrow_rule_tr .o. input_transducer

        // <- || Left replace up
        HfstTransducer tmp = larrow_replaceUp;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result_left1 ) );

        // <- Left replace up without context
        tmp = larrow_replaceUpWithoutContext;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result_left1_WithoutContext ) );

        // (<-) || Left replace up optional
        tmp = larrow_replaceUpOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result_left1_optional ) );

        // (<-) Left replace up optional without context
        tmp = larrow_replaceUpWithoutContextOptional;
        tmp.compose(input2).minimize();
        assert ( tmp.compare( result_left1_WithoutContext_optional ) );

        // <- \\ Left replace left
        tmp = larrow_replaceLeft;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result_left2 ) );

        // (<-) \\ Left replace left optional
        tmp = larrow_replaceLeftOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result_left2_optional ) );

        // <- // Left replace right
        tmp = larrow_replaceRight;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result_left3 ) );

        // (<-) // Left replace right optional
        tmp = larrow_replaceRightOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result_left3_optional ) );

        // <-  \/ Left replace down
        tmp = larrow_replaceDown;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result_left4 ) );

        // (<-) \/ Left replace down optional
        tmp = larrow_replaceDownOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result_left4_optional ) );

}
/*
 * Both contexts are epsilon transducers
 */
void left_arrow_test2( ImplementationType type )
{
        HfstTokenizer TOK;
        TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

        // Both contexts are epsilon transducers

        // Set context
        HfstTransducer leftEpsilon("@_EPSILON_SYMBOL_@", type);
        HfstTransducer rightEpsilon("@_EPSILON_SYMBOL_@", type);
        HfstTransducerPair contextEpsilon = HfstTransducerPair( leftEpsilon, rightEpsilon );

        // Set mapping
        HfstTransducer mapping("a", "d", TOK, type);

        // Set alphabet
        StringPair a1 = StringPair("a", "a");
        StringPair a2 = StringPair("c", "c");
        StringPair a3 = StringPair("d", "d");
        StringPairSet alphabet;
        alphabet.insert(a1);
        alphabet.insert(a2);
        alphabet.insert(a3);

        // Input stream
        HfstTransducer input1("caadaaa", TOK, type);

        HfstTransducer replaceUp        = left_replace_up(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceDown      = left_replace_down_karttunen(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceLeft      = left_replace_left(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceRight = left_replace_right(contextEpsilon, mapping, 0, alphabet);

        HfstTransducer replaceUpOptional        = left_replace_up(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceDownOptional      = left_replace_down_karttunen(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceLeftOptional      = left_replace_left(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceRightOptional = left_replace_right(contextEpsilon, mapping, 1, alphabet);

        // results
        HfstTransducer result1("cdddddd", "caadaaa", TOK, type);
        HfstTransducer result1Optional(type);
        result1Optional.disjunct(result1).minimize().disjunct(input1).minimize();

        // <- ||
        HfstTransducer tmp = replaceUp;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1 ) );
        // (<-) ||
        tmp = replaceUpOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1Optional ) );
        // <- \\  Replace left
        tmp = replaceLeft;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1 ) );
        // (<-) \\  Replace left optional
        tmp = replaceLeftOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1Optional ) );
        // <- //
        tmp = replaceRight;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1 ) );
        // (<-) //
        tmp = replaceRightOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1Optional ) );
        // <- \/
        tmp = replaceDown;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1 ) );
        // (<-) \/
        tmp = replaceDownOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1Optional ) );
}
/*
 *  Left context is epsilon transducer
 */
void left_arrow_test3( ImplementationType type )
{
        HfstTokenizer TOK;
        TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

        // Set context
        HfstTransducer leftEpsilon("@_EPSILON_SYMBOL_@", type);
        HfstTransducer right("d", type);
        HfstTransducerPair contextEpsilon = HfstTransducerPair( leftEpsilon, right );

        // Set mapping
        HfstTransducer mapping("a", "d", TOK, type);

        // Set alphabet
        StringPair a1 = StringPair("a", "a");
        StringPair a2 = StringPair("c", "c");
        StringPair a3 = StringPair("d", "d");
        StringPairSet alphabet;
        alphabet.insert(a1);
        alphabet.insert(a2);
        alphabet.insert(a3);

        // Input stream
        HfstTransducer input1("caadaaa", TOK, type);

        // Create replace transducers
        HfstTransducer replaceUp        = left_replace_up(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceDown      = left_replace_down_karttunen(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceLeft      = left_replace_left(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceRight = left_replace_right(contextEpsilon, mapping, 0, alphabet);

        HfstTransducer replaceUpOptional        = left_replace_up(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceDownOptional      = left_replace_down_karttunen(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceLeftOptional      = left_replace_left(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceRightOptional = left_replace_right(contextEpsilon, mapping, 1, alphabet);

        // Results
        HfstTransducer result1("caddaaa", "caadaaa", TOK, type);
        HfstTransducer result1Optional(type);
        result1Optional.disjunct(result1).minimize().disjunct(input1).minimize();

        HfstTransducer result2("cdddaaa", "caadaaa", TOK, type);
        HfstTransducer result2Optional(type);
        result2Optional.disjunct(result2).minimize().disjunct(input1).minimize();

        // <- ||
        HfstTransducer tmp = replaceUp;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1 ) );
        // (<-) ||
        tmp = replaceUpOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1Optional ) );
        // <- \\  Replace left
        tmp = replaceLeft;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result2 ) );
        // (<-) \\  Replace left optional
        tmp = replaceLeftOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result2Optional ) );
        // <- //
        tmp = replaceRight;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1 ) );
        // (<-) //
        tmp = replaceRightOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1Optional ) );
        // <- \/
        tmp = replaceDown;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result2 ) );
        // (<-) \/
        tmp = replaceDownOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result2Optional ) );
}

/*
 * Right context is epsilon transducer
 */
void left_arrow_test4( ImplementationType type )
{
        HfstTokenizer TOK;
        TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

        // Set context
        HfstTransducer left("d", type);
        HfstTransducer rightEpsilon("@_EPSILON_SYMBOL_@", type);
        HfstTransducerPair contextEpsilon = HfstTransducerPair( left, rightEpsilon );

        // Set mapping
        HfstTransducer mapping("a", "d", TOK, type);

        // Set alphabet
        StringPair a1 = StringPair("a", "a");
        StringPair a2 = StringPair("c", "c");
        StringPair a3 = StringPair("d", "d");
        StringPairSet alphabet;
        alphabet.insert(a1);
        alphabet.insert(a2);
        alphabet.insert(a3);

        // Inpust stream
        HfstTransducer input1("caadaaa", TOK, type);

        HfstTransducer replaceUp        = left_replace_up(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceDown      = left_replace_down_karttunen(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceLeft      = left_replace_left(contextEpsilon, mapping, 0, alphabet);
        HfstTransducer replaceRight = left_replace_right(contextEpsilon, mapping, 0, alphabet);

        HfstTransducer replaceUpOptional        = left_replace_up(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceDownOptional      = left_replace_down_karttunen(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceLeftOptional      = left_replace_left(contextEpsilon, mapping, 1, alphabet);
        HfstTransducer replaceRightOptional = left_replace_right(contextEpsilon, mapping, 1, alphabet);

        // Results
        HfstTransducer result1("caaddaa", "caadaaa", TOK, type);
        HfstTransducer result1Optional(type);
        result1Optional.disjunct(result1).minimize().disjunct(input1).minimize();

        HfstTransducer result2("caadddd", "caadaaa", TOK, type);
        HfstTransducer result2Optional(type);
        result2Optional.disjunct(result2).minimize().disjunct(input1).minimize();

        // <- ||
        HfstTransducer tmp = replaceUp;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1 ) );
        // (<-) ||
        tmp = replaceUpOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1Optional ) );
        // <- \\  Replace left
        tmp = replaceLeft;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1 ) );
        // (<-) \\  Replace left optional
        tmp = replaceLeftOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result1Optional ) );
        // <- //
        tmp = replaceRight;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result2 ) );
        // (<-) //
        tmp = replaceRightOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result2Optional ) );
        // <- \/
        tmp = replaceDown;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result2 ) );
        // (<-) \/
        tmp = replaceDownOptional;
        tmp.compose(input1).minimize();
        assert ( tmp.compare( result2Optional ) );
}


int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":" << std::endl;
    

    ImplementationType types[] = {SFST_TYPE, TROPICAL_OPENFST_TYPE, FOMA_TYPE};
    unsigned int NUMBER_OF_TYPES=3;

    for (unsigned int i=0; i < NUMBER_OF_TYPES; i++)
    {
      if (! HfstTransducer::is_implementation_type_available(types[i]))
        continue;

        // Normal
        right_arrow_test1(types[i]);
        left_arrow_test1(types[i]);
        // Both contexts are epsilon transducers
        right_arrow_test2(types[i]);
        left_arrow_test2(types[i]);
        // Left context is epsilon transducer
        right_arrow_test3(types[i]);
        left_arrow_test3(types[i]);
        // Right context is epsilon transducer
        right_arrow_test4(types[i]);
        left_arrow_test4(types[i]);
    }



/*
 *
 */
    std::cout << "ok" << std::endl;
    return 0;
}

#endif // MAIN_TEST
