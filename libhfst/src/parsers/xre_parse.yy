%{
// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#define YYDEBUG 1

#include <stdio.h>
#include <assert.h>
#include <iostream>

#include "HfstTransducer.h"
#include "HfstInputStream.h"
#include "HfstXeroxRules.h"

using namespace hfst;
using hfst::HfstTransducer;
using namespace hfst::xeroxRules;
using namespace hfst::implementations;

#include "xre_utils.h"

namespace hfst {
  namespace xre {
    // number of characters read, used for scanning function definition xre for argument symbols
    extern unsigned int cr;
    extern bool harmonize_;
    extern bool harmonize_flags_;
    extern bool allow_extra_text_at_end;

    bool has_weight_been_zeroed = false; // to control how many times a warning is given
    float zero_weights(float f)
    {
        if ((! has_weight_been_zeroed) && (f != 0))
        {
            hfst::xre::warn("warning: ignoring weights in rule context\n");
            has_weight_been_zeroed = true;
        }
        return 0;
    }

    bool is_weighted()
    {
        return (hfst::xre::format == hfst::TROPICAL_OPENFST_TYPE ||
                hfst::xre::format == hfst::LOG_OPENFST_TYPE);
    }
  }
}

using hfst::xre::harmonize_;
using hfst::xre::harmonize_flags_;

union YYSTYPE;
struct yy_buffer_state;
typedef yy_buffer_state * YY_BUFFER_STATE;
typedef void * yyscan_t;

extern int xreparse(yyscan_t);
extern int xrelex_init (yyscan_t*);
extern YY_BUFFER_STATE xre_scan_string (const char *, yyscan_t);
extern void xre_delete_buffer (YY_BUFFER_STATE, yyscan_t);
extern int xrelex_destroy (yyscan_t);

extern int xreerror(yyscan_t, const char*);
extern int xreerror(const char*);
int xrelex ( YYSTYPE * , yyscan_t );

%}

%name-prefix="xre"
%define api.pure
%lex-param {void * scanner}
%parse-param {void * scanner}
%error-verbose
%debug
  
%union {

    int value;
    int* values;
    double weight;
    char* label;
    
    char *subval1, *subval2;
    
    hfst::HfstTransducer* transducer;
    hfst::HfstTransducerPair* transducerPair;
    hfst::HfstTransducerPairVector* transducerPairVector;
    hfst::HfstTransducerVector* transducerVector;

   std::pair<hfst::xeroxRules::ReplaceArrow, std::vector<hfst::xeroxRules::Rule> >* replaceRuleVectorWithArrow;
   std::pair< hfst::xeroxRules::ReplaceArrow, hfst::xeroxRules::Rule>* replaceRuleWithArrow;
   std::pair< hfst::xeroxRules::ReplaceArrow, hfst::HfstTransducerPairVector>* mappingVectorWithArrow;
   std::pair< hfst::xeroxRules::ReplaceArrow, hfst::HfstTransducerPair>* mappingWithArrow;
       
   std::pair<hfst::xeroxRules::ReplaceType, hfst::HfstTransducerPairVector>* contextWithMark;
   
   hfst::xeroxRules::ReplaceType replType;
   hfst::xeroxRules::ReplaceArrow replaceArrow;

}

/* some parts have been ripped from foma’s parser now */

%type <replaceArrow> REPLACE_ARROW

%type <transducer> XRE REGEXP1 REGEXP2  REGEXP4 REGEXP5 REGEXP6 REGEXP7
                    REGEXP8 REGEXP9 REGEXP10 REGEXP11 REGEXP12 LABEL
                   REPLACE REGEXP3 SUB1  SUB3 SYMBOL_LIST
%type <replaceRuleVectorWithArrow> PARALLEL_RULES
%type <replaceRuleWithArrow>  RULE
%type <mappingVectorWithArrow> MAPPINGPAIR_VECTOR
%type <mappingWithArrow> MAPPINGPAIR

%type <contextWithMark> CONTEXTS_WITH_MARK
%type <transducerPairVector> CONTEXTS_VECTOR RESTR_CONTEXTS_VECTOR
%type <transducerPair> CONTEXT RESTR_CONTEXT
%type <replType>  CONTEXT_MARK
%type <label>     HALFARC SUB2
%type <label>     SYMBOL_OR_QUOTED

%type <transducerVector> REGEXP_LIST   // function call
%type <label> FUNCTION                 // function call

%nonassoc <weight> WEIGHT
%nonassoc <label> SYMBOL MULTICHAR_SYMBOL CURLY_BRACKETS

%left  CROSS_PRODUCT COMPOSITION LENIENT_COMPOSITION INTERSECTION MERGE_RIGHT_ARROW MERGE_LEFT_ARROW
%left  CENTER_MARKER MARKUP_MARKER
%left  SHUFFLE
%right LEFT_RESTRICTION LEFT_ARROW RIGHT_ARROW LEFT_RIGHT_ARROW
%left  REPLACE_RIGHT REPLACE_LEFT
       OPTIONAL_REPLACE_RIGHT OPTIONAL_REPLACE_LEFT
       REPLACE_LEFT_RIGHT OPTIONAL_REPLACE_LEFT_RIGHT
       RTL_LONGEST_MATCH RTL_SHORTEST_MATCH
       LTR_LONGEST_MATCH LTR_SHORTEST_MATCH
      
%right REPLACE_CONTEXT_UU REPLACE_CONTEXT_LU
       REPLACE_CONTEXT_UL REPLACE_CONTEXT_LL

%left  UNION MINUS UPPER_MINUS LOWER_MINUS UPPER_PRIORITY_UNION
       LOWER_PRIORITY_UNION

