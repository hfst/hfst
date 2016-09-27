/*     Foma: a finite-state toolkit and library.                             */
/*     Copyright © 2008-2015 Mans Hulden                                     */

/*     This file is part of foma.                                            */

/*     Foma is free software: you can redistribute it and/or modify          */
/*     it under the terms of the GNU General Public License version 2 as     */
/*     published by the Free Software Foundation.                            */

/*     Foma is distributed in the hope that it will be useful,               */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/*     GNU General Public License for more details.                          */

/*     You should have received a copy of the GNU General Public License     */
/*     along with foma.  If not, see <http://www.gnu.org/licenses/>.         */

#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include "foma.h"
#ifdef ORIGINAL
#include "zlib.h"
#else // #ifdef ORIGINAL
  #ifdef ZLIB
    #include "zlib.h"
  #endif
#endif // #ifdef ORIGINAL

extern int g_show_flags;
extern int g_obey_flags;
extern int g_flag_is_epsilon;
extern int g_print_space;
extern int g_print_pairs;
extern int g_minimal;
extern int g_name_nets;
extern int g_print_sigma;
extern int g_quit_on_fail;
extern int g_quote_special;
extern int g_recursive_define;
extern int g_sort_arcs;
extern int g_verbose;
extern int g_minimize_hopcroft;
extern int g_list_limit;
extern int g_list_random_limit;
extern int g_compose_tristate;
extern int g_med_limit ;
extern int g_med_cutoff ;
extern int g_lexc_align ;
extern char *g_att_epsilon;

extern struct defined_networks   *g_defines;
extern struct defined_functions  *g_defines_f;

#ifdef ORIGINAL
extern int foma_net_print(struct fsm *net, gzFile outfile);
#else // #ifdef ORIGINAL
  #ifdef ZLIB
    extern int foma_net_print(struct fsm *net, gzFile outfile);
  #endif
#endif // #ifdef ORIGINAL

static char *sigptr(struct sigma *sigma, int number);
static int print_dot(struct fsm *net, char *filename);
static int print_net(struct fsm *net, char *filename);
static int print_sigma(struct sigma *sigma, FILE *out);
static int view_net(struct fsm *net);

#define FVAR_BOOL   1
#define FVAR_INT    2
#define FVAR_STRING 3

#define LINE_LIMIT 8192

struct g_v {
    void *ptr;
    char *name;
    int  type;
} global_vars[] = {
    {&g_flag_is_epsilon,  "flag-is-epsilon",  FVAR_BOOL},
    {&g_minimal,          "minimal",          FVAR_BOOL},
    {&g_name_nets,        "name-nets",        FVAR_BOOL},
    {&g_obey_flags,       "obey-flags",       FVAR_BOOL},
    {&g_print_pairs,      "print-pairs",      FVAR_BOOL},
    {&g_print_sigma,      "print-sigma",      FVAR_BOOL},
    {&g_print_space,      "print-space",      FVAR_BOOL},
    {&g_quit_on_fail,     "quit-on-fail",     FVAR_BOOL},
    {&g_recursive_define, "recursive-define", FVAR_BOOL},
    {&g_quote_special,    "quote-special",    FVAR_BOOL},
    {&g_show_flags,       "show-flags",       FVAR_BOOL},
    {&g_sort_arcs,        "sort-arcs",        FVAR_BOOL},
    {&g_verbose,          "verbose",          FVAR_BOOL},
    {&g_minimize_hopcroft,"hopcroft-min",     FVAR_BOOL},
    {&g_compose_tristate, "compose-tristate", FVAR_BOOL},
    {&g_med_limit,        "med-limit",        FVAR_INT},
    {&g_med_cutoff,       "med-cutoff",       FVAR_INT},
    {&g_lexc_align,       "lexc-align",       FVAR_BOOL},
    {&g_att_epsilon,      "att-epsilon",      FVAR_STRING},
    {NULL, NULL, 0}
};

char warranty[] = "\nThis program is free software; you can redistribute it and/or modify\nit under the terms of the GNU General Public License version 2 as published by\nthe Free Software Foundation.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\nYou should have received a copy of the GNU General Public License\nalong with this program.  If not, see <http://www.gnu.org/licenses/>.\n\n";
 
