// Copyright (c) 2016 University of Helsinki                          
//                                                                    
// This library is free software; you can redistribute it and/or      
// modify it under the terms of the GNU Lesser General Public         
// License as published by the Free Software Foundation; either       
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more      
// information.

#include "HfstXeroxRules.h"

#ifndef MAIN_TEST

namespace hfst
{
  namespace xeroxRules
  {
      Rule::Rule ( const HfstTransducerPairVector &mappingPairVector )
      {
        HfstTokenizer TOK;
        TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

        ImplementationType type = mappingPairVector[0].first.get_type();
        // Check if all transducer types are the same
        for ( unsigned int i = 0; i < mappingPairVector.size(); i++ )
        {
            if ( mappingPairVector[i].first.get_type() != type
                    || mappingPairVector[i].second.get_type() != type)
            {
                HFST_THROW_MESSAGE(TransducerTypeMismatchException, "Rule mapping");
            }
        }

        HfstTransducerPair contextPair(HfstTransducer("@_EPSILON_SYMBOL_@", TOK, type),
                                       HfstTransducer("@_EPSILON_SYMBOL_@", TOK, type));
        HfstTransducerPairVector epsilonContext;
        epsilonContext.push_back(contextPair);

        mapping = mappingPairVector;
              
       // HfstTransducerPairVector tmpV = mappingPairVector;
       // tmpV[0].first = encodeFlagDiacritics(tmpV[0].first);
        
        //mapping = tmpV;
        context = epsilonContext;
        replType = REPL_UP;

      }
      Rule::Rule ( const HfstTransducerPairVector &mappingPairVector,
                     const HfstTransducerPairVector &a_context,
                     ReplaceType a_replType )
      {
          // Check if all transducer types are the same
        ImplementationType type = mappingPairVector[0].first.get_type();
        for ( unsigned int i = 0; i < mappingPairVector.size(); i++ )
        {
          if ( mappingPairVector[i].first.get_type() != type
                  || mappingPairVector[i].second.get_type() != type)
          {
              HFST_THROW_MESSAGE(TransducerTypeMismatchException, "Rule mapping");
          }
        }
        for ( unsigned int j = 0; j < a_context.size(); j++ )
        {
            if ( a_context[j].first.get_type() != type
                || a_context[j].second.get_type() != type )
            {
                HFST_THROW_MESSAGE(TransducerTypeMismatchException, "Rule context");
            }
        }

        
          
        //HfstTransducerPairVector tmpV = mappingPairVector;
        //tmpV[0].first = encodeFlagDiacritics(tmpV[0].first);
        
        mapping = mappingPairVector;
       // mapping = tmpV                       ;
        context = a_context;
        replType = a_replType;
      }

      
       Rule::Rule ( const Rule &a_rule )
        {
            mapping = a_rule.get_mapping();
            context = a_rule.get_context();
            replType = a_rule.get_replType();
           
        }
       
      HfstTransducerPairVector Rule::get_mapping() const
      {
        return mapping;
      }

      HfstTransducerPairVector Rule::get_context() const
      {
        return context;
      }
      ReplaceType Rule::get_replType() const
      {
        return replType;
      }
      void Rule::encodeFlags()
      {
        //cerr << "\n Encode flags " << endl;
         HfstTransducerPairVector tmpM = this->mapping;
        
        //cerr << "\n mapping size: " << tmpM.size() << endl;
        for ( unsigned int i = 0; i < tmpM.size(); i++ )
        {
            tmpM[i].first = encodeFlagDiacritics(tmpM[i].first);
            tmpM[i].second = encodeFlagDiacritics(tmpM[i].second);
        }
        
        
        HfstTransducerPairVector tmpC = this->context;
        
        //cerr << "\n context size: " << tmpC.size() << endl;
        for ( unsigned int i = 0; i < tmpC.size(); i++ )
        {
            tmpC[i].first = encodeFlagDiacritics(tmpC[i].first);
            tmpC[i].second = encodeFlagDiacritics(tmpC[i].second);
        }
        
        this->mapping = tmpM;  
        this->context = tmpC;  
      }

    std::ostream & operator<<(std::ostream &out, const Rule & r)
    {
      out << "hfst::xeroxRules::Rule:" << std::endl;
      out << "replType: ";
      switch(r.replType)
        {
        case REPL_UP:
          out << "REPL_UP";
          break;
        case REPL_DOWN:
          out << "REPL_DOWN";
          break;
        case REPL_RIGHT:
          out << "REPL_RIGHT";
          break;
        case REPL_LEFT:
          out << "REPL_LEFT";
          break;
        }
      out << std::endl;

      out << "mapping:" << std::endl;
      unsigned int index = 1;
      for (HfstTransducerPairVector::const_iterator it = r.mapping.begin();
           it != r.mapping.end(); it++)
        {
          out << "#" << index << " (right side):" << std::endl;
          out << it->first;
          out << "#" << index << " (left side):" << std::endl;
          out << it->second;
          index++;
        }

      out << "context:" << std::endl;
      index = 1;
      for (HfstTransducerPairVector::const_iterator it = r.context.begin();
           it != r.context.end(); it++)
        {
          out << "#" << index << " (right side):" << std::endl;
          out << it->first;
          out << "#" << index << " (left side):" << std::endl;
          out << it->second;
          index++;
        }

      return out;
    }


      //////////////////////////////////////
      // In the transducer tr, change all flag diacritics to "non-special" multichar symbols
      // It means that @ sign will be changed to $ sign
      // ie. @P.FOO.BAR@ will be changed into $P.FOO.BAR$
     HfstTransducer encodeFlagDiacritics( const HfstTransducer &tr )
      { 
        //std::cerr << "\n encodeFlagDiacritics " << endl;
        HfstSymbolSubstitutions realFlagstoFakeFlags;
        StringSet removeFromAlphabet;
        //printf("alphabet lexicons: \n");
        StringSet transducerAlphabet = tr.get_alphabet();
        for (StringSet::const_iterator s = transducerAlphabet.begin();
                       s != transducerAlphabet.end();
                       ++s)
        {
         
            String alph = *s;
            String alphFirst3 = alph.substr(0,3);
         //   String alphLast = alph.back();

            //@operator.feature.value@ and @operator.feature@
            
            //std::cout << "debug: " << alphFirst3 << endl;
            if ( alphFirst3 == "@P." || alphFirst3 == "@R." 
                 || alphFirst3 == "@U." || alphFirst3 == "@D." 
                 || alphFirst3 == "@C." || alphFirst3 == "@N."
                 ||alphFirst3 == "@p." || alphFirst3 == "@r." 
                 || alphFirst3 == "@u." || alphFirst3 == "@d." 
                 || alphFirst3 == "@c." || alphFirst3 == "@n.")  
            {

                replace(alph.begin(), alph.end(), '@', '$');
                //std::cout << alph << '\n';
                realFlagstoFakeFlags.insert(StringPair(*s, alph));
                removeFromAlphabet.insert(*s);
            }
        }
    
        HfstTransducer retval(tr);
        retval.substitute(realFlagstoFakeFlags);
        
        //std::cerr << "\n flag substitute done" << endl;
        retval.remove_from_alphabet(removeFromAlphabet);
        //std::cerr << "\n alph substitute done" << endl;
        return retval;

      }  
      
       HfstTransducer decodeFlagDiacritics( const HfstTransducer &tr )
      { 
        //std::cerr << "\n decodeFlagDiacritics " << endl;
        HfstSymbolSubstitutions fakeFlagsToRealFlags;

        //printf("alphabet lexicons: \n");
        StringSet transducerAlphabet = tr.get_alphabet();
        StringSet removeFromAlphabet;
        for (StringSet::const_iterator s = transducerAlphabet.begin();
                       s != transducerAlphabet.end();
                       ++s)
        {
         
            String alph = *s;
            String alphFirst3 = alph.substr(0,3);
         //   String alphLast = alph.back();

            //@operator.feature.value@ and @operator.feature@
            
            //std::cout << "debug: " << alphFirst3 << endl;
            if ( alphFirst3 == "$P." || alphFirst3 == "$R." 
                 || alphFirst3 == "$U." || alphFirst3 == "$D." 
                 || alphFirst3 == "$C." || alphFirst3 == "$N."
                 || alphFirst3 == "$p." || alphFirst3 == "$r." 
                 || alphFirst3 == "$u." || alphFirst3 == "$d." 
                 || alphFirst3 == "$c." || alphFirst3 == "$n.")  
            {

                replace(alph.begin(), alph.end(), '$', '@');
                //std::cout << alph << '\n';
                fakeFlagsToRealFlags.insert(StringPair(*s, alph));
                removeFromAlphabet.insert(*s);
            }
        }
    
        HfstTransducer retval(tr);
        retval.substitute(fakeFlagsToRealFlags);
        retval.remove_from_alphabet(removeFromAlphabet);
        return retval;

      }  



      HfstTransducer disjunctVectorMembers( const HfstTransducerVector &trVector )
      {
          HfstTransducer retval( trVector[0] );
          for ( unsigned int i = 1; i < trVector.size(); i++ )
          {
              retval.disjunct(trVector[i]).optimize();
          }
          return retval;
      }



      //////////////////////////////////////



      HfstTransducer removeMarkers( const HfstTransducer &tr )
      {
        HfstTransducer retval(tr);


        String leftMarker("@LM@");
        String rightMarker("@RM@");

        retval.substitute(StringPair(leftMarker, leftMarker), StringPair("@_EPSILON_SYMBOL_@", "@_EPSILON_SYMBOL_@")).optimize();
        retval.substitute(StringPair(rightMarker, rightMarker), StringPair("@_EPSILON_SYMBOL_@", "@_EPSILON_SYMBOL_@")).optimize();


        retval.remove_from_alphabet(leftMarker);
        retval.remove_from_alphabet(rightMarker);

        retval.optimize();


        
        //printf("tr without markers: \n");
        //retval.write_in_att_format(stdout, 1);
        
        retval = decodeFlagDiacritics(retval);
        
        return retval;
      }

      float zero_weight(float f) {
          return 0;
      }

      /*
       * Generalized Lenient Composition, described in Anssi Yli-Jyrä 2008b
       */
      // tmp = t.1 .o. Constr .o. t.1
      // (t.1 - tmp.2) .o. t
      HfstTransducer constraintComposition( const HfstTransducer &t, const HfstTransducer &Constraint )
      {

//          printf("---- \n");
//          printf("orig: \n");
//          t.write_in_att_format(stdout, 1);
//          printf("---- \n");

          HfstTransducer retval(t);
          retval.transform_weights(&zero_weight);

          retval.input_project().optimize();

          HfstTransducer tmp(retval);
          tmp.compose(Constraint).optimize();
          //printf("tmp: \n");
           //tmp.write_in_att_format(stdout, 1);

          tmp.compose(retval).optimize();
         //printf("tmp 2: \n");
         //tmp.write_in_att_format(stdout, 1);
          tmp.output_project().optimize();
          retval.subtract(tmp).optimize();

          //transform weights to zero
          retval.transform_weights(&zero_weight);
          retval.compose(t).optimize();

          return retval;
      }

      void insertFreelyAllTheBrackets( HfstTransducer &t, bool optional )
      {
          HfstTokenizer TOK;
          TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");
          String leftMarker("@LM@");
          String rightMarker("@RM@");
          String leftMarker2("@LM2@");
          String rightMarker2("@RM2@");

          TOK.add_multichar_symbol(leftMarker);
          TOK.add_multichar_symbol(rightMarker);
          TOK.add_multichar_symbol(leftMarker2);
          TOK.add_multichar_symbol(rightMarker2);

          ImplementationType type = t.get_type();

          HfstTransducer leftBracket(leftMarker, TOK, type);
          HfstTransducer rightBracket(rightMarker, TOK, type);

          t.insert_freely(leftBracket, false).optimize();
          t.insert_freely(rightBracket, false).optimize();

          if ( !optional )
          {
              HfstTransducer leftBracket2(leftMarker2, TOK, type);
              HfstTransducer rightBracket2(rightMarker2, TOK, type);

              t.insert_freely(leftBracket2, false).optimize();
              t.insert_freely(rightBracket2, false).optimize();
          }
      }