%left  IGNORING IGNORE_INTERNALLY LEFT_QUOTIENT

%left  COMMACOMMA

%left  COMMA
       
%left  BEFORE AFTER

%nonassoc SUBSTITUTE_LEFT TERM_COMPLEMENT
%nonassoc COMPLEMENT CONTAINMENT CONTAINMENT_ONCE CONTAINMENT_OPT
%nonassoc REVERSE INVERT XRE_UPPER XRE_LOWER STAR PLUS
%nonassoc <values> CATENATE_N_TO_K
%nonassoc <value> CATENATE_N CATENATE_N_PLUS CATENATE_N_MINUS

%nonassoc <label> READ_BIN READ_TEXT READ_SPACED READ_PROLOG READ_RE
%nonassoc <label> FUNCTION_NAME   // function call
%token LEFT_BRACKET RIGHT_BRACKET LEFT_PARENTHESIS RIGHT_PARENTHESIS
       LEFT_BRACKET_DOTTED RIGHT_BRACKET_DOTTED SUBVAL
%token EPSILON_TOKEN ANY_TOKEN BOUNDARY_MARKER
%token LEXER_ERROR
%token END_OF_EXPRESSION
%token PAIR_SEPARATOR
%nonassoc <label> QUOTED_LITERAL QUOTED_MULTICHAR_LITERAL
%%

XRE: REGEXP1 { }
     |
     {
       // only comments
       hfst::xre::contains_only_comments = true;
       return 0;
     }
     ;
REGEXP1: REGEXP2 END_OF_EXPRESSION {
       hfst::xre::last_compiled = & $1->optimize();
       $$ = hfst::xre::last_compiled;
       if (hfst::xre::allow_extra_text_at_end) {
         return 0;
       }
   }
   | REGEXP2 {
        hfst::xre::last_compiled = & $1->optimize();
        $$ = hfst::xre::last_compiled;
   }
;


REGEXP2: REPLACE
         {
            $$ = & $1->optimize();
         }
       | REGEXP2 COMPOSITION REPLACE
       {
        if ($1->has_flag_diacritics() && $3->has_flag_diacritics())
          {
            if (! harmonize_flags_) {
                 hfst::xre::warn("warning: both composition arguments contain flag diacritics that are not harmonized\n");
            }
            else {
                $1->harmonize_flag_diacritics(*$3);
            }
          }

         try {
            $$ = & $1->compose(*$3, harmonize_).optimize();
         }
         catch (const FlagDiacriticsAreNotIdentitiesException & e)
             {
               xreerror("Error: flag diacritics must be identities in composition if flag-is-epsilon is ON.\n"
               "I.e. only FLAG:FLAG is allowed, not FLAG1:FLAG2, FLAG:bar or foo:FLAG\n"
               "Apply twosided flag-diacritics (tfd) before composition.\n");
               YYABORT;
             }
            delete $3;
        }
       | REGEXP2 CROSS_PRODUCT REPLACE {
            $$ = & $1->cross_product(*$3);
            delete $3;
        }
       | REGEXP2 LENIENT_COMPOSITION REPLACE {
            $$ = & $1->lenient_composition(*$3).optimize();
            delete $3;
        }
       | REGEXP2 MERGE_RIGHT_ARROW REPLACE {
          try {
            $$ = hfst::xre::merge_first_to_second($1, $3);
          }
          catch (const TransducersAreNotAutomataException & e)
          {
            xreerror("Error: transducers must be automata in merge operation.");
            delete $1;
            delete $3;
            YYABORT;
          }
          delete $1;
       }
       | REGEXP2 MERGE_LEFT_ARROW REPLACE {
            $$ = hfst::xre::merge_first_to_second($3, $1);
            delete $3;
       }
        // substitute
       | SUB1 HALFARC PAIR_SEPARATOR HALFARC COMMA HALFARC PAIR_SEPARATOR HALFARC RIGHT_BRACKET {
            $1->substitute(StringPair($2,$4), StringPair($6,$8));
            $$ = $1;
       }
       | SUB1 SUB2 SUB3 {

            StringSet alpha = $1->get_alphabet();
            if (hfst::xre::is_definition($2))
            {
                hfst::xre::warn("warning: using definition as an ordinary label, cannot substitute\n");
                $$ = $1;
            }
            else if (alpha.find($2) == alpha.end())
            {
                $$ = $1;
            }
            else
            {
                alpha = $3->get_alphabet();

                StringPair tmp($2, $2);
                HfstTransducer * tmpTr = new HfstTransducer(*$1);

	        bool empty_replace_transducer=false;
	        HfstTransducer empty(hfst::xre::format);
	        if (empty.compare(*$3))
	        {
                        empty_replace_transducer=true;
	        }

	        if (empty_replace_transducer)
	        {
                        // substitute all transitions {b:a, a:b, b:b} with b:b
		        // as they will be removed anyway
		        hfst::xre::set_substitution_function_symbol($2);
		        tmpTr->substitute(&hfst::xre::substitution_function);
	        }

                // `[ a:b, b, x y ]
                // substitute b with x | y
                tmpTr->substitute(tmp, *$3, false); // no harmonization

	        if (!empty_replace_transducer)
                {
                        // a:b .o. b -> x | y
                        // [[a:b].i .o. b -> x | y].i - this is for cases when b is on left side

	                // build Replace transducer
                        HfstTransducerPair mappingPair(HfstTransducer($2, $2, hfst::xre::format), *$3);
                        HfstTransducerPairVector mappingPairVector;
                        mappingPairVector.push_back(mappingPair);
                        Rule rule(mappingPairVector);
                        HfstTransducer replaceTr(hfst::xre::format);
                        replaceTr = replace(rule, false);

                        // if we are replacing with flag diacritics, the rule must allow
                        // flags to be replaced with themselves
                        StringSet alpha = $3->get_alphabet();
                        for (StringSet::const_iterator it = alpha.begin(); it != alpha.end(); it++)
                        {
                          if (FdOperation::is_diacritic(*it))
                          {
                            replaceTr.insert_freely(StringPair(*it, *it), false);
                          }
                        }
                        replaceTr.optimize();

                        tmpTr->compose(replaceTr).optimize();
                        tmpTr->invert().compose(replaceTr).invert();
	        }
            
                if (alpha.find($2) == alpha.end())
                {
                  tmpTr->remove_from_alphabet($2);
                }
                tmpTr->optimize();
                $$ = tmpTr;
                delete($1, $2, $3);
            }
         }
        ;

