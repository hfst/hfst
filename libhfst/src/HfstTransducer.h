// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#ifndef _HFST_TRANSDUCER_H_
#define _HFST_TRANSDUCER_H_

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "FormatSpecifiers.h"

#include "HfstDataTypes.h"
#include "HfstSymbolDefs.h"
#include "parsers/LexcCompiler.h"

#if HAVE_SFST
#include "implementations/SfstTransducer.h"
#endif

#if HAVE_OPENFST
#include "implementations/TropicalWeightTransducer.h"
#include "implementations/LogWeightTransducer.h"
#endif

#if HAVE_FOMA
#include "implementations/FomaTransducer.h"
#endif

#if HAVE_XFSM
#include "implementations/XfsmTransducer.h"
#endif

/* Include here the header file of the files that take care
   of the interaction between HFST and your transducer library. */
//#if HAVE_MY_TRANSDUCER_LIBRARY
//#include "implementations/MyTransducerLibraryTransducer.h"
//#endif

#include "implementations/HfstOlTransducer.h"
#include "HfstTokenizer.h"
#include "implementations/ConvertTransducerFormat.h"
#include "HfstExceptionDefs.h"
#include "HfstInputStream.h"
#include "HfstOutputStream.h"

#include <string>
#include <cassert>
#include <iostream>
#include <vector>
#include <map>
#include <set>

#include "hfstdll.h"

/** @file HfstTransducer.h
    \brief Declarations of HFST API functions and datatypes.

    This file must be included to a program that uses the HFST API. */

/** \brief A namespace for HFST functions and datatypes. */
namespace hfst
{
  namespace implementations {
    class HfstBasicTransducer;
  }
  class HfstCompiler;
  class HfstTransducer;
  namespace xfst {
    class XfstCompiler;
  }
  namespace xeroxRules
  {
      class Rule;
      HfstTransducer bracketedReplace( const Rule &rule, bool optional);
  }

  using hfst::implementations::HfstOlTransducer;

#if HAVE_SFST
  using hfst::implementations::SfstTransducer;
#endif // #if HAVE_SFST

#if HAVE_OPENFST
  using hfst::implementations::TropicalWeightTransducer;
#if HAVE_OPENFST_LOG
  using hfst::implementations::LogWeightTransducer;
#endif // #if HAVE_OPENFST_LOG
#endif // #if HAVE_OPENFST

#if HAVE_FOMA
  using hfst::implementations::FomaTransducer;
#endif // #if HAVE_FOMA

#if HAVE_XFSM
  using hfst::implementations::XfsmTransducer;
#endif // #if HAVE_XFSM

  /* Add here the transducer class of your transducer library. */
  //#if HAVE_MY_TRANSDUCER_LIBRARY
  //  using hfst::implementations::MyTransducerLibraryTransducer;
  //#endif // #if HAVE_MY_TRANSDUCER_LIBRARY

  // *** TESTING AND OPTIMIZATION...

#if HAVE_XFSM
  HFSTDLL void initialize_xfsm();

  class InitializeXfsm
  {
  public:
    HFSTDLL InitializeXfsm();
  };
#endif

  enum MinimizationAlgorithm { HOPCROFT, BRZOZOWSKI };
  /* Which minimization algorithm is used.
     In foma, Hopcroft is always used.
     In OpenFst and SFST, the default algorithm is Hopcroft. */
  HFSTDLL void set_minimization_algorithm(MinimizationAlgorithm);
  HFSTDLL MinimizationAlgorithm get_minimization_algorithm();

  HFSTDLL void set_encode_weights(bool);
  HFSTDLL bool get_encode_weights();

  HFSTDLL void set_minimize_even_if_already_minimal(bool);
  HFSTDLL bool get_minimize_even_if_already_minimal();

  HFSTDLL void set_minimization(bool);
  HFSTDLL bool get_minimization();

  HFSTDLL void set_xerox_composition(bool);
  HFSTDLL bool get_xerox_composition();

  HFSTDLL void set_flag_is_epsilon_in_composition(bool);
  HFSTDLL bool get_flag_is_epsilon_in_composition();

  /* Whether in harmonization the smaller transducer is always harmonized
     according to the bigger transducer. */
  HFSTDLL void set_harmonize_smaller(bool);
  HFSTDLL bool get_harmonize_smaller();

  /* Whether unknown and identity symbols are used. By default, they are used.
     These symbols are always reserved for use and included in alphabets
     of transducers, but code optimization is possible if it is known
     that they do not appear in transducer transitions. */
  HFSTDLL void set_unknown_symbols_in_use(bool);
  HFSTDLL bool get_unknown_symbols_in_use();

  void set_warning_stream(std::ostream * os);
  std::ostream * get_warning_stream();

  // *** ...TESTING AND OPTIMIZATION


  /** \brief A synchronous finite-state transducer.

    \section argument_handling Argument handling

    Transducer functions modify their calling object and return
    a reference to the calling object after modification,
    unless otherwise mentioned.
    Transducer arguments are usually not modified.
\verbatim
    // transducer is reversed
    transducer.reverse();
    // transducer2 is not modified, but a copy of it is disjuncted with
    // transducer1
    transducer1.disjunct(transducer2);
    // a chain of functions is possible
    transducer.reverse().determinize().reverse().determinize();
\endverbatim

    \section implementation_types Implementation types

    Currently, an HfstTransducer has four implementation types as
    defined by the enumeration ImplementationType.
    When an HfstTransducer is created, its type is defined with an
    ImplementationType argument.
    For functions that take a transducer as an argument, the type of
    the calling transducer
    must be the same as the type of the argument transducer:
\verbatim
    // this will cause an error
    log_transducer.disjunct(sfst_transducer);
    // this works, but weights are lost in the conversion
    log_transducer.convert(SFST_TYPE).disjunct(sfst_transducer);
    // this works, information is not lost
    log_transducer.disjunct(sfst_transducer.convert(LOG_OPENFST_TYPE));
\endverbatim

    \section creating_transducers Creating transducers

    With HfstTransducer constructors it is possible to create empty,
    epsilon, one-transition and single-path transducers.
    Transducers can also be created from scratch with HfstBasicTransducer
    and converted to an HfstTransducer.
    More complex transducers can be combined from simple ones with various
    functions.
    
    <a name="symbols"></a>
    \section special_symbols Special symbols

    The HFST transducers support transitions with epsilon, unknown
    and identity symbols.
    The special symbols are explained in documentation of datatype #String.

An example:
\verbatim

  // In the xerox formalism used here, "?" means the unknown symbol
  // and "?:?" the identity pair

  HfstBasicTransducer tr1;
  tr1.add_state(1);
  tr1.set_final_weight(1, 0);
  tr1.add_transition
    (0, HfstBasicTransition(1, "@_UNKNOWN_SYMBOL_@", "foo", 0) );

  // tr1 is now [ ?:foo ]
  
  HfstBasicTransducer tr2;
  tr2.add_state(1);
  tr2.add_state(2);
  tr2.set_final_weight(2, 0);
  tr2.add_transition
    (0, HfstBasicTransition(1, "@_IDENTITY_SYMBOL_@",
                    "@_IDENTITY_SYMBOL_@", 0) );
  tr2.add_transition
    (1, HfstBasicTransition(2, "bar", "bar", 0) );

  // tr2 is now [ [ ?:? ] [ bar:bar ] ]

  ImplementationType type = SFST_TYPE;
  HfstTransducer Tr1(tr1, type);
  HfstTransducer Tr2(tr2, type);
  Tr1.disjunct(Tr2);

  // Tr1 is now [ [ ?:foo | bar:foo ]  |  [[ ?:? | foo:foo ] [ bar:bar ]] ]

\endverbatim

  */
  class HfstTransducer
  {

    // ***** PROTECTED VARIABLES AND INTERFACE *****

  protected:

    /* The backend implementation type of the transducer */
    ImplementationType type;

    bool anonymous;    // currently not used
    bool is_trie;      // currently not used
    std::string name;  /* The name of the transducer */
    std::map<std::string,std::string> props;    // rest of fst metadata
    /* The union of possible backend implementations. */
    union TransducerImplementation
    {
#if HAVE_SFST
      hfst::implementations::Transducer * sfst;
#endif
#if HAVE_OPENFST
      hfst::implementations::StdVectorFst * tropical_ofst;
#if HAVE_OPENFST_LOG
      hfst::implementations::LogFst * log_ofst;
#endif
#endif
#if HAVE_FOMA
      fsm * foma;
#endif
#if HAVE_XFSM
      NETptr xfsm;
#endif

      /* Add here your own transducer backend implementation. */
      //#if HAVE_MY_TRANSDUCER_LIBRARY
      //      hfst::implementations::MyFst * my_transducer_library;
      //#endif

      hfst_ol::Transducer * hfst_ol;

#if HAVE_OPENFST // is this needed?
      hfst::implementations::StdVectorFst * internal;
#endif
    };

    /* The backend implementation */
    TransducerImplementation implementation;

    /* Interfaces through which the backend implementations can be accessed */
#if HAVE_SFST
    static hfst::implementations::SfstTransducer sfst_interface;
#endif
#if HAVE_OPENFST
    static hfst::implementations::TropicalWeightTransducer
      tropical_ofst_interface;
#if HAVE_OPENFST_LOG
    static hfst::implementations::LogWeightTransducer log_ofst_interface;
#endif
#endif
#if HAVE_FOMA
    static hfst::implementations::FomaTransducer foma_interface;
#endif
    static hfst::implementations::HfstOlTransducer hfst_ol_interface;
#if HAVE_XFSM
    static hfst::implementations::XfsmTransducer xfsm_interface;
#endif