struct global_help {
    char *name;
    char *help;
    char *longhelp;
} global_help[] = {
    {"regex <regex>", "read a regular expression","Enter a regular expression and add result to top of stack.\nShort form: re\nSee `help operator' for operators, or `help precedence' for operator precedence."},
    {"ambiguous upper","returns the input words which have multiple paths in a transducer","Short form: ambiguous\n"},
    {"apply up <string>","apply <string> up to the top network on stack","Short form: up <string>\n"},
    {"apply down <string>","apply <string> down to the top network on stack","Short form: down <string>\n" },
    {"apply med <string>","find approximate matches to string in top network by minimum edit distance","Short form: med <string>\n" },
    {"apply up","enter apply up mode (Ctrl-D exits)","Short form: up\n"},
    {"apply down","enter apply down mode (Ctrl-D exits)","Short form: down\n"},
    {"apply med","enter apply med mode (Ctrl-D exits)","Short form: med\n"},
    {"apropos <string>","search help for <string>",""},
    {"clear stack","clears the stack",""},
    {"close sigma","removes unknown symbols from FSM","" },
    {"compact sigma","removes redundant symbols from FSM","" },
    {"complete net","completes the FSM","" },
    {"compose net","composes networks on stack",""},
    {"concatenate","concatenates networks on stack","" },
    {"crossproduct net","cross-product of top two FSMs on stack","See ×\n" },
    {"define <name> <r.e.>","define a network","Example: \ndefine A x -> y;\n  and\nA = x -> y;\n\nare equivalent\n"},
    {"define <fname>(<v1,..,vn>) <r.e.>","define function","Example: define Remove(X) [X -> 0].l;"},
    {"determinize net","determinizes top FSM on stack",""},
    {"echo <string>","echo a string",""},
    {"eliminate flag <name>","eliminate flag <name> diacritics from the top network",""},
    {"eliminate flags","eliminate all flag diacritics from the top network",""},
    {"export cmatrix (filename)","export the confusion matrix as an AT&T transducer",""},
    {"extract ambiguous","extracts the ambiguous paths of a transducer","Short form: examb"},
    {"extract unambiguous","extracts the unambiguous paths of a transducer","Short form: exunamb"},
    {"help license","prints license",""},
    {"help warranty","prints warranty information",""},
    {"ignore net","applies ignore to top two FSMs on stack","See /\n"},
    {"intersect net","intersects FSMs on stack","See ∩ (or &)\n" },
    {"invert net","inverts top FSM","See ⁻¹ (or .i)\n"},
    {"label net","extracts all attested symbol pairs from FSM","See also: sigma net"},
    {"letter machine","Converts top FSM to a letter machine","See also: _lm(L)"},
    {"load stack <filename>","Loads networks and pushes them on the stack","Short form: load"},
    {"load defined <filename>","Restores defined networks from file","Short form: loadd"},
    {"lower-side net","takes lower projection of top FSM","See ₂ (or .l)\n"},
    {"minimize net","minimizes top FSM","Minimization can be controlled through the variable minimal: when set to OFF FSMs are never minimized.\nAlso, hopcroft-min can be set to OFF in which case minimization is done by double reversal and determinization (aka Brzozowski's algorithm).  It is likely to be much slower.\n"},
    {"name net <string>","names top FSM",""},
    {"negate net","complements top FSM","See ¬\n" },
    {"one-plus net","Kleene plus on top FSM","See +\n" },
    {"pop stack","remove top FSM from stack","" },
    {"print cmatrix","prints the confusion matrix associated with the top network in tabular format",""},
    {"print defined","prints defined symbols and functions",""},
    {"print dot (>filename)","prints top FSM in Graphviz dot format",""},
    {"print lower-words","prints words on the lower side of top FSM",""},
    {"print lower-words > filename","prints words on the lower side of top FSM to file",""},
    {"print name","prints the name of the top FSM","" },
    {"print net","prints all information about top FSM","Short form: net\n" },
    {"print pairs","prints input-output pairs from top FSM","Short form: pairs\n"},
    {"print pairs > filename","prints input-output pairs from top FSM to file","Short form: pairs\n"},
    {"print random-lower","prints random words from lower side","Short form: random-lower\n" },
    {"print random-upper","prints random words from upper side","Short form: random-upper" },
    {"print random-words","prints random words from top FSM","Short form: random-words\n"},
    {"print random-pairs","prints random input-output pairs from top FSM","Short form: random-pairs\n"},
    {"print sigma","prints the alphabet of the top FSM","Short form: sigma\n"},
    {"print size","prints size information about top FSM","Short form: size\n"},
    {"print shortest-string","prints the shortest string of the top FSM","Short form: pss\n"},
    {"print shortest-string-size","prints length of shortest string","Short form: psz\n"},
    {"print upper-words","prints words on the upper side of top FSM","Short form: upper-words"},
    {"print upper-words > filename","prints words on the upper side of top FSM to file","Short form:upper-words"},
    {"print words","prints words of top FSM","Short form: words"},
    {"print words > filename","prints words of top FSM to file","Short form: words"},
    {"prune net","makes top network coaccessible",""},
    {"push (defined) <name>","adds a defined FSM to top of stack",""},
    {"quit","exit foma",""},
    {"read att <filename>","read a file in AT&T FSM format and add to top of stack","Short form: ratt"},
    {"read cmatrix <filename>","read a confusion matrix and associate it with the network on top of the stack",""},
    {"read prolog <filename>","reads prolog format file",""},
    {"read lexc <filename>","read and compile lexc format file",""},
    {"read spaced-text <filename>","compile space-separated words/word-pairs separated by newlines into a FST",""},
    {"read text <filename>","compile a list of words separated by newlines into an automaton",""},
    {"reverse net","reverses top FSM","Short form: rev\nSee .r\n"},
    {"rotate stack","rotates stack",""},
    {"save defined <filename>","save all defined networks to binary file","Short form: saved" },
    {"save stack <filename>","save stack to binary file","Short form: ss" },
    {"set <variable> <ON|OFF>","sets a global variable (see show variables)","" },
    {"show variables","prints all variable/value pairs",""},
    {"shuffle net","asynchronous product on top two FSMs on stack","See ∥ (or <>)\n"},
    {"sigma net","Extracts the alphabet and creates a FSM that accepts all single symbols in it","See also: label net"},
    {"source <file>","read and compile script file",""},
    {"sort net","sorts arcs topologically on top FSM",""},
    {"sort in","sorts input arcs by sigma numbers on top FSM",""},
    {"sort out","sorts output arcs by sigma number on top FSM",""},
    {"substitute defined X for Y","substitutes defined network X at all arcs containing Y ",""},
    {"substitute symbol X for Y","substitutes all occurrences of Y in an arc with X",""},
    {"system <cmd>","execute a system command","" },
    {"test unambiguous","test if top FST is unambiguous","Short form: tunam\n"},
    {"test equivalent","test if the top two FSMs are equivalent","Short form: equ\nNote: equivalence is undecidable for transducers in the general case.  The result is reliable only for recognizers.\n"},
    {"test functional","test if the top FST is functional (single-valued)","Short form: tfu\n"},
    {"test identity","test if top FST represents identity relations only","Short form: tid\n"},
    {"test lower-universal","test if lower side is Σ*","Short form: tlu\n"},
    {"test upper-universal","test if upper side is Σ*","Short form: tuu\n"},
    {"test non-null","test if top machine is not the empty language","Short form:tnn\n" },
    {"test null","test if top machine is the empty language (∅)","Short form: tnu\n" },
    {"test sequential","tests if top machine is sequential","Short form: tseq\n"},
    {"test star-free","test if top FSM is star-free","Short form: tsf\n"},
    {"turn stack","turns stack upside down","" },
    {"twosided flag-diacritics","changes flags to always be identity pairs","Short form: tfd" },
    {"undefine <name>","remove <name> from defined networks","See define\n"},
    {"union net","union of top two FSMs","See ∪ (or |)\n"},
    {"upper-side net","upper projection of top FSM","See ₁ (or .u)\n"},
    {"view net","display top network (if supported)",""},
    {"zero-plus net","Kleene star on top fsm","See *\n"},
    {"variable compose-tristate","use the tristate composition algorithm","Default value: OFF\n"},
    {"variable show-flags","show flag diacritics in `apply'","Default value: ON\n"},
    {"variable obey-flags","obey flag diacritics in `apply'","Default value: ON\n"},
    {"variable minimal","minimize resulting FSMs","Default value: ON\n"},
    {"variable print-pairs","always print both sides when applying","Default value: OFF\n"},
    {"variable print-space","print spaces between symbols","Default value: OFF\n"},
    {"variable print-sigma","print the alphabet when printing network","Default value: ON\n"},
    {"quit-on-fail","Abort operations when encountering errors","Default value: ON\n"},
    {"variable recursive-define","Allow recursive definitions","Default value: OFF\n"},
    {"variable verbose","Verbosity of interface","Default value: ON\n"},
    {"variable hopcroft-min","ON = Hopcroft minimization, OFF = Brzozowski minimization","Default value: ON\n"},
    {"variable med-limit","the limit on number of matches in apply med","Default value: 3\n"},
    {"variable med-cutoff","the cost limit for terminating a search in apply med","Default value: 3\n"},
    {"variable att-epsilon","the EPSILON symbol when reading/writing AT&T files","Default value: @0@\n"},
    {"variable lexc-align","Forces X:0 X:X or 0:X alignment of lexicon entry symbols","Default value: OFF\n"},
    {"write prolog (> filename)","writes top network to prolog format file/stdout","Short form: wpl"},
    {"write att (> <filename>)","writes top network to AT&T format file/stdout","Short form: watt"},
    {"re operator: (∀<var name>)(F)","universal quantification","Example: $.A is equivalent to:\n(∃x)(x ∈ A ∧ (∀y)(¬(y ∈ A ∧ ¬(x = y))))"},
    {"re operator: (∃<var name>)(F)","existential quantification","Example: $.A is equivalent to:\n(∃x)(x ∈ A ∧ ¬(∃y)(y ∈ A ∧ ¬(x = y)))"},
    {"logic re operator: ∈","`in' predicate for logical formulae",""},
    {"logic re operator: S(t1,t2)","successor-of predicate for logical formulae",""},
    {"logic re operator: ≤","less-than or equal-to","Refers to position of quantified substring\n" },
    {"logic re operator: ≥","more-than or equal-to","Refers to position of quantified substring\n" },
    {"logic re operator: ≺","precedes","Refers to position of quantified substring\n"},
    {"logic re operator: ≻","follows","Refers to position of quantified substring\n"},
    {"logic re operator: ∧","conjunction","Operationally equivalent to ∩\n"},
    {"logic re operator: ∨","disjunction","Operationally equivalent to ∪\n"},
    {"logic re operator: →","implication","A → B is equivalent to ¬A ∨ B "},
    {"logic re operator: ↔","biconditional","A ↔ B is equivalent to (¬A ∨ B) ∧ (¬B ∨ A)"},
    {"re operator: ∘ (or .o.) ","compose","A .o. B is the composition of transducers/recognizers A and B\nThe composition algorithm can be controlled with the variable\ncompose-tristate.  The default algorithm is a `bistate' composition that eliminates redundant paths but may fail to find the shortest path.\n"},
    {"re operator: × (or .x.) ","cross-product","A × B (where A and B are recognizers, not transducers\nyields the cross-product of A and B.\n"},
    {"re operator: .O. ","`lenient' composition","Lenient composition as defined in Karttunen(1998)  A .O. B = [A ∘ B] .P. B\n"},
    {"re operator: ∥ (or <>) ","shuffle (asynchronous product)","A ∥ B yields the asynchronous (or shuffle) product of FSM A and B.\n" },
    {"re operator: => ","context restriction, e.g. A => B _ C, D _ E","A => B _ C yields the language where every instance of a substring drawn from A is surrounded by B and C.  Multiple contexts can be specified if separated by commas, e.g.: A => B _ C, D _ E"},
    {"re operator: ->, <-, <->, etc.","replacement operators","If LHS is a transducer, no RHS is needed in rule."},
    {"re operator: @->, @>, etc.","directed replacement operators",""},
    {"re operator: (->), (@->), etc. ","optional replacements","Optional replacement operators variants.  Note that the directional modes leftmost/rightmost/longest/shortest are not affected by optionality, i.e. only replacement is optional, not mode.  Hence A (@->) B is not in general equivalent to the parallel rule A @-> B, A -> ... "},
    {"re operator: ||,\\/,\\\\,// ","replacement direction specifiers","Rewrite rules direction specifier meaning is:\nA -> B || C _ D (replace if C and D match on upper side)\nA -> B // C _ D (replace if C matches of lower side and D matches on upper side)\nA -> B \\\\ C _ D (replace if C matches on upper side and D matches on lower side)\nA -> B \\/ C _ D (replace if C and D match on lower side)\n"},
    {"re operator: _ ","replacement or restriction context specifier",""},
    {"re operator: ,,","parallel context replacement operator","Separates parallel rules, e.g.:\nA -> B , C @-> D || E _ F ,, G -> H \\/ I _ J\n"},
    {"re operator: ,","parallel replacement operator","Separates rules and contexts. Example: A -> B, C <- D || E _ F"},
    {"re operator: [.<r.e.>.]","single-epsilon control in replacement LHS, e.g. [..] -> x","If the LHS contains the empty string, as does [.a*.] -> x, the rule yields a transducer where the empty string is assumed to occur exactly once between each symbol."},
    {"re operator: ...","markup replacement control (e.g. A -> B ... C || D _ E)","A -> B ... C yields a replacement transducer where the center A is left untouched and B and C inserted around A." },
    {"re operator:  ","concatenation","Binary operator: A B\nConcatenation is performed implicitly according to its precedence level without overt specification\n"},
    {"re operator: ∪ (or |) ","union","Binary operator: A|B"},
    {"re operator: ∩ (or &) ","intersection","Binary operator: A&B" },
    {"re operator: - ","set minus","Binary operator A-B"},
    {"re operator: .P.","priority union (upper)","Binary operator A .P. B\nEquivalent to: A .P. B = A ∪ [¬[A₁] ∘ B]\n" },
    {"re operator: .p.","priority union (lower)","Binary operator A .p. B\nEquivalent to: A .p. B = A ∪ [¬[A₂] ∘ B]" },
    {"re operator: <","precedes","Binary operator A < B\nYields the language where no instance of A follows an instance of B."},
    {"re operator: >","follows","Binary operator A > B\nYields the language where no instance of A precedes an instance of B."},
    {"re operator: /","ignore","Binary operator A/B\nYield the language/transducer where arbitrary sequences of strings/mappings from B are interspersed in A.  For single-symbol languages B, A/B = A ∥ B*"},
    {"re operator: ./.","ignore except at edges","Yields the language where arbitrary sequences from B are interspersed in A, except as the very first and very last symbol."},
    {"re operator: \\\\\\","left quotient","Binary operator: A\\\\\\B\nInformally:  the set of suffixes one can add to A to get strings in B\n"},
    {"re operator: ///","right quotient","Binary operator A///B\nInformally: the set of prefixes one can add to B to get a string in A\n"},
    {"re operator: /\\/","interleaving quotient","Binary operator A/\\/B\nInformally: the set of strings you can interdigitate (non-continuously) to B to get strings in A\n"},
    {"re operator: ¬ (or ~) ","complement","Unary operator ~A, equivalent to Σ* - A\n"},
    {"re operator: $","contains a factor of","Unary operator $A\nEquivalent to: Σ* A Σ*\n"},
    {"re operator: $.","contains exactly one factor of","Unary operator $.A\nYields the language that contains exactly one factor from A.\nExample: if A = [a b|b a], $.A contains strings ab, ba, abb, bba, but not abab, baba, aba, bab, etc.\n"},
    {"re operator: $?","contains maximally one factor of","Unary operator: $?A, yields the language that contains zero or one factors from A. See also $.A."},
    {"re operator: +","Kleene plus","Unary operator A+\n"},
    {"re operator: *","Kleene star","Unary operator A*\n" },
    {"re operator: ^n ^<n ^>n ^{m,n}","m, n-ary concatenations","A^n: A concatenated with itself exactly n times\nA^<n: A concatenated with itself less than n times\nA^>n: A concatenated with itself more than n times\nA^{m,n}: A concatenated with itself between m and n times\n"},
    {"re operator: ₁ (or .1 or .u)","upper projection","Unary operator A.u\n"},
    {"re operator: ₂ (or .2 or .l)","lower projection","Unary operator A.l\n"},
    {"re operator: ⁻¹ (or .i)","inverse of transducer","Unary operator A.i\n"},
    {"re operator: .f","eliminate all flags","Unary operator A.f: eliminates all flag diacritics in A"},
    {"re operator: .r","reverse of FSM","Unary operator A.r\n"},
    {"re operator: :","cross-product","Binary operator A:B, see also A × B\n"},
    {"re operator: \\","term complement (\\x = [Σ-x])","Unary operator \\A\nSingle symbols not in A.  Equivalent to [Σ-A]\n"},
    {"re operator: `","substitution/homomorphism","Ternary operator `[A,B,C] Replace instances of symbol B with symbol C in language A.  Also removes the substituted symbol from the alphabet.\n"},
    {"re operator: { ... }","concatenate symbols","Single-symbol-concatenation\nExample: {abcd} is equivalent to a b c d\n"},
    {"re operator: (A)","optionality","Equivalent to A | ε\nNote: parentheses inside logical formulas function as grouping, see ∀,∃\n"},
    {"re operator: @\"filename\"","read saved network from file","Note: loads networks stored with e.g. \"save stack\" but if file contains more than one network, only the first one is used in the regular expression.  See also \"load stack\" and \"load defined\"\n"},
    {"special symbol: Σ (or ?)","`any' symbol in r.e.",""},
    {"special symbol: ε (or 0, [])","epsilon symbol in r.e.",""},
    {"special symbol: ∅","the empty language symbol in r.e.",""},
    {"special symbol: .#.","word boundary symbol in replacements, restrictions","Signifies both end and beginning of word/string\nExample: A => B _ .#. (allow A only between B and end-of-string)\nExample: A -> B || .#. _ C (replace A with B if it occurs in the beginning of a word and is followed by C)\n"},
    {"operator precedence: ","see: `help precedence'","\\ `\n:\n+ * ^ ₁ ₂ ⁻¹ .f .r\n¬ $ $. $?\n(concatenation)\n> <\n∪ ∩ - .P. .p.\n=> -> (->) @-> etc.\n∥\n× ∘ .O.\nNote: compatibility variants (i.e. | = ∪ etc.) are not listed."},

    {NULL,NULL,NULL}
};