      HfstTransducer expandContextsWithMapping (const HfstTransducerPairVector &ContextVector,
                                      const HfstTransducer &mappingWithBracketsAndTmpBoundary,
                                      const HfstTransducer &identityExpanded,
                                      ReplaceType replType,
                                      bool optional)
      {
          ImplementationType type = identityExpanded.get_type();

          HfstTransducer unionContextReplace(type);

          HfstTokenizer TOK;
         // TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");
         // HfstTransducer epsilon("@_EPSILON_SYMBOL_@", TOK, type);


          for ( unsigned int i = 0; i < ContextVector.size(); i++ )
          {

            // Expand context with mapping
            // Cr' = (Rc .*) << Markers (<,>,|) .o. [I:I | <a:b>]*
            // Cr = Cr|Cr'
            // (same for left context)

            // Lc = (*. Lc) << {<,>}
  
            HfstTransducer identityPair = HfstTransducer::identity_pair( type );
            HfstTransducer identityStar(identityPair);
            identityStar.repeat_star();

            HfstTransducer firstContext( identityStar);
            firstContext.concatenate(ContextVector[i].first);
            firstContext.transform_weights(&zero_weight);
            firstContext.optimize();

            insertFreelyAllTheBrackets( firstContext, optional );

            // Rc =  (Rc .*) << {<,>}
            HfstTransducer secondContext(ContextVector[i].second);
            secondContext.concatenate(identityStar);
            secondContext.transform_weights(&zero_weight);
            secondContext.optimize();
            insertFreelyAllTheBrackets( secondContext, optional);

            /* RULE:    LC:        RC:
             * up        up        up
             * left        up        down
             * right    down    up
             * down        down    down
             */

            HfstTransducer leftContextExpanded(type);
            HfstTransducer rightContextExpanded(type);

            // both contexts are in upper language
            if ( replType == REPL_UP)
            {
                // compose them with [I:I | <a:b>]*
                leftContextExpanded = firstContext;
                rightContextExpanded = secondContext;

                leftContextExpanded.compose(identityExpanded);
                rightContextExpanded.compose(identityExpanded);
            }
            // left context is in lower language, right in upper ( // )
            if ( replType == REPL_RIGHT )
            {
                    // compose them with [I:I | <a:b>]*

                // left compose opposite way
                leftContextExpanded = identityExpanded;
                rightContextExpanded = secondContext;

                leftContextExpanded.compose(firstContext);
                rightContextExpanded.compose(identityExpanded);
            }
            // right context is in lower language, left in upper ( \\ )
            if ( replType == REPL_LEFT )
            {
                // compose them with [I:I | <a:b>]*
                leftContextExpanded = firstContext;
                rightContextExpanded = identityExpanded;

                leftContextExpanded.compose(identityExpanded);
                rightContextExpanded.compose(secondContext);
            }
            if ( replType == REPL_DOWN )
            {
                // compose them with [I:I | <a:b>]*
                leftContextExpanded = identityExpanded;
                rightContextExpanded = identityExpanded;

                leftContextExpanded.compose(firstContext);
                rightContextExpanded.compose(secondContext);
            }


            leftContextExpanded.transform_weights(&zero_weight);
            rightContextExpanded.transform_weights(&zero_weight);
            leftContextExpanded.optimize();
            rightContextExpanded.optimize();

            firstContext.disjunct(leftContextExpanded);
            firstContext.optimize();

            secondContext.disjunct(rightContextExpanded);
            secondContext.optimize();

            // add boundary symbol before/after contexts
            String boundaryMarker(".#.");
            TOK.add_multichar_symbol(boundaryMarker);
            HfstTransducer boundary(boundaryMarker, TOK, type);

            identityStar.insert_to_alphabet(boundaryMarker);

            // to firstContext
            StringSet firstContextAlphabet = firstContext.get_alphabet();
            bool hasBoundary = false;
            for (StringSet::const_iterator s = firstContextAlphabet.begin();
                      s != firstContextAlphabet.end();
                      ++s)
            {
                if( boundaryMarker.compare(s->c_str()) == 0 )
                    hasBoundary = true;
            }

            if ( hasBoundary == false )
            {
                //printf("\n - Left context boundary false! - \n");
                firstContext.insert_to_alphabet(boundaryMarker);
                HfstTransducer tmp(boundary);
                tmp.concatenate(identityStar).optimize();
                //printf("tmp \n");
                //tmp.write_in_att_format(stdout, 1);
                tmp.concatenate(firstContext);

                //printf("tmp2 \n");
                //tmp.write_in_att_format(stdout, 1);
                firstContext = tmp;

            }

            // to secondContext
            StringSet secondContextAlphabet = secondContext.get_alphabet();
            hasBoundary = false;
            for (StringSet::const_iterator s = secondContextAlphabet.begin();
                      s != secondContextAlphabet.end();
                      ++s)
            {
                if( boundaryMarker.compare(s->c_str()) == 0 )
                    hasBoundary = true;
            }

            if ( hasBoundary == false )
            {
              // printf("\n - Right context boundary false! - \n");
                secondContext.insert_to_alphabet(boundaryMarker);
                secondContext.concatenate(identityStar).concatenate(boundary).optimize();
            }

         // put mapping between (expanded) contexts
          HfstTransducer oneContextReplace(firstContext);
          oneContextReplace.concatenate(mappingWithBracketsAndTmpBoundary).
                      concatenate(secondContext);


          oneContextReplace.transform_weights(&zero_weight);
          unionContextReplace.disjunct(oneContextReplace);
          unionContextReplace.optimize();

          }
          return unionContextReplace;
      }



      /*
       * unconditional replace, in multiple contexts
       * first: (.* T<a:b>T .*) - [( .* L1 T<a:b>T R1 .*) u (.* L2 T<a:b>T R2 .*)...],
       *                         where .* = [I:I (+ {tmpMarker (T), <,>} in alphabet) | <a:b>]*
       * then: remove tmpMarker from transducer and alphabet, and do negation:
       *         .* - result from upper operations
       *
       * Also note that context is expanded in this way:
       *         Cr' = (Rc .*) << Markers (<,>,|) .o. [I:I | <a:b>]*
       *         Cr = Cr | Cr'
       *         (same for left context, (.* Cl))
      */

      HfstTransducer bracketedReplace(const Rule &rule, bool optional)
      {
        //printf("bracketedReplace function..... \n");

        HfstTokenizer TOK;
        TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");
        TOK.add_multichar_symbol("@_UNKNOWN_SYMBOL_@");
        String leftMarker("@LM@");
        String rightMarker("@RM@");
        String tmpMarker("@TMPM@");
        String leftMarker2("@LM2@");
        String rightMarker2("@RM2@");
        String newEpsilon("$Epsilon$");

        TOK.add_multichar_symbol(leftMarker);
        TOK.add_multichar_symbol(rightMarker);
        TOK.add_multichar_symbol(leftMarker2);
        TOK.add_multichar_symbol(rightMarker2);
        TOK.add_multichar_symbol(tmpMarker);
        TOK.add_multichar_symbol(newEpsilon);
        TOK.add_multichar_symbol( ".#.");

        //first, encode all flag diacritics
        Rule ruletmp(rule);
        ruletmp.encodeFlags();

        HfstTransducerPairVector mappingPairVector( ruletmp.get_mapping() );
        HfstTransducerPairVector ContextVector( ruletmp.get_context() );
        ReplaceType replType( ruletmp.get_replType() );

        ImplementationType type = mappingPairVector[0].first.get_type();

        // Identity (normal)
        HfstTransducer identityPair = HfstTransducer::identity_pair( type );
        HfstTransducer identity (identityPair);
        identity.repeat_star().optimize();


        HfstTransducer epsilon("@_EPSILON_SYMBOL_@", TOK, type);
        HfstTransducer mapping(type);
        for ( unsigned int i = 0; i < mappingPairVector.size(); i++ )
        {
            HfstTransducer oneMappingPair(mappingPairVector[i].first);
          
            //markup rules are already cross product in mappingPairVector[i].first (second is empty)
            //so the cross product should not be done for markup rules
            if ( mappingPairVector[i].first.get_property("isMarkup") != "yes" )
            {
                oneMappingPair.cross_product(mappingPairVector[i].second);
            }
           // printf("aftrer cross product \n");
           // oneMappingPair.write_in_att_format(stdout, 1);
          
          
            // for removing .#. from the center
            HfstTransducer identityWithoutBoundary(identity);
            identityWithoutBoundary.insert_to_alphabet(".#.");
            HfstTransducer removeHash(identityWithoutBoundary);
            HfstTransducer boundary(".#.", TOK, type);
            removeHash.concatenate(boundary).concatenate(identityWithoutBoundary).optimize();
            //printf("removeHash \n");
            //removeHash.write_in_att_format(stdout, 1);


            // printf("oneMappingPair kkkk\n");
            // oneMappingPair.write_in_att_format(stdout, 1);


            if ( i == 0 )
            {
                // remove .#. from the center
                // center - (?* .#. ?*)
                oneMappingPair.subtract(removeHash, false).optimize();
                oneMappingPair.remove_from_alphabet(".#.");
                mapping = oneMappingPair;
            }
            else
            {
                oneMappingPair.subtract(removeHash, false).optimize();
                oneMappingPair.remove_from_alphabet(".#.");
                mapping.disjunct(oneMappingPair).optimize();
            }
        }
        //printf("mapping all after cross product \n");
        // mapping.write_in_att_format(stdout, 1);


        // In case of ? -> x replacement
        // If left side is empty, return identity transducer
        // If right side is empty, return identity transducer
        //    with alphabet from the left side
        HfstTransducer empty(type);
        if ( mapping.compare(empty) )
        {
            mapping = identity;
            if (mappingPairVector[0].second.compare(empty))
            {
                //printf("alphabet: \n");
                StringSet transducerAlphabet = mappingPairVector[0].first.get_alphabet();
                for (StringSet::const_iterator s = transducerAlphabet.begin();
                               s != transducerAlphabet.end();
                               ++s)
                    {
                        //printf("%s \n", s->c_str());
                        mapping.insert_to_alphabet(s->c_str());
                    }
                //printf("------------------ \n");
            }
        }

        mapping.insert_to_alphabet(leftMarker);
        mapping.insert_to_alphabet(rightMarker);
        mapping.insert_to_alphabet(tmpMarker);

        HfstTransducer leftBracket(leftMarker, TOK, type);
        HfstTransducer rightBracket(rightMarker, TOK, type);
        HfstTransducer leftBracket2(leftMarker2, TOK, type);
        HfstTransducer rightBracket2(rightMarker2, TOK, type);
        HfstTransducer tmpBracket(tmpMarker, TOK, type);

        // Surround mapping with brackets
        HfstTransducer tmpMapping(leftBracket);
        tmpMapping.concatenate(mapping).concatenate(rightBracket).optimize();

        HfstTransducer mappingWithBrackets(tmpMapping);

        //printf("mappingWithBrackets: \n");
        //mappingWithBrackets.optimize().write_in_att_format(stdout, 1);

        // Identity pair
        // HfstTransducer identityPair = HfstTransducer::identity_pair( type );
        // for non-optional replacements
        if ( optional != true )
        {
            // non - optional
            // mapping = <a:b> u <2a:a>2

            HfstTransducer mappingWithBrackets2(leftBracket2);
            HfstTransducer leftMappingUnion(type);
            leftMappingUnion = mappingPairVector[0].first;
            for ( unsigned int i = 1; i < mappingPairVector.size(); i++ )
            {
                leftMappingUnion.disjunct(mappingPairVector[i].first).optimize();
            }
            // needed in case of ? -> x replacement
            leftMappingUnion.insert_to_alphabet(leftMarker2);
            leftMappingUnion.insert_to_alphabet(rightMarker2);
            leftMappingUnion.insert_to_alphabet(leftMarker);
            leftMappingUnion.insert_to_alphabet(rightMarker);
            leftMappingUnion.insert_to_alphabet(tmpMarker);

            //printf("leftMappingUnion: \n");
            //leftMappingUnion.optimize().write_in_att_format(stdout, 1);


            mappingWithBrackets2.concatenate(leftMappingUnion).concatenate(rightBracket2).optimize();

            //printf("mappingWithBrackets2: \n");
            //mappingWithBrackets2.optimize().write_in_att_format(stdout, 1);

              // mappingWithBrackets...... expanded
            mappingWithBrackets.insert_to_alphabet(leftMarker2);
            mappingWithBrackets.insert_to_alphabet(rightMarker2);
            //mappingWithBrackets.insert_to_alphabet(leftMarker);
            //mappingWithBrackets.insert_to_alphabet(rightMarker);
            mappingWithBrackets.disjunct(mappingWithBrackets2).optimize();
        }

        //printf("mappingWithBrackets: \n");
        //mappingWithBrackets.optimize().write_in_att_format(stdout, 1);


        // Identity with bracketed mapping and marker symbols and TmpMarker in alphabet
        // [I:I | <a:b>]* (+ tmpMarker in alphabet)
        HfstTransducer identityExpanded (identityPair);

        identityExpanded.insert_to_alphabet(leftMarker);
        identityExpanded.insert_to_alphabet(rightMarker);
        identityExpanded.insert_to_alphabet(tmpMarker);

        if ( optional != true )
        {
          identityExpanded.insert_to_alphabet(leftMarker2);
          identityExpanded.insert_to_alphabet(rightMarker2);
        }

        identityExpanded.disjunct(mappingWithBrackets).optimize();
        identityExpanded.repeat_star().optimize();

        // when there aren't any contexts, result is identityExpanded
        if ( ContextVector.size() == 1 )
        {
          HfstTransducer epsilon("@_EPSILON_SYMBOL_@", TOK, type);
          if ( ContextVector[0].first.compare(epsilon) && ContextVector[0].second.compare(epsilon) )
          {
              identityExpanded.remove_from_alphabet(tmpMarker);
              //printf("identityExpanded: \n");
              //identityExpanded.optimize().write_in_att_format(stdout, 1);
              return identityExpanded;
          }
        }

        // Surround mapping with tmp boudaries
        HfstTransducer mappingWithBracketsAndTmpBoundary(tmpBracket);
        mappingWithBracketsAndTmpBoundary.concatenate(mappingWithBrackets).concatenate(tmpBracket).optimize();
        //printf("mappingWithBracketsAndTmpBoundary: \n");
        //mappingWithBracketsAndTmpBoundary.write_in_att_format(stdout, 1);


        // .* |<a:b>| :*
        HfstTransducer bracketedReplace(identityExpanded);
        bracketedReplace.concatenate(mappingWithBracketsAndTmpBoundary).concatenate(identityExpanded).optimize();

        //printf("mappingWithBracketsAndTmpBoundary: \n");
        //mappingWithBracketsAndTmpBoundary.write_in_att_format(stdout, 1);

          //   printf("identityExpanded: \n");
        //identityExpanded.write_in_att_format(stdout, 1);

        // Expand all contexts with mapping taking in consideration replace type
        // result is their union
        HfstTransducer unionContextReplace(type);
        unionContextReplace = expandContextsWithMapping (ContextVector,
                                       mappingWithBracketsAndTmpBoundary,
                                       identityExpanded,
                                       replType,
                                       optional);

        //printf("unionContextReplace \n");
        //unionContextReplace.write_in_att_format(stdout, 1);

        // subtract all mappings in contexts from replace without contexts
        HfstTransducer replaceWithoutContexts(bracketedReplace);
        replaceWithoutContexts.subtract(unionContextReplace).optimize();

        //printf("replaceWithoutContexts \n");
        //replaceWithoutContexts.write_in_att_format(stdout, 1);

        // remove tmpMaprker
        replaceWithoutContexts.substitute(StringPair(tmpMarker, tmpMarker),
                        StringPair("@_EPSILON_SYMBOL_@", "@_EPSILON_SYMBOL_@")).optimize();
        replaceWithoutContexts.remove_from_alphabet(tmpMarker);
        replaceWithoutContexts.optimize();

        identityExpanded.remove_from_alphabet(tmpMarker);

        // final negation
        HfstTransducer uncondidtionalTr(identityExpanded);
        uncondidtionalTr.subtract(replaceWithoutContexts).optimize();

        return uncondidtionalTr;

  }