    /* Add here the class that takes care of the interaction between
       HFST and your transducer library. */
    //#if HAVE_MY_TRANSDUCER_LIBRARY
    //static hfst::implementations::MyTransducerLibraryTransducer
    //  my_transducer_library_interface;
    //#endif

    /* The same as harmonize but does not change \a another but
       returns a harmonized version of that. If this and \a another
       have type FOMA_TYPE, NULL is returned. */
    HfstTransducer * harmonize_(const HfstTransducer &another);

    HfstTransducer * harmonize_symbol_encodings(const HfstTransducer &another);

    /* Check if transducer \a another has in its alphabet flag diacritics
       that are not found in the alphabet of this transducer and insert
       all missing flag diacritics to \a missing_flags.
       \a return_on_first_miss defines whether the function returns as soon as
       a missing flag is found so that only that flag is inserted to
       \a missing flags. */
    bool check_for_missing_flags_in(const HfstTransducer &another,
                                    StringSet &missing_flags,
                                    bool return_on_first_miss) const;

    /* Disjunct trie transducers efficiently so that the result is also
       a trie.
       Currently not implemented, TODO */
    HfstTransducer &disjunct_as_tries(HfstTransducer &another,
                                      ImplementationType type);


    /* Remove paths where @..._2@ transitions immediately preceede
       @..._1@ transitions, i.e. transitions resulting from incorrect
       ordering of flag diacritics. */
    HfstTransducer &remove_illegal_flag_paths(void);


    /* For internal use */
    static HfstTransducer &read_in_att_format
      (FILE *ifile, ImplementationType type, const std::string &epsilon_symbol);

  public:
    /* Whether the conversion requested can be done without losing
       any information */
    static bool is_safe_conversion(ImplementationType original,
                                   ImplementationType conversion);

    /* whether HFST is linked to the transducer library
       needed by implementation type \a type. */
    HFSTDLL static bool is_implementation_type_available(ImplementationType type);

    HFSTDLL unsigned int number_of_states() const;
    HFSTDLL unsigned int number_of_arcs() const;

    HFSTDLL void twosided_flag_diacritics();

  protected:
    /* For internal use: Create a tokenizer that recognizes all symbols
       that occur in the transducer. */
    HfstTokenizer create_tokenizer();

    /* For internal use. Implemented only for SFST_TYPE.
       Get all symbol pairs that occur in the transitions of the transducer. */
    StringPairSet get_symbol_pairs();

  protected:

    /* Get the number used to represent the symbol \a symbol. */
    unsigned int get_symbol_number(const std::string &symbol);
    
    /* For internal use, implemented only for SFST_TYPE. */
    std::vector<HfstTransducer*> extract_path_transducers();

    /* For internal use:
       Create a new transducer equivalent to \a t in format \a type. */
    static HfstTransducer &convert
      (const HfstTransducer &t, ImplementationType type);

    /* For internal use:
       Create an HfstBasicTransducer equivalent to \a t end delete
       the backend implementation of \a t. */
    implementations::HfstBasicTransducer * convert_to_basic_transducer();

    /* For internal use:
       Create an HfstBasicTransducer equivalent to \a t. */
    implementations::HfstBasicTransducer * get_basic_transducer() const;

    /* For internal use:
       Create a backend implementation of the same type that this transducer
       has and that is equivalent to \a t and delete \a t. Assign the
       backend implementation as the value of the implementation of this
       transducer. */
    HfstTransducer &convert_to_hfst_transducer
      (implementations::HfstBasicTransducer *t);

    /* \brief For internal use: Create a transducer of type \a type as
       defined in AT&T format in file named \a filename.
       \a epsilon_symbol defines how epsilons are represented.

        NOTE: If the transition symbols contain space characters
        they must be represented as "@_SPACE_@" because
        whitespace characters are used as field separators
        in AT&T format.

       @pre The file exists, otherwise an exception is thrown.
       @see HfstTransducer(FILE, ImplementationType, const std::string&)
       @throws StreamNotReadableException
       @throws NotValidAttFormatException
    */
    static HfstTransducer &read_in_att_format
      (const std::string &filename, ImplementationType type,
       const std::string &epsilon_symbol);

  public:
    HFSTDLL static HfstTransducer * prolog_file_to_xfsm_transducer(const char * filename);


    /* For debugging */
  public:
    HFSTDLL void print_alphabet();
  protected:
    static float get_profile_seconds(ImplementationType type);

#include "hfst_apply_schemas.h"



    // ***** THE PUBLIC INTERFACE *****

  public:


    // ------------------------------------------------
    // ----- Constructors, destructor, assignment -----
    // ------------------------------------------------

    /** \brief Create an uninitialized transducer (use with care).
        
        @note This constructor leaves the backend implementation variable
        uninitialized. An uninitialized transducer is likely to cause a
        TransducerHasWrongTypeException at some point unless it is given
    a value at some point. */
    HFSTDLL HfstTransducer();

    /** \brief Create an empty transducer, i.e. a transducer that does not
        recognize any string. The type of the transducer is defined by \a type.

        @note Use HfstTransducer("@_EPSILON_SYMBOL_@")
        to create an epsilon transducer.
     **/
    HFSTDLL HfstTransducer(ImplementationType type);

    /** \brief Create a transducer by tokenizing the utf8 string \a utf8_string
        with tokenizer \a multichar_symbol_tokenizer.
        The type of the transducer is defined by \a type.

        \a utf8_str is read one token at a time and for each token
        a new transition is created in the resulting
        transducer. The input and output symbols of that transition are
        the same as the token read.

        An example:
\verbatim
       std::string ustring = "foobar";
       HfstTokenizer TOK;
       HfstTransducer tr(ustring, TOK, LOG_OPENFST_TYPE);
       // tr now contains one path [f o o b a r]
\endverbatim

        @see HfstTokenizer **/
    HFSTDLL HfstTransducer(const std::string& utf8_str,
           const HfstTokenizer &multichar_symbol_tokenizer,
                   ImplementationType type);

    /** \brief Create a transducer by tokenizing
        the utf8 input string \a input_utf8_string
        and output string \a output_utf8_string
        with tokenizer \a multichar_symbol_tokenizer.
        The type of the transducer is defined by \a type.

        \a input_utf8_str and \a output_utf8_str are read one token at a time
        and for each token a new transition
        is created in the resulting transducer. The input and output symbols
        of that transition are the same as
        the input and output tokens read. If either string contains less tokens
        than another, epsilons are used
        as transition symbols for the shorter string.

        An example:
\verbatim
       std::string input = "foo";
       std::string output = "barr";
       HfstTokenizer TOK;
       HfstTransducer tr(input, output, TOK, SFST_TYPE);
       // tr now contains one path [f:b o:a o:r 0:r]
\endverbatim

        @see HfstTokenizer **/
    HFSTDLL HfstTransducer(const std::string& input_utf8_str,
                   const std::string& output_utf8_str,
                   const HfstTokenizer &multichar_symbol_tokenizer,
                   ImplementationType type);

    /* @brief Create a transducer that recognizes the union of string pairs in
       \a sps. The type of the transducer is defined by \a type. \a cyclic
       defines whether the transducer recognizes any number (from zero to
       infinity, inclusive) of consecutive string pairs in \s sps. */
    HFSTDLL HfstTransducer(const StringPairSet & sps, ImplementationType type,
                   bool cyclic=false);

    /* \brief Create a transducer that recognizes the concatenation of
       string pairs in \a spv. The type of the transducer is defined
       by \a type. */
    HFSTDLL HfstTransducer(const StringPairVector & spv, ImplementationType type);

    HFSTDLL HfstTransducer(const StringVector & sv, ImplementationType type);

    /* \brief Create a transducer that recognizes the concatenation of the
       unions of string pairs in string pair sets in \a spsv. The type of
       the transducer is defined by \a type. */
    HFSTDLL HfstTransducer(const std::vector<StringPairSet> & spsv,
                   ImplementationType type);

    /** \brief Read a binary transducer from transducer stream \a in.

        The stream can contain HFST tranducers or OpenFst, foma or SFST
        transducers without an HFST header. If the backend implementations
        are used as such, they are converted into HFST transducers.

        For more information on transducer conversions and the HFST header
        structure, see
        <a href="https://kitwiki.csc.fi/twiki/bin/view/KitWiki/HfstTransducerHeader">here</a>.

        @pre ( in.is_eof() == in.is_bad() == false && in.is_fst() ).
        Otherwise, an exception is thrown.

        @throws NotTransducerStreamException
        @throws StreamNotReadableException
        @throws StreamIsClosedException
        @throws TransducerTypeMismatchException
        @throws MissingOpenFstInputSymbolTableException
  
        @see HfstInputStream **/
    HFSTDLL HfstTransducer(HfstInputStream &in);

    /** \brief Create a deep copy of transducer \a another. **/
    HFSTDLL HfstTransducer(const HfstTransducer &another);

    /** \brief Create an HFST transducer equivalent to
        HFST basic transducer \a t. The type of the created transducer
        is defined by \a type.  **/
    HFSTDLL HfstTransducer(const hfst::implementations::HfstBasicTransducer &t,
                   ImplementationType type);

    /** \brief Create a transducer that recognizes the string pair
        &lt;"symbol","symbol"&gt;, i.e. [symbol:symbol].
        The type of the transducer is defined by \a type.
        @see String **/
    HFSTDLL HfstTransducer(const std::string &symbol, ImplementationType type);

    /** \brief Create a transducer that recognizes the string pair
        &lt;"isymbol","osymbol"&gt;, i.e [isymbol:osymbol].
        The type of the transducer is defined by \a type.
        @see String **/
    HFSTDLL HfstTransducer(const std::string &isymbol, const std::string &osymbol,
                           ImplementationType type);