void iface_help() {
    struct global_help *gh;
    int i, maxlen;

    for (maxlen = 0, gh = global_help; gh->name != NULL; gh++) {
        maxlen = maxlen < utf8strlen(gh->name) ? utf8strlen(gh->name) : maxlen;
    }
    for (gh = global_help; gh->name != NULL; gh++) {
        printf("%s",gh->name);
        for (i = maxlen - utf8strlen(gh->name); i>=0; i--) {
            printf("%s"," ");
        }
        printf("%s\n",gh->help);
    }
}

void iface_ambiguous_upper() {
    if (iface_stack_check(1))
        stack_add(fsm_extract_ambiguous_domain(stack_pop()));
}

void iface_apropos(char *s) {
    struct global_help *gh;
    int i, maxlen;

    for (maxlen = 0, gh = global_help; gh->name != NULL; gh++) {
        if (strstr(gh->name,s) != NULL || strstr(gh->help,s) != NULL) {
            maxlen = maxlen < utf8strlen(gh->name) ? utf8strlen(gh->name) : maxlen;
        }
    }
    for (gh = global_help; gh->name != NULL; gh++) {
        if (strstr(gh->name,s) != NULL || strstr(gh->help,s) != NULL) {
            printf("%s",gh->name);
            for (i = maxlen - utf8strlen(gh->name); i>=0; i--) {
                printf("%s"," ");
            }
            printf("%s\n",gh->help);
        }
    }
}

void iface_help_search(char *s) {
    struct global_help *gh;
    
    for (gh = global_help; gh->name != NULL; gh++) {
        if (strstr(gh->name,s) != NULL || strstr(gh->help,s) != NULL) {
            printf("##\n");
            printf("%-32.32s%s\n%s\n",gh->name,gh->help,gh->longhelp);
        }
    }
}

void iface_print_bool(int value) {
    printf("%i (1 = TRUE, 0 = FALSE)\n",value);
}

void iface_warranty() {
    printf("%s",warranty);
}

void iface_apply_set_params(struct apply_handle *h) {
    apply_set_print_space(h, g_print_space);
    apply_set_print_pairs(h, g_print_pairs);
    apply_set_show_flags(h, g_show_flags);
    apply_set_obey_flags(h, g_obey_flags);
}

void iface_apply_med(char *word) {
    char *result;
    struct apply_med_handle *amedh;
    if (!iface_stack_check(1)) {
        return;
    }
    amedh = stack_get_med_ah();

    apply_med_set_heap_max(amedh,4194304+1);
    apply_med_set_med_limit(amedh,g_med_limit);
    apply_med_set_med_cutoff(amedh,g_med_cutoff);

    result = apply_med(amedh, word);
    if (result == NULL) {
        printf("???\n");
        return;
    } else {
        printf("%s\n",result);
	printf("%s\n", apply_med_get_instring(amedh));
	printf("Cost[f]: %i\n\n", apply_med_get_cost(amedh));
    }
    while ((result = apply_med(amedh,NULL)) != NULL) {
        printf("%s\n",result);
	printf("%s\n", apply_med_get_instring(amedh));
	printf("Cost[f]: %i\n\n", apply_med_get_cost(amedh));
    }
}