SUB1: SUBSTITUTE_LEFT LEFT_BRACKET REPLACE COMMA { $$ = $3; } ; // first argument
SUB2: HALFARC COMMA { $$ = $1; } ;  // symbol that needs to be replaced
SUB3: SYMBOL_LIST RIGHT_BRACKET {  $$ = $1;  }  // symbol list
      |
      RIGHT_BRACKET { $$ = new HfstTransducer(hfst::xre::format); } // an empty symbol list
      ;

////////////////////////////
// Replace operators
///////////////////////////

REPLACE : REGEXP3 { }
       |  PARALLEL_RULES
         {
            switch ( $1->first )
            {
               case E_REPLACE_RIGHT:
                 $$ = new HfstTransducer( replace( $1->second, false ) );
                 break;
               case E_OPTIONAL_REPLACE_RIGHT:
                 $$ = new HfstTransducer( replace( $1->second, true ) );
                 break;
              case E_REPLACE_LEFT:
                 $$ = new HfstTransducer( replace_left( $1->second, false ) );
                 break;
               case E_OPTIONAL_REPLACE_LEFT:
                 $$ = new HfstTransducer( replace_left( $1->second, true ) );
                 break;
               case E_RTL_LONGEST_MATCH:
                 $$ = new HfstTransducer( replace_rightmost_longest_match( $1->second ) );
                 break;
               case E_RTL_SHORTEST_MATCH:
                 $$ = new HfstTransducer( replace_rightmost_shortest_match($1->second) );
                 break;
               case E_LTR_LONGEST_MATCH:
                 $$ = new HfstTransducer( replace_leftmost_longest_match( $1->second ) );
                 break;
               case E_LTR_SHORTEST_MATCH:
                 $$ = new HfstTransducer( replace_leftmost_shortest_match( $1->second ) );
                 break;
               case E_REPLACE_RIGHT_MARKUP:
               default:
                xreerror("Unhandled arrow stuff I suppose");
                YYABORT;
                break;
            }
       
            delete $1;
         }
      ;

PARALLEL_RULES: PARALLEL_RULES COMMACOMMA RULE
         {
           //std::cerr << "parallel_rules: parallel_rules ,, rule"<< std::endl;
           if ($3->first != $1->first)
           {
             xreerror("Replace type mismatch in parallel rules");
             YYABORT;
           }
            Rule tmpRule($3->second);
            $1->second.push_back(tmpRule);
            $$ =  new std::pair< ReplaceArrow, std::vector<Rule> > ($3->first, $1->second);
            delete $3;
         }
         | RULE
         {
         //std::cerr << "parallel_rules:rule"<< std::endl;
            std::vector<Rule> * ruleVector = new std::vector<Rule>();
            ruleVector->push_back($1->second);
            
            $$ =  new std::pair< ReplaceArrow, std::vector<Rule> > ($1->first, *ruleVector);
            delete $1;
         }
         ;

RULE: MAPPINGPAIR_VECTOR
      {
         // std::cerr << "rule: mapping_vector"<< std::endl;
        // HfstTransducer allMappingsDisjuncted = disjunctVectorMembers($1->second);
         
         Rule rule( $1->second );;
         $$ =  new std::pair< ReplaceArrow, Rule> ($1->first, rule);
         delete $1;
      }
      | MAPPINGPAIR_VECTOR CONTEXTS_WITH_MARK
      {
       //  std::cerr << "rule: mapping_vector contextsWM"<< std::endl;
     //   HfstTransducer allMappingsDisjuncted = disjunctVectorMembers($1->second);
        
        Rule rule( $1->second, $2->second, $2->first );
        $$ =  new std::pair< ReplaceArrow, Rule> ($1->first, rule);
        delete $1, $2;
      }
      ;
      
// Mappings: ( ie. a -> b , c -> d , ... , g -> d)
MAPPINGPAIR_VECTOR: MAPPINGPAIR_VECTOR COMMA MAPPINGPAIR
      {
        // std::cerr << "mapping_vector : mapping_vector comma mapping"<< std::endl;
         // check if new Arrow is the same as the first one

         if ($1->first != $3->first)
         {
            hfst::xre::warn("Replace arrows should be the same. Calculated as if all replacements had the first arrow.");
            //exit(1);
         }
 
         $1->second.push_back($3->second);
         $$ =  new std::pair< ReplaceArrow, HfstTransducerPairVector> ($1->first, $1->second);
         delete $3;
            
      }
      
      | MAPPINGPAIR
      {
         // std::cerr << "mapping_vector : mapping"<< std::endl;
         HfstTransducerPairVector * mappingPairVector = new HfstTransducerPairVector();
         mappingPairVector->push_back( $1->second );
         $$ =  new std::pair< ReplaceArrow, HfstTransducerPairVector> ($1->first, * mappingPairVector);
         delete $1;
      }
     
      ;

    