    /** \brief Create a transducer of type \a type as defined in AT&T format
        in FILE \a ifile. \a epsilon_symbol defines how epsilons
        are represented.
        
        In AT&T format, the transition lines are of the form:

\verbatim
        [0-9]+[\w]+[0-9]+[\w]+[^\w]+[\w]+[^\w]([\w]+(-)[0-9]+(\.[0-9]+))
\endverbatim

        and final state lines:

\verbatim
        [0-9]+[\w]+([\w]+(-)[0-9]+(\.[0-9]+))
\endverbatim

        If several transducers are listed in the same file,
        they are separated by lines of
        two consecutive hyphens "--". If the weight
        (<tt>([\\w]+(-)[0-9]+(\.[0-9]+))</tt>)
        is missing, the transition or final state is given a zero weight.

        NOTE: If transition symbols contains spaces, they must be escaped
        as "@_SPACE_@" because spaces are used as field separators.
        Both "@0@" and "@_EPSILON_SYMBOL_@" are always interpreted as
        epsilons.


An example:
\verbatim
0      1      foo      bar      0.3
1      0.5
--
0      0.0
--
--
0      0.0
0      0      a        <eps>    0.2
\endverbatim

        The example lists four transducers in AT&T format:
        one transducer accepting the string pair &lt;"foo","bar"&gt;, one
        epsilon transducer, one empty transducer and one transducer
        that accepts any number of 'a's and produces an empty string
        in all cases. The transducers
        can be read with the following commands (from a file named
        "testfile.att"):
\verbatim
std::vector<HfstTransducer> transducers;
FILE * ifile = fopen("testfile.att", "rb");
try {
  while (not eof(ifile))
    {
    HfstTransducer t(ifile, TROPICAL_OPENFST_TYPE, "<eps>");
    transducers.push_back(t);
    printf("read one transducer\n");
    }
} catch (NotValidAttFormatException e) {
    printf("Error reading transducer: not valid AT&T format.\n"); }
fclose(ifile);
fprintf(stderr, "Read %i transducers in total.\n", (int)transducers.size());
\endverbatim

Epsilon will be represented as "@_EPSILON_SYMBOL_@" in the resulting transducer.
The argument \a epsilon_symbol only denotes how epsilons are represented
in \a ifile.

@throws NotValidAttFormatException
@throws StreamNotReadableException
@throws StreamIsClosedException
@see #write_in_att_format(FILE*,bool)const
@see String
**/
    HFSTDLL HfstTransducer(FILE * ifile, ImplementationType type,
                   const std::string &epsilon_symbol, unsigned int & linecount);

    HFSTDLL HfstTransducer(FILE * ifile, ImplementationType type,
                   const std::string &epsilon_symbol);


    /** \brief Destructor. **/
    HFSTDLL virtual ~HfstTransducer(void);

    /** @brief Assign this transducer a new value equivalent to transducer
        \a another. */
    HFSTDLL HfstTransducer &operator=(const HfstTransducer &another);

    HFSTDLL HfstTransducer &assign(const HfstTransducer &another);

    // ------------------------------------------------------------
    // ----------- Properties, comparison, conversion -------------
    // ------------------------------------------------------------

    /** \brief Rename the transducer \a name.
        @see get_name */
    HFSTDLL void set_name(const std::string &name);

    /** \brief Get the name of the transducer.
        @see set_name */
    HFSTDLL std::string get_name() const;

    /**
     * @brief Set arbitrary string property @a property to @a value.
     *        set_property("name") equals @c set_name(string&).
     * @note  While this function is capable of creating endless amounts of
     *        arbitrary metadata, it is suggested that property names are
     *        drawn from central repository, or prefixed with "x-". A property
     *        that does not follow this convention may affect the behavior of
     *        transducer in future releases.
     */
    HFSTDLL void set_property(const std::string& property, const std::string& value);
    
    /**
     * @brief Get arbitrary string propert @a property.
     *        get_property("name") works like get_name.
     */
    HFSTDLL std::string get_property(const std::string& property) const;
    /**
     *  @brief Get all properties form transducer.
     */
    HFSTDLL const std::map<std::string,std::string>& get_properties() const;
    /** \brief Get the alphabet of the transducer.
    
    The alphabet is defined as the set of symbols known
    to the transducer. */
    HFSTDLL StringSet get_alphabet() const;

    /** \brief Get first input level symbols of strings recognized
        (or rejected, if they end in a non-final state) by the transducer. */
    HFSTDLL StringSet get_first_input_symbols() const;

    /** \brief Harmonize transducers this and another.

       @note In harmonization,
       the symbol-to-number correspondencies of this transducer are recoded
       so that they are equivalent to the ones used in transducer another.
       Then the unknown and identity symbols are expanded
       in both transducers. If this and \a another have type FOMA_TYPE,
       nothing is done, since foma takes care of harmonization. */
    HFSTDLL void harmonize(HfstTransducer &another, bool force=false);

    /** \brief Explicitly insert \a symbol to the alphabet
    of the transducer.

    @note Usually this function is not needed since new symbols are
    added to the alphabet by default. */
    HFSTDLL void insert_to_alphabet(const std::string &symbol);
    HFSTDLL void insert_to_alphabet(const std::set<std::string> &symbols);

    /** \brief Remove \a symbol from the alphabet of the transducer.
    CURRENTLY NOT IMPLEMENTED.

    @pre \a symbol does not occur in any transition of the transducer.
    @note Use with care, removing a symbol that occurs in a transition
    of the transducer can have unexpected results. */
    HFSTDLL void remove_from_alphabet(const std::string &symbol);
    HFSTDLL void remove_from_alphabet(const std::set<std::string> &symbols);

    // For XFSM format
    HFSTDLL void remove_symbols_from_alphabet(const StringSet & symbols);


    /** @brief Remove all symbols that do not occur in transitions of
        the transducer from its alphabet.

        If unknown or identity symbols occur in transitions of the transducer,
        pruning is not carried out by default.

        @param force Whether unused symbols are removed even if
        unknown or identity symbols occur in transitions.

        Epsilon, unknown and identity \link hfst::String symbols\endlink
        are always included in the alphabet. */
    HFSTDLL HfstTransducer &prune_alphabet(bool force=true);

    /** \brief Whether the transducer is cyclic. */
    HFSTDLL bool is_cyclic(void) const;

    /** \brief Whether the transducer is an automaton. */
    HFSTDLL bool is_automaton(void) const;

    /** \brief The implementation type of the transducer. */
    HFSTDLL ImplementationType get_type(void) const;

    /** \brief Whether this transducer and \a another are equivalent.

        Two transducers are equivalent iff they accept the same input/output
        string pairs with the same weights
        and the same alignments.
    */
    HFSTDLL bool compare(const HfstTransducer &another, bool harmonize=true) const;

    /** \brief Convert the transducer into an equivalent transducer
        in format \a type.

        If a weighted transducer is converted into an unweighted one,
        all weights are lost.
        In the reverse case, all weights are initialized to the
        semiring's one.

        A transducer of type #SFST_TYPE, #TROPICAL_OPENFST_TYPE,
        #LOG_OPENFST_TYPE or #FOMA_TYPE can be converted into an
        #HFST_OL_TYPE or #HFST_OLW_TYPE transducer, but an #HFST_OL_TYPE
        or #HFST_OLW_TYPE transducer cannot be converted to any other type.

        @note For conversion between implementations::HfstTransitionGraph and HfstTransducer,
        see HfstTransducer(const hfst::implementations::HfstBasicTransducer&, ImplementationType) and #hfst::implementations::HfstTransitionGraph::HfstTransitionGraph(const hfst::HfstTransducer&).
    */
    HFSTDLL HfstTransducer &convert(ImplementationType type, std::string options="");


    // --------------------------------------------------------
    // --- String lookup and conversion to/from AT&T format ---
    // --------------------------------------------------------

    /** \brief Write the transducer in AT&T format to FILE \a ofile.
        \a write_weights defines whether weights are written.

        The fields in the resulting AT&T format are separated
        by tabulator characters.

        NOTE: If the transition symbols contain space characters,
        the spaces are printed as "@_SPACE_@" because
        whitespace characters are used as field separators
        in AT&T format. Epsilon symbols are printed as "@0@".


        If several transducers are written in the same file, they must
        be separated by a line of two consecutive hyphens "--", so that
        they will be read correctly by
        HfstTransducer(FILE*, ImplementationType, const std::string&).

An example:
\verbatim
ImplementationType type = FOMA_TYPE;
HfstTransducer foobar("foo","bar",type);
HfstTransducer epsilon("@_EPSILON_SYMBOL_@",type);
HfstTransducer empty(type);
HfstTransducer a_star("a",type);
a_star.repeat_star();

FILE * ofile = fopen("testfile.att", "wb");
foobar.write_in_att_format(ofile);
fprintf(ofile, "--\n");
epsilon.write_in_att_format(ofile);
fprintf(ofile, "--\n");
empty.write_in_att_format(ofile);
fprintf(ofile, "--\n");
a_star.write_in_att_format(ofile);
fclose(ofile);
\endverbatim

This will yield a file "testfile.att" that looks as follows:
\verbatim
0    1    foo  bar  0.0
1    0.0
--
0    0.0
--
--
0    0.0
0    0    a    a    0.0
\endverbatim

        @throws StreamCannotBeWrittenException
        @throws StreamIsClosedException

        @see operator<<(std::ostream &out, const HfstTransducer &t)
        @see HfstTransducer(FILE*, ImplementationType, const std::string&) */
    HFSTDLL void write_in_att_format(FILE * ofile, bool write_weights=true) const;

    HFSTDLL void write_in_att_format(char * buffer, bool write_weights=true) const;

