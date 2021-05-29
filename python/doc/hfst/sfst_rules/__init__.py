## @package hfst.sfst_rules
# rules...

## A transducer that obligatorily performs the mappings defined by \a mappings in the context \a context when the alphabet is \a alphabet.
#
# @param context A pair of transducers where the first transducer defines the left context and the second transducer the right context.
# @param mappings A set of mappings (a tuple of string pairs) that the resulting transducer will perform in the context given in \a context.
# @param alphabet The set of symbol pairs (a tuple of string pairs) that defines the alphabet (see the example).
#
# For example, a transducer yielded by the following arguments (in pseudcode)
# \verbatim
# context = pair( [c|d], [e] )
# mappings = set(a:b)
# alphabet = set(a, a:b, b, c, d, e, ...)
# \endverbatim
# obligatorily maps the symbol a to b if c or d precedes and e follows. (Elsewhere,
# the mapping of a to b is optional). This expression is identical to ![.* [c|d] [a:. & !a:b] [e] .*]
# Note that the alphabet must contain the pair a:b here.
# @see <a href="https://www.cis.lmu.de/~schmid/tools/SFST/data/SFST-Manual.pdf">SFST manual</a>
def two_level_if(context, mappings, alphabet):
    pass

     
## A transducer that allows the mappings defined by \a mappings only in the context \a context, when the alphabet is \a alphabet.
#
# If called with the same arguments as in the example of #two_level_if, the transducer
# allows the mapping of symbol a to b only if c or d precedes and e follows.
# The mapping of a to b is optional in this context but cannot occur in any other context.
#
# The expression is equivalent to
# ![  [ ![.* [c|d]] a:b .* ] | [ .* a:b ![[e] .*] ]  ]
#
# @see #two_level_if
def two_level_only_if(context, mappings, alphabet):
    pass
 
## A transducer that always performs the mappings defined by \a mappings in the context \a context and only in that context, when the alphabet is \a alphabet.
#
# If called with the same arguments as in the example of #two_level_if, the transducer
# maps symbol a to b only and only if c or d precedes and e follows.
# The mapping of a to b is obligatory in this context and cannot occur in any other context.
#
# The expression is equivalent to ![.* [c|d] [a:. & !a:b] [e] .*]  &
# ![  [ ![.* [c|d]] a:b .* ] | [ .* a:b ![[e] .*] ]  ]
#
# @see #two_level_if
def two_level_if_and_only_if(context, mappings, alphabet):
    pass
 
## A transducer that performs an upward mapping \a mapping in the context \a context when the alphabet is \a alphabet. \a optional defines whether the mapping is optional.
#
# @param context A pair of transducers where the first transducer defines the left context and the second transducer the right context. Both transducers must be automata, i.e. map strings onto themselves.
# @param mapping The mapping (transducer) that the resulting transducer will perform in the context given in \a context.
# @param optional Whether the mapping is optional.
# @param alphabet The set of symbol pairs that defines the alphabet (a tuple of string pairs).
#
# Each substring s of the input string which is in the input language of the transducer \a mapping and whose left context is matched
# by the expression [.* l] (where l is the first element of \a context) and whose right context is matched by [r .*]
# (where r is the second element in the context) is mapped to the respective surface strings defined by transducer \a mapping. Any other
# character is mapped to the characters specified in \a alphabet. The left and right contexts must
# be automata (i.e. transducers which map strings onto themselves).
#
# For example, a transducer yielded by the following arguments (in pseudocode)
# \verbatim
# context = pair( [c], [c] )
# mappings = [ a:b a:b ]
# alphabet = set(a, b, c)
# \endverbatim
# would map the string "caacac" to "cbbcac".
#
# Note that the alphabet must contain the characters a and b, but not the pair
# a:b (unless this replacement is to be allowed everywhere in the context).
#
# Note that replace operations (unlike the two-level rules) have to be combined by composition
# rather than intersection.
#
# @throws ContextTransducersAreNotAutomataException
#
# @see <a href="https://www.cis.lmu.de/~schmid/tools/SFST/data/SFST-Manual.pdf">SFST manual</a>
def replace_up(context, mapping, optional, alphabet):
    pass