MAPPINGPAIR: REPLACE REPLACE_ARROW REPLACE
      {
	  hfst::xre::warn_about_special_symbols_in_replace($1);
	  hfst::xre::warn_about_special_symbols_in_replace($3);
          HfstTransducerPair mappingPair(*$1, *$3);
          $$ =  new std::pair< ReplaceArrow, HfstTransducerPair> ($2, mappingPair);

          delete $1, $3;
      }
      | REPLACE REPLACE_ARROW REPLACE MARKUP_MARKER REPLACE
      {
          HfstTransducerPair marks(*$3, *$5);
          HfstTransducerPair tmpMappingPair(*$1, HfstTransducer(hfst::xre::format));
          HfstTransducerPair mappingPair = create_mapping_for_mark_up_replace( tmpMappingPair, marks );
          
          $$ =  new std::pair< ReplaceArrow, HfstTransducerPair> ($2, mappingPair);
          delete $1, $3, $5;
      }
      | REPLACE REPLACE_ARROW REPLACE MARKUP_MARKER
      {
          HfstTransducer epsilon(hfst::internal_epsilon, hfst::xre::format);
          HfstTransducerPair marks(*$3, epsilon);
          HfstTransducerPair tmpMappingPair(*$1, HfstTransducer(hfst::xre::format));
          HfstTransducerPair mappingPair = create_mapping_for_mark_up_replace( tmpMappingPair, marks );
                   
          $$ =  new std::pair< ReplaceArrow, HfstTransducerPair> ($2, mappingPair);
          delete $1, $3;
      }
      | REPLACE REPLACE_ARROW MARKUP_MARKER REPLACE
      {
          HfstTransducer epsilon(hfst::internal_epsilon, hfst::xre::format);
          HfstTransducerPair marks(epsilon, *$4);
          HfstTransducerPair tmpMappingPair(*$1, HfstTransducer(hfst::xre::format));
          HfstTransducerPair mappingPair = create_mapping_for_mark_up_replace( tmpMappingPair, marks );
          
          $$ =  new std::pair< ReplaceArrow, HfstTransducerPair> ($2, mappingPair);
          delete $1, $4;
      }
       | LEFT_BRACKET_DOTTED RIGHT_BRACKET_DOTTED REPLACE_ARROW REPLACE
      {
          HfstTransducer epsilon(hfst::internal_epsilon, hfst::xre::format);
          //HfstTransducer mappingTr(epsilon);
          //mappingTr.cross_product(*$4);
          HfstTransducerPair mappingPair(epsilon, *$4);
          
          $$ =  new std::pair< ReplaceArrow, HfstTransducerPair> ($3, mappingPair);
          delete $4;
      }
      | LEFT_BRACKET_DOTTED REPLACE RIGHT_BRACKET_DOTTED REPLACE_ARROW REPLACE
      {
	  HfstTransducerPair mappingPair(*$2, *$5);
          $$ =  new std::pair< ReplaceArrow, HfstTransducerPair> ($4, mappingPair);
          delete $2, $5;
      }
      
       | REPLACE REPLACE_ARROW LEFT_BRACKET_DOTTED RIGHT_BRACKET_DOTTED
      {
          HfstTransducer epsilon(hfst::internal_epsilon, hfst::xre::format);
          HfstTransducerPair mappingPair(*$1, epsilon);
          
          $$ =  new std::pair< ReplaceArrow, HfstTransducerPair> ($2, mappingPair);
          delete $1;
      }
      | REPLACE REPLACE_ARROW LEFT_BRACKET_DOTTED REPLACE RIGHT_BRACKET_DOTTED
      {
          HfstTransducerPair mappingPair(*$1, *$4);
          $$ =  new std::pair< ReplaceArrow, HfstTransducerPair> ($2, mappingPair);
          delete $1, $4;
      }
      
      ;

// Contexts: ( ie. || k _ f , ... , f _ s )
CONTEXTS_WITH_MARK:  CONTEXT_MARK CONTEXTS_VECTOR
         {
         $$ =  new std::pair< ReplaceType, HfstTransducerPairVector> ($1, *$2);
         }
         ;
CONTEXTS_VECTOR: CONTEXT
         {
            HfstTransducerPairVector * ContextVector = new HfstTransducerPairVector();
            ContextVector->push_back(*$1);
            $$ = ContextVector;
            delete $1;
         }

      | CONTEXTS_VECTOR COMMA CONTEXT
         {
            $1->push_back(*$3);
            $$ = $1;
            delete $3;
         }
      
      ;