    /* For XFSM_TYPE. */
    HFSTDLL void write_xfsm_transducer_in_att_format(const char * filename) const;
    HFSTDLL void write_xfsm_transducer_in_prolog_format(const char * filename) const;

    /* For other types. */
    HFSTDLL void write_in_prolog_format(FILE * file, const std::string & name,
                                bool write_weights=true);

    HFSTDLL void write_in_att_format_number
      (FILE * ofile, bool write_weights=true) const;


    /** \brief \brief Write the transducer in AT&T format to FILE
        named \a filename. \a write_weights
        defines whether weights are written.

        If the file exists, it is overwritten.
        If the file does not exist, it is created.

        @see #write_in_att_format */
    HFSTDLL void write_in_att_format(const std::string &filename,
                                     bool write_weights=true) const;


  public:
    /* \brief Call \a callback with some or all string pairs recognized
       by the transducer?

        If the callback returns false the search will be terminated.
        The \a cycles parameter
        indicates how many times a cycle will be followed, with negative numbers
        indicating unlimited. Note that if the transducer is cyclic and
        cycles aren't capped,
        the search will not end until the callback returns false. */
    HFSTDLL void extract_paths(ExtractStringsCb& callback, int cycles=-1) const;

    /** \brief Extract a maximum of \a max_num paths that are
        recognized by the transducer following a maximum of \a cycles cycles
        and store the paths into \a results.

        @param results The extracted paths are inserted here.
        @param max_num The total number of resulting strings is capped at
                       \a max_num, with 0 or negative indicating unlimited.
        @param cycles Indicates how many times a cycle will be followed, with
                      negative numbers indicating unlimited.

        This is a version of extract_paths that handles flag diacritics
        as ordinary symbols and does not validate the sequences prior to
        outputting as opposed to
        #extract_paths_fd(HfstTwoLevelPaths &, int, int, bool) const.
 
        If this function is called on a cyclic transducer with unlimited
        values for both \a max_num and \a cycles, an exception will be thrown.

        This example

\verbatim
    ImplementationType type = SFST_TYPE;
    HfstTransducer tr1("a", "b", type);
    tr1.repeat_star();
    HfstTransducer tr2("c", "d", type);
    tr2.repeat_star();
    tr1.concatenate(tr2).minimize();
    HfstTwoLevelPaths results;
    tr1.extract_paths(results, MAX_NUM, CYCLES);

    // Go through all paths.
    for (HfstTwoLevelPaths::const_iterator it = results.begin();
         it != results.end(); it++)
      {
        std::string istring;
        std::string ostring;

        for (StringPairVector::const_iterator IT = it->second.begin();
             IT != it->second.end(); IT++)
          {
            istring.append(IT->first);
            ostring.append(IT->second);
          }
        // Print input and output strings of each path
        std::cerr << istring << ":" << ostring;
        // and optionally the weight of the path.
        //std::cerr << "\t" << it->first;
        std::cerr << std::endl;
      }
\endverbatim

        prints with values MAX_NUM == -1 and CYCLES == 1 all paths
        that have no consecutive cycles:

\verbatim
a : b
ac : bd
acc : bdd
c : d
cc : dd
\endverbatim

        and with values MAX_NUM == 7 and CYCLES == 2 a maximum of 7 paths
        that follow a cycle a maximum of 2 times (there are 11 such paths,
        but MAX_NUM limits their number to 7):

\verbatim
a : b
aa : bb
aac : bbd
aacc : bbdd
c : d
cc : dd
ccc : ddd
\endverbatim

        @bug Does not work for HFST_OL_TYPE or HFST_OLW_TYPE?
        @throws TransducerIsCyclicException
        @see #n_best
        @see hfst::HfstTransducer::extract_paths_fd(hfst::HfstTwoLevelPaths&, int, int, bool) const
    */
    HFSTDLL void extract_paths
      (HfstTwoLevelPaths &results, int max_num=-1, int cycles=-1) const;

    HFSTDLL void extract_random_paths
      (HfstTwoLevelPaths &results, int max_num) const;

    HFSTDLL void extract_random_paths_fd
      (HfstTwoLevelPaths &results, int max_num, bool filter_fd) const;

    /* \brief Call \a callback with extracted strings that are not
       invalidated by flag diacritic rules.

       @see extract_paths(HfstTwoLevelPaths&, int, int) */
    HFSTDLL void extract_paths_fd
      (ExtractStringsCb& callback, int cycles=-1, bool filter_fd=true) const;

    // todo: handle flag diacritics
    // todo: throw TransducerIsCyclicException, if cyclic
    HFSTDLL void extract_shortest_paths
      (HfstTwoLevelPaths &results) const;
    
    HFSTDLL bool extract_longest_paths
      (HfstTwoLevelPaths &results, bool obey_flags=true /*,bool show_flags=false*/) const;

    HFSTDLL int longest_path_size(bool obey_flags=true) const;

  public:
    /** \brief Extract a maximum of \a max_num paths that are
        recognized by the transducer and are not invalidated by flag
        diacritic rules following a maximum of \a cycles cycles
        and store the paths into \a results. \a filter_fd defines whether
        the flag diacritics themselves are filtered out of the result strings.

        @param results  The extracted paths are inserted here.
        @param max_num  The total number of resulting strings is capped at
                        \a max_num, with 0 or negative indicating unlimited.
        @param cycles  Indicates how many times a cycle will be followed, with
                       negative numbers indicating unlimited.
        @param filter_fd  Whether the flag diacritics are filtered out of the
                          result strings.

        If this function is called on a cyclic transducer with unlimited
        values for both \a max_num and \a cycles, an exception will be thrown.

        Flag diacritics are of the form @[PNDRCU][.][A-Z]+([.][A-Z]+)?@.
        
        For example the transducer

\verbatim
[[@P.FEATURE.FOO@ foo] | [@P.FEATURE.BAR@ bar]]  |  [[foo @U.FEATURE.FOO@] | [bar @U.FEATURE.BAR@]]
\endverbatim

        will yield the paths <CODE>[foo foo]</CODE> and <CODE>[bar bar]</CODE>.
        <CODE>[foo bar]</CODE> and <CODE>[bar foo]</CODE> are invalidated
        by the flag diacritics so thay will not be included in \a results.


        @bug Does not work for HFST_OL_TYPE or HFST_OLW_TYPE?
        @throws TransducerIsCyclicException
        @see extract_paths(HfstTwoLevelPaths&, int, int) const */
    HFSTDLL void extract_paths_fd
      (HfstTwoLevelPaths &results, int max_num=-1, int cycles=-1,
       bool filter_fd=true) const;

    //! @brief Lookup or apply a single tokenized string \a s and
    //! return a maximum of \a limit results.
    //!
    //! This is a version of lookup that handles flag diacritics as ordinary
    //! symbols and does not validate the sequences prior to outputting.
    //! Currently, this function calls lookup_fd.
    //!
    //! @todo Handle flag diacritics as ordinary symbols instead of calling
    //!       lookup_fd.
    //! @sa lookup_fd
    HFSTDLL HfstOneLevelPaths * lookup(const StringVector& s,
                                       ssize_t limit = -1,
                                       double time_cutoff = 0.0) const;

    //! @brief Lookup or apply a single string \a s and
    //! return a maximum of \a limit results.
    //!
    //! This is an overloaded lookup function that leaves tokenizing to the
    //! transducer.
    HFSTDLL HfstOneLevelPaths * lookup(const std::string & s,
                                       ssize_t limit = -1,
                                       double time_cutoff = 0.0) const;

    //! @brief Lookup or apply a single string \a s and
    //! return a maximum of \a limit results in string pair format.
    //!
    HFSTDLL HfstTwoLevelPaths * lookup_pairs(const std::string & s,
                                             ssize_t limit = -1,
                                             double time_cutoff = 0.0) const;

    //! @brief Lookup or apply a single string \a s minding flag diacritics
    //! properly and store a maximum of \a limit results to \a results.
    //!
    //! Traverse all paths on logical first level of the transducer to produce
    //! all possible outputs on the second.
    //! This is in effect a fast composition of single path from left
    //! hand side.
    //!
    //! This is a version of lookup that handles flag diacritics as epsilons
    //! and validates the sequences prior to outputting.
    //! Epsilons on the second level are represented by empty strings
    //! in \a results. For an example of flag diacritics, see
    //! #hfst::HfstTransducer::extract_paths_fd(hfst::HfstTwoLevelPaths&, int, int, bool) const
    //!
    //!
    //! @pre The transducer must be of type #HFST_OL_TYPE or #HFST_OLW_TYPE.
    //!      This function is not implemented for other transducer types.
    //!
    //! @param s  String to look up. The weight is ignored.
    //! @param limit  (Currently ignored.) Number of strings to look up.
    //!               -1 tries to look up all and may get stuck
    //!               if infinitely ambiguous.
    //! @param time_cutoff Number of seconds that can pass before lookup is stopped.
    //! \return{A pointer to a HfstOneLevelPaths container allocated by callee}
    //!
    //! @see HfstTokenizer::tokenize_one_level
    //! @see is_lookup_infinitely_ambiguous(const StringVector&) const
    //!
    //! @todo Do not ignore argument \a limit.
    //!
    HFSTDLL HfstOneLevelPaths * lookup_fd(const StringVector& s,
                                          ssize_t limit = -1,
                                          double time_cutoff = 0.0) const;