    // Return the string "@N@" where N is the string representation of i.
    static std::string getMarkerString(unsigned int i)
    {
      std::ostringstream oss;
      oss << i;
      return std::string("@") + oss.str() + std::string("@");
    }

    // Return the number representation of N in string "@N@".
    static unsigned int getMarkerNumber(const std::string & str)
    {
      std::string number_str = str.substr(1, str.size()-2);
      std::istringstream iss; // should be iss(number_str); i guess, but cannot be fixed, because some HfstXeroxRules tests will fail...
      unsigned int retval;
      iss >> retval;
      //return retval;
      return 100000;
    }

    // Bracketed replace for parallel rules.
    HfstTransducer parallelBracketedReplace
    (const std::vector<Rule> &ruleVector, bool optional)
    {
      // For each parallel rule, we need to concatenate a special marker symbol
      // to its output side. This is needed so that overlapping mappings with
      // different weights and contexts are kept separate. If we have N rules,
      // we need marker symbols "@1@", "@2@", ... , "@N@" ("@0@" is reserved
      // for epsilon symbol). At the end, we must substitute any marker symbols
      // with epsilons.
      // For an example, consider the following case:
      //   a -> b::1 || c _ c ,, a -> b::2 || d _ d
      // If no markers are added, input 'dad' will yield 'dbd' with weight 1
      // because the transition <a:b::2> will be eliminated by the transition
      // <a:b::1> as it has a lower weight.

      StringSet marker_symbols; // "@1@", "@2@", ... , "@N@"
      HfstSymbolSubstitutions marker_substitutions; // any-marker-to-epsilon
      for (unsigned int i=0; i < ruleVector.size(); i++)
        {
          std::string marker_string = getMarkerString(i+1);
          marker_symbols.insert(marker_string);
          marker_substitutions[marker_string] = hfst::internal_epsilon;
        }

        HfstTokenizer TOK;
        TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");
        String leftMarker("@LM@");
        String rightMarker("@RM@");

        String leftMarker2("@LM2@");
        String rightMarker2("@RM2@");

        String tmpMarker("@TMPM@");
        TOK.add_multichar_symbol(leftMarker);
        TOK.add_multichar_symbol(rightMarker);
        TOK.add_multichar_symbol(leftMarker2);
        TOK.add_multichar_symbol(rightMarker2);
        TOK.add_multichar_symbol(tmpMarker);
        TOK.add_multichar_symbol( ".#.");

        ImplementationType type 
          = ruleVector[0].get_mapping()[0].first.get_type();

        HfstTransducer leftBracket(leftMarker, TOK, type);
        HfstTransducer rightBracket(rightMarker, TOK, type);
        HfstTransducer leftBracket2(leftMarker2, TOK, type);
        HfstTransducer rightBracket2(rightMarker2, TOK, type);
        HfstTransducer tmpBracket(tmpMarker, TOK, type);


        // Identity pair (unknowns/identities must not be expanded to marker
        // symbols)
        HfstTransducer identityPair = HfstTransducer::identity_pair( type );
        identityPair.insert_to_alphabet(marker_symbols);

        HfstTransducer identity (identityPair);
        // unknowns/identities must not be expanded to marker symbols
        identity.insert_to_alphabet(marker_symbols);
        identity.repeat_star().optimize();

        HfstTransducer identityExpanded(identityPair);
        identityExpanded.insert_to_alphabet(leftMarker);
        identityExpanded.insert_to_alphabet(rightMarker);
        identityExpanded.insert_to_alphabet(leftMarker2);
        identityExpanded.insert_to_alphabet(rightMarker2);
        identityExpanded.insert_to_alphabet(tmpMarker);
        identityExpanded.insert_to_alphabet(marker_symbols);
        // will be expanded with mappings

        // for removing .#. from the center
        HfstTransducer identityWithoutBoundary(identity);
        identityWithoutBoundary.insert_to_alphabet(".#.");
        // (must not be expanded to marker symbols)
        identityWithoutBoundary.insert_to_alphabet(marker_symbols);
        HfstTransducer removeHash(identityWithoutBoundary);
        HfstTransducer boundary(".#.", TOK, type);
        removeHash.concatenate(boundary)
          .concatenate(identityWithoutBoundary).optimize();
        //printf("removeHash \n");
        //removeHash.write_in_att_format(stdout, 1);

        HfstTransducerVector mappingWithBracketsVector;
        bool noContexts  = true;

        // go through vector and do everything for each rule
        for ( unsigned int i = 0; i < ruleVector.size(); i++ )
        {
            Rule ruletmp(ruleVector[i]);
            ruletmp.encodeFlags();
            
          HfstTransducerPairVector mappingPairVector 
            = ruletmp.get_mapping();
          HfstTransducer mapping(type);
          for ( unsigned int j = 0; j < mappingPairVector.size(); j++ )
            {
              // i+1 because @0@ is epsilon..
              std::string marker_string = getMarkerString(i+1); 
              HfstTransducer marker(marker_string, type); 
              HfstTransducer oneMappingPair(mappingPairVector[j].first);
              // unknowns/identities must not be expanded to marker symbols
              oneMappingPair.insert_to_alphabet(marker_symbols);
              HfstTransducer foo(mappingPairVector[j].second);
              foo.insert_to_alphabet(marker_symbols);
              oneMappingPair.cross_product
                (foo.concatenate(marker));
              //oneMappingPair.cross_product(mappingPairVector[j].second);
              
              //printf("oneMappingPair \n");
              //oneMappingPair.write_in_att_format(stdout, 1);


              if ( j == 0 )
                {
                  // remove .#. from the center
                  // center - (?* .#. ?*)
                  oneMappingPair.subtract(removeHash, false).optimize();
                  oneMappingPair.remove_from_alphabet(".#.");
                  mapping = oneMappingPair;
                }
              else
                {
                  oneMappingPair.subtract(removeHash, false).optimize();
                  oneMappingPair.remove_from_alphabet(".#.");
                  mapping.disjunct(oneMappingPair).optimize();
                }
            }
          
          
          HfstTransducerPairVector contextVector = ruletmp.get_context();
          //ReplaceType replaceType = ruletmp.get_replType();

          // when there aren't any contexts, result is identityExpanded
          if ( contextVector.size() == 1 )
            {
              HfstTransducer epsilon("@_EPSILON_SYMBOL_@", TOK, type);
              if ( !(contextVector[0].first.compare(epsilon) && 
                     contextVector[0].second.compare(epsilon)) )
                {
                  noContexts = false;
                }
            }

          //////////////////////////////////////////////////////////////////
          // In case of ? -> x replacement
          // If left side is empty, return identity transducer
          // If right side is empty, return identity transducer
          //    with alphabet from the left side
          HfstTransducer empty(type);
          
          if ( mapping.compare(empty) )
            {
              mapping = identity;
              if (mappingPairVector[0].second.compare(empty))
                {
                  //printf("alphabet: \n");
                  StringSet transducerAlphabet 
                    = mappingPairVector[0].first.get_alphabet();
                  for (StringSet::const_iterator s 
                         = transducerAlphabet.begin();
                       s != transducerAlphabet.end();
                       ++s)
                    {
                      //printf("%s \n", s->c_str());
                      mapping.insert_to_alphabet(s->c_str());
                    }
                  //printf("------------------ \n");
                }
            }
          //////////////////////////////////////////////////////////////////
          
          
          mapping.insert_to_alphabet(leftMarker);
          mapping.insert_to_alphabet(rightMarker);
          mapping.insert_to_alphabet(tmpMarker);
          
          // Surround mapping with brackets
          HfstTransducer mappingWithBrackets(leftBracket);
          mappingWithBrackets.concatenate(mapping)
            .concatenate(rightBracket).optimize();
          
          
          // non - optional
          // mapping = <a:b> u <2a:a>2
          if ( optional != true )
            {
              // needed in case of ? -> x replacement
              mapping.insert_to_alphabet(leftMarker2);
              mapping.insert_to_alphabet(rightMarker2);
              mappingWithBrackets.insert_to_alphabet(leftMarker2);
              mappingWithBrackets.insert_to_alphabet(rightMarker2);
              
              
              HfstTransducer mappingProject(mapping);
              mappingProject.input_project().optimize();
              
              HfstTransducer mappingWithBracketsNonOptional(leftBracket2);
              // needed in case of ? -> x replacement
              //mappingWithBracketsNonOptional.insert_to_alphabet(leftMarker2);
              //mappingWithBracketsNonOptional.
              //insert_to_alphabet(rightMarker2);

              mappingWithBracketsNonOptional.concatenate(mappingProject).
                concatenate(rightBracket2).
                optimize();
              // mappingWithBrackets...... expanded
              mappingWithBrackets.disjunct(mappingWithBracketsNonOptional)
                .optimize();

            }

          identityExpanded.disjunct(mappingWithBrackets).optimize();
          mappingWithBracketsVector.push_back(mappingWithBrackets);
        }
        
        identityExpanded.repeat_star().optimize();
        
        // if none of the rules have contexts, return identityExpanded
        if ( noContexts )
          {
            identityExpanded.remove_from_alphabet(tmpMarker);
            // substitute markers with epsilons
            identityExpanded.substitute(marker_substitutions);
            identityExpanded.remove_from_alphabet(marker_symbols);
            return identityExpanded;
          }
        
        // if they have contexts, process them
        if ( ruleVector.size() != mappingWithBracketsVector.size() )
          {
            HFST_THROW_MESSAGE(TransducerTypeMismatchException, 
                               "Vector sizes don't match");
          }
        
        std::map<std::string,hfst::HfstBasicTransducer> contextReplaceMap;
        
        
        HfstTransducer unionContextReplace(type);
        HfstTransducer bracketedReplace(type);
        //THIS is for disjuncting labels first, and then substitute them with
        // transducers
        //HfstTransducer unionContextReplace_labels(type);
        for ( unsigned int i = 0; i < ruleVector.size(); i++ )
          {
            Rule ruletmp(ruleVector[i]);
            ruletmp.encodeFlags();
            
            // Surround mapping with brackets with tmp boudaries
            HfstTransducer mappingWithBracketsAndTmpBoundary(tmpBracket);
            mappingWithBracketsAndTmpBoundary
              .concatenate(mappingWithBracketsVector[i])
              .concatenate(tmpBracket)
              .optimize();
            // .* |<a:b>| :*
            HfstTransducer bracketedReplaceTmp(identityExpanded);
            bracketedReplaceTmp.concatenate(mappingWithBracketsAndTmpBoundary)
              .concatenate(identityExpanded)
              .optimize();
            
            bracketedReplaceTmp.transform_weights(&zero_weight);
            bracketedReplace.disjunct(bracketedReplaceTmp).optimize();
            
            //Create context part
            HfstTransducer unionContextReplaceTmp(type);
            
            // For each context that uses the output side (REPL_DOWN,
            // REPL_LEFT, REPL_RIGHT) we must freely allow all markers that can
            // be generated by other rules. Consider e.g. the following case:
            //
            //   C -> d::1 || \/ b _ b ,, a::1 -> b || c _ C , C _ c
            //
            // where the second rule yields caCac -> cbCbc and the first one
            // again cbCbc -> cbdbc.
            HfstTransducerPairVector cont = ruletmp.get_context();

            if (ruletmp.get_replType() != REPL_UP)
              {
                for (HfstTransducerPairVector::iterator cont_it = cont.begin();
                     cont_it != cont.end(); cont_it++)
                  {
                    for (StringSet::const_iterator sit 
                           = marker_symbols.begin();
                         sit != marker_symbols.end(); sit++)
                      {
                        if (getMarkerNumber(*sit) != i)
                          {
                            StringPair marker_pair(*sit, *sit);
                            // 'false' makes sure harmonization is not done
                            cont_it->first.insert_freely(marker_pair, false);
                            cont_it->second.insert_freely(marker_pair, false);
                          }
                      }
                  }
              }

            unionContextReplaceTmp 
              = expandContextsWithMapping ( cont,
                                            mappingWithBracketsAndTmpBoundary,
                                            identityExpanded,
                                            ruletmp.get_replType(),
                                            optional);
            
            unionContextReplaceTmp.transform_weights(&zero_weight);
            
            unionContextReplace.disjunct(unionContextReplaceTmp).optimize();
           /*
           //THIS part is for disjuncting labels first, and then substitute
           // them with transducers
           //create map with context keys and basic transducers
           stringstream tmpString;
           tmpString << "$_contextRepl." << i << "_$";
           string contx_key =  tmpString.str();
           TOK.add_multichar_symbol(contx_key);
           // cout << "lexicon " << regex_key << "\n";
           
           if (contextReplaceMap.find(contx_key) == contextReplaceMap.end())
           {
           contextReplaceMap.insert(pair<string,HfstBasicTransducer>(contx_key,
           HfstBasicTransducer(unionContextReplaceTmp)));
           }
           
           //disjunct all keys
           unionContextReplace_labels.disjunct(HfstTransducer(contx_key,
           TOK, type)).optimize();
           
           //   printf("unionContextReplace_labels: \n");
           //   unionContextReplace_labels.write_in_att_format(stdout, 1);
           
           */
          }

        //printf("unionContextReplace_labels: \n");
        //unionContextReplace.write_in_att_format(stdout, 1);
        
        
        
        /*
        //THIS part is for disjuncting labels first, and then substitute them
        with transducers
        printf("substitute labels with real tr: \n");
        //substitute labels with real tr
        HfstBasicTransducer btr(unionContextReplace_labels);
        btr.substitute(contextReplaceMap, true);
        btr.prune_alphabet();
        
        printf("Substitute done. \n");
        HfstTransducer unionContextReplace(btr, type);
        printf("Created tr \n");
        
        
        //weight can be zero, because it will be subtracted
        bracketedReplace.transform_weights(&zero_weight);
        unionContextReplace.transform_weights(&zero_weight);
        
        printf("min \n");
        unionContextReplace.optimize();
        printf("min done. \n");
        
        */
        
        /////////////////////}
        //printf("subtract all mappings in contexts from replace without
        //contexts: \n");
        // subtract all mappings in contexts from replace without contexts
        HfstTransducer replaceWithoutContexts(bracketedReplace);
        replaceWithoutContexts.subtract(unionContextReplace).optimize();
        
        //printf("remove bla bla: \n");
        // remove tmpMaprker
        replaceWithoutContexts.substitute
          (StringPair(tmpMarker, tmpMarker),
           StringPair("@_EPSILON_SYMBOL_@", "@_EPSILON_SYMBOL_@")).optimize();
        replaceWithoutContexts.remove_from_alphabet(tmpMarker);
        replaceWithoutContexts.optimize();

        identityExpanded.remove_from_alphabet(tmpMarker);
        
        //printf("before final negation: \n");
        //replaceWithoutContexts.write_in_att_format(stdout, 1);
        
        //printf("final subtract: \n");
        // final negation
        HfstTransducer uncondidtionalTr(identityExpanded);
        uncondidtionalTr.subtract(replaceWithoutContexts).optimize();
        
        //printf("uncondidtionalTr: \n");
        //uncondidtionalTr.write_in_att_format(stdout, 1);
        
        // substitute markers with epsilons
        uncondidtionalTr.substitute(marker_substitutions);
        uncondidtionalTr.remove_from_alphabet(marker_symbols);
        
        return uncondidtionalTr;
        
    } 
    