CONTEXT: REPLACE CENTER_MARKER REPLACE
         {
            if (hfst::xre::has_non_identity_pairs($1)) // if non-identity symbols present..
            {
              xreerror("Contexts need to be automata");
              YYABORT;
            }
            if (hfst::xre::has_non_identity_pairs($3)) // if non-identity symbols present..
            {
              xreerror("Contexts need to be automata");
              YYABORT;
            }
            
            HfstTransducer t1(*$1);
            HfstTransducer t2(*$3);

             if (hfst::xre::is_weighted())
             {
               hfst::xre::has_weight_been_zeroed=false;
               t1.transform_weights(&hfst::xre::zero_weights);
             }
             t1.optimize().prune_alphabet(false);

             if (hfst::xre::is_weighted())
             {
               t2.transform_weights(&hfst::xre::zero_weights);
               hfst::xre::has_weight_been_zeroed=false;
             }
             t2.optimize().prune_alphabet(false);

            $$ = new HfstTransducerPair(t1, t2);
            delete $1, $3;
         }
      | REPLACE CENTER_MARKER
         {
            if (hfst::xre::has_non_identity_pairs($1)) // if non-identity symbols present..
            {
              xreerror("Contexts need to be automata");
              YYABORT;
            }

            HfstTransducer t1(*$1);
            
            if (hfst::xre::is_weighted())
            {
              hfst::xre::has_weight_been_zeroed=false;
              t1.transform_weights(&hfst::xre::zero_weights);
              hfst::xre::has_weight_been_zeroed=false;
            }
            t1.optimize().prune_alphabet(false);

            HfstTransducer epsilon(hfst::internal_epsilon, hfst::xre::format);
            $$ = new HfstTransducerPair(t1, epsilon);
            delete $1;
         }
      | CENTER_MARKER REPLACE
         {

            if (hfst::xre::has_non_identity_pairs($2)) // if non-identity symbols present..
            {
              xreerror("Contexts need to be automata");
              YYABORT;
            }
            
            HfstTransducer t1(*$2);

            if (hfst::xre::is_weighted())
            {
              hfst::xre::has_weight_been_zeroed=false;
              t1.transform_weights(&hfst::xre::zero_weights);
              hfst::xre::has_weight_been_zeroed=false;
            }
            t1.optimize().prune_alphabet(false);
             
            HfstTransducer epsilon(hfst::internal_epsilon, hfst::xre::format);
            $$ = new HfstTransducerPair(epsilon, t1);
            delete $2;
         }
       | CENTER_MARKER
          {
            HfstTransducer epsilon(hfst::internal_epsilon, hfst::xre::format);
            $$ = new HfstTransducerPair(epsilon, epsilon);
          }
      ;
 CONTEXT_MARK: REPLACE_CONTEXT_UU
         {
            $$ = REPL_UP;
         }
         | REPLACE_CONTEXT_LU
         {
            $$ = REPL_RIGHT;
         }
         | REPLACE_CONTEXT_UL
         {
            $$ = REPL_LEFT;
         }
         | REPLACE_CONTEXT_LL
         {
            $$ = REPL_DOWN;
         }
         ;
     
REPLACE_ARROW: REPLACE_RIGHT
         {
            $$ = E_REPLACE_RIGHT;
         }
         | OPTIONAL_REPLACE_RIGHT
         {
            $$ = E_OPTIONAL_REPLACE_RIGHT;
         }
         | RTL_LONGEST_MATCH
          {
            $$ = E_RTL_LONGEST_MATCH;
         }
         | RTL_SHORTEST_MATCH
          {
            $$ = E_RTL_SHORTEST_MATCH;
         }
         | LTR_LONGEST_MATCH
          {
            $$ = E_LTR_LONGEST_MATCH;
         }
         | LTR_SHORTEST_MATCH
          {
            $$ = E_LTR_SHORTEST_MATCH;
         }
         |  REPLACE_LEFT
         {
        	 $$ =  E_REPLACE_LEFT;
         }
         |   OPTIONAL_REPLACE_LEFT
         {
        	 $$ = E_OPTIONAL_REPLACE_LEFT;
         }
         ;

////////////////
REGEXP3: REGEXP4 { }
       | REGEXP3 SHUFFLE REGEXP4 {
            xreerror("No shuffle");
            //$$ = $1;
            delete $3;
            YYABORT;
        }
       | REGEXP3 BEFORE REGEXP4 {
            $$ = new HfstTransducer( before (*$1, *$3) );
            delete $1, $3;
        }
       | REGEXP3 AFTER REGEXP4 {
            $$ = new HfstTransducer( after (*$1, *$3) );
            delete $1, $3;
        }

       ;
  
REGEXP4: REGEXP5 { }
        // restriction rule
       | REGEXP4 RIGHT_ARROW RESTR_CONTEXTS_VECTOR {
            $$ = new HfstTransducer( restriction(*$1, *$3) ) ;
            delete $1;
            delete $3;
        }
       // doesn't exist in xfst
       | REGEXP4 LEFT_ARROW REGEXP5 CENTER_MARKER REGEXP5 {
            xreerror("No Arrows");
            //$$ = $1;
            delete $3;
            delete $5;
            YYABORT;
        }
       // doesn't exist in xfst
       | REGEXP4 LEFT_RIGHT_ARROW REGEXP5 CENTER_MARKER REGEXP5 {
            xreerror("No Arrows");
            //$$ = $1;
            delete $3;
            delete $5;
            YYABORT;
        }
       ;

RESTR_CONTEXTS_VECTOR: RESTR_CONTEXT
         {
            HfstTransducerPairVector * ContextVector = new HfstTransducerPairVector();
            ContextVector->push_back(*$1);
            $$ = ContextVector;
            delete $1;
         }

      | RESTR_CONTEXTS_VECTOR COMMA RESTR_CONTEXT
         {
            $1->push_back(*$3);
            $$ = $1;
            delete $3;
         }
      
      ;
      
RESTR_CONTEXT: REGEXP4 CENTER_MARKER REGEXP4
         {
            $$ = new HfstTransducerPair(*$1, *$3);
            delete $1, $3;
         }
      | REGEXP4 CENTER_MARKER
         {
           // std::cerr << "Mapping: \n" << *$1  << std::endl;
            
            HfstTransducer epsilon(hfst::internal_epsilon, hfst::xre::format);
            
           // std::cerr << "Epsilon: \n" << epsilon  << std::endl;
            $$ = new HfstTransducerPair(*$1, epsilon);
            delete $1;
         }
      | CENTER_MARKER REGEXP4
         {
            HfstTransducer epsilon(hfst::internal_epsilon, hfst::xre::format);
            $$ = new HfstTransducerPair(epsilon, *$2);
            delete $2;
         }
      | CENTER_MARKER
         {
            HfstTransducer empty(hfst::xre::format);
            $$ = new HfstTransducerPair(empty, empty);
         }
      ;