    //! @brief Lookup or apply a single string \a s minding flag diacritics
    //! properly and store a maximum of \a limit results to \a results.
    //!
    //! This is an overloaded lookup_fd that leaves tokenizing to the
    //! transducer.
    //!
    //! \warning{This function will convert the transducer into
    //! HFST_OLW_TYPE which may be very slow for large transducers. Lookup
    //! speed can therefore be *extremely* slow.}
    //!
    //! @param s  String to look up. The weight is ignored.
    //! @param limit  (Currently ignored.) Number of strings to look up.
    //!               -1 tries to look up all and may get stuck
    //!               if infinitely ambiguous.
    //! @param time_cutoff Number of seconds that can pass before lookup is stopped.
    //! \return{A pointer to a HfstOneLevelPaths container allocated by callee}
    //!
    //!
    //!@sa lookup_fd
    HFSTDLL HfstOneLevelPaths * lookup_fd(const std::string& s,
                                          ssize_t limit = -1,
                                          double time_cutoff = 0.0) const;

    //! @brief Lookup or apply a single string \a s and store a maximum of
    //! \a limit results to \a results. \a tok defined how \a s is tokenized.
    //!
    //!
    //! This function is the same as
    //! #lookup(const StringVector&, ssize_t, double) const
    //! but lookup is not done using a string and a tokenizer instead of
    //! a StringVector.
      HFSTDLL HfstOneLevelPaths * lookup(const HfstTokenizer& tok,
                                         const std::string &s,
                                         ssize_t limit = -1,
                                         double time_cutoff = 0.0) const;

    //! @brief Lookup or apply a single string \a s minding flag diacritics
    //! properly and store a maximum of \a limit results to \a results.
    //! \a tok defines how s is tokenized.
    //!
    //! The same as
    //! #lookup_fd(const StringVector&, ssize_t, double) const
    //! but uses a tokenizer and a string instead of a StringVector.
    //!
    HFSTDLL HfstOneLevelPaths * lookup_fd(
        const HfstTokenizer& tok,
        const std::string &s, ssize_t limit = -1,
        double time_cutoff = 0.0) const;

    //! @brief (Not implemented) Lookdown a single string \a s and return
    //! a maximum of \a limit results.
    //!
    //! Traverse all paths on logical second level of the transducer to produce
    //! all possible inputs on the first.
    //! This is in effect a fast composition of single
    //! path from left hand side.
    //!
    //! @param s  string to look down
    //! <!-- @param tok  tokenizer to split string in arcs? -->
    //! @param limit  number of strings to extract. -1 tries to extract all and
    //!             may get stuck if infinitely ambiguous
    //! @return  output parameter to store unique results
    //! @todo todo
    HFSTDLL HfstOneLevelPaths * lookdown(const StringVector& s,
                 ssize_t limit = -1) const;

    HFSTDLL HfstOneLevelPaths * lookdown(const std::string& s,
                 ssize_t limit = -1) const;

    //! @brief (Not implemented) Lookdown a single string minding
    //! flag diacritics properly.
    //!
    //! This is a version of lookdown that handles flag diacritics as epsilons
    //! and validates the sequences prior to outputting.
    //!
    //! @sa lookdown
    //! @todo todo
    HFSTDLL HfstOneLevelPaths * lookdown_fd(StringVector& s,
                    ssize_t limit = -1) const;

    HFSTDLL HfstOneLevelPaths * lookdown_fd(const std::string& s,
                    ssize_t limit = -1) const;

    //! @brief Whether lookup of path \a s will have infinite results.
    //!
    //! Currently, this function will return whether the transducer
    //! is infinitely ambiguous on any lookup path found in the transducer,
    //! i.e. the argument \a s is ignored.
    //!
    //! @see lookup(HfstOneLevelPaths&, const StringVector&, ssize_t) const
    HFSTDLL bool is_lookup_infinitely_ambiguous(const StringVector & s) const;
    HFSTDLL bool is_lookup_infinitely_ambiguous(const std::string & s) const;

    //! @brief (Not implemented) Whether lookdown of path \a s will have
    //! infinite results.
    //! @todo todo
    HFSTDLL bool is_lookdown_infinitely_ambiguous(const StringVector& s) const;

    HFSTDLL bool is_infinitely_ambiguous() const ;


    // -------------------------------------------
    // --------- Optimization operations ---------
    // -------------------------------------------

    HFSTDLL HfstTransducer &eliminate_flags();
    HFSTDLL HfstTransducer &eliminate_flag(const std::string &flag);

    /** \brief Remove all <i>epsilon:epsilon</i> transitions
        from the transducer so that the transducer remains equivalent. */
    HFSTDLL HfstTransducer &remove_epsilons();

    /** \brief Make transducer coaccessible. */
    HFSTDLL HfstTransducer &prune();

    /** \brief Determinize the transducer.

        Determinizing a transducer yields an equivalent transducer that has
        no state with two or more transitions whose input:output symbol
        pairs are the same. */
    HFSTDLL HfstTransducer &determinize();

    /** \brief Minimize the transducer.

        Minimizing a transducer yields an equivalent transducer with
        the smallest number of states.
     
        @bug OpenFst's minimization algorithm seems to add epsilon
        transitions to weighted transducers? */
    HFSTDLL HfstTransducer &minimize();

    /** \brief Minimize or determinize the transducer.

        If hfst::set_minimization(false) has been called, determinize the transducer.
        Else, minimize it. */
    HFSTDLL HfstTransducer &optimize();

    /** \brief Extract \a n best paths of the transducer.

        In the case of a weighted transducer (#TROPICAL_OPENFST_TYPE or
        #LOG_OPENFST_TYPE), best paths are defined as paths with
        the lowest weight.
        In the case of an unweighted transducer (#SFST_TYPE or #FOMA_TYPE),
        the function returns random paths.

        This function is not implemented for #FOMA_TYPE or #SFST_TYPE.
        If this function is called by an HfstTransducer of type #FOMA_TYPE
        or #SFST_TYPE, it is converted to #TROPICAL_OPENFST_TYPE,
        paths are extracted and it is converted back to #FOMA_TYPE or
        #SFST_TYPE. If HFST is not linked to OpenFst library, an
        ImplementationTypeNotAvailableException is thrown.
    */
    HFSTDLL HfstTransducer &n_best(unsigned int n);


    // ------------------------------------------------
    // ------------- Algebraic operations -------------
    // ------------------------------------------------

    /** \brief A concatenation of N transducers where N is any number
        from zero to infinity. */
    HFSTDLL HfstTransducer &repeat_star();

    /** \brief A concatenation of N transducers where N is any number
        from one to infinity. */
    HFSTDLL HfstTransducer &repeat_plus();

    /** \brief A concatenation of \a n transducers. */
    HFSTDLL HfstTransducer &repeat_n(unsigned int n);

    /** \brief A concatenation of N transducers where N is any number
        from zero to \a n, inclusive.*/
    HFSTDLL HfstTransducer &repeat_n_minus(unsigned int n);

    /** \brief A concatenation of N transducers where N is any number
        from \a n to infinity, inclusive.*/
    HFSTDLL HfstTransducer &repeat_n_plus(unsigned int n);

    /** \brief A concatenation of N transducers where N is any number
        from \a n to \a k, inclusive.*/
    HFSTDLL HfstTransducer& repeat_n_to_k(unsigned int n, unsigned int k);

    /** \brief Disjunct the transducer with an epsilon transducer. */
    HFSTDLL HfstTransducer &optionalize();

    /** \brief Swap the input and output symbols of each transition
        in the transducer. */
    HFSTDLL HfstTransducer &invert();

    /** \brief Reverse the transducer.

        A reverted transducer accepts the string "n(0) n(1) ... n(N)"
        iff the original
        transducer accepts the string "n(N) n(N-1) ... n(0)" */
    HFSTDLL HfstTransducer &reverse();

    /** \brief Extract the input language of the transducer.

        All transition symbol pairs <i>isymbol:osymbol</i> are changed
        to <i>isymbol:isymbol</i>. */
    HFSTDLL HfstTransducer &input_project();

    /** \brief Extract the output language of the transducer.

        All transition symbol pairs <i>isymbol:osymbol</i> are changed
        to <i>osymbol:osymbol</i>. */
    HFSTDLL HfstTransducer &output_project();

    /** \brief Compose this transducer with \a another. */
    HFSTDLL HfstTransducer &compose(const HfstTransducer &another,
                            bool harmonize=true);

    HFSTDLL HfstTransducer &merge(const HfstTransducer &another, const std::map<std::string, std::set<std::string> > & list_symbols);

    HFSTDLL HfstTransducer &merge(const HfstTransducer &another, const struct hfst::xre::XreConstructorArguments & args);

    /** \brief Compose this transducer with the intersection of
        transducers in \a v. If \a invert is true, then compose the
        intersection of the transducers in \a v with this transducer.

        The algorithm used by this function is faster than intersecting
        all transducers one by one and then composing this transducer
        with the intersection.

        @pre The transducers in \a v are deterministic and epsilon-free.
    */
    HFSTDLL HfstTransducer &compose_intersect(const HfstTransducerVector &v,
                                      bool invert=false, bool harmonize=true);

    /** \brief Concatenate this transducer with \a another. */
    HFSTDLL HfstTransducer &concatenate(const HfstTransducer &another, bool harmonize=true);

    /** \brief Disjunct this transducer with \a another. */
    HFSTDLL HfstTransducer &disjunct(const HfstTransducer &another, bool harmonize=true);

    /** \brief Make priority union of this transducer with \a another.
     *
     * For the operation t1.priority_union(t2), the result is a union of t1 and t2,
     * except that whenever t1 and t2 have the same string on the upper side,
     * the path in t1 overrides the path in t2.
     *
     * Example
     *
     * Transducer 1 (t1):
     * a : a
     * b : b
     *
     * Transducer 2 (t2):
     * b : B
     * c : C
     *
     * Result ( t1.priority_union(t2) ):
     * a : a
     * b : b
     * c : C
     *
     * For more information, read: www.fsmbook.com
     *  */
    HFSTDLL HfstTransducer &priority_union(const HfstTransducer &another);