int iface_apply_file(char *infilename, char *outfilename, int direction) {
    char *result, inword[LINE_LIMIT];
    struct apply_handle *ah;
    FILE *OUTFILE, *INFILE;

    if (direction != AP_D && direction != AP_U) {
        perror("Invalid direction in iface_apply_file().\n");
        return 1;
    }
    if (!iface_stack_check(1)) { return 0; }
    INFILE = fopen(infilename, "r");
    if (INFILE == NULL) {
	fprintf(stderr, "%s: ", infilename);
        perror("Error opening file");
        return 1;
    }
    
    if (outfilename == NULL) {
        OUTFILE = stdout;
    } else {
        OUTFILE = fopen(outfilename, "w");
        printf("Writing output to file %s.\n", outfilename);
        if (OUTFILE == NULL) {
	    fprintf(stderr, "%s: ", outfilename);
            perror("Error opening output file.");
            return 1;
        }
    }
    ah = stack_get_ah();
    iface_apply_set_params(ah);
    while ((fgets(inword,LINE_LIMIT,INFILE)) != NULL) {
        if (inword[strlen(inword)-1] == '\n') {
            inword[strlen(inword)-1] = '\0';
        }

        fprintf(OUTFILE,"\n%s\n", inword);
        if (direction == AP_D)
            result = apply_down(ah,inword);
        else
            result = apply_up(ah,inword);
        
        if (result == NULL) {
            fprintf(OUTFILE,"???\n");
            continue;
        } else {
            fprintf(OUTFILE,"%s\n",result);
        }
        for (;;) {
            if (direction == AP_D)
                result = apply_down(ah,NULL);
            if (direction == AP_U)
                result = apply_up(ah,NULL);
            if (result == NULL)
                break;
            fprintf(OUTFILE,"%s\n", result);
        }
    }
    if (outfilename != NULL)
        fclose(OUTFILE);
    return 0;
}

void iface_apply_down(char *word) {
    int i;
    char *result;
    struct apply_handle *ah;
    if (!iface_stack_check(1)) {
        return;
    }
    ah = stack_get_ah();
    iface_apply_set_params(ah);
    result = apply_down(ah, word);
    if (result == NULL) {
        printf("???\n");
        return;
    } else {
        printf("%s\n",result);
    }
    for (i = g_list_limit; i > 0; i--) {
        result = apply_down(ah, NULL);
        if (result == NULL)
            break;
        printf("%s\n",result);
    }
}

void iface_apply_up(char *word) {
    int i;
    char *result;
    struct apply_handle *ah;
    if (!iface_stack_check(1)) {
        return;
    }
    ah = stack_get_ah();
    
    iface_apply_set_params(ah);
    result = apply_up(ah, word);

    if (result == NULL) {
        printf("???\n");
        return;
    } else {
        printf("%s\n",result);
    }
    for (i = g_list_limit; i > 0; i--) {
        result = apply_up(ah, NULL);
        if (result == NULL)
            break;
        printf("%s\n",result);
    }
}

void iface_close() {
    if (iface_stack_check(1)) {
      stack_add(fsm_topsort(fsm_minimize(fsm_close_sigma(stack_pop(),0))));
    }
}

void iface_compact() {
    if (iface_stack_check(1)) {
        fsm_compact(stack_find_top()->fsm);
	sigma_sort(stack_find_top()->fsm);
        stack_add(fsm_topsort(fsm_minimize(stack_pop())));
    }
}

void iface_complete() {
    if (iface_stack_check(1))
        stack_add(fsm_complete(stack_pop()));
}


void iface_compose() {
    struct fsm *one, *two;
    if (iface_stack_check(2)) {
        while (stack_size()>1) {
	    one = stack_pop();
	    two = stack_pop();
	    stack_add(fsm_topsort(fsm_minimize(fsm_compose(one,two))));
	}
    }
}

void iface_conc() {
    struct fsm *one, *two;
    if (iface_stack_check(2)) {
        while (stack_size()>1) {
	    printf("dd");
	    one = stack_pop();
	    two = stack_pop();
	    stack_add(fsm_topsort(fsm_minimize(fsm_concat(one,two))));
	}
    }
}

void iface_crossproduct() {
    struct fsm *one, *two;
    if (iface_stack_check(2)) {
	one = stack_pop();
	two = stack_pop();
        stack_add(fsm_topsort(fsm_minimize(fsm_cross_product(one,two))));
    }
}
void iface_determinize() {
    if (iface_stack_check(1))
        stack_add(fsm_determinize(stack_pop()));
}

void iface_eliminate_flags() {
    if (iface_stack_check(1))
        stack_add(flag_eliminate(stack_pop(), NULL));
}

void iface_extract_ambiguous() {
    if (iface_stack_check(1))
        stack_add(fsm_extract_ambiguous(stack_pop()));
}

void iface_extract_unambiguous() {
    if (iface_stack_check(1))
        stack_add(fsm_extract_unambiguous(stack_pop()));
}

int iface_extract_number(char *s) {
    int i;
    for (i=0; *(s+i) != '\0' && ((unsigned char) *(s+i) < '0' || (unsigned char) *(s+i) > '9'); i++) { }
    return(atoi(s+i));
}

void iface_eliminate_flag(char *name) {
    if (iface_stack_check(1))
        stack_add(flag_eliminate(stack_pop(), name));
}

void iface_factorize() {
    if (iface_stack_check(1))
        stack_add(fsm_bimachine(stack_pop()));
}

void iface_sequentialize() {
    if (iface_stack_check(1))
        stack_add(fsm_sequentialize(stack_pop()));
}

void iface_ignore() {
    struct fsm *one, *two;
    if (iface_stack_check(2)) {
	one = stack_pop();
	two = stack_pop();
        stack_add(fsm_topsort(fsm_minimize(fsm_ignore(one,two,OP_IGNORE_ALL))));
    }
}

void iface_intersect() {
    if (iface_stack_check(2)) {
        while (stack_size()>1)
            stack_add(fsm_topsort(fsm_minimize(fsm_intersect(stack_pop(),stack_pop()))));
    }
}

void iface_invert() {
    if (iface_stack_check(1))
        stack_add(fsm_invert(stack_pop()));
}

void iface_label_net() {
    if (iface_stack_check(1))
        stack_add(fsm_sigma_pairs_net(stack_pop()));
}

void iface_letter_machine() {
    if (iface_stack_check(1))
        stack_add(fsm_topsort(fsm_minimize(fsm_letter_machine(stack_pop()))));
}

void iface_load_defined(char *filename) {
    load_defined(g_defines, filename);
}

void iface_load_stack(char *filename) {
    struct fsm *net;
    fsm_read_binary_handle fsrh;

    if ((fsrh = fsm_read_binary_file_multiple_init(filename)) == NULL) {
	fprintf(stderr, "%s: ", filename);
        perror("File error");
        return;
    }
    while ((net = fsm_read_binary_file_multiple(fsrh)) != NULL)
        stack_add(net);
    return;
}

void iface_lower_side() {
    if (iface_stack_check(1))
        stack_add(fsm_topsort(fsm_minimize(fsm_lower(stack_pop()))));
}

void iface_minimize() {
    int store_minimal_var;
    if (iface_stack_check(1)) {
        store_minimal_var = g_minimal;
        g_minimal = 1;
        stack_add(fsm_topsort(fsm_minimize(stack_pop())));
        g_minimal = store_minimal_var;
    }
}

void iface_one_plus() {
    if (iface_stack_check(1))
        stack_add(fsm_topsort(fsm_minimize(fsm_kleene_plus(stack_pop()))));
}

void iface_pop() {
    struct fsm *net;
    if (stack_size() < 1)
        printf("Stack is empty.\n");
    else {
        net = stack_pop();
	fsm_destroy(net);
    }
}

void iface_lower_words(int limit) {
    char *result;
    struct apply_handle *ah;
    int i;
    if (!iface_stack_check(1)) {
        return;
    }
    limit = (limit == -1) ? g_list_limit : limit;
    if (iface_stack_check(1)) {
      ah = stack_get_ah();
      iface_apply_set_params(ah);
        for (i = limit; i > 0; i--) {
            result = apply_lower_words(ah);
            if (result == NULL)
                break;
            printf("%s\n",result);
        }
	apply_reset_enumerator(ah);
    }
}

void iface_name_net(char *name) {
    if (iface_stack_check(1)) {
        strncpy(stack_find_top()->fsm->name, name, 40);
        iface_print_name();
    }
}
void iface_negate() {
    if (iface_stack_check(1))
        stack_add(fsm_topsort(fsm_minimize(fsm_complement(stack_pop()))));
}

void iface_print_dot(char *filename) {
    if (iface_stack_check(1)) {
        if (filename != NULL)
            printf("Writing dot file to %s.\n",filename);
        print_dot(stack_find_top()->fsm, filename);
    }
}