REGEXP5: REGEXP6 { }
       | REGEXP5 UNION REGEXP6 {
            $$ = & $1->disjunct(*$3, harmonize_);
            delete $3;
        }
       | REGEXP5 INTERSECTION REGEXP6 {
        // std::cerr << "Intersection: \n"  << std::endl;
            $$ = & $1->intersect(*$3, harmonize_).optimize().prune_alphabet(false);
            delete $3;
        }
       | REGEXP5 MINUS REGEXP6 {
            $$ = & $1->subtract(*$3, harmonize_).prune_alphabet(false);
            delete $3;
        }
       | REGEXP5 UPPER_MINUS REGEXP6 {
            xreerror("No upper minus");
            //$$ = $1;
            delete $3;
            YYABORT;
        }
       | REGEXP5 LOWER_MINUS REGEXP6 {
            xreerror("No lower minus");
            //$$ = $1;
            delete $3;
            YYABORT;
        }
       | REGEXP5 UPPER_PRIORITY_UNION REGEXP6 {
            $$ = & $1->priority_union(*$3);
            delete $3;
        }
       | REGEXP5 LOWER_PRIORITY_UNION REGEXP6 {
            HfstTransducer* left = new HfstTransducer(*$1);
            HfstTransducer* right =  new HfstTransducer(*$3);
            right->invert();
            left->invert();
            $$ = & (left->priority_union(*right).invert());
            delete $1, $3;
        }
       ;

REGEXP6: REGEXP7 { }
       | REGEXP6 REGEXP7 {
        $$ = & $1->concatenate(*$2, harmonize_);
        delete $2;
        }
       ;

REGEXP7: REGEXP8 { }
       | REGEXP7 IGNORING REGEXP8 {
            // this is how ignoring is done in foma and xfst
            $1->harmonize(*$3, true /*force harmonization also for foma type*/);
            $$ = & $1->insert_freely(*$3, false); // no harmonization
            delete $3;
        }
       | REGEXP7 IGNORE_INTERNALLY REGEXP8 {
            xreerror("No ignoring internally");
            //$$ = $1;
            delete $3;
            YYABORT;
        }
       | REGEXP7 LEFT_QUOTIENT REGEXP8 {
            xreerror("No left quotient");
            //$$ = $1;
            delete $3;
            YYABORT;
        }
       ;

REGEXP8: REGEXP9 { }
       | COMPLEMENT REGEXP8 {
       		// TODO: forbid pair complement (ie ~a:b)
       		HfstTransducer complement = HfstTransducer::identity_pair( hfst::xre::format );
       		complement.repeat_star().optimize();
       		complement.subtract(*$2).prune_alphabet(false);
       		$$ = new HfstTransducer(complement);
   			delete $2;
        }
       | CONTAINMENT REGEXP8 {
            // std::cerr << "Containment: \n" << std::endl;
            if (hfst::xre::has_non_identity_pairs($2)) // if non-identity symbols present..
            {
              hfst::xre::warn("warning: using transducer that is non an automaton in containment\n");
              $$ = hfst::xre::contains($2); // ..resort to simple containment
            }
            else
            {
              $$ = hfst::xre::contains_with_weight($2, 0);
            }
            delete $2;
        }
       | CONTAINMENT WEIGHT REGEXP8 {
            // std::cerr << "Containment: \n" << std::endl;
            if (hfst::xre::has_non_identity_pairs($3)) // if non-identity symbols present..
            {
              xreerror("Containment with weight only works with automata");
              YYABORT;
            }
            $$ = hfst::xre::contains_with_weight($3, $2);
            delete $3;
        }
       | CONTAINMENT_ONCE REGEXP8 {
            //std::cerr << "Contain 1 \n"<< std::endl;

            $$ = hfst::xre::contains_once($2);
            delete $2;
        }
       | CONTAINMENT_OPT REGEXP8 {
            $$ = hfst::xre::contains_once_optional($2);
            delete $2;
        }
       ;

REGEXP9: REGEXP10 { }
       | REGEXP9 STAR {
            $$ = & $1->repeat_star();
        }
       | REGEXP9 PLUS {
            $$ = & $1->repeat_plus();
        }
       | REGEXP9 REVERSE {
            $$ = & $1->reverse();
        }
       | REGEXP9 INVERT {
            $$ = & $1->invert();
        }
       | REGEXP9 XRE_UPPER {
            $$ = & $1->input_project();
        }
       | REGEXP9 XRE_LOWER {
            $$ = & $1->output_project();
        }
       | REGEXP9 CATENATE_N {
            $$ = & $1->repeat_n($2);
        }
       | REGEXP9 CATENATE_N_PLUS {
            //std::cerr << "value is ::::: \n"<< $2 << std::endl;
            $$ = & $1->repeat_n_plus($2+1);
        }
       | REGEXP9 CATENATE_N_MINUS {
            $$ = & $1->repeat_n_minus($2-1);
        }
       | REGEXP9 CATENATE_N_TO_K {
            $$ = & $1->repeat_n_to_k($2[0], $2[1]);
            free($2);
        }
       ;

REGEXP10: REGEXP11 { }
       | TERM_COMPLEMENT REGEXP10 {
            HfstTransducer* any = new HfstTransducer(hfst::internal_identity,
                                        hfst::xre::format);
            $$ = & ( any->subtract(*$2));
            delete $2;
        }
        /*
       | SUBSTITUTE_LEFT REGEXP10 COMMA REGEXP10 COMMA REGEXP10 RIGHT_BRACKET {
            xreerror("no substitute");
            //$$ = $2;
            YYABORT;
        }
        */
       ;