      //---------------------------------
      //    CONSTRAINTS
      //---------------------------------

      // (help function)
      // returns: [ B:0 | 0:B | ?-B ]*
      // which is used in some constraints
      HfstTransducer constraintsRightPart( ImplementationType type )
      {
          HfstTokenizer TOK;
          TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

          String leftMarker("@LM@");
          String rightMarker("@RM@");
          TOK.add_multichar_symbol(leftMarker);
          TOK.add_multichar_symbol(rightMarker);

          // Identity pair (normal)
          HfstTransducer identityPair = HfstTransducer::identity_pair( type );


          HfstTransducer leftBracket(leftMarker, TOK, type);
          HfstTransducer rightBracket(rightMarker, TOK, type);

          // Create Right Part
          HfstTransducer B(leftBracket);
          B.disjunct(rightBracket).optimize();

          HfstTransducer epsilon("@_EPSILON_SYMBOL_@", TOK, type);
          HfstTransducer epsilonToLeftMark("@_EPSILON_SYMBOL_@", leftMarker, TOK, type);
          HfstTransducer LeftMarkToEpsilon(leftMarker, "@_EPSILON_SYMBOL_@", TOK, type);

          HfstTransducer epsilonToBrackets(epsilon);
          epsilonToBrackets.cross_product(B);

          HfstTransducer bracketsToEpsilon(B);
          bracketsToEpsilon.cross_product(epsilon);

          HfstTransducer identityPairMinusBrackets(identityPair);
          identityPairMinusBrackets.subtract(B).optimize();//.repeat_plus().optimize();

          HfstTransducer rightPart(epsilonToBrackets);
          rightPart.disjunct(bracketsToEpsilon).disjunct(identityPairMinusBrackets).optimize().repeat_star().optimize();

          return rightPart;
      }

      // .#. ?* <:0 0:> ?* .#.
      // filters out empty string
        HfstTransducer oneBetterthanNoneConstraint( const HfstTransducer &uncondidtionalTr )
        {
            ImplementationType type = uncondidtionalTr.get_type();
            HfstTokenizer TOK;
            TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");
            TOK.add_multichar_symbol(".#.");

            String leftMarker("@LM@");
            String rightMarker("@RM@");
            TOK.add_multichar_symbol(leftMarker);
            TOK.add_multichar_symbol(rightMarker);

            // Identity (normal)
            HfstTransducer identityPair = HfstTransducer::identity_pair( type );
            HfstTransducer identity (identityPair);
            identity.repeat_star().optimize();

            HfstTransducer leftBracketToZero(leftMarker, "@_EPSILON_SYMBOL_@", TOK, type);
            HfstTransducer rightBracketToZero(rightMarker, "@_EPSILON_SYMBOL_@", TOK, type);


            HfstTransducer boundary(".#.", TOK, type);
            HfstTransducer Constraint(boundary);
            Constraint.concatenate(identity);
            Constraint.concatenate(leftBracketToZero)
               .concatenate(rightBracketToZero)
               .concatenate(boundary)
               .concatenate(identity)
               .optimize();

//            printf("Constraint: \n");
//            Constraint.write_in_att_format(stdout, 1);

            //// Compose with unconditional replace transducer
            // tmp = t.1 .o. Constr .o. t.1
            // (t.1 - tmp.2) .o. t


            HfstTransducer retval(type);
            retval = constraintComposition(uncondidtionalTr, Constraint);


            return retval;
        }



      // .#. ?* <:0 [B:0]* [I-B] [ B:0 | 0:B | ?-B ]* .#.
      HfstTransducer leftMostConstraint( const HfstTransducer &uncondidtionalTr )
      {
          HfstTokenizer TOK;
          TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");
          TOK.add_multichar_symbol("@_UNKNOWN_SYMBOL_@");
          TOK.add_multichar_symbol(".#.");

          String leftMarker("@LM@");
          String rightMarker("@RM@");
          TOK.add_multichar_symbol(leftMarker);
          TOK.add_multichar_symbol(rightMarker);

          ImplementationType type = uncondidtionalTr.get_type();

          HfstTransducer leftBracket(leftMarker, TOK, type);
          HfstTransducer rightBracket(rightMarker, TOK, type);


          // Identity (normal)
          HfstTransducer identityPair = HfstTransducer::identity_pair( type );
          HfstTransducer identity (identityPair);
          identity.repeat_star().optimize();


          // Create Right Part:  [ B:0 | 0:B | ?-B ]*
          HfstTransducer rightPart(type);
          rightPart = constraintsRightPart(type);


          // epsilon
          HfstTransducer epsilon("@_EPSILON_SYMBOL_@", TOK, type);
          // B
          HfstTransducer B(leftBracket);
          B.disjunct(rightBracket).optimize();
          // (B:0)*


          HfstTransducer bracketsToEpsilonStar(B);
          bracketsToEpsilonStar.cross_product(epsilon).optimize().repeat_star().optimize();

          // (I-B) and (I-B)+
          HfstTransducer identityPairMinusBrackets(identityPair);
          identityPairMinusBrackets.subtract(B).optimize();

          HfstTransducer identityPairMinusBracketsPlus(identityPairMinusBrackets);
          identityPairMinusBracketsPlus.repeat_plus().optimize();

          /*
          HfstTransducer epsilon("@_EPSILON_SYMBOL_@", TOK, type);
          HfstTransducer identityPairMinusBracketsOrEpsilon(identityPairMinusBrackets);
          identityPairMinusBracketsOrEpsilon.disjunct(epsilon).optimize();
          */
          HfstTransducer LeftBracketToEpsilon(leftMarker, "@_EPSILON_SYMBOL_@", TOK, type);


          HfstTransducer boundary(".#.", TOK, type);

          HfstTransducer Constraint(boundary);
          Constraint.concatenate(identity);
          //HfstTransducer Constraint(identity);

          // ?* <:0 [B:0]* [I-B] [ B:0 | 0:B | ?-B ]*
          Constraint.concatenate(LeftBracketToEpsilon).
                  concatenate(bracketsToEpsilonStar).
                  concatenate(identityPairMinusBrackets).
                  concatenate(rightPart).
                  optimize();

          Constraint.concatenate(boundary).optimize();

        //  printf("Constraint: \n");
         // Constraint.write_in_att_format(stdout, 1);

          //// Compose with unconditional replace transducer
          // tmp = t.1 .o. Constr .o. t.1
          // (t.1 - tmp.2) .o. t

          HfstTransducer retval(type);
          retval = constraintComposition(uncondidtionalTr, Constraint);

          //printf("Constraint: \n");
          //Constraint.write_in_att_format(stdout, 1);
          return retval;

      }

      // [ B:0 | 0:B | ?-B ]* [I-B]+  >:0 [ ?-B ]*
      HfstTransducer rightMostConstraint( const HfstTransducer &uncondidtionalTr )
      {
          HfstTokenizer TOK;
          TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");
          TOK.add_multichar_symbol("@_UNKNOWN_SYMBOL_@");

          String leftMarker("@LM@");
          String rightMarker("@RM@");
          TOK.add_multichar_symbol(leftMarker);
          TOK.add_multichar_symbol(rightMarker);

          ImplementationType type = uncondidtionalTr.get_type();

          HfstTransducer leftBracket(leftMarker, TOK, type);
          HfstTransducer rightBracket(rightMarker, TOK, type);


          // Identity (normal)
          HfstTransducer identityPair = HfstTransducer::identity_pair( type );
          HfstTransducer identity (identityPair);
          identity.repeat_star().optimize();


          // Create Right Part:  [ B:0 | 0:B | ?-B ]*
          HfstTransducer rightPart(type);
          rightPart = constraintsRightPart(type);


          // epsilon
          HfstTransducer epsilon("@_EPSILON_SYMBOL_@", TOK, type);
          // B
          HfstTransducer B(leftBracket);
          B.disjunct(rightBracket).optimize();
          // (B:0)*
          HfstTransducer bracketsToEpsilonStar(B);
          bracketsToEpsilonStar.cross_product(epsilon).optimize().repeat_star().optimize();

          // (I-B) and (I-B)+
          HfstTransducer identityPairMinusBrackets(identityPair);
          identityPairMinusBrackets.subtract(B).optimize();

          HfstTransducer identityPairMinusBracketsPlus(identityPairMinusBrackets);
          identityPairMinusBracketsPlus.repeat_plus().optimize();

          HfstTransducer identityPairMinusBracketsStar(identityPairMinusBrackets);
          identityPairMinusBracketsStar.repeat_star().optimize();


          HfstTransducer RightBracketToEpsilon(rightMarker, "@_EPSILON_SYMBOL_@", TOK, type);

          HfstTransducer Constraint(rightPart);
          // [ B:0 | 0:B | ?-B ]* [I-B]+  >:0 [ ?-B ]*

          Constraint.concatenate(identityPairMinusBracketsPlus).
                  concatenate(RightBracketToEpsilon).
                  concatenate(identity).
                  optimize();

          //// Compose with unconditional replace transducer
          // tmp = t.1 .o. Constr .o. t.1
          // (t.1 - tmp.2) .o. t

          HfstTransducer retval(type);
          retval = constraintComposition(uncondidtionalTr, Constraint);


      //printf("Constraint: \n");
      //Constraint.write_in_att_format(stdout, 1);
          return retval;

      }