## The same as replace_up, but matching is done on the output side of \a mapping
#
# @see replace_up
# <a href="https://www.cis.lmu.de/~schmid/tools/SFST/data/SFST-Manual.pdf">SFST manual</a>.
def replace_down(context, mapping, optional, alphabet):
    pass

## TODO: document
def replace_down_karttunen(context, mapping, optional, alphabet):
    pass
 
## The same as replace_up, but left context matching is done on the output side of \a mapping and right context on the input side of \a mapping.
#
# @see replace_up
def replace_left(context, mapping, optional, alphabet):
    pass

## The same as replace_up, but left context matching is done on the input side of \a mapping and right context on the output side of \a mapping.
#
# @see replace_up
# <a href="https://www.cis.lmu.de/~schmid/tools/SFST/data/SFST-Manual.pdf">SFST manual</a>.
def replace_right(context, mapping, optional, alphabet):
    pass

## The same as replace_up but \a mapping is performed in every context.
#
# @see replace_up
def replace_up(mapping, optional, alphabet):
    pass

## The same as replace_down(context, mapping, optional, alphabet) but \a mapping is performed in every context.
#
# @see replace_up
def replace_down(mapping, optional, alphabet):
    pass

## Inversion of the replace_up and the result needs to be composed on the upper side of the input language.
#
# B <- A is the inversion of A -> B.
# \a mapping is performed in every context.
# @see replace_up
def left_replace_up(mapping, optional, alphabet):
    pass

## Inversion of the replace_up and the result needs to be composed on the upper side of the input language.
#
# B <- A is the inversion of A -> B.
# @see replace_up
def left_replace_up(context, mapping, optional, alphabet):
    pass

## Inversion of the replace_up and the result needs to be composed on the upper side of the input language. However, matching is done on the output side of \a mapping.
#
# @see replace_up
def left_replace_down(context, mapping, optional, alphabet):
    pass
 
## Inversion of the replace_up and the result needs to be composed on the upper side of the input language. However, matching is done on the output side of \a mapping.
#
# @see replace_up
def left_replace_down_karttunen(context, mapping, optional, alphabet):
    pass
 
## Inversion of the replace_up and the result needs to be composed on the upper side of the input language. However, left context matching is done on the input side of \a mapping and right context on the output side of \a mapping.
#
# @see replace_up */
def left_replace_left(context, mapping, optional, alphabet):
    pass
 
## Inversion of the replace_up and the result needs to be composed on the upper side of the input language. However, left context matching is done on the output side of \a mapping and right context on the input side of \a mapping.
#
# @see replace_up
def left_replace_right(context, mapping, optional, alphabet):
    pass



## A transducer that allows any (substring) mapping defined by \a mapping only if it occurs in any of the contexts in \a contexts. Symbols outside of the matching substrings are mapped to any symbol allowed by \a alphabet.
#
# @throws EmptySetOfContextsException
#
# @see <a href="https://www.cis.lmu.de/~schmid/tools/SFST/data/SFST-Manual.pdf">SFST manual</a>.
# @param contexts A tuple of HfstTransducer pairs.
# @param mapping An HfstTransducer.
# @param alphabet A tuple of string pairs.
def restriction(contexts, mapping, alphabet):
    pass

## A transducer that requires that one of the mappings defined by \a mapping must occur in each context in \a contexts. Symbols outside of the matching substrings are mapped to any symbol allowed by \a alphabet.
#
# @see <a href="https://www.cis.lmu.de/~schmid/tools/SFST/data/SFST-Manual.pdf">SFST manual</a>.
# @param contexts A tuple of HfstTransducer pairs.
# @param mapping An HfstTransducer.
# @param alphabet A tuple of string pairs.
def coercion(contexts, mapping, alphabet):
    pass