void iface_print_net(char *netname, char *filename) {
    struct fsm *net;
    if (netname != NULL) {
        if ((net = find_defined(g_defines, netname)) == NULL) {
            printf("No defined network %s.\n", netname);
            return;
        }
        print_net(net, filename);
    } else {
        if (iface_stack_check(1))
            print_net(stack_find_top()->fsm, filename);
    }
}

void iface_print_cmatrix_att(char *filename) {
    FILE *outfile;
    if (iface_stack_check(1)) {
        if (stack_find_top()->fsm->medlookup == NULL || stack_find_top()->fsm->medlookup->confusion_matrix == NULL) {
            printf("No confusion matrix defined.\n");
        } else {
            if (filename == NULL) {
                outfile = stdout;
            } else {
                outfile = fopen(filename,"w");
                printf("Writing confusion matrix to file '%s'.\n", filename);
            }
            cmatrix_print_att(stack_find_top()->fsm, outfile);
        }
    }
}

void iface_print_cmatrix() {
    if (iface_stack_check(1)) {
        if (stack_find_top()->fsm->medlookup == NULL || stack_find_top()->fsm->medlookup->confusion_matrix == NULL) {
            printf("No confusion matrix defined.\n");
        } else {
            cmatrix_print(stack_find_top()->fsm);
        }
    }
}

void iface_print_defined() {
    struct defined_networks  *defined;
    struct defined_functions *defined_f;
    if (g_defines == NULL) {
        printf("No defined symbols.\n");
    }
    for (defined = g_defines; defined != NULL; defined = defined->next) {
	if (defined->name != NULL) {
	    printf("%s\t",defined->name);
	    print_stats(defined->net);
	}
    }
    for (defined_f = g_defines_f; defined_f != NULL; defined_f = defined_f->next) {
	if (defined_f->name != NULL) {
		printf("%s@%i)\t",defined_f->name,defined_f->numargs);
		printf("%s\n",defined_f->regex);
	}
    }
}

void iface_print_name() {
    if (iface_stack_check(1))
        printf("%s\n",stack_find_top()->fsm->name);
}

void iface_quit() {
    struct fsm *net;
    remove_defined(g_defines, NULL);
    while (!(stack_isempty())) {
        net = stack_pop();
        fsm_destroy(net);
    }
    exit(0);
}

void iface_random_lower(int limit) {
    iface_apply_random(&apply_random_lower, limit);
}

void iface_random_upper(int limit) {
    iface_apply_random(&apply_random_upper, limit);
}

void iface_random_words(int limit) {
    iface_apply_random(&apply_random_words, limit);
}

void iface_apply_random(char *(*applyer)(), int limit) {
    char *result;
    struct apply_handle *ah;
    int i;
    struct apply_results {
	char *string;
	int count;
    } *results, *tempresults;

    limit = (limit == -1) ? g_list_random_limit : limit;
    if (iface_stack_check(1)) {
	results = xxcalloc(limit, sizeof(struct apply_results));
	ah = stack_get_ah();
	iface_apply_set_params(ah);
        for (i = limit; i > 0; i--) {
	    result = NULL;
            result = applyer(ah);
            if (result != NULL) {
		for (tempresults = results; tempresults - results < limit; tempresults++) {
		    if (tempresults->string == NULL) {
			tempresults->string = xxstrdup(result);
			tempresults->count = 1;
			break;
		    }
		    else if (strcmp(tempresults->string, result) == 0) {
			tempresults->count++;
			break;
		    }
		}
	    }
        }
	for (tempresults = results; tempresults - results < limit; tempresults++) {
	    if (tempresults->string != NULL) {
		printf("[%i] %s\n", tempresults->count, tempresults->string);
		xxfree(tempresults->string);
	    }
	}
	xxfree(results);
	apply_reset_enumerator(ah);
    }
}

void iface_print_sigma() {
    if (iface_stack_check(1))
        print_sigma(stack_find_top()->fsm->sigma,stdout);
}
void iface_print_stats() {
    if (iface_stack_check(1))
        print_stats(stack_find_top()->fsm);
}

void iface_print_shortest_string() {
    /* L -  ?+  [[L .o. [?:"@TMP@"]*].l .o. "@TMP@":?*].l; */
    struct fsm *Result, *ResultU, *ResultL, *one, *onel, *oneu;
    struct apply_handle *ah;
    char *word;
    if (iface_stack_check(1)) {
        one = fsm_copy(stack_find_top()->fsm);
        /* L -  ?+  [[L .o. [?:"@TMP@"]*].l .o. "@TMP@":?*].l; */
        if (stack_find_top()->fsm->arity == 1) {
            Result = fsm_minimize(fsm_minus(fsm_copy(one),fsm_concat(fsm_kleene_plus(fsm_identity()),fsm_lower(fsm_compose(fsm_lower(fsm_compose(fsm_copy(one),fsm_kleene_star(fsm_cross_product(fsm_identity(),fsm_symbol("@TMP@"))))),fsm_kleene_star(fsm_cross_product(fsm_symbol("@TMP@"),fsm_identity())))))));
            ah = apply_init(Result);
            word = apply_words(ah);
            if (word != NULL) printf("%s\n",word);
	    apply_clear(ah);
        } else {
            onel = fsm_lower(fsm_copy(one));
            oneu = fsm_upper(one);
            ResultU = fsm_minimize(fsm_minus(fsm_copy(oneu),fsm_concat(fsm_kleene_plus(fsm_identity()),fsm_lower(fsm_compose(fsm_lower(fsm_compose(fsm_copy(oneu),fsm_kleene_star(fsm_cross_product(fsm_identity(),fsm_symbol("@TMP@"))))),fsm_kleene_star(fsm_cross_product(fsm_symbol("@TMP@"),fsm_identity())))))));
            ResultL = fsm_minimize(fsm_minus(fsm_copy(onel),fsm_concat(fsm_kleene_plus(fsm_identity()),fsm_lower(fsm_compose(fsm_lower(fsm_compose(fsm_copy(onel),fsm_kleene_star(fsm_cross_product(fsm_identity(),fsm_symbol("@TMP@"))))),fsm_kleene_star(fsm_cross_product(fsm_symbol("@TMP@"),fsm_identity())))))));
            ah = apply_init(ResultU);
            word = apply_words(ah);
            if (word == NULL) word = "";
            printf("Upper: %s\n",word);
	    apply_clear(ah);
            ah = apply_init(ResultL);
            word = apply_words(ah);
            if (word == NULL) word = "";
            printf("Lower: %s\n",word);
	    apply_clear(ah);
        }
    }
}

void iface_print_shortest_string_size() {
    struct fsm *Result, *ResultU, *ResultL, *one, *onel, *oneu;
    if (iface_stack_check(1)) {
        one = fsm_copy(stack_find_top()->fsm);
        /* [L .o. [?:a]*].l; */
        if (stack_find_top()->fsm->arity == 1) {
            Result = fsm_minimize(fsm_lower(fsm_compose(one,fsm_kleene_star(fsm_cross_product(fsm_identity(),fsm_symbol("a"))))));
            printf("Shortest acyclic path length: %i\n",Result->statecount-1);

        } else {
            onel = fsm_lower(fsm_copy(one));
            oneu = fsm_upper(one);
            ResultU = fsm_minimize(fsm_lower(fsm_compose(oneu,fsm_kleene_star(fsm_cross_product(fsm_identity(),fsm_symbol("a"))))));
            ResultL = fsm_minimize(fsm_lower(fsm_compose(onel,fsm_kleene_star(fsm_cross_product(fsm_identity(),fsm_symbol("a"))))));
            printf("Shortest acyclic upper path length: %i\n",(ResultU->statecount)-1);
            printf("Shortest acyclic lower path length: %i\n",(ResultL->statecount)-1);
        }
    }
}

int iface_read_att(char *filename) {
    struct fsm *tempnet;
    printf("Reading AT&T file: %s\n",filename);
    tempnet = read_att(filename);
    if (tempnet == NULL) {
	fprintf(stderr, "%s: ", filename);
        perror("Error opening file");
        return 1;
    } else {
        stack_add(tempnet);
        return 0;
    }
}

int iface_read_prolog(char *filename) {
    struct fsm *tempnet;
    printf("Reading prolog: %s\n",filename);
    tempnet = fsm_read_prolog(filename);
    if (tempnet == NULL) {
	fprintf(stderr, "%s: ", filename);
        perror ("Error opening file");
        return 1;
    } else {
        stack_add(tempnet);
        return 0;
    }
}