    /**  \brief Make lenient composition of this transducer with \a.
     *  A .O. B = [ A .o. B ] .P. A
     */
    HFSTDLL HfstTransducer &lenient_composition(const HfstTransducer &another, bool harmonize=true);

    /**  \brief Make cross product of this transducer with \a.
     *  It pairs every string of this with every string of \a.
     *
     *  Both transducers must be automata, i.e. map strings onto themselves.
     *
     *  If strings are not the same length, epsilon padding will be added in the end of the shorter string.
     *
     */
    HFSTDLL HfstTransducer &cross_product(const HfstTransducer &another, bool harmonize=true);


    /*
     *  \brief Shuffle this transducer with transducer \@ another.
     *
     *  If transducer A accepts string "foo" and transducer B string "bar",
     *  the transducer that results from shuffling A and B accepts all strings
     *  [(f|b)(o|a)(o|r)].
     *
     *  @pre Both transducers must be automata, i.e. map strings onto themselves.
     *
     */
    HFSTDLL HfstTransducer &shuffle(const HfstTransducer &another, bool harmonize=true);

    /** \brief Create universal pair transducer of \a type.
     *
     *  The transducer has only one state, and it accepts:
     *  Identity:Identity, Unknown:Unknown, Unknown:Epsilon and Epsilon:Unknown
     *
     *  Transducer weight is 0.
     */
    HFSTDLL static HfstTransducer universal_pair ( ImplementationType type );

    /** \brief Create identity pair transducer of \a type.
      *
      * The transducer has only one state, and it accepts:
      * Identity:Identity
      *
      * Transducer weight is 0.
      */
    HFSTDLL static HfstTransducer identity_pair ( ImplementationType type );





    /* For HfstCompiler: Optimized disjunction function. */
    HFSTDLL HfstTransducer &disjunct(const StringPairVector &spv);

    /** \brief Intersect this transducer with \a another. */
    HFSTDLL HfstTransducer &intersect(const HfstTransducer &another, bool harmonize=true);

    /** \brief Subtract transducer \a another from this transducer. */
    HFSTDLL HfstTransducer &subtract(const HfstTransducer &another, bool harmonize=true);


    // ------------------------------------------------
    // ---------- Insertion and substitution ----------
    // ------------------------------------------------

    /** \brief Freely insert symbol pair \a symbol_pair into the transducer.

        To each state in this transducer is added a transition that
        leads from that state to itself with input and output symbols
        defined by \a symbol_pair.

    If harmonize is true, then identity and unknown symbols in the
    transducer will be exapanded byt the symbols in symbol
    pair. Otherwise they aren't.
     */
    HFSTDLL HfstTransducer &insert_freely(const StringPair &symbol_pair, bool harmonize=true);

    /** \brief Freely insert a copy of \a tr into the transducer.

        A copy of \a tr is attached with epsilon transitions
        to each state of this transducer. After the operation, for each
        state S in this transducer, there is an epsilon transition
        that leads from state S to the initial state of \a tr,
        and for each final state of \a tr, there is an epsilon transition
        that leads from that final state to state S in this transducer.
        The weights of the final states in \a tr are copied to the
        epsilon transitions leading to state S.
        
        Implemented only for implementations::HfstBasicTransducer.
        Conversion is carried out for an HfstTransducer, if this function
        is called.
     */
    HFSTDLL HfstTransducer &insert_freely(const HfstTransducer &tr, bool harmonize=true);

    /** \brief Substitute all transition \a sp with transitions \a sps
        as defined by function \a func.

        @param func A pointer to a function that takes as its argument
        a StringPair sp and inserts to StringPairSet sps all StringPairs
        with which sp is to be substituted. Returns whether any substituting
        string pairs were inserted in sps, i.e. whether there is a need to
        perform substitution on transition sp.

        An example:
\verbatim
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

...

// For all transitions in transducer t whose input and output wovels
// are equivalent, substitute the output wovel with a symbol that defines
// whether the wovel in question is a front or back wovel.
t.substitute(&function);
\endverbatim

@see String
*/
  HFSTDLL HfstTransducer &substitute
  (bool (*func)(const StringPair &sp, StringPairSet &sps));

    /** \brief Substitute all transition symbols equal to \a old_symbol
        with symbol \a new_symbol.
        \a input_side and \a output_side define whether
        the substitution is made on input and output sides.

        @param old_symbol Symbol to be substituted.
        @param new_symbol The substituting symbol.
        @param input_side Whether the substitution is made on the input side
                          of a transition.
        @param output_side Whether the substitution is made on the output side
                           of a transition.

        The transition weights remain the same.

        @see String */
    HFSTDLL HfstTransducer &substitute(const std::string &old_symbol,
                               const std::string &new_symbol,
                               bool input_side=true,
                               bool output_side=true);
                                                      
    /** \brief Substitute all transition symbol pairs equal to
        \a old_symbol_pair with \a new_symbol_pair.

        The transition weights remain the same.

        Implemented only for #TROPICAL_OPENFST_TYPE and #LOG_OPENFST_TYPE.
        If this function is called by an unweighted HfstTransducer,
        it is converted to a weighted one,
        substitution is made and the transducer is converted back
        to the original format.

        @see String
     */
    HFSTDLL HfstTransducer &substitute(const StringPair &old_symbol_pair,
                               const StringPair &new_symbol_pair);

    /** \brief Substitute all transitions equal to \a old_symbol_pair
        with a set of transitions equal to \a new_symbol_pair_set.

        The weight of the original transition is copied to all new transitions.

        Implemented only for #TROPICAL_OPENFST_TYPE and #LOG_OPENFST_TYPE.
        If this function is called by an unweighted HfstTransducer
        (#SFST_TYPE or #FOMA_TYPE), it is converted to #TROPICAL_OPENFST_TYPE,
        substitution is done and it is converted back to the original format.

        @see String
     */
    HFSTDLL HfstTransducer &substitute(const StringPair &old_symbol_pair,
                               const StringPairSet &new_symbol_pair_set);

    HFSTDLL HfstTransducer & substitute_symbol(const std::string &old_symbol, const std::string &new_symbol, bool input_side=true, bool output_side=true);
    HFSTDLL HfstTransducer & substitute_symbol_pair(const StringPair &old_symbol_pair, const StringPair &new_symbol_pair);
    HFSTDLL HfstTransducer & substitute_symbol_pair_with_set(const StringPair &old_symbol_pair, const hfst::StringPairSet &new_symbol_pair_set);
    HFSTDLL HfstTransducer & substitute_symbol_pair_with_transducer(const StringPair &symbol_pair, HfstTransducer &transducer, bool harmonize=true);


    /** \brief Substitute all transition symbols as defined in \a substitutions.

        Each symbol old_symbol is substituted with symbol new_symbol, iff
        substitutions.find(old_symbol) == new_symbol != substitutions.end().
        Otherwise, old_symbol remains the same.

        This function performs all substitutions at the same time, so it is
        more efficient than calling substitute separately for each substitution.
     */
    HFSTDLL HfstTransducer &substitute(const HfstSymbolSubstitutions &substitutions);

    HFSTDLL HfstTransducer &substitute_symbols(const HfstSymbolSubstitutions &substitutions);

    /** \brief Substitute all transition symbol pairs as defined in \a substitutions.

        Each symbol pair old_isymbol:old_osymbol is substituted with symbol pair
        new_isymbol:new_osymbol, iff substitutions.find(old_isymbol:old_osymbol) ==
        new_isymbol:new_osymbol != substitutions.end().
        Otherwise, old_isymbol:old_osymbol remains the same.

        This function performs all substitutions at the same time, so it is
        more efficient than calling substitute separately for each substitution.
     */
    HFSTDLL HfstTransducer &substitute(const HfstSymbolPairSubstitutions &substitutions);

    HFSTDLL HfstTransducer &substitute_symbol_pairs(const HfstSymbolPairSubstitutions &substitutions);

    /** \brief Substitute all transitions equal to \a symbol_pair
        with a copy of transducer \a transducer.

        A copy of \a transducer is attached (using epsilon transitions)
        between the source and target states of the transition
        to be substituted.
        The weight of the original transition is copied to
        the epsilon transition leaving from the source state.

        Implemented only for #TROPICAL_OPENFST_TYPE and #LOG_OPENFST_TYPE.
        If this function is called by an unweighted HfstTransducer
        (#SFST_TYPE or #FOMA_TYPE), it is converted to #TROPICAL_OPENFST_TYPE,
        substitution is done and it is converted back to the original format.

        @see String
     */
    HFSTDLL HfstTransducer &substitute(const StringPair &symbol_pair,
                               HfstTransducer &transducer, bool harmonize=true);

    // -----------------------------------------------
    // --------------- Weight handling ---------------
    // -----------------------------------------------

    /** \brief Set the weights of all final states to \a weight.
        \a increment defines whether the old weight is incremented by \a weight
        or overwritten.

        If the HfstTransducer is of unweighted type
        (#SFST_TYPE or #FOMA_TYPE), nothing is done.
    */
    HFSTDLL HfstTransducer &set_final_weights(float weight, bool increment=false);

    /** \brief Transform all transition and state weights as defined
        in \a func.

        @param func A pointer to a function that takes a weight as its
                    argument and returns a weight that will be the new
                    value of the weight given as the argument.

     An example:
\verbatim
float func(float f) {
  return 2*f + 0.5;
}

...

// All transition and final weights are multiplied by two and summed with 0.5.
transducer.transform_weights(&func);
\endverbatim

    If the HfstTransducer is of unweighted type
    (#SFST_TYPE or #FOMA_TYPE), nothing is done.
    */
    HFSTDLL HfstTransducer &transform_weights(float (*func)(float));