## A transducer that is equivalent to the intersection of restriction and coercion and requires that the mappings defined by \a mapping occur always and only in the given contexts in \a contexts. Symbols outside of the matching substrings are mapped to any symbol allowed by \a alphabet.
#
# @see #restriction #coercion <a href="https://www.cis.lmu.de/~schmid/tools/SFST/data/SFST-Manual.pdf">SFST manual</a>
# @param contexts A tuple of HfstTransducer pairs.
# @param mapping An HfstTransducer.
# @param alphabet A tuple of string pairs.
def restriction_and_coercion(contexts, mapping, alphabet):
    pass

 
## A transducer that specifies that a string from the input language of the transducer \a mapping may only be mapped to one of its output strings (according to transducer \a mapping) if it appears in any of the contexts in \a contexts. Symbols outside of the matching substrings are mapped to any symbol allowed by \a alphabet.
#
# @see <a href="https://www.cis.lmu.de/~schmid/tools/SFST/data/SFST-Manual.pdf">SFST manual</a>.
# @param contexts A tuple of HfstTransducer pairs.
# @param mapping An HfstTransducer.
# @param alphabet A tuple of string pairs.
def surface_restriction(contexts, mapping, alphabet):
    pass

## A transducer that specifies that a string from the input language of the transducer \a mapping always has to the mapped to one of its output strings according to transducer \a mapping if it appears in any of the contexts in \a contexts. Symbols outside of the matching substrings are mapped to any symbol allowed by \a alphabet.
#
# @see <a href="https://www.cis.lmu.de/~schmid/tools/SFST/data/SFST-Manual.pdf">SFST manual</a>.
# @param contexts A tuple of HfstTransducer pairs.
# @param mapping An HfstTransducer.
# @param alphabet A tuple of string pairs.
def surface_coercion(contexts, mapping, alphabet):
    pass

## A transducer that is equivalent to the intersection of surface_restriction and surface_coercion.
#
# @see #surface_restriction #surface_coercion <a href="https://www.cis.lmu.de/~schmid/tools/SFST/data/SFST-Manual.pdf">SFST manual</a>.
# @param contexts A tuple of HfstTransducer pairs.
# @param mapping An HfstTransducer.
# @param alphabet A tuple of string pairs.
def surface_restriction_and_coercion(contexts, mapping, alphabet):
    pass

## A transducer that specifies that a string from the output language of the transducer \a mapping may only be mapped to one of its input strings (according to transducer \a mappings) if it appears in any of the contexts in \a contexts.Symbols outside of the matching substrings are mapped to any symbol allowed by \a alphabet.
#
# @see <a href="https://www.cis.lmu.de/~schmid/tools/SFST/data/SFST-Manual.pdf">SFST manual</a>.
# @param contexts A tuple of HfstTransducer pairs.
# @param mapping An HfstTransducer.
# @param alphabet A tuple of string pairs.
def deep_restriction(contexts, mapping, alphabet):
    pass

## A transducer that specifies that a string from the output language of the transducer \a mapping always has to be mapped to one of its input strings (according to transducer \a mappings) if it appears in any of the contexts in \a contexts. Symbols outside of the matching substrings are mapped to any symbol allowed by \a alphabet.
#
# @see <a href="https://www.cis.lmu.de/~schmid/tools/SFST/data/SFST-Manual.pdf">SFST manual</a>.
# @param contexts A tuple of HfstTransducer pairs.
# @param mapping An HfstTransducer.
# @param alphabet A tuple of string pairs.
def deep_coercion(contexts, mapping, alphabet):
    pass

## A transducer that is equivalent to the intersection of deep_restriction and deep_coercion.
#
# @see #deep_restriction #deep_coercion <a href="https://www.cis.lmu.de/~schmid/tools/SFST/data/SFST-Manual.pdf">SFST manual</a>.
# @param contexts A tuple of HfstTransducer pairs.
# @param mapping An HfstTransducer.
# @param alphabet A tuple of string pairs.
def deep_restriction_and_coercion(contexts, mapping, alphabet):
    pass