int iface_read_spaced_text(char *filename) {
    struct fsm *net;
    net = fsm_read_spaced_text_file(filename);
    if (net == NULL) {
	fprintf(stderr, "%s: ", filename);
	perror("File error");
	return 1;
    }
    stack_add(fsm_topsort(fsm_minimize(net)));
    return 0;
}

int iface_read_text(char *filename) {
    struct fsm *net;
    net = fsm_read_text_file(filename);
    if (net == NULL) {
	fprintf(stderr, "%s: ", filename);
	perror("File error");
	return 1;
    }
    stack_add(fsm_topsort(fsm_minimize(net)));
    return 0;
}

int iface_stack_check (int size) {
    if (stack_size() < size) {
        printf("Not enough networks on stack. Operation requires at least %i.\n",size);
        return 0;
    }
    return 1;
}

void iface_substitute_symbol (char *original, char *substitute) {
    if (iface_stack_check(1)) {
        dequote_string(original);
        dequote_string(substitute);
        stack_add(fsm_topsort(fsm_minimize(fsm_substitute_symbol(stack_pop(), original, substitute))));
        printf("Substituted '%s' for '%s'.\n", substitute, original);
    }
}

void iface_substitute_defined (char *original, char *substitute) {
    struct fsm *subnet;
    struct fsm *newnet;
    if (iface_stack_check(1)) {
        dequote_string(original);
        dequote_string(substitute);
	if ((subnet = find_defined(g_defines, substitute)) == NULL) {
	    printf("No defined network '%s'.\n",substitute);
	} else {
	    if (fsm_symbol_occurs(stack_find_top()->fsm, original, M_UPPER + M_LOWER) == 0) {
		printf("Symbol '%s' does not occur.\n", original);
	    } else {
		newnet = fsm_substitute_label(stack_find_top()->fsm, original, subnet);
		stack_pop();
		printf("Substituted network '%s' for '%s'.\n", substitute, original);
		stack_add(fsm_topsort(fsm_minimize(newnet)));
	    }
	}
    }
}

void iface_upper_words(int limit) {
    char *result;
    struct apply_handle *ah;
    int i;
    limit = (limit == -1) ? g_list_limit : limit;
    if (iface_stack_check(1)) {
        ah = stack_get_ah();
	iface_apply_set_params(ah);
        for (i = limit; i > 0; i--) {
            result = apply_upper_words(ah);
            if (result == NULL)
                break;
            printf("%s\n",result);
        }
	apply_reset_enumerator(ah);
    }
}

void iface_prune() {
    if (iface_stack_check(1))
        stack_add(fsm_topsort(fsm_coaccessible(stack_pop())));
}
void iface_reverse() {
    if (iface_stack_check(1))
        stack_add(fsm_topsort(fsm_determinize(fsm_reverse((stack_pop())))));
}
void iface_rotate() {
    if (iface_stack_check(1))
        stack_rotate();
}
void iface_save_defined(char *filename) {
    save_defined(g_defines, filename);
}

#if defined(ORIGINAL) || defined(ZLIB)
void iface_save_stack(char *filename) {
    gzFile outfile;
    struct stack_entry *stack_ptr;

    if (iface_stack_check(1)) {
      if ((outfile = gzopen(filename, "wb")) == NULL) {
            printf("Error opening file %s for writing.\n", filename);
            return;
        }
        printf("Writing to file %s.\n", filename);
        for (stack_ptr = stack_find_bottom(); stack_ptr->next != NULL; stack_ptr = stack_ptr->next) {
          foma_net_print(stack_ptr->fsm, outfile);
        }
        gzclose(outfile);
        return;
    }
}
#else
void iface_save_stack(char *filename) {
  fprintf(stderr, "IFACE_C_ERROR\n"); exit(1); }
#endif

void iface_show_variables() {
    int i;
    for (i=0; global_vars[i].name != NULL; i++) {
        if (global_vars[i].type == FVAR_BOOL) {
            printf("%-17.17s: %s\n",global_vars[i].name, *((int *)(global_vars[i].ptr)) == 1 ? "ON" : "OFF");
        }
        if (global_vars[i].type == FVAR_INT) {
            printf("%-17.17s: %i\n",global_vars[i].name, *((int *)(global_vars[i].ptr)));
        }
        if (global_vars[i].type == FVAR_STRING) {
            printf("%-17.17s: %s\n",global_vars[i].name, *((char **)(global_vars[i].ptr)) );
        }
    }
}
void iface_show_variable(char *name) {
    int i;
    for (i=0; global_vars[i].name != NULL; i++) {
        if (strncmp(name,global_vars[i].name,8) == 0) {
	    printf("%s = %s\n",global_vars[i].name, *((int *)(global_vars[i].ptr)) == 1 ? "ON" : "OFF");
            return;
        }
    }
    printf("*There is no global variable '%s'.\n",name);
}

void iface_set_variable(char *name, char *value) {
    int i,j;
    char *endptr;
    for (i=0; global_vars[i].name != NULL; i++) {
        if (strncmp(name,global_vars[i].name,8) == 0) {
            if (global_vars[i].type == FVAR_BOOL) {
                if ((strcmp(value,"ON") == 0) || (strcmp(value, "1") == 0)) {
                    j = 1;
                } else if ((strcmp(value,"OFF") == 0) || (strcmp(value, "0") == 0)) {
                    j = 0;
                } else {
                    printf("Invalid value '%s' for variable '%s'\n",value, global_vars[i].name);
                    return;
                }
                *((int *)(global_vars[i].ptr)) = j;
                printf("variable %s = %s\n",global_vars[i].name, *((int *)(global_vars[i].ptr)) == 1 ? "ON" : "OFF");
                return;
            }
            if (global_vars[i].type == FVAR_STRING) {
                *((char **)(global_vars[i].ptr)) = xxstrdup(value);
                printf("variable %s = %s\n",global_vars[i].name, value);
                return;
            }
            if (global_vars[i].type == FVAR_INT) {
                errno = 0;
                j = strtol(value, &endptr, 10);
                if ((errno != 0 || endptr == value) || j < 0) {
                    printf("invalid value %s for variable %s\n", value, global_vars[i].name);
                    return;
                } else {
                    printf("variable %s = %i\n", global_vars[i].name, j);
                    *((int *)(global_vars[i].ptr)) = j;
                    return;
                }
            }
        }
    }
    printf("*There is no global variable '%s'.\n",name);
}

void iface_shuffle() {
    if (iface_stack_check(2))
        while (stack_size()>1)
            stack_add(fsm_minimize(fsm_shuffle(stack_pop(),stack_pop())));
}

void iface_sigma_net() {
    if (iface_stack_check(1))
        stack_add(fsm_sigma_net(stack_pop()));
}

void iface_sort_input() {
    if (iface_stack_check(1)) {
        fsm_sort_arcs(stack_find_top()->fsm,1);
    }
}

void iface_sort_output() {
    if (iface_stack_check(1)) {
        fsm_sort_arcs(stack_find_top()->fsm,2);
    }
}

void iface_sort() {
    if (iface_stack_check(1)) {
        sigma_sort(stack_find_top()->fsm);
        stack_add(fsm_topsort(stack_pop()));
    }
}


void iface_test_equivalent() {
  struct fsm *one, *two;
    if (iface_stack_check(2)) {
        one = fsm_copy(stack_find_top()->fsm);
        two = fsm_copy(stack_find_second()->fsm);
	fsm_count(one);
	fsm_count(two);
	
	//if (one->arccount != two->arccount || one->statecount != two->statecount || one->finalcount != two->finalcount) {
	//iface_print_bool(0);
	    //} else {
	    iface_print_bool(fsm_equivalent(one, two));
	    //iface_print_bool(fsm_isempty(fsm_union(fsm_minus(fsm_copy(one),fsm_copy(two)),fsm_minus(fsm_copy(two),fsm_copy(one)))));
	    //}
    }
}

void iface_test_functional() {
    if (iface_stack_check(1))
        iface_print_bool(fsm_isfunctional(stack_find_top()->fsm));
}

void iface_test_identity() {
    if (iface_stack_check(1))
        iface_print_bool(fsm_isidentity(stack_find_top()->fsm));
}

void iface_test_nonnull() {
    if (iface_stack_check(1))
        iface_print_bool(!fsm_isempty(fsm_copy(stack_find_top()->fsm)));
}

void iface_test_null() {
    if (iface_stack_check(1))
        iface_print_bool(fsm_isempty(fsm_copy(stack_find_top()->fsm)));
}

void iface_test_unambiguous() {
    if (iface_stack_check(1))
        iface_print_bool(fsm_isunambiguous(stack_find_top()->fsm));
}

void iface_test_lower_universal() {
    if (iface_stack_check(1))
        iface_print_bool(fsm_isempty(fsm_complement(fsm_lower(fsm_copy(stack_find_top()->fsm)))));
}