    /** \brief Push weights towards initial or final state(s)
        as defined by \a type.

        If the HfstTransducer is of unweighted type
        (#SFST_TYPE or #FOMA_TYPE), nothing is done.
        @see PushType
    */
    HFSTDLL HfstTransducer &push_weights(PushType type);


    /** \brief Compile a lexc file in file \a filename into an HfstTransducer
    of type \a type and return the transducer. */
    HFSTDLL static HfstTransducer * read_lexc_ptr(const std::string &filename,
                                          ImplementationType type,
                                          bool verbose);

    HFSTDLL static HfstTransducer read_lexc(const std::string &filename,
                                    ImplementationType type,
                                    bool verbose);

    // *** For commandline programs. ***

    /* For each flag diacritic fd that is included in the alphabet of
       transducer \a another but not in the alphabet of this transducer,
       insert freely a transition fd:fd in this transducer. */
    HFSTDLL void insert_freely_missing_flags_from
      (const HfstTransducer &another);

    /*
      If both \a this and \a another contain flag diacritics, replace flag
      diacritic @X.Y.(.Z)@ by @X.Y_1(.Z)@ in \a this and replace it by
      @X.Y_2(.Z)@ in \a another.
      
      If \a insert_renamed_flags is true, then the flags from \a this are
      inserted freely in \a another and vice versa after replacing.
     */
    HFSTDLL void harmonize_flag_diacritics(HfstTransducer &another,
                                   bool insert_renamed_flags=true);
    
    HFSTDLL void insert_missing_symbols_to_alphabet_from(const HfstTransducer &another, bool only_special_symbols=false);

    HFSTDLL StringSet insert_missing_diacritics_to_alphabet_from(const HfstTransducer &another);

    HFSTDLL static bool is_special_symbol(const std::string & symbol);
    
    /* Whether the alphabet of transducer \a another includes flag diacritics
       that are not included in the alphabet of this transducer. */
    HFSTDLL bool check_for_missing_flags_in(const HfstTransducer &another) const;

    /* Return true if \a this has flag diacritics in the alphabet. */
    HFSTDLL bool has_flag_diacritics(void) const;

    

    // *** Friends **** //

    HFSTDLL friend std::ostream& operator<<(std::ostream &out, const HfstTransducer &t);
    HFSTDLL friend std::ostream& redirect(std::ostream &out, const HfstTransducer &t);
    friend class HfstInputStream;
    friend class HfstOutputStream;
    friend class hfst::implementations::HfstTransitionGraph<class C>;
    friend class HfstCompiler;
    friend class hfst::implementations::ConversionFunctions;
    friend class HfstGrammar;
    friend class xfst::XfstCompiler;
    //friend HfstTransducer bracketedReplace( const hfst::xeroxRules::Rule &rule, bool optional);
    friend hfst::HfstTransducer hfst::xeroxRules::bracketedReplace(const hfst::xeroxRules::Rule&, bool);
  };

  /** \brief Write transducer \a t in AT&T format to ostream \a out.

      The same as
      #hfst::HfstTransducer::write_in_att_format(FILE*, bool) const
      with ostreams. Weights are written if the type of \a t is weighted. */
  HFSTDLL std::ostream &operator<<(std::ostream &out,const HfstTransducer &t);

  HFSTDLL std::ostream &redirect(std::ostream &out,const HfstTransducer &t);

  /** \brief A namespace for functions that create two-level, replace,
      restriction and coercion rule transducers. */
  namespace rules
  {
    enum ReplaceType {REPL_UP, REPL_DOWN, REPL_RIGHT, REPL_LEFT,
              REPL_DOWN_KARTTUNEN};
    enum TwolType {twol_right, twol_left, twol_both};

    /* helping methods */
    HFSTDLL HfstTransducer universal_fst
      (const StringPairSet &alphabet, ImplementationType type);
    HFSTDLL HfstTransducer negation_fst
      (const HfstTransducer &t, const StringPairSet &alphabet);

    HFSTDLL HfstTransducer replace
      (HfstTransducer &t, ReplaceType repl_type, bool optional,
       StringPairSet &alphabet);
    HFSTDLL HfstTransducer replace_transducer
      (HfstTransducer &t, std::string lm, std::string rm,
       ReplaceType repl_type, StringPairSet &alphabet);
    HFSTDLL HfstTransducer replace_context
      (HfstTransducer &t, std::string m1, std::string m2,
       StringPairSet &alphabet);
    HFSTDLL HfstTransducer replace_in_context
      (HfstTransducerPair &context, ReplaceType repl_type,
       HfstTransducer &t, bool optional, StringPairSet &alphabet);

    /* Used by hfst-calculate. */
    HFSTDLL HfstTransducer restriction
      (HfstTransducerPairVector &contexts, HfstTransducer &mapping,
       StringPairSet &alphabet,        TwolType twol_type, int direction );



    // ***** THE PUBLIC INTERFACE *****

    /** \brief A transducer that obligatorily performs the mappings
        defined by \a mappings in the context \a context
        when the alphabet is \a alphabet.

        @param context A pair of transducers where the first transducer
                       defines the left context and the second transducer
                       the right context.
        @param mappings A set of mappings that the resulting transducer
                        will perform in the context given in \a context.
        @param alphabet The set of symbol pairs that defines the alphabet
                        (see the example).

        For example, a transducer yielded by the following arguments
\verbatim
context = pair( [c|d], [e] )
mappings = set(a:b)
alphabet = set(a, a:b, b, c, d, e, ...)
\endverbatim
        obligatorily maps the symbol a to b if c or d precedes
        and e follows. (Elsewhere,
        the mapping of a to b is optional)
        This expression is identical to ![.* [c|d] [a:. & !a:b] [e] .*]
        Note that the alphabet must contain the pair a:b here.
        
        @see
    <a href="ftp://ftp.ims.uni-stuttgart.de/pub/corpora/SFST/SFST-Manual.pdf">
    SFST manual</a>
    */
    HFSTDLL HfstTransducer two_level_if(HfstTransducerPair &context,
                                StringPairSet &mappings,
                                StringPairSet &alphabet);
    
    /** \brief A transducer that allows the mappings defined by
        \a mappings only in the context \a context,
        when the alphabet is \a alphabet.
        
        If called with the same arguments as in the example of
        #two_level_if, the transducer
        allows the mapping of symbol a to b only if c or d precedes
        and e follows. The
        mapping of a to b is optional in this context but cannot occur
        in any other context.
        The expression is equivalent to
        ![  [ ![.* [c|d]] a:b .* ] | [ .* a:b ![[e] .*] ]  ]
        
        @see #two_level_if
    */
    HFSTDLL HfstTransducer two_level_only_if(HfstTransducerPair &context,
                                     StringPairSet &mappings,
                                     StringPairSet &alphabet);

    /** \brief A transducer that always performs the mappings defined
        by \a mappings in the context \a context
        and only in that context, when the alphabet is \a alphabet.

        If called with the same arguments as in the example of
        #two_level_if, the transducer
        maps symbol a to b only and only if c or d precedes and e follows.
        The mapping of a to b is obligatory in this context and cannot
        occur in any other context.
        The expression is equivalent to ![.* [c|d] [a:. & !a:b] [e] .*]  &
        ![  [ ![.* [c|d]] a:b .* ] | [ .* a:b ![[e] .*] ]  ]
        
        @see #two_level_if
    */
    HFSTDLL HfstTransducer two_level_if_and_only_if(HfstTransducerPair &context,
                                            StringPairSet &mappings,
                                            StringPairSet &alphabet);


    /** \brief A transducer that performs an upward mapping \a mapping
        in the context \a context when the alphabet is \a alphabet.
        \a optional defines whether the mapping is optional.

        @param context A pair of transducers where the first transducer
                       defines the left context and the second transducer
                   the right context. Both transducers must be automata,
                   i.e. map strings onto themselves.
        @param mapping The mapping that the resulting transducer
                       will perform in the context given in \a context.
        @param optional Whether the mapping is optional.
        @param alphabet The set of symbol pairs that defines the alphabet
                        (see the explanation below).

        Each substring s of the input string which is in the input language
        of the transducer \a mapping and whose left context is matched
        by the expression
        [.* l] (where l is the first element of \a context) and
        whose right context is matched by [r .*]
        (where r is the second element in the context) is mapped
        to the respective
        surface strings defined by transducer \a mapping. Any other
        character is mapped to
        the characters specified in \a alphabet. The left and right
        contexts must
        be automata (i.e. transducers which map strings onto themselves).

        For example, a transducer yielded by the following arguments
\verbatim
context = pair( [c], [c] )
mappings = [ a:b a:b ]
alphabet = set(a, b, c)
\endverbatim
        would map the string "caacac" to "cbbcac".

        Note that the alphabet must contain the characters a and b,
        but not the pair
        a:b (unless this replacement is to be allowed everywhere
        in the context).

        Note that replace operations (unlike the two-level rules)
        have to be combined by composition
        rather than intersection.

        @throws ContextTransducersAreNotAutomataException

        @see
     <a href="ftp://ftp.ims.uni-stuttgart.de/pub/corpora/SFST/SFST-Manual.pdf">
     SFST manual</a>
    */
    HFSTDLL HfstTransducer replace_up(HfstTransducerPair &context,
                              HfstTransducer &mapping,
                              bool optional,
                              StringPairSet &alphabet);

    /** \brief The same as replace_up, but matching is done on
        the output side of \a mapping

        @see replace_up
<a href="ftp://ftp.ims.uni-stuttgart.de/pub/corpora/SFST/SFST-Manual.pdf">
SFST manual</a>. */
    HFSTDLL HfstTransducer replace_down(HfstTransducerPair &context,
                                HfstTransducer &mapping,
                                bool optional,
                                StringPairSet &alphabet);