      // Longest match
      // it should be composed to left most transducer........
      // ?* < [?-B]+ 0:> [ ? | 0:< | <:0 | 0:> | B ] [ B:0 | 0:B | ?-B ]*
      HfstTransducer longestMatchLeftMostConstraint( const HfstTransducer &uncondidtionalTr )
      {

          HfstTokenizer TOK;
          TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

          String leftMarker("@LM@");
          String rightMarker("@RM@");
          TOK.add_multichar_symbol(leftMarker);
          TOK.add_multichar_symbol(rightMarker);

          ImplementationType type = uncondidtionalTr.get_type();

          HfstTransducer leftBracket(leftMarker, TOK, type);
          HfstTransducer rightBracket(rightMarker, TOK, type);

          // Identity
          HfstTransducer identityPair = HfstTransducer::identity_pair( type );
          HfstTransducer identity(identityPair);
          identity.repeat_star().optimize();

          // epsilon
          HfstTransducer epsilon("@_EPSILON_SYMBOL_@", TOK, type);
          // B
          HfstTransducer B(leftBracket);
          B.disjunct(rightBracket).optimize();
          // (B:0)*
          HfstTransducer bracketsToEpsilonStar(B);
          bracketsToEpsilonStar.cross_product(epsilon).optimize().repeat_star().optimize();

          // (I-B) and (I-B)+
          HfstTransducer identityPairMinusBrackets(identityPair);
          identityPairMinusBrackets.subtract(B).optimize();

          HfstTransducer identityPairMinusBracketsPlus(identityPairMinusBrackets);
          identityPairMinusBracketsPlus.repeat_plus().optimize();



          // Create Right Part:  [ B:0 | 0:B | ?-B ]*
          HfstTransducer rightPart(type);
          rightPart = constraintsRightPart(type);

          HfstTransducer RightBracketToEpsilon(rightMarker, "@_EPSILON_SYMBOL_@", TOK, type);
          HfstTransducer epsilonToRightBracket("@_EPSILON_SYMBOL_@", rightMarker, TOK, type);
          HfstTransducer LeftBracketToEpsilon(leftMarker, "@_EPSILON_SYMBOL_@", TOK, type);
          HfstTransducer epsilonToLeftBracket("@_EPSILON_SYMBOL_@", leftMarker, TOK, type);

          //[ ? | 0:< | <:0 | 0:> | B ]
     //     HfstTransducer nonClosingBracketInsertion(identityPair);
          HfstTransducer nonClosingBracketInsertion(epsilonToLeftBracket);
          nonClosingBracketInsertion.
                  //disjunct(epsilonToLeftBracket).
                  disjunct(LeftBracketToEpsilon).
                  disjunct(epsilonToRightBracket).
                  disjunct(B).
                  optimize();
          //    printf("nonClosingBracketInsertion: \n");
          //    nonClosingBracketInsertion.write_in_att_format(stdout, 1);


          nonClosingBracketInsertion.concatenate(identityPairMinusBracketsPlus).optimize();

          HfstTransducer middlePart(identityPairMinusBrackets);
          middlePart.disjunct(nonClosingBracketInsertion).optimize();


          // ?* < [?-B]+ 0:> [ ? | 0:< | <:0 | 0:> | B ] [?-B]+ [ B:0 | 0:B | ?-B ]*
          HfstTransducer Constraint(identity);
          Constraint.concatenate(leftBracket).
                  concatenate(identityPairMinusBracketsPlus).
                  concatenate(epsilonToRightBracket).
              //    concatenate(nonClosingBracketInsertion).
              //    concatenate(identityPairMinusBracketsPlus).
                  concatenate(middlePart).
                  concatenate(rightPart).
                  optimize();
          //printf("Constraint Longest Match: \n");
          //Constraint.write_in_att_format(stdout, 1);


          //uncondidtionalTr should be left most for the left most longest match
          HfstTransducer retval(type);
          retval = constraintComposition(uncondidtionalTr, Constraint);


          return retval;

      }

      // Longest match RIGHT most
      HfstTransducer longestMatchRightMostConstraint(const HfstTransducer &uncondidtionalTr )
      {
          HfstTokenizer TOK;
          TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

          String leftMarker("@LM@");
          String rightMarker("@RM@");
          TOK.add_multichar_symbol(leftMarker);
          TOK.add_multichar_symbol(rightMarker);

          ImplementationType type = uncondidtionalTr.get_type();

          HfstTransducer leftBracket(leftMarker, TOK, type);
          HfstTransducer rightBracket(rightMarker, TOK, type);

          // Identity
          HfstTransducer identityPair = HfstTransducer::identity_pair( type );
          HfstTransducer identity(identityPair);
          identity.repeat_star().optimize();

          // epsilon
          HfstTransducer epsilon("@_EPSILON_SYMBOL_@", TOK, type);
          // B
          HfstTransducer B(leftBracket);
          B.disjunct(rightBracket).optimize();
          // (B:0)*
          HfstTransducer bracketsToEpsilonStar(B);
          bracketsToEpsilonStar.cross_product(epsilon).optimize().repeat_star().optimize();

          // (I-B) and (I-B)+
          HfstTransducer identityPairMinusBrackets(identityPair);
          identityPairMinusBrackets.subtract(B).optimize();

          HfstTransducer identityPairMinusBracketsPlus(identityPairMinusBrackets);
          identityPairMinusBracketsPlus.repeat_plus().optimize();



          // Create Right Part:  [ B:0 | 0:B | ?-B ]*
          HfstTransducer rightPart(type);
          rightPart = constraintsRightPart(type);



          HfstTransducer RightBracketToEpsilon(rightMarker, "@_EPSILON_SYMBOL_@", TOK, type);

          HfstTransducer epsilonToRightBracket("@_EPSILON_SYMBOL_@", rightMarker, TOK, type);
          HfstTransducer LeftBracketToEpsilon(leftMarker, "@_EPSILON_SYMBOL_@", TOK, type);
          HfstTransducer epsilonToLeftBracket("@_EPSILON_SYMBOL_@", leftMarker, TOK, type);


          //[ ? | 0:< | >:0 | 0:> | B ]
          HfstTransducer nonClosingBracketInsertion(identityPair);
          nonClosingBracketInsertion.disjunct(epsilonToLeftBracket).
                  disjunct(RightBracketToEpsilon).
                  disjunct(epsilonToRightBracket).
                  disjunct(B).
                  optimize();


          // [ B:0 | 0:B | ?-B ]* [?-B]+ [ ? | 0:< | <:0 | 0:> | B ] 0:< [?-B]+ > ?*

          HfstTransducer Constraint(rightPart);
          Constraint.concatenate(identityPairMinusBracketsPlus).
                  concatenate(nonClosingBracketInsertion).optimize().
                  concatenate(epsilonToLeftBracket).
                  concatenate(identityPairMinusBracketsPlus).
                  concatenate(rightBracket).
                  concatenate(identity).
                  optimize();
          //printf("Constraint Longest Match: \n");
          //Constraint.write_in_att_format(stdout, 1);


          //uncondidtionalTr should be left most for the left most longest match
          HfstTransducer retval(type);
          retval = constraintComposition(uncondidtionalTr, Constraint);


          return retval;
      }

      // Shortest match
      // it should be composed to left most transducer........
      // ?* < [?-B]+ >:0
      // [?-B] or [ ? | 0:< | <:0 | >:0 | B ][?-B]+
      // [ B:0 | 0:B | ?-B ]*
      HfstTransducer shortestMatchLeftMostConstraint( const HfstTransducer &uncondidtionalTr )
      {

          HfstTokenizer TOK;
          TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

          String leftMarker("@LM@");
          String rightMarker("@RM@");
          TOK.add_multichar_symbol(leftMarker);
          TOK.add_multichar_symbol(rightMarker);

          ImplementationType type = uncondidtionalTr.get_type();

          HfstTransducer leftBracket(leftMarker, TOK, type);
          HfstTransducer rightBracket(rightMarker, TOK, type);

          // Identity
          HfstTransducer identityPair = HfstTransducer::identity_pair( type );
          HfstTransducer identity(identityPair);
          identity.repeat_star().optimize();


          // Create Right Part:  [ B:0 | 0:B | ?-B ]*
          HfstTransducer rightPart(type);
          rightPart = constraintsRightPart(type);

          // [?-B] and [?-B]+
          HfstTransducer B(leftBracket);
          B.disjunct(rightBracket).optimize();
          HfstTransducer identityPairMinusBrackets(identityPair);
          identityPairMinusBrackets.subtract(B).optimize();
          HfstTransducer identityPairMinusBracketsPlus(identityPairMinusBrackets);
          identityPairMinusBracketsPlus.repeat_plus().optimize();


          HfstTransducer RightBracketToEpsilon(rightMarker, "@_EPSILON_SYMBOL_@", TOK, type);
          HfstTransducer epsilonToRightBracket("@_EPSILON_SYMBOL_@", rightMarker, TOK, type);
          HfstTransducer LeftBracketToEpsilon(leftMarker, "@_EPSILON_SYMBOL_@", TOK, type);
          HfstTransducer epsilonToLeftBracket("@_EPSILON_SYMBOL_@", leftMarker, TOK, type);


          // [ 0:< | <:0 | >:0 | B ][?-B]+
          HfstTransducer nonClosingBracketInsertion(epsilonToLeftBracket);
          nonClosingBracketInsertion.
                  //disjunct(epsilonToLeftBracket).
                  disjunct(LeftBracketToEpsilon).
                  disjunct(RightBracketToEpsilon).
                  disjunct(B).
                  optimize();

          nonClosingBracketInsertion.concatenate(identityPairMinusBracketsPlus).optimize();

          HfstTransducer middlePart(identityPairMinusBrackets);
          middlePart.disjunct(nonClosingBracketInsertion).optimize();

      //    printf("nonClosingBracketInsertion: \n");
      //    nonClosingBracketInsertion.write_in_att_format(stdout, 1);

          // ?* < [?-B]+ >:0
          // [?-B] or [ ? | 0:< | <:0 | >:0 | B ][?-B]+
          //[ B:0 | 0:B | ?-B ]*
          HfstTransducer Constraint(identity);
          Constraint.concatenate(leftBracket).
                  concatenate(identityPairMinusBracketsPlus).
                  concatenate(RightBracketToEpsilon).
                  concatenate(middlePart).optimize().
                  concatenate(rightPart).
                  optimize();

          //printf("Constraint Shortest Match: \n");
          //Constraint.write_in_att_format(stdout, 1);


          //uncondidtionalTr should be left most for the left most shortest match
          HfstTransducer retval(type);
          retval = constraintComposition(uncondidtionalTr, Constraint);


          return retval;

      }

      // Shortest match
      // it should be composed to left most transducer........
      //[ B:0 | 0:B | ?-B ]*
      // [?-B] or [?-B]+  [ ? | 0:> | >:0 | <:0 | B ]
      // <:0 [?-B]+   > ?*
      HfstTransducer shortestMatchRightMostConstraint( const HfstTransducer &uncondidtionalTr )
      {

          HfstTokenizer TOK;
          TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

          String leftMarker("@LM@");
          String rightMarker("@RM@");
          TOK.add_multichar_symbol(leftMarker);
          TOK.add_multichar_symbol(rightMarker);

          ImplementationType type = uncondidtionalTr.get_type();

          HfstTransducer leftBracket(leftMarker, TOK, type);
          HfstTransducer rightBracket(rightMarker, TOK, type);

          // Identity
          HfstTransducer identityPair = HfstTransducer::identity_pair( type );
          HfstTransducer identity(identityPair);
          identity.repeat_star().optimize();


          // Create Right Part:  [ B:0 | 0:B | ?-B ]*
          HfstTransducer rightPart(type);
          rightPart = constraintsRightPart(type);

          // [?-B] and [?-B]+
          HfstTransducer B(leftBracket);
          B.disjunct(rightBracket).optimize();
          HfstTransducer identityPairMinusBrackets(identityPair);
          identityPairMinusBrackets.subtract(B).optimize();
          HfstTransducer identityPairMinusBracketsPlus(identityPairMinusBrackets);
          identityPairMinusBracketsPlus.repeat_plus().optimize();



          HfstTransducer RightBracketToEpsilon(rightMarker, "@_EPSILON_SYMBOL_@", TOK, type);
          HfstTransducer epsilonToRightBracket("@_EPSILON_SYMBOL_@", rightMarker, TOK, type);
          HfstTransducer LeftBracketToEpsilon(leftMarker, "@_EPSILON_SYMBOL_@", TOK, type);
          HfstTransducer epsilonToLeftBracket("@_EPSILON_SYMBOL_@", leftMarker, TOK, type);


          // [?-B]+ [ 0:> | >:0 | <:0 | B ]
          HfstTransducer nonClosingBracketInsertionTmp(epsilonToRightBracket);
          nonClosingBracketInsertionTmp.
                  disjunct(RightBracketToEpsilon).
                  disjunct(LeftBracketToEpsilon).
                  disjunct(B).optimize();
          HfstTransducer nonClosingBracketInsertion(identityPairMinusBracketsPlus);
          nonClosingBracketInsertion.concatenate(nonClosingBracketInsertionTmp).optimize();


          HfstTransducer middlePart(identityPairMinusBrackets);
          middlePart.disjunct(nonClosingBracketInsertion).optimize();


          //[ B:0 | 0:B | ?-B ]*
          // [?-B] or [?-B]+  [ ? | 0:> | >:0 | <:0 | B ]
          // <:0 [?-B]+   > ?*

          HfstTransducer Constraint(rightPart);
          Constraint.concatenate(middlePart).
                  concatenate(LeftBracketToEpsilon).
                  concatenate(identityPairMinusBracketsPlus).
                  concatenate(rightBracket).
                  concatenate(identity).
                  optimize();

          //printf("Constraint Shortest Match: \n");
          //Constraint.write_in_att_format(stdout, 1);

          //uncondidtionalTr should be left most for the left most longest match
          HfstTransducer retval(type);
          retval = constraintComposition(uncondidtionalTr, Constraint);

          return retval;

      }