void iface_test_sequential() {
    if (iface_stack_check(1))
        iface_print_bool(fsm_issequential(stack_find_top()->fsm));
}

void iface_test_upper_universal() {
    if (iface_stack_check(1))
        iface_print_bool(fsm_isempty(fsm_complement(fsm_upper(fsm_copy(stack_find_top()->fsm)))));
}

void iface_turn() {
    if (iface_stack_check(1))
        stack_rotate();
}

void iface_twosided_flags() {
  if (iface_stack_check(1)) {
    stack_add(flag_twosided(stack_pop()));
  }
}

void iface_union() {
    if (iface_stack_check(2))
        while (stack_size()>1)
            stack_add(fsm_minimize(fsm_union(stack_pop(),stack_pop())));
}
void iface_upper_side() {
    if (iface_stack_check(1))
        stack_add(fsm_topsort(fsm_minimize(fsm_upper(stack_pop()))));
}

void iface_view() {
    if (iface_stack_check(1))
        view_net(stack_find_top()->fsm);
}

void iface_words_file(char *filename, int type) {
    /* type 0 (words), 1 (upper-words), 2 (lower-words) */
    FILE *outfile;
    char *result;
    static char *(*applyer)() = &apply_words;
    struct apply_handle *ah;

    if (type == 1) {
	applyer = &apply_upper_words;
    }
    if (type == 2) {
	applyer = &apply_lower_words;
    }
    if (iface_stack_check(1)) {
	if (stack_find_top()->fsm->pathcount == PATHCOUNT_CYCLIC) {
	    printf("FSM is cyclic: can't write all words to file.\n");
	    return;
	}
	printf("Writing to %s.\n",filename);
	if ((outfile = fopen(filename, "w")) == NULL) {
	    perror("Error opening file");
	    return;
	}
        ah = stack_get_ah();
	iface_apply_set_params(ah);
        for (;;) {
            result = applyer(ah);
            if (result == NULL)
                break;
            fprintf(outfile,"%s\n",result);
        }
        apply_reset_enumerator(ah);
	fclose(outfile);
    }
}

void iface_words(int limit) {
    char *result;
    struct apply_handle *ah;
    int i;
    limit = (limit == -1) ? g_list_limit : limit;
    if (iface_stack_check(1)) {
        ah = stack_get_ah();
	iface_apply_set_params(ah);
        for (i = limit; i > 0; i--) {
            result = apply_words(ah);
            if (result == NULL)
                break;
            printf("%s\n",result);
        }
        apply_reset_enumerator(ah);
    }
}

/* Splits string of upper:lower pairs with space separator into two strings */
/* e.g. a:b c:d e 0:g => ace,bdeg  */

void iface_split_string(char *result, char *string) {
    int i;
    char space = '\001', epsilon = '\002', separator = '\003';
    /* Simulate: SEPARATOR \SPACE+ @-> 0 .o. SPACE|SEPARATOR|EPSILON -> 0 */
    /*           to extract only upper side of string                     */
    for (i = 0 ; ; ) {
    zero:
	if (result[i] == '\0') {
	    break;
	} else if (result[i] == space || result[i] == epsilon) {
	    i++;
	    goto zero;
	} else if (result[i] == separator) {
	    i++;
	    goto one;
	} else {
	    strncat(string, result+i, 1);
	    i++;
	    goto zero;
	}
    one:
	if (result[i] == '\0') {
	    break;
	} else if (result[i] == space) {
	    i++;
	    goto zero;
	} else {
	    i++;
	    goto one;
	}
    }
}

void iface_split_result(char *result, char **upper, char **lower) {
    *upper = calloc(strlen(result), sizeof(char));
    *lower = calloc(strlen(result), sizeof(char));
    /* Split string into upper by filtering input side */
    /* and lower by the same filter, but reversed      */
    iface_split_string(result, *upper);
    xstrrev(result);
    iface_split_string(result, *lower);
    xstrrev(*lower);
    xstrrev(result);
}


void iface_pairs_call(int limit, int random) {
    char *result, *upper, *lower;
    struct apply_handle *ah;
    int i;
    limit = (limit == -1) ? g_list_limit : limit;
    if (iface_stack_check(1)) {
        ah = stack_get_ah();
	apply_set_show_flags(ah, g_show_flags);
	apply_set_obey_flags(ah, g_obey_flags);
	apply_set_space_symbol(ah, "\001");
	apply_set_epsilon(ah, "\002");
	apply_set_separator(ah, "\003");
        for (i = limit; i > 0; i--) {
	    if (random == 1)
		result = apply_random_words(ah);
	    else
		result = apply_words(ah);
            if (result == NULL)
                break;
	    iface_split_result(result, &upper, &lower);
            printf("%s\t%s\n",upper, lower);
	    xxfree(upper);
	    xxfree(lower);
        }
	apply_set_space_symbol(ah, " ");
	apply_set_epsilon(ah, "0");
	apply_set_separator(ah, ":");
        apply_reset_enumerator(ah);
    }
}

void iface_random_pairs(int limit) {
    iface_pairs_call(limit, 1);
}

void iface_pairs(int limit) {
    iface_pairs_call(limit, 0);
}

void iface_pairs_file(char *filename) {
    FILE *outfile;
    char *result, *upper, *lower;
    struct apply_handle *ah;
    if (iface_stack_check(1)) {
	if (stack_find_top()->fsm->pathcount == PATHCOUNT_CYCLIC) {
	    printf("FSM is cyclic: can't write all pairs to file.\n");
	    return;
	}
	printf("Writing to %s.\n",filename);
	if ((outfile = fopen(filename, "w")) == NULL) {
	    perror("Error opening file");
	    return;
	}
	ah = stack_get_ah();
	apply_set_show_flags(ah, g_show_flags);
	apply_set_obey_flags(ah, g_obey_flags);
	apply_set_space_symbol(ah, "\001");
	apply_set_epsilon(ah, "\002");
	apply_set_separator(ah, "\003");
        for (;;) {
	    result = apply_words(ah);
            if (result == NULL)
                break;
	    iface_split_result(result, &upper, &lower);
	    fprintf(outfile, "%s\t%s\n", upper, lower);
	    xxfree(upper);
	    xxfree(lower);
        }
	apply_set_space_symbol(ah, " ");
	apply_set_epsilon(ah, "0");
	apply_set_separator(ah, ":");
        apply_reset_enumerator(ah);
	fclose(outfile);
    }
}

int iface_write_att(char *filename) {
    FILE    *outfile;
    struct fsm *net;
    if (!iface_stack_check(1)) {
	return 1;
    }
    net = stack_find_top()->fsm;
    if (filename == NULL) {
        outfile = stdout;
    } else {
        printf("Writing AT&T file: %s\n",filename);
        outfile = fopen(filename, "w");
        if(outfile == NULL) {
	    fprintf(stderr, "%s: ", filename);
            perror("File error opening.");
            return 1;
        }
    }
    net_print_att(net, outfile);
    if (filename != NULL)
        fclose(outfile);
    return 0;
}

void iface_write_prolog(char *filename) {
  if (iface_stack_check(1))
    foma_write_prolog(stack_find_top()->fsm, filename);
}

void iface_zero_plus() {
    if (iface_stack_check(1))
        stack_add(fsm_topsort(fsm_minimize(fsm_kleene_star(stack_pop()))));
}

static char *sigptr(struct sigma *sigma, int number) {
    char *mystr;
    if (number == EPSILON)
        return "0";
    if (number == UNKNOWN)
        return "?";
    if (number == IDENTITY)
        return "@";

    for (; sigma != NULL; sigma = sigma->next) {
        if (sigma->number == number) {
            if (strcmp(sigma->symbol,"0") == 0)
                return("\"0\"");
            if (strcmp(sigma->symbol,"?") == 0)
                return("\"?\"");
            if (strcmp(sigma->symbol,"\n") == 0)
                return("\\n");
            if (strcmp(sigma->symbol,"\r") == 0)
                return("\\r");
            return (sigma->symbol);
        }
    }
    mystr = xxmalloc(sizeof(char)*40);
    snprintf(mystr, 40, "NONE(%i)",number);
    return(mystr);
}