    HFSTDLL HfstTransducer replace_down_karttunen(HfstTransducerPair &context,
                      HfstTransducer &mapping,
                      bool optional,
                      StringPairSet &alphabet);

    /** \brief The same as replace_up, but left context matching is
        done on the output side of \a mapping
        and right context on the input side of \a mapping

        @see replace_up
<a href="ftp://ftp.ims.uni-stuttgart.de/pub/corpora/SFST/SFST-Manual.pdf">
SFST manual</a>. */
    HFSTDLL HfstTransducer replace_right(HfstTransducerPair &context,
                                 HfstTransducer &mapping,
                                 bool optional,
                                 StringPairSet &alphabet);

    /** \brief The same as replace_up, but left context matching is
        done on the input side of \a mapping
        and right context on the output side of \a mapping.

        @see replace_up
<a href="ftp://ftp.ims.uni-stuttgart.de/pub/corpora/SFST/SFST-Manual.pdf">
SFST manual</a>. */
    HFSTDLL HfstTransducer replace_left(HfstTransducerPair &context,
                                HfstTransducer &mapping,
                                bool optional,
                                StringPairSet &alphabet);


    /** \brief The same as replace_up but \a mapping is performed
        in every context.

        @see replace_up */
    HFSTDLL HfstTransducer replace_up(HfstTransducer &mapping,
                              bool optional,
                              StringPairSet &alphabet);

    /** \brief The same as replace_down(HfstTransducerPair&, HfstTransducer&, bool, StringPairSet&)
        but \a mapping is performed in every context.

        @see replace_up */
    HFSTDLL HfstTransducer replace_down(HfstTransducer &mapping,
                                bool optional,
                                StringPairSet &alphabet);

    /** \brief Inversion of the replace_up and the result needs to be composed on the upper side
     *   of the input language.
     *
     *   B <- A is the inversion of A -> B.
     *
     *   \a Mapping is performed in every context.
     *
     *   @see replace_up */
    HFSTDLL HfstTransducer left_replace_up(     HfstTransducer          &mapping,
                                    bool                optional,
                                    StringPairSet       &alphabet);

    /** \brief Inversion of the replace_up and the result needs to be composed on the upper side
     *   of the input language.
     *
     *   B <- A is the inversion of A -> B.
     *
     *   @see replace_up */
    HFSTDLL HfstTransducer left_replace_up( HfstTransducerPair  &context,
                                    HfstTransducer      &mapping,
                                    bool                optional,
                                    StringPairSet       &alphabet);
    /** \brief Inversion of the replace_up and the result needs to be composed on the upper side
     *   of the input language. However, matching is done on the output side of \a mapping
     *
     *   @see replace_up */
    HFSTDLL HfstTransducer left_replace_down(HfstTransducerPair &context,
                                         HfstTransducer         &mapping,
                                         bool                           optional,
                                         StringPairSet          &alphabet);

    /** \brief Inversion of the replace_up and the result needs to be composed on the upper side
       *         of the input language. However, matching is done on the output side of \a mapping
       *
       *         @see replace_up */
    HFSTDLL HfstTransducer left_replace_down_karttunen( HfstTransducerPair      &context,
                                                                                HfstTransducer          &mapping,
                                                                                bool                            optional,
                                                                                StringPairSet           &alphabet);

    /** \brief Inversion of the replace_up and the result needs to be composed on the upper side
     *   of the input language. However, left context matching is done on the input side of \a mapping
     *   and right context on the output side of \a mapping.
     *
     *   @see replace_up */
    HFSTDLL HfstTransducer left_replace_left(HfstTransducerPair &context,
                                         HfstTransducer         &mapping,
                                         bool                           optional,
                                         StringPairSet          &alphabet);

    /** \brief Inversion of the replace_up and the result needs to be composed on the upper side
     *   of the input language. However, left context matching is done on the output side of \a mapping
     *   and right context on the input side of \a mapping.
     *
     *   @see replace_up */
    HFSTDLL HfstTransducer left_replace_right(HfstTransducerPair        &context,
                                          HfstTransducer                &mapping,
                                          bool                          optional,
                                          StringPairSet         &alphabet);





    /** \brief A transducer that allows any (substring) mapping defined
        by \a mapping
        only if it occurs in any of the contexts in \a contexts.
        Symbols outside of the matching
        substrings are mapped to any symbol allowed by \a alphabet.

        @throws EmptySetOfContextsException

        @see
     <a href="ftp://ftp.ims.uni-stuttgart.de/pub/corpora/SFST/SFST-Manual.pdf">
     SFST manual</a>. */
    HFSTDLL HfstTransducer restriction(HfstTransducerPairVector &contexts,
                               HfstTransducer &mapping,
                               StringPairSet &alphabet);

    /** \brief A transducer that requires that one of the mappings
        defined by \a mapping
        must occur in each context in \a contexts. Symbols outside of
        the matching
        substrings are mapped to any symbol allowed by \a alphabet.

        @see
     <a href="ftp://ftp.ims.uni-stuttgart.de/pub/corpora/SFST/SFST-Manual.pdf">
     SFST manual</a>. */
    HFSTDLL HfstTransducer coercion(HfstTransducerPairVector &contexts,
                            HfstTransducer &mapping,
                            StringPairSet &alphabet);

    /** \brief A transducer that is equivalent to the intersection
        of restriction and coercion
        and requires that the mappings defined by \a mapping occur
        always and only in the
        given contexts in \a contexts. Symbols outside of the matching
        substrings are mapped to any symbol allowed by \a alphabet.

        @see
        restriction(HfstTransducerPairVector&, HfstTransducer&, StringPairSet&)
        #coercion
     <a href="ftp://ftp.ims.uni-stuttgart.de/pub/corpora/SFST/SFST-Manual.pdf">
     SFST manual</a> */
    HFSTDLL HfstTransducer restriction_and_coercion(HfstTransducerPairVector &contexts,
                                            HfstTransducer &mapping,
                                            StringPairSet &alphabet);

    /** \brief A transducer that specifies that a string from
        the input language of the
        transducer \a mapping may only be mapped to one of its
        output strings (according
        to transducer \a mapping) if it appears in any of the contexts
        in \a contexts.
        Symbols outside of the matching substrings are mapped
        to any symbol allowed by \a alphabet.

        @see
     <a href="ftp://ftp.ims.uni-stuttgart.de/pub/corpora/SFST/SFST-Manual.pdf">
     SFST manual</a>. */
    HFSTDLL HfstTransducer surface_restriction(HfstTransducerPairVector &contexts,
                                       HfstTransducer &mapping,
                                       StringPairSet &alphabet);

    /** \brief A transducer that specifies that a string from
        the input language of the transducer
        \a mapping always has to the mapped to one of its output
        strings according to
        transducer \a mapping if it appears in any of the contexts
        in \a contexts.
        Symbols outside of the matching substrings are mapped to
        any symbol allowed by \a alphabet.

        @see
     <a href="ftp://ftp.ims.uni-stuttgart.de/pub/corpora/SFST/SFST-Manual.pdf">
     SFST manual</a>. */
    HFSTDLL HfstTransducer surface_coercion(HfstTransducerPairVector &contexts,
                                    HfstTransducer &mapping,
                                    StringPairSet &alphabet);

    /** \brief A transducer that is equivalent to the intersection of
        surface_restriction and surface_coercion.

        @see #surface_restriction #surface_coercion
   <a href="ftp://ftp.ims.uni-stuttgart.de/pub/corpora/SFST/SFST-Manual.pdf">
   SFST manual</a>. */
    HFSTDLL HfstTransducer surface_restriction_and_coercion
      (HfstTransducerPairVector &contexts,
       HfstTransducer &mapping,
       StringPairSet &alphabet);

    /** \brief A transducer that specifies that a string from
        the output language of the transducer
        \a mapping may only be mapped to one of its input strings
        (according to transducer \a mappings)
        if it appears in any of the contexts in \a contexts.
        Symbols outside of the matching substrings are mapped
        to any symbol allowed by \a alphabet.

        @see
  <a href="ftp://ftp.ims.uni-stuttgart.de/pub/corpora/SFST/SFST-Manual.pdf">
  SFST manual</a>. */
    HFSTDLL HfstTransducer deep_restriction(HfstTransducerPairVector &contexts,
                                    HfstTransducer &mapping,
                                    StringPairSet &alphabet);

    /** \brief A transducer that specifies that a string from
        the output language of the transducer
        \a mapping always has to be mapped to one of its input strings
        (according to transducer \a mappings)
        if it appears in any of the contexts in \a contexts.
        Symbols outside of the matching substrings are mapped
        to any symbol allowed by \a alphabet.

        @see
   <a href="ftp://ftp.ims.uni-stuttgart.de/pub/corpora/SFST/SFST-Manual.pdf">
   SFST manual</a>. */
    HFSTDLL HfstTransducer deep_coercion(HfstTransducerPairVector &contexts,
                                 HfstTransducer &mapping,
                                 StringPairSet &alphabet);

    /** \brief A transducer that is equivalent to the intersection
        of deep_restriction and deep_coercion.

        @see #deep_restriction #deep_coercion
  <a href="ftp://ftp.ims.uni-stuttgart.de/pub/corpora/SFST/SFST-Manual.pdf">
  SFST manual</a>. */
    HFSTDLL HfstTransducer deep_restriction_and_coercion
      (HfstTransducerPairVector &contexts,
       HfstTransducer &mapping,
       StringPairSet &alphabet);
  }

}


// vim: set ft=cpp.doxygen:
#endif // #ifndef _HFST_TRANSDUCER_H_