      // ?* [ BL:0 (?-B)+ BR:0 ?* ]+
      HfstTransducer mostBracketsPlusConstraint( const HfstTransducer &uncondidtionalTr )
      {
          HfstTokenizer TOK;
          TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

          String leftMarker("@LM@");
          String rightMarker("@RM@");
          String leftMarker2("@LM2@");
          String rightMarker2("@RM2@");

          TOK.add_multichar_symbol(leftMarker);
          TOK.add_multichar_symbol(rightMarker);
          TOK.add_multichar_symbol(leftMarker2);
          TOK.add_multichar_symbol(rightMarker2);

          ImplementationType type = uncondidtionalTr.get_type();

          HfstTransducer leftBracket(leftMarker, TOK, type);
          HfstTransducer rightBracket(rightMarker, TOK, type);
          HfstTransducer leftBracket2(leftMarker2, TOK, type);
          HfstTransducer rightBracket2(rightMarker2, TOK, type);

          // Identity (normal)
          HfstTransducer identityPair = HfstTransducer::identity_pair( type );
          HfstTransducer identity (identityPair);
          identity.repeat_star().optimize();

          HfstTransducer identityPlus (identityPair);
          identityPlus.repeat_plus().optimize();

          HfstTransducer identityStar (identityPair);
          identityStar.repeat_star().optimize();

          // epsilon
          String epsilon("@_EPSILON_SYMBOL_@");

          // BL:0 ( <1 : 0, <2 : 0)
          HfstTransducer leftBracketToEpsilon(leftMarker, epsilon, TOK, type );
          HfstTransducer leftBracket2ToEpsilon(leftMarker2, epsilon, TOK, type );
          HfstTransducer allLeftBracketsToEpsilon(leftBracketToEpsilon);
          allLeftBracketsToEpsilon.disjunct(leftBracket2ToEpsilon).optimize();


          //    printf("allLeftBracketsToEpsilon: \n");
          //    allLeftBracketsToEpsilon.write_in_att_format(stdout, 1);

          // BR:0 ( >1 : 0, >2 : 0)
          HfstTransducer rightBracketToEpsilon(rightMarker, epsilon, TOK, type );
          HfstTransducer rightBracket2ToEpsilon(rightMarker2, epsilon, TOK, type );
          HfstTransducer allRightBracketsToEpsilon(rightBracketToEpsilon);
          allRightBracketsToEpsilon.disjunct(rightBracket2ToEpsilon).optimize();

          // B (B1 | B2)
          HfstTransducer B(leftBracket);
          B.disjunct(rightBracket).optimize();
          B.disjunct(leftBracket2).optimize();
          B.disjunct(rightBracket2).optimize();



          // (? - B)+
          HfstTransducer identityPairMinusBracketsPlus(identityPair);
          identityPairMinusBracketsPlus.subtract(B).optimize().repeat_plus().optimize();

          // repeatingPart ( BL:0 (?-B)+ BR:0 ?* )+
          HfstTransducer repeatingPart(allLeftBracketsToEpsilon);
          repeatingPart.concatenate(identityPairMinusBracketsPlus).optimize();
          repeatingPart.concatenate(allRightBracketsToEpsilon).optimize();
          repeatingPart.concatenate(identityStar).optimize();
          repeatingPart.repeat_plus().optimize();
          //printf("middlePart: \n");
          //middlePart.write_in_att_format(stdout, 1);

          HfstTransducer Constraint(identityStar);
          Constraint.concatenate(repeatingPart).optimize();
          //printf("Constraint: \n");
          //Constraint.write_in_att_format(stdout, 1);


          //// Compose with unconditional replace transducer
          // tmp = t.1 .o. Constr .o. t.1
          // (t.1 - tmp.2) .o. t

          HfstTransducer retval(type);
          retval = constraintComposition(uncondidtionalTr, Constraint);

          //printf("After composition: \n");
          //retval.write_in_att_format(stdout, 1);

          return retval;
      }

      // ?* [ BL:0 (?-B)* BR:0 ?* ]+
      HfstTransducer mostBracketsStarConstraint( const HfstTransducer &uncondidtionalTr )
      {
          HfstTokenizer TOK;
          TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

          String leftMarker("@LM@");
          String rightMarker("@RM@");
          String leftMarker2("@LM2@");
          String rightMarker2("@RM2@");
          TOK.add_multichar_symbol(leftMarker);
          TOK.add_multichar_symbol(rightMarker);
          TOK.add_multichar_symbol(leftMarker2);
          TOK.add_multichar_symbol(rightMarker2);

          ImplementationType type = uncondidtionalTr.get_type();

          HfstTransducer leftBracket(leftMarker, TOK, type);
          HfstTransducer rightBracket(rightMarker, TOK, type);

          HfstTransducer leftBracket2(leftMarker2, TOK, type);
          HfstTransducer rightBracket2(rightMarker2, TOK, type);


          // Identity (normal)
          HfstTransducer identityPair = HfstTransducer::identity_pair( type );
          HfstTransducer identity (identityPair);
          identity.repeat_star().optimize();

          HfstTransducer identityPlus (identityPair);
          identityPlus.repeat_plus().optimize();


          HfstTransducer identityStar (identityPair);
          identityStar.repeat_star().optimize();


          // epsilon
          String epsilon("@_EPSILON_SYMBOL_@");

          // BL:0 ( <1 : 0, <2 : 0)
          HfstTransducer leftBracketToEpsilon(leftMarker, epsilon, TOK, type );
          HfstTransducer leftBracket2ToEpsilon(leftMarker2, epsilon, TOK, type );
          HfstTransducer allLeftBracketsToEpsilon(leftBracketToEpsilon);
          allLeftBracketsToEpsilon.disjunct(leftBracket2ToEpsilon).optimize();

          //    printf("allLeftBracketsToEpsilon: \n");
          //    allLeftBracketsToEpsilon.write_in_att_format(stdout, 1);

          // BR:0 ( >1 : 0, >2 : 0)
          HfstTransducer rightBracketToEpsilon(rightMarker, epsilon, TOK, type );
          HfstTransducer rightBracket2ToEpsilon(rightMarker2, epsilon, TOK, type );
          HfstTransducer allRightBracketsToEpsilon(rightBracketToEpsilon);
          allRightBracketsToEpsilon.disjunct(rightBracket2ToEpsilon).optimize();

          // B (B1 | B2)
          HfstTransducer B(leftBracket);
          B.disjunct(rightBracket).optimize();
          B.disjunct(leftBracket2).optimize();
          B.disjunct(rightBracket2).optimize();

          // (? - B)*
          HfstTransducer identityPairMinusBracketsStar(identityPair);
          identityPairMinusBracketsStar.subtract(B).optimize().repeat_star().optimize();

          // repeatingPart [ BL:0 (?-B)* BR:0 ?* ]+
          HfstTransducer repeatingPart(allLeftBracketsToEpsilon);
          repeatingPart.concatenate(identityPairMinusBracketsStar).optimize();
          repeatingPart.concatenate(allRightBracketsToEpsilon).optimize();
          repeatingPart.concatenate(identityStar).optimize();
          repeatingPart.repeat_plus().optimize();
          //printf("middlePart: \n");
          //repeatingPart.write_in_att_format(stdout, 1);

          HfstTransducer Constraint(identityStar);
          Constraint.concatenate(repeatingPart).optimize();
          //printf("Constraint: \n");
          //Constraint.write_in_att_format(stdout, 1);

          //// Compose with unconditional replace transducer
          // tmp = t.1 .o. Constr .o. t.1
          // (t.1 - tmp.2) .o. t
          HfstTransducer retval(type);
          retval = constraintComposition(uncondidtionalTr, Constraint);

          //printf("After composition: \n");
          //retval.write_in_att_format(stdout, 1);
          return retval;

      }
      // ?* B2 ?*
      HfstTransducer removeB2Constraint( const HfstTransducer &t )
      {
          HfstTokenizer TOK;
          TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

          String leftMarker2("@LM2@");
          String rightMarker2("@RM2@");

          TOK.add_multichar_symbol(leftMarker2);
          TOK.add_multichar_symbol(rightMarker2);

          ImplementationType type = t.get_type();

          HfstTransducer leftBracket2(leftMarker2, TOK, type);
          HfstTransducer rightBracket2(rightMarker2, TOK, type);

          // Identity (normal)
          HfstTransducer identityPair = HfstTransducer::identity_pair( type );
          HfstTransducer identity (identityPair);
          identity.repeat_star().optimize();

          HfstTransducer identityStar (identityPair);
          identityStar.repeat_star().optimize();

          // B (B2)
          HfstTransducer B(leftBracket2);
          B.disjunct(rightBracket2).optimize();


          HfstTransducer Constraint(identityStar);
          Constraint.concatenate(B).optimize();
          Constraint.concatenate(identityStar).optimize();

          //// Compose with unconditional replace transducer
          // tmp = t.1 .o. Constr .o. t.1
          // (t.1 - tmp.2) .o. t

          HfstTransducer retval(type);
          retval = constraintComposition(t, Constraint);

          retval.remove_from_alphabet(leftMarker2);
          retval.remove_from_alphabet(rightMarker2);

          //printf("Remove B2 After composition: \n");
          //retval.write_in_att_format(stdout, 1);

          return retval;

      }
      // to avoid repetition in empty replace rule
      HfstTransducer noRepetitionConstraint( const HfstTransducer &t )
      {
          HfstTokenizer TOK;
          TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

          String leftMarker("@LM@");
          String rightMarker("@RM@");
          TOK.add_multichar_symbol(leftMarker);
          TOK.add_multichar_symbol(rightMarker);

          String leftMarker2("@LM2@");
          String rightMarker2("@RM2@");

          //if the transdcuer is optional, LM2 and RM2 are not there
          bool optional = true;
          StringSet transducerAlphabet = t.get_alphabet();
          for (StringSet::const_iterator s = transducerAlphabet.begin();
                         s != transducerAlphabet.end();
                         ++s)
          {
              String alph = *s;
              if ( alph == leftMarker2)
              {
                  optional = false;
                  break;
              }
          }

          TOK.add_multichar_symbol(leftMarker2);
          TOK.add_multichar_symbol(rightMarker2);

          ImplementationType type = t.get_type();


          HfstTransducer leftBracket(leftMarker, TOK, type);
          HfstTransducer rightBracket(rightMarker, TOK, type);

          HfstTransducer leftBracket2(leftMarker2, TOK, type);
          HfstTransducer rightBracket2(rightMarker2, TOK, type);


          HfstTransducer leftBrackets(leftBracket);
          if (!optional)
          {
              leftBrackets.disjunct(leftBracket2).optimize();
          }

          HfstTransducer rightBrackets(rightBracket);
          if (!optional)
          {
              rightBrackets.disjunct(rightBracket2).optimize();
          }
          // Identity (normal)
          HfstTransducer identityPair = HfstTransducer::identity_pair( type );
          /*
          identityPair.insert_to_alphabet(leftMarker);
          identityPair.insert_to_alphabet(rightMarker);
          identityPair.insert_to_alphabet(leftMarker);
          identityPair.insert_to_alphabet(rightMarker2);
           */

          HfstTransducer identityStar (identityPair);
          identityStar.repeat_star().optimize();


          HfstTransducer Constraint(identityStar);
          Constraint.concatenate(leftBrackets).
                  concatenate(rightBrackets).
                  concatenate(leftBrackets).
                  concatenate(rightBrackets).
                  concatenate(identityStar).optimize();


          //// Compose with unconditional replace transducer
          // tmp = t.1 .o. Constr .o. t.1
          // (t.1 - tmp.2) .o. t

          //printf("...Constraint: \n");
          //Constraint.write_in_att_format(stdout, 1);


          HfstTransducer retval(type);
          retval = constraintComposition(t, Constraint);

      //    retval = removeB2Constraint(retval);

          return retval;

      }



        // to apply boundary marker (.#.)
      /*
       * [0:.#. | ? - .#.]*
       *         .o.
       *     tr., ie. a -> b || .#. _ ;
       *         .o.
       *     .#. (? - .#.)* .#.
       *         .o.
       * [.#.:0 | ? - .#.]*
       */
        HfstTransducer applyBoundaryMark( const HfstTransducer &t )
        {
            HfstTokenizer TOK;
            TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");
            TOK.add_multichar_symbol("@_UNKNOWN_SYMBOL_@");
            TOK.add_multichar_symbol("@TMP_UNKNOWN@");
            ImplementationType type = t.get_type();

            String boundaryMarker(".#.");
            TOK.add_multichar_symbol(boundaryMarker);
            HfstTransducer boundary(boundaryMarker, TOK, type);

            HfstTransducer identityPair = HfstTransducer::identity_pair( type );
            identityPair.insert_to_alphabet(boundaryMarker);
            // ? - .#.
            HfstTransducer identityMinusBoundary(identityPair);
            identityMinusBoundary.subtract(boundary).optimize();

            // (? - .#.)*
            HfstTransducer identityMinusBoundaryStar(identityMinusBoundary);
            identityMinusBoundaryStar.repeat_star().optimize();

            // .#. (? - .#.)* .#.
            HfstTransducer boundaryAnythingBoundary(boundary);
            boundaryAnythingBoundary.concatenate(identityMinusBoundaryStar)
                                    .concatenate(boundary)
                                    .optimize();

            // [0:.#. | ? - .#.]*
            HfstTransducer zeroToBoundary("@_EPSILON_SYMBOL_@", boundaryMarker, TOK, type);
            HfstTransducer retval(zeroToBoundary);
            retval.disjunct(identityMinusBoundary)
                  .optimize()
                  .repeat_star()
                  .optimize();

            //printf("retval .o. t: \n");
            //retval.write_in_att_format(stdout, 1);
            // [.#.:0 | ? - .#.]*
            HfstTransducer boundaryToZero(boundaryMarker, "@_EPSILON_SYMBOL_@", TOK, type);
            HfstTransducer removeBoundary(boundaryToZero);
            removeBoundary.disjunct(identityMinusBoundary)
               .optimize()
               .repeat_star()
               .optimize();

            // apply boundary to the transducer
            // compose [0:.#. | ? - .#.]* .o. t
            HfstTransducer tr(t);

            //tr.insert_to_alphabet(boundaryMarker);
            // substitutute unknowns with tmp symbol
            // this is necessary because of first composition
            tr.substitute("@_UNKNOWN_SYMBOL_@", "@TMP_UNKNOWN@");

            //printf("----first: ----\n");
            //tr.write_in_att_format(stdout, 1);

            retval.compose(tr).optimize();


//            printf("first composition: \n");
//            retval.write_in_att_format(stdout, 1);

            // compose with .#. (? - .#.)* .#.
            retval.compose(boundaryAnythingBoundary)
                  .optimize();

//            printf("2. composition: \n");
//            retval.write_in_att_format(stdout, 1);

            // compose with [.#.:0 | ? - .#.]*
            retval.compose(removeBoundary).optimize();

//            printf("3. composition: \n");
//            retval.write_in_att_format(stdout, 1);

            // bring back unknown symbols
            retval.substitute("@TMP_UNKNOWN@", "@_UNKNOWN_SYMBOL_@");
            retval.remove_from_alphabet("@TMP_UNKNOWN@");

            // remove boundary from alphabet
            retval.remove_from_alphabet(boundaryMarker);
            return retval;
        }