static int print_net(struct fsm *net, char *filename) {
  struct fsm_state *stateptr;
  int previous_state = -1, i;
  int *finals;
  FILE *out;
  if (filename == NULL) {
      out = stdout;
  } else {
      if ((out = fopen(filename, "w")) == NULL) {
          printf("Error writing to file %s. Using stdout.\n", filename);
          out = stdout;
      }
      printf("Writing network to file %s.\n", filename);
  }
  fsm_count(net);
  finals = xxmalloc(sizeof(int)*(net->statecount));
  stateptr = net->states;

  for (i=0; (stateptr+i)->state_no != -1; i++) {
    if ((stateptr+i)->final_state == 1) {
      *(finals+((stateptr+i)->state_no)) = 1;
    } else {
      *(finals+((stateptr+i)->state_no)) = 0;
    }
    if ((stateptr+i)->in != (stateptr+i)->out) {
      net->arity = 2;
    }
  }
  print_sigma(net->sigma, out);
  fprintf(out,"Net: %s\n",net->name);
  fprintf(out,"Flags: ");
  if (net->is_deterministic == YES) { fprintf(out,"deterministic ");}
  if (net->is_pruned == YES) { fprintf(out,"pruned ");}
  if (net->is_minimized == YES) { fprintf(out,"minimized ");}
  if (net->is_epsilon_free == YES) { fprintf(out,"epsilon_free ");}
  if (net->is_loop_free) { fprintf(out,"loop_free "); }
  if (net->arcs_sorted_in) { fprintf(out,"arcs_sorted_in "); }
  if (net->arcs_sorted_out) { fprintf(out,"arcs_sorted_out "); }
  fprintf(out,"\n");
  fprintf(out,"Arity: %i\n", net->arity);
  for (; stateptr->state_no != -1; stateptr++) {
    if (stateptr->state_no != previous_state) {
      if (stateptr->start_state) {
          fprintf(out,"S");
      }
      if (stateptr->final_state) {
          fprintf(out,"f");
      }
      if (stateptr->in==-1) {
          fprintf(out,"s%i:\t(no arcs).\n",stateptr->state_no);
	continue;
      } else {
          fprintf(out,"s%i:\t",stateptr->state_no);
      }
    }
    previous_state = stateptr->state_no;
    if (stateptr->in == stateptr->out) {
      if (stateptr->in == IDENTITY) {
          fprintf(out,"@ -> ");
      } else if (stateptr->in == UNKNOWN) {
          fprintf(out,"?:? -> ");
      } else {
          fprintf(out,"%s -> ",sigptr(net->sigma, stateptr->in));
      }
    } else {
        fprintf(out,"<%s:%s> -> ",sigptr(net->sigma, stateptr->in),sigptr(net->sigma, stateptr->out));
    }
    if (*(finals+(stateptr->target)) == 1) {
        fprintf(out,"f");
    }
    fprintf(out,"s%i",stateptr->target);
    if ((stateptr+1)->state_no == stateptr->state_no) {
        fprintf(out,", ");
    } else {
        fprintf(out,".\n");
    }

  }
  if (filename != NULL) {
      fclose(out);
  }
  xxfree(finals);
  return 0;
}

void print_mem_size(struct fsm *net) {
    char size[20];
    struct sigma *sigma;
    unsigned int s;
    float sf;
    s = 0;
    for (sigma = net->sigma; sigma != NULL && sigma->number != -1; sigma = sigma->next) {
        s += strlen(sigma->symbol)+1+sizeof(struct sigma);
    }
    s += sizeof(struct fsm);
    s += sizeof(struct fsm_state) * net->linecount;
    sf = s;
    if (s < 1024) {
        sprintf(size, "%i bytes. ", s);
    } else if (s >= 1024 && s < 1048576) {
        sprintf(size, "%.1f kB. ", sf/1024);
    } else if (s >= 1048576 && s < 1073741824) {
        sprintf(size, "%.1f MB. ", sf/1048576);
    } else if (s >= 1073741824) {
        sprintf(size, "%.1f GB. ", sf/1073741824);
    }
    fprintf(stdout, "%s", size);
    fflush(stdout);
}

int print_stats(struct fsm *net) {
    print_mem_size(net);
    if (net->statecount == 1) { printf("1 state, "); } else { printf("%i states, ",net->statecount); }
    if (net->arccount == 1)   { printf("1 arc, "); } else { printf("%i arcs, ",net->arccount); }
    if (net->pathcount == 1)
        printf("1 path");
    else if (net->pathcount == -1)
        printf("Cyclic");
    else if (net->pathcount == -2)
        printf("more than %lld paths",LLONG_MAX);
    else if (net->pathcount == -3)
        printf("unknown number of paths");
    else
        printf("%lld paths",net->pathcount);
    printf(".\n");
    return 0;
}

static int print_sigma(struct sigma *sigma, FILE *out) {
  int size;
  fprintf (out,"Sigma:");
  for (size = 0; sigma != NULL; sigma = sigma->next) {
      if (sigma->number > 2) {
          fprintf(out," %s",(sigma->symbol));
          size++;
      }
      if (sigma->number == IDENTITY) {
          fprintf(out," %s","@");
      }
      if (sigma->number == UNKNOWN) {
          fprintf(out," %s","?");
      }
  }
  fprintf(out,"\n");
  fprintf(out,"Size: %i.\n",size);
  return(1);
}

static int print_dot(struct fsm *net, char *filename) {
    struct fsm_state *stateptr;
    FILE *dotfile;
    int i, j, linelen;
    short *finals, *printed;
    
    fsm_count(net);
    
    finals = xxmalloc(sizeof(short)*net->statecount);
    stateptr = net->states;
    
    for (i=0; (stateptr+i)->state_no != -1; i++) {
        if ((stateptr+i)->final_state == 1) {
            *(finals+((stateptr+i)->state_no)) = 1;
        } else {
            *(finals+((stateptr+i)->state_no)) = 0;
        }
    }
    
    if (filename != NULL) {
        dotfile = fopen(filename,"w");
    } else {
        dotfile = stdout;
    }

  fprintf(dotfile,"digraph A {\nrankdir = LR;\n");
  /* Go through states */
  for (i=0; i < net->statecount; i++) {
    if (*(finals+i)) {
      fprintf(dotfile,"node [shape=doublecircle,style=filled] %i\n",i);
    } else {
      fprintf(dotfile,"node [shape=circle,style=filled] %i\n",i);
    }
  }

  printed = xxcalloc(net->linecount,sizeof(printed));
  /* Go through arcs */
  for (i=0; (stateptr+i)->state_no != -1; i++) {
      if ((stateptr+i)->target == -1 || printed[i] == 1)
          continue;
      fprintf(dotfile,"%i -> %i [label=\"", (stateptr+i)->state_no, (stateptr+i)->target);
      linelen = 0;
      for (j=i; (stateptr+j)->state_no == (stateptr+i)->state_no; j++) {
          if (((stateptr+i)->target == ((stateptr+j)->target)) && printed[j] == 0) {
              printed[j] = 1;

              if (((stateptr+j)->in == ((stateptr+j)->out)) && (stateptr+j)->out != UNKNOWN ) {
                  fprintf(dotfile,"%s", escape_string(sigptr(net->sigma, (stateptr+j)->in),'"'));
                  linelen += strlen((sigptr(net->sigma, (stateptr+j)->in)));
              } else {
                  fprintf(dotfile,"<%s:%s>", escape_string(sigptr(net->sigma, (stateptr+j)->in),'"'), escape_string(sigptr(net->sigma, (stateptr+j)->out),'"'));
                  linelen += strlen((sigptr(net->sigma, (stateptr+j)->in))) + strlen(sigptr(net->sigma, (stateptr+j)->out)) + 3;
              }
              if (linelen > 12) {
                  fprintf(dotfile, "\\n");
                  linelen = 0;
              } else {
                  fprintf(dotfile, " ");
              }
          }
      }
      fprintf(dotfile,"\"];\n");
  }

  
  xxfree(finals);
  xxfree(printed);
  fprintf(dotfile, "}\n");
  if (filename != NULL)
      fclose(dotfile);
  return(1);
}

static int view_net(struct fsm *net) {

  char tmpstr[255];
  char *dotname;
#ifndef __APPLE__
  char *pngname;
#endif  /* __APPLE__ */

  dotname = strncpy(tmpstr,tempnam(NULL,"foma"), 250);
  strcat(dotname, ".dot");
  dotname = xxstrdup(tmpstr);
  print_dot(net, dotname);

#ifdef __APPLE__
  sprintf(tmpstr,"/usr/bin/open -a Graphviz %s &",dotname);
  if (system(tmpstr) == -1)
      printf("Error opening viewer.\n");
  
#endif /* __APPLE__ */

#ifndef __APPLE__
  pngname = xxstrdup(tempnam(NULL, "foma"));
  sprintf(tmpstr,"dot -Tpng %s > %s ",dotname,pngname);
  if (system(tmpstr) == -1)
      printf("Error writing tempfile.\n");
  sprintf(tmpstr,"/usr/bin/xdg-open %s 2>/dev/null &",pngname);
  if (system(tmpstr) == -1)
      printf("Error opening viewer.\n");
  xxfree(pngname);
#endif /* __APPLE__ */

  xxfree(dotname);
  
  return(1);
}