REGEXP11: REGEXP12 { }
        | LEFT_BRACKET REGEXP2 RIGHT_BRACKET {
            $$ = & $2->optimize();
        }
        // [foo]:[bar]
        | LEFT_BRACKET REGEXP2 RIGHT_BRACKET PAIR_SEPARATOR LEFT_BRACKET REGEXP2 RIGHT_BRACKET {
            $$ = & $2->cross_product(*$6);
            delete $6;
        }
        // [foo]:{bar}
        | LEFT_BRACKET REGEXP2 RIGHT_BRACKET PAIR_SEPARATOR CURLY_BRACKETS {
     	    HfstTransducer * tmp = hfst::xre::xfst_curly_label_to_transducer($5,$5);
            free($5);
            $$ = & $2->cross_product(*tmp);
            delete tmp;
        }
        // {foo}:[bar]
        | CURLY_BRACKETS PAIR_SEPARATOR LEFT_BRACKET REGEXP2 RIGHT_BRACKET {
     	    HfstTransducer * tmp = hfst::xre::xfst_curly_label_to_transducer($1,$1);
            free($1);
            $$ = & $4->cross_product(*tmp);
            delete tmp;
        }
        // [foo]:bar
        | LEFT_BRACKET REGEXP2 RIGHT_BRACKET PAIR_SEPARATOR HALFARC {
            HfstTransducer * tmp = hfst::xre::expand_definition($5);
            free($5);
            $$ = & $2->cross_product(*tmp);
            delete tmp;
        }
        // foo:[bar]
        | HALFARC PAIR_SEPARATOR LEFT_BRACKET REGEXP2 RIGHT_BRACKET {
            $$ = hfst::xre::expand_definition($1);
            free($1);
            $$ = & $$->cross_product(*$4);
            delete $4;
        }
        | LEFT_BRACKET REGEXP2 RIGHT_BRACKET WEIGHT {
            $$ = & $2->set_final_weights($4, true).optimize();
        }
        | LEFT_PARENTHESIS REGEXP2 RIGHT_PARENTHESIS {
            $$ = & $2->optionalize();
        }
        ;

// building 3rd argument in the substitute list
SYMBOL_LIST: HALFARC {
            if (strcmp($1, hfst::internal_unknown.c_str()) == 0)
              {
                $$ = new HfstTransducer(hfst::internal_identity, hfst::xre::format);
              }
            else
              {
                $$ = new HfstTransducer($1, $1, hfst::xre::format);
              }
            free($1);
        }
        | SYMBOL_LIST HALFARC {
            HfstTransducer * tmp ;
            if (strcmp($2, hfst::internal_unknown.c_str()) == 0)
              {
                 tmp = new HfstTransducer(hfst::internal_identity, hfst::xre::format);
              }
            else
              {
                 tmp = new HfstTransducer($2, $2, hfst::xre::format);
              }

            $1->disjunct(*tmp, false); // do not harmonize
            $$ = & $1->optimize();
            delete $2, tmp;
            }
        ;

REGEXP12: LABEL { }
        | LABEL WEIGHT {
            $$ = & $1->set_final_weights($2, true);
        }
        | READ_BIN {
            try {
              hfst::HfstInputStream instream($1);
              $$ = new HfstTransducer(instream);
              instream.close();
              free($1);
            }
            catch (const HfstException & e) {
              (void) e; // todo handle the exception
              char msg [256];
              sprintf(msg, "Error reading transducer file '%s'.", $1);
              xreerror(msg);
              YYABORT;
            }
        }
        | READ_TEXT {
            FILE * f = NULL;
            f = fopen($1, "r");
            if (f == NULL) {
              xreerror("File cannot be opened.\n");
              YYABORT;
            }
            else {
              HfstBasicTransducer tmp;
              HfstTokenizer tok;
              char line [1000];

              while( fgets(line, 1000, f) != NULL )
              {
                hfst::xre::strip_newline(line);
                StringPairVector spv = tok.tokenize(line);
                tmp.disjunct(spv, 0);
              }
              fclose(f);
              HfstTransducer * retval = new HfstTransducer(tmp, hfst::xre::format);
              retval->optimize();
              $$ = retval;
            }
        }
        | READ_SPACED {
            FILE * f = NULL;
            f = fopen($1, "r");
            if (f == NULL) {
              xreerror("File cannot be opened.\n");
              YYABORT;
            }
            else {
              HfstTokenizer tok;
              HfstBasicTransducer tmp;
              char line [1000];

              while( fgets(line, 1000, f) != NULL )
              {
                hfst::xre::strip_newline(line);
                StringPairVector spv = HfstTokenizer::tokenize_space_separated(line);
                tmp.disjunct(spv, 0);
              }
              fclose(f);
              HfstTransducer * retval = new HfstTransducer(tmp, hfst::xre::format);
              retval->optimize();
              $$ = retval;
            }
        }
        | READ_PROLOG {
            FILE * f = NULL;
            f = fopen($1, "r");
            if (f == NULL) {
              xreerror("File cannot be opened.\n");
              YYABORT;
            }
            else {
              try {
                unsigned int linecount = 0;
                HfstBasicTransducer tmp = HfstBasicTransducer::read_in_prolog_format(f, linecount);
                fclose(f);
                HfstTransducer * retval = new HfstTransducer(tmp, hfst::xre::format);
                retval->optimize();
                $$ = retval;
              }
              catch (const HfstException & e) {
                (void) e; // todo handle the exception
                fclose(f);
                xreerror("Error reading prolog file.\n");
                YYABORT;
              }
            }
        }
        | READ_RE {
            FILE * f = NULL;
            f = fopen($1, "r");
            if (f == NULL) {
              xreerror("File cannot be opened.\n");
              fclose(f);
              YYABORT;
            }
            else {
              fclose(f);
              // read the regex in a string
              std::ifstream ifs($1);
              std::stringstream buffer;
              buffer << ifs.rdbuf();
              char * regex_string = strdup(buffer.str().c_str());

              // create a new scanner for evaluating the regex
              yyscan_t scanner;
              xrelex_init(&scanner);
              YY_BUFFER_STATE bs = xre_scan_string(regex_string, scanner);

              unsigned int chars_read = hfst::xre::cr;
              hfst::xre::cr = 0;

              int parse_retval = xreparse(scanner);

              xre_delete_buffer(bs,scanner);
              xrelex_destroy(scanner);

              free(regex_string);

              hfst::xre::cr = chars_read;

              $$ = hfst::xre::last_compiled;

              if (parse_retval != 0)
              {
                xreerror("Error parsing regex.\n");
                YYABORT;
              }
            }
        }
        ;