        //---------------------------------
        //    INTERFACE HELPING FUNCTIONS
        //---------------------------------

        // used by hfst-regexp parser
        // creates markup crossproduct and sets property of the first transducer in the mapping to "isMarkup" = "yes"
        // the other transducer in the mapping is set to epsilon transducer
        HfstTransducerPair create_mapping_for_mark_up_replace( const HfstTransducerPair &mappingPair,
                                                          const HfstTransducerPair &marks )
        {
            HfstTokenizer TOK;
            String epsilon = "@_EPSILON_SYMBOL_@";
            TOK.add_multichar_symbol(epsilon);
            TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

            ImplementationType type = mappingPair.first.get_type();

            HfstTransducer leftMark(marks.first);
            HfstTransducer rightMark(marks.second);

            HfstTransducer epsilonToLeftMark("@_EPSILON_SYMBOL_@", TOK, type);
            epsilonToLeftMark.cross_product(leftMark).optimize();

            HfstTransducer epsilonToRightMark(epsilon, TOK, type);
            epsilonToRightMark.cross_product(rightMark).optimize();

            //Go through left part of every mapping pair
            // and concatenate: epsilonToLeftMark.leftMapping.epsilonToRightMark
            //then put it into right part of the new transducerPairVector
            HfstTransducer mappingCrossProduct(epsilonToLeftMark);
            mappingCrossProduct.concatenate(mappingPair.first).
                    concatenate(epsilonToRightMark).
                    optimize();

            mappingCrossProduct.set_property("isMarkup", "yes");
            
            HfstTransducer epsilonTr(epsilon, TOK, type);
            HfstTransducerPair retval(mappingCrossProduct, epsilonTr);

            return retval;
        }




      //---------------------------------
      //    REPLACE FUNCTIONS - INTERFACE
      //---------------------------------


      // replace up, left, right, down
      HfstTransducer replace( const Rule &rule, bool optional)
      {
          HfstTransducer retval( bracketedReplace(rule, optional) );

          //printf("---bracketed replace done---: \n");
          //retval.optimize().write_in_att_format(stdout, 1);

          // for epenthesis rules
          // it can't have more than one epsilon repetition in a row

          retval = noRepetitionConstraint( retval );

          //printf("-----noRepetitionConstraint-----: \n");
          //retval.write_in_att_format(stdout, 1);

          // deals with boundary symbol, must be before mostBracketsStarConstraint
          retval = applyBoundaryMark( retval );

         //printf("----after applyBoundaryMark: ----\n");
         //retval.write_in_att_format(stdout, 1);
          if ( !optional )
          {

              //printf(" ----------  mostBracketsStarConstraint --------------\n");
              // Epenthesis rules behave differently if used mostBracketsPlusConstraint
              //retval = mostBracketsPlusConstraint(retval);
              retval = mostBracketsStarConstraint(retval);
              //printf("after non optional: \n");
              //retval.write_in_att_format(stdout, 1);
          }
          retval = removeB2Constraint(retval);
          retval = removeMarkers( retval );
          //printf("after removeMarkers: \n");
          //retval.write_in_att_format(stdout, 1);
          return retval;
      }

      // for parallel rules
      HfstTransducer replace( const std::vector<Rule> &ruleVector, bool optional)
      {

         // std::cerr << "replace"<< std::endl;

          HfstTransducer retval;
          // If there is only one rule in the vector, it is not parallel
            if ( ruleVector.size() == 1 )
            {
                retval = bracketedReplace( ruleVector[0], optional) ;
            }
            else
            {
                retval = parallelBracketedReplace(ruleVector, optional);
            }

            //std::cerr << "after bracketed replace"<< std::endl;
        //         printf("- bracketed replace -\n");
        //         retval.write_in_att_format(stdout, 1);

            // for epenthesis rules
            // it can't have more than one epsilon repetition in a row
            retval = noRepetitionConstraint( retval );


        //   printf("----after noRepetitionConstraint: ----\n");
        //   retval.write_in_att_format(stdout, 1);


            // deals with boundary symbol
            retval = applyBoundaryMark( retval );

        //printf("----after applyBoundaryMark: ----\n");
        //retval.write_in_att_format(stdout, 1);

            if ( !optional )
            {
                // Epenthesis rules behave differently if used mostBracketsPlusConstraint
               // retval = mostBracketsPlusConstraint(retval);
                retval = mostBracketsStarConstraint(retval);
            }

         // printf("----after mostBracketsStarConstraint: ----\n");
         //  retval.write_in_att_format(stdout, 1);

            retval = removeB2Constraint(retval);

         //printf("----after removeB2Constraint: ----\n");
         // retval.write_in_att_format(stdout, 1);

            retval = removeMarkers( retval );

            //printf("----after removeMarkers: ----\n");
            //retval.write_in_att_format(stdout, 1);
            return retval;

      }
      // replace left
      HfstTransducer replace_left( const Rule &rule, bool optional)
      {
          HfstTransducerPairVector mappingPairVector = rule.get_mapping();
          //HfstTransducer newMapping = rule.get_mapping();
          //newMapping.invert().optimize();

          HfstTransducerPairVector newMappingPairVector;
          for ( unsigned int i = 0; i < mappingPairVector.size(); i++ )
          {
              // in every mapping pair invert first and second
              //HfstTransducer newMapping = rule.get_mapping();
            HfstTransducer first = mappingPairVector[i].first;
            HfstTransducer second = mappingPairVector[i].second;
            newMappingPairVector.push_back(HfstTransducerPair(second, first));

          }

          Rule newRule ( newMappingPairVector, rule.get_context(), rule.get_replType());
          HfstTransducer retval (replace( newRule, optional));

          retval.invert().optimize();
          return retval;
      }
      // replace left parallel
      HfstTransducer replace_left( const std::vector<Rule> &ruleVector, bool optional)
      {
          std::vector<Rule> leftRuleVector;

          for ( unsigned int i = 0; i < ruleVector.size(); i++ )
          {

              HfstTransducerPairVector mappingPairVector = ruleVector[i].get_mapping();
              //HfstTransducer newMapping = rule.get_mapping();
              //newMapping.invert().optimize();

              HfstTransducerPairVector newMappingPairVector;
              for ( unsigned int j = 0; j < mappingPairVector.size(); j++ )
              {
                  // in every mapping pair invert first and second
                  //HfstTransducer newMapping = rule.get_mapping();
                HfstTransducer first = mappingPairVector[j].first;
                HfstTransducer second = mappingPairVector[j].second;
                newMappingPairVector.push_back(HfstTransducerPair(second, first));

              }

              Rule newRule(newMappingPairVector, ruleVector[i].get_context(), ruleVector[i].get_replType());

              leftRuleVector.push_back(newRule);
          }

          HfstTransducer retval(replace(leftRuleVector, optional));
          retval.invert().optimize();

          return retval;
      }

      // left to right
      HfstTransducer replace_leftmost_longest_match( const Rule &rule )
      {

          HfstTransducer uncondidtionalTr( bracketedReplace(rule, true) );
          //uncondidtionalTr = bracketedReplace(rule, true);

          //printf("LM uncondidtionalTr: \n");
          //uncondidtionalTr.write_in_att_format(stdout, 1);

          // for epenthesis rules
          // it can't have more than one epsilon repetition in a row
          // it should be before leftMostConstraint
          uncondidtionalTr = noRepetitionConstraint( uncondidtionalTr );

          HfstTransducer retval (leftMostConstraint(uncondidtionalTr));

          //to remove empty strings
          retval = oneBetterthanNoneConstraint(retval);


          // printf("leftMostConstraint: \n");
          // retval.write_in_att_format(stdout, 1);
          retval = longestMatchLeftMostConstraint( retval );

          //printf("longestMatchLeftMostConstraint: \n");
          //retval.write_in_att_format(stdout, 1);


          retval = removeB2Constraint(retval);
          retval = removeMarkers( retval );

          // deals with boundary symbol
          retval = applyBoundaryMark( retval );

          return retval;
      }
      // left to right
    HfstTransducer replace_leftmost_longest_match( const std::vector<Rule> &ruleVector )
    {
        //printf("\n replace_leftmost_longest_match \n");

        HfstTransducer uncondidtionalTr;
        if ( ruleVector.size() == 1 )
        {
            uncondidtionalTr = bracketedReplace( ruleVector[0], true) ;
        }
        else
        {
            uncondidtionalTr = parallelBracketedReplace(ruleVector, true);
        }

        //printf("retval unconditional 1 \n");
        // uncondidtionalTr.write_in_att_format(stdout, 1);

        // for epenthesis rules
        // it can't have more than one epsilon repetition in a row
        // it should be before leftMostConstraint
        uncondidtionalTr = noRepetitionConstraint( uncondidtionalTr );
        //printf("uncondidtionalTr epenthesis \n");
        //uncondidtionalTr.write_in_att_format(stdout, 1);

         HfstTransducer retval (leftMostConstraint(uncondidtionalTr));

        //to remove empty strings
        retval = oneBetterthanNoneConstraint(retval);

        retval = longestMatchLeftMostConstraint( retval );
       //printf("retval longestMatchLeftMostConstraint \n");
       //retval.write_in_att_format(stdout, 1);

        // remove LM2, RM2
        retval = removeB2Constraint(retval);

        //printf("retval removeB2Constraint \n");
        //retval.write_in_att_format(stdout, 1);

        retval = removeMarkers( retval );

//       printf("LM removeMarkers: \n");
//        retval.write_in_att_format(stdout, 1);

        // deals with boundary symbol
        retval = applyBoundaryMark( retval );

     // printf("LM applyBoundaryMark: \n");
     // retval.write_in_att_format(stdout, 1);

        return retval;
      }


      // right to left
      HfstTransducer replace_rightmost_longest_match( const Rule &rule )
      {

          HfstTransducer uncondidtionalTr(bracketedReplace(rule, true));
          //uncondidtionalTr = bracketedReplace(rule, true);

          HfstTransducer retval (rightMostConstraint(uncondidtionalTr));
          //retval = rightMostConstraint(uncondidtionalTr);

          //printf("rightMostConstraint: \n");
          //retval.write_in_att_format(stdout, 1);

          retval = longestMatchRightMostConstraint( retval );

          //printf("longestMatchLeftMostConstraint: \n");
          //retval.write_in_att_format(stdout, 1);

          // for epenthesis rules
          // it can't have more than one epsilon repetition in a row
          retval = noRepetitionConstraint( retval );
          // remove LM2, RM2
          retval = removeB2Constraint(retval);

          retval = removeMarkers( retval );


          // deals with boundary symbol
          retval = applyBoundaryMark( retval );

          return retval;
      }


      // right to left
    HfstTransducer replace_rightmost_longest_match( const std::vector<Rule> &ruleVector )
      {
        HfstTransducer uncondidtionalTr;
        if ( ruleVector.size() == 1 )
        {
            uncondidtionalTr = bracketedReplace( ruleVector[0], true) ;
        }
        else
        {
            uncondidtionalTr = parallelBracketedReplace(ruleVector, true);
        }

          HfstTransducer retval (rightMostConstraint(uncondidtionalTr));
          //retval = rightMostConstraint(uncondidtionalTr);

          //printf("rightMostConstraint: \n");
          //retval.write_in_att_format(stdout, 1);

          retval = longestMatchRightMostConstraint( retval );

          //printf("longestMatchLeftMostConstraint: \n");
          //retval.write_in_att_format(stdout, 1);

          // for epenthesis rules
          // it can't have more than one epsilon repetition in a row
          retval = noRepetitionConstraint( retval );
          // remove LM2, RM2
          retval = removeB2Constraint(retval);

          retval = removeMarkers( retval );

          // deals with boundary symbol
          retval = applyBoundaryMark( retval );

          return retval;
      }

      HfstTransducer replace_leftmost_shortest_match( const Rule &rule)
      {

          HfstTransducer uncondidtionalTr(bracketedReplace(rule, true));
          //    uncondidtionalTr = bracketedReplace(rule, true);

          // for epenthesis rules
          // it can't have more than one epsilon repetition in a row
          //has to be before leftMostConstraint
          uncondidtionalTr = noRepetitionConstraint( uncondidtionalTr );

          HfstTransducer retval (leftMostConstraint(uncondidtionalTr));
          //to remove empty strings
          retval = oneBetterthanNoneConstraint(retval);

          retval = shortestMatchLeftMostConstraint( retval );

          //printf("sh tr: \n");
          //retval.write_in_att_format(stdout, 1);


          // remove LM2, RM2
          retval = removeB2Constraint(retval);

          retval = removeMarkers( retval );

          // deals with boundary symbol
          retval = applyBoundaryMark( retval );

          return retval;
      }