LABEL: HALFARC {
        if (strcmp($1, hfst::internal_unknown.c_str()) == 0)
          {
            $$ = new HfstTransducer(hfst::internal_identity, hfst::xre::format);
          }
        else
          {
            // HfstTransducer * tmp = new HfstTransducer($1, hfst::xre::format);
	    // $$ = hfst::xre::expand_definition(tmp, $1);
            $$ = hfst::xre::expand_definition($1);
          }
        free($1);
     }
     |
     HALFARC PAIR_SEPARATOR HALFARC {
     	$$ = hfst::xre::xfst_label_to_transducer($1,$3);
        free($1);
        free($3);
     }
     | HALFARC PAIR_SEPARATOR CURLY_BRACKETS {
        $$ = hfst::xre::xfst_label_to_transducer($1,$1);
        free($1);
        HfstTransducer * tmp = hfst::xre::xfst_curly_label_to_transducer($3,$3);
        free($3);
        $$ = & $$->cross_product(*tmp);
        delete tmp;
     }
     | CURLY_BRACKETS PAIR_SEPARATOR HALFARC {
        HfstTransducer * tmp = hfst::xre::xfst_label_to_transducer($3,$3);
        free($3);
        $$ = hfst::xre::xfst_curly_label_to_transducer($1,$1);
        free($1);
        $$ = & $$->cross_product(*tmp);
        delete tmp;
     }
     | CURLY_BRACKETS {
     	$$ = hfst::xre::xfst_curly_label_to_transducer($1,$1);
        free($1);
     }
     | CURLY_BRACKETS PAIR_SEPARATOR CURLY_BRACKETS {
     	$$ = hfst::xre::xfst_curly_label_to_transducer($1,$3);
        free($1);
	free($3);
     }
        // function call
       | FUNCTION REGEXP_LIST RIGHT_PARENTHESIS {
            if (! hfst::xre::is_valid_function_call($1, $2)) {
              return EXIT_FAILURE;
            }
            else {
              // create a new scanner for evaluating the function
              yyscan_t scanner;
              xrelex_init(&scanner);
              YY_BUFFER_STATE bs = xre_scan_string(hfst::xre::get_function_xre($1),scanner);

              // define special variables so that function arguments get the values given in regexp list
              if (! hfst::xre::define_function_args($1, $2))
              {
                xreerror("Could not define function args.\n");  // TODO: more informative message
                YYABORT;
              }

              // if we are scanning a function definition for argument symbols,
              // do not include the characters read when evaluating functions inside it
              unsigned int chars_read = hfst::xre::cr;

              int parse_retval = xreparse(scanner);

              hfst::xre::cr = chars_read;
              hfst::xre::undefine_function_args($1);

              xre_delete_buffer(bs,scanner);
              xrelex_destroy(scanner);

              $$ = hfst::xre::last_compiled;

              if (parse_retval != 0)
              {
                YYABORT;
              }
            }
        }
     /*
     | QUOTE {
        HfstTokenizer TOK;
        $$ = new HfstTransducer($1, TOK, hfst::xre::format);
        free($1);
     }
     */
     ;

SYMBOL_OR_QUOTED: SYMBOL
     | MULTICHAR_SYMBOL {
       hfst::xre::check_multichar_symbol($1);
       $$ = $1;
     }
     | QUOTED_MULTICHAR_LITERAL {
       hfst::xre::check_multichar_symbol($1);
       $$ = $1;
     }
     | QUOTED_LITERAL
     ;

HALFARC: SYMBOL_OR_QUOTED
     {
       // Symbols of form <foo> are not harmonized in xfst, that is why
       // they need to be escaped as @_<foo>_@.
       // $$ = hfst::xre::escape_enclosing_angle_brackets($1);
       hfst::xre::warn_about_hfst_special_symbol($1);
       hfst::xre::warn_about_xfst_special_symbol($1);
       $$ = $1;
     }
     | EPSILON_TOKEN {
        $$ = strdup(hfst::internal_epsilon.c_str());
     }
     | ANY_TOKEN {
        $$ = strdup(hfst::internal_unknown.c_str());
     }
     | BOUNDARY_MARKER {
        $$ = strdup("@#@");
     }
     ;

REGEXP_LIST: REGEXP_LIST COMMA REGEXP2 {
       $$->push_back(*($3));
       delete $3;
     }
     | REGEXP2 {
       $$ = new hfst::HfstTransducerVector();
       $$->push_back(*($1));
       delete $1;
     }
     ;

FUNCTION: FUNCTION_NAME {
        $$ = strdup($1);
    };

%%