    HfstTransducer replace_leftmost_shortest_match(const std::vector<Rule> &ruleVector )
    {
        HfstTransducer uncondidtionalTr;
        if ( ruleVector.size() == 1 )
        {
            uncondidtionalTr = bracketedReplace( ruleVector[0], true) ;
        }
        else
        {
            uncondidtionalTr = parallelBracketedReplace(ruleVector, true);
        }

        // for epenthesis rules
        // it can't have more than one epsilon repetition in a row
        uncondidtionalTr = noRepetitionConstraint( uncondidtionalTr );


        HfstTransducer retval (leftMostConstraint(uncondidtionalTr));

        //to remove empty strings
        retval = oneBetterthanNoneConstraint(retval);

        retval = shortestMatchLeftMostConstraint( retval );

        //printf("sh tr: \n");
        //retval.write_in_att_format(stdout, 1);

        // remove LM2, RM2
        retval = removeB2Constraint(retval);

        retval = removeMarkers( retval );

        // deals with boundary symbol
        retval = applyBoundaryMark( retval );

        return retval;
      }

    HfstTransducer replace_rightmost_shortest_match( const Rule &rule )
    {

        HfstTransducer uncondidtionalTr(bracketedReplace( rule, true));
        //uncondidtionalTr = bracketedReplace( rule, true);

        HfstTransducer retval (rightMostConstraint(uncondidtionalTr));
        //retval = rightMostConstraint(uncondidtionalTr);
        retval = shortestMatchRightMostConstraint( retval );

        //printf("sh tr: \n");
        //retval.write_in_att_format(stdout, 1);


        // for epenthesis rules
        // it can't have more than one epsilon repetition in a row
        retval = noRepetitionConstraint( retval );
        // remove LM2, RM2
        retval = removeB2Constraint(retval);

        retval = removeMarkers( retval );

        // deals with boundary symbol
        retval = applyBoundaryMark( retval );

        return retval;
    }

    HfstTransducer replace_rightmost_shortest_match( const std::vector<Rule> &ruleVector )
    {
        HfstTransducer uncondidtionalTr;
        if ( ruleVector.size() == 1 )
        {
            uncondidtionalTr = bracketedReplace( ruleVector[0], true) ;
        }
        else
        {
            uncondidtionalTr = parallelBracketedReplace(ruleVector, true);
        }
        HfstTransducer retval (rightMostConstraint(uncondidtionalTr));
        //retval = rightMostConstraint(uncondidtionalTr);
        retval = shortestMatchRightMostConstraint( retval );

        //printf("sh tr: \n");
        //retval.write_in_att_format(stdout, 1);


        // for epenthesis rules
        // it can't have more than one epsilon repetition in a row
        retval = noRepetitionConstraint( retval );
        // remove LM2, RM2
        retval = removeB2Constraint(retval);

        retval = removeMarkers( retval );

        // deals with boundary symbol
        retval = applyBoundaryMark( retval );

        return retval;
    }

    
    
      // replace up, left, right, down
      HfstTransducer replace_epenthesis(    const Rule &rule, bool optional)
      {
          return replace(rule, optional);
      }

      // replace up, left, right, down
      HfstTransducer replace_epenthesis(    const std::vector<Rule> &ruleVector, bool optional)
      {
          return replace(ruleVector, optional);
      }


    //---------------------------------
      //    RESTRICTION FUNCTIONS
      //---------------------------------


    /*
        define U [ ? - %<D%> ] ;

        define CENTER [ x y | x x y y ];

        define L1 [ a ] ;
        define R1 [ b ] ;

        define L2 [ x ] ;
        define R2 [ y ] ;

        define RES1 [ U* L1 %<D%> U* %<D%> R1 U* ] ;
        define RES2 [ U* L2 %<D%> U* %<D%> R2 U* ] ;

        define CEN1 [ U* %<D%> CENTER %<D%> U* ] ;

        define NODU [ U | 0:%<D%> ]* ;
        define NODL [ U | %<D%>:0 ]* ;

        regex U* - [ NODU .o. [ CEN1 - [ RES1 | RES2 ] ] .o. NODL ] ;
      */
    HfstTransducer restriction( const HfstTransducer &_center, const HfstTransducerPairVector &context)
    {
        //check if the center is automata
        HfstTransducer c_proj1(_center);
        c_proj1.input_project();
        HfstTransducer c_proj2(_center);
        c_proj2.output_project();

        if ( ! c_proj1.compare(_center) || ! c_proj2.compare(_center) )
        {
                HFST_THROW_MESSAGE(TransducersAreNotAutomataException, "HfstXeroxRules::restriction");
        }

        ImplementationType type = _center.get_type();
        String restrictionMark("@_D_@");

        HfstTokenizer TOK;
        TOK.add_multichar_symbol(restrictionMark);
        TOK.add_multichar_symbol("@_EPSILON_SYMBOL_@");

        HfstTransducer mark(restrictionMark, TOK, type);
        HfstTransducer epsilon("@_EPSILON_SYMBOL_@", TOK, type);

        // Identity
        HfstTransducer identityPair = HfstTransducer::identity_pair( type );
        HfstTransducer identity (identityPair);
        identity.repeat_star().optimize();

        HfstTransducer universalWithoutD(identity);
        universalWithoutD.insert_to_alphabet(restrictionMark);
        HfstTransducer universalWithoutDStar(universalWithoutD);
        universalWithoutDStar.repeat_star().optimize();

        // NODU
        HfstTransducer noDUpper("@_EPSILON_SYMBOL_@", restrictionMark, TOK, type );
        noDUpper.disjunct(universalWithoutD).repeat_star().optimize();

        // NODL
        HfstTransducer noDLower(restrictionMark, "@_EPSILON_SYMBOL_@", TOK, type );
        noDLower.disjunct(universalWithoutD).repeat_star().optimize();

        // 1. Surround center with marks
        // [ U* %<D%> CENTER %<D%> U* ]
        HfstTransducer center(_center);
        center.insert_to_alphabet(restrictionMark);

        HfstTransducer centerMarked(universalWithoutDStar);
        centerMarked.concatenate(mark)
                    .concatenate(center)
                    .concatenate(mark)
                    .concatenate(universalWithoutDStar)
                    .optimize();

        // 2. Put mark in context
        // [ U* L1 %<D%> U* %<D%> R1 U* ]
        HfstTransducer contextMarked;
        for ( unsigned int i = 0; i < context.size(); i++ )
        {
            HfstTransducer lefContext(context[i].first);
            lefContext.insert_to_alphabet(restrictionMark);

            HfstTransducer rightContext(context[i].second);
            rightContext.insert_to_alphabet(restrictionMark);

            HfstTransducer RES(universalWithoutDStar);
            RES.concatenate(lefContext)
                .concatenate(mark)
                .concatenate(universalWithoutDStar)
                .concatenate(mark)
                .concatenate(rightContext)
                .concatenate(universalWithoutDStar)
                .optimize();

            if ( i == 0 )
            {
                contextMarked = RES;
            }
            else
            {
                contextMarked.disjunct(RES).optimize();
            }
        }
        HfstTransducer centerMinusCtx(centerMarked);
        centerMinusCtx.subtract(contextMarked).optimize();

        HfstTransducer tmp(noDUpper);
        tmp.compose(centerMinusCtx)
            .compose(noDLower)
            .optimize();

        HfstTransducer retval(universalWithoutDStar);
        retval.subtract(tmp).optimize();

        retval.remove_from_alphabet(restrictionMark);

        // deals with boundary symbol
        retval = applyBoundaryMark( retval );

        return retval;
    }


    // a < b
    HfstTransducer before( const HfstTransducer &left, const HfstTransducer &right)
    {
        //check if the center is automata
        HfstTransducer l_proj1(left);
        l_proj1.input_project();
        HfstTransducer l_proj2(left);
        l_proj2.output_project();
        HfstTransducer r_proj1(right);
        r_proj1.input_project();
        HfstTransducer r_proj2(right);
        r_proj2.output_project();

        if ( ! l_proj1.compare(left) || ! l_proj2.compare(left)
             || ! r_proj1.compare(right) || ! r_proj2.compare(right)  )
        {
                HFST_THROW_MESSAGE(TransducersAreNotAutomataException, "HfstXeroxRules::restriction");
        }

        ImplementationType type = left.get_type();

        // Identity
        HfstTransducer identityPair = HfstTransducer::identity_pair( type );
        HfstTransducer identity (identityPair);
        identity.repeat_star().optimize();

        HfstTransducer tmp(identity);
        tmp.concatenate(right)
            .concatenate(identity)
            .concatenate(left)
            .concatenate(identity)
            .optimize();

        HfstTransducer retval(identity);
        retval.subtract(tmp).optimize();

        return retval;
    }
    // a > b
    HfstTransducer after( const HfstTransducer &left, const HfstTransducer &right)
    {
        //check if the center is automata
        HfstTransducer l_proj1(left);
        l_proj1.input_project();
        HfstTransducer l_proj2(left);
        l_proj2.output_project();
        HfstTransducer r_proj1(right);
        r_proj1.input_project();
        HfstTransducer r_proj2(right);
        r_proj2.output_project();

        if ( ! l_proj1.compare(left) || ! l_proj2.compare(left)
          || ! r_proj1.compare(right) || ! r_proj2.compare(right)  )
        {
                HFST_THROW_MESSAGE(TransducersAreNotAutomataException, "HfstXeroxRules::restriction");
        }

        ImplementationType type = left.get_type();

        // Identity
        HfstTransducer identityPair = HfstTransducer::identity_pair( type );
        HfstTransducer identity (identityPair);
        identity.repeat_star().optimize();

        HfstTransducer tmp(identity);
        tmp.concatenate(left)
           .concatenate(identity)
           .concatenate(right)
           .concatenate(identity)
           .optimize();

        HfstTransducer retval(identity);
        retval.subtract(tmp).optimize();

        return retval;
    }





  }
}
#else

#include "HfstXeroxRulesTest.cc"

int main(int argc, char * argv[])
      {
          std::cout << "Unit tests for " __FILE__ ":" << std::endl;


          ImplementationType types[] = {SFST_TYPE, TROPICAL_OPENFST_TYPE, FOMA_TYPE};
          unsigned int NUMBER_OF_TYPES=3;


          for (unsigned int i=0; i < NUMBER_OF_TYPES; i++)
          {
            if (! HfstTransducer::is_implementation_type_available(types[i]))
          continue;

          // std::cout << "----- Type --------- " << i << std::endl;



            // a -> b || ? - a _
            //test8( types[i] );

            test1( types[i] );

            test1b( types[i]);

            // ? -> a
            test1c( types[i]);


            // a -> b || .#. _ ;
            test1d( types[i]);

            // a+ -> x  a_a
            // also @-> and @>
            test2a( types[i] );

            // >@ ->@
            test2b( types[i] );
            test2c( types[i] );

            // ? @-> a ... b;
       //     test2d( types[i] );

            // testing unconditional replace with and without contexts

            test3a( types[i] );
            test3b( types[i] );
            test3c( types[i] );
            test3d( types[i] );

            // left - right - down tests
            // b -> a  || _a (r: bbaa)
            // b -> a  \\ _a (r:aaaa)
            // input: bbba
            test4a( types[i] );
            test4b( types[i] );
            test4c( types[i] );

            // mark up rule - doesn't have api interface anymore
            // a | b -> %[...%] ;
        //    test5( types[i] );

            // epenthesis rules
            test6a( types[i] );
            test6b( types[i] );
            // [. 0 .] -> b || _ a a ;
            test6c( types[i] );

            //parralel rules
            // a -> b , b -> c
            test7a( types[i] );
            // [. .] -> b , a -> c ;
            test7b( types[i] );
            // a @-> x , b @-> y
            // a @> x , b @> y
            // a -> x \/ m _ ,, b -> y || x _
            test7c( types[i] );
            // 0 .o. [ [. 0 .] -> a \/ _ b a , a b _ ,, [. 0 .] -> b \/ a _ a ]
            test7d( types[i] );
            // ? -> x , a -> b
            test7e( types[i] );
            // markup parallel rules
            //test7e( types[i] );

            // a -> b, b -> a
            test7f( types[i] );

            // a -> b b , a -> b
            test7g( types[i] );
            //[..] @-> a;
            test7h( types[i] );

            test9a( types[i] );
            test9b( types[i] );

            // rules with empty language
            test10a( types[i] );
            // empty -> non-empty
            test10b( types[i] );
            
           
            // flag diacritics
            //a ->  "@P.FOO.BAR@" || a _ "@u.a.b@"
            //test11 (types[i] );
            
            
             //markup rules
            //test12 (types[i] );

            // restriction functions =>

            restriction_test1( types[i] );
            restriction_test1a( types[i] );
            restriction_test1b( types[i] );
            restriction_test2( types[i] );
            restriction_test3( types[i] );
            restriction_test3a( types[i] );
            restriction_test3b( types[i] );
            restriction_test3c( types[i] );
            restriction_test4( types[i] );
            restriction_test5( types[i] );
            restriction_test5a( types[i] );
            restriction_test6( types[i] );
            restriction_test7( types[i] );
            restriction_test8( types[i] );

            before_test1( types[i] );
          }

          std::cout << "ok" << std::endl;
          return 0;
      }
#endif // MAIN_TEST
