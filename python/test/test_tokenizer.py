# -*- coding: utf-8 -*-
import sys
if len(sys.argv) > 1:
    sys.path.insert(0, sys.argv[1])
import hfst

for type in [hfst.ImplementationType.SFST_TYPE, hfst.ImplementationType.TROPICAL_OPENFST_TYPE, hfst.ImplementationType.FOMA_TYPE]:
    if hfst.HfstTransducer.is_implementation_type_available(type):
        
        tok = hfst.HfstTokenizer()
        tok.add_multichar_symbol('foo')
        tok.add_skip_symbol('b')
        assert tok.tokenize('foobar', False) == (('foo', 'foo'), ('a', 'a'), ('r', 'r'))
        assert tok.tokenize_one_level('foobar', False) == ('foo', 'a', 'r')
        assert tok.tokenize('foobar','barfoo', False) == (('foo', 'a'), ('a', 'r'), ('r', 'foo'))
        
        tok = hfst.HfstTokenizer()
        
        tok = hfst.HfstTokenizer()
        tok.add_skip_symbol('foo')
        assert tok.tokenize_one_level('foofofoobar', False) == ('f', 'o', 'b', 'a', 'r')
        
        tok = hfst.HfstTokenizer()
        tok.add_multichar_symbol('foo')
        tok.add_skip_symbol('fo')
        assert tok.tokenize_one_level('foofo', False) == ('foo',)
        
        tok = hfst.HfstTokenizer()
        tok.add_multichar_symbol('fo')
        tok.add_skip_symbol('foo')
        assert tok.tokenize_one_level('foofo', False) == ('fo',)
        
        tok = hfst.HfstTokenizer()
        tok.add_multichar_symbol('fo')
        tok.add_multichar_symbol('foo')
        tok.add_multichar_symbol('of')
        assert tok.tokenize_one_level('fofoofooof', False) == ('fo', 'foo', 'foo', 'of')
        
        tok = hfst.HfstTokenizer()
        t = tok.tokenize('foobar', False)
        assert t == (('f', 'f'), ('o', 'o'), ('o', 'o'), ('b', 'b'), ('a', 'a'), ('r', 'r'))
        
        tok = hfst.HfstTokenizer()
        t = tok.tokenize_one_level('foobar', False)
        assert t == ('f', 'o', 'o', 'b', 'a', 'r')
        
        tok = hfst.HfstTokenizer()
        tok.add_multichar_symbol('foo')
        tok.add_skip_symbol('b')
        assert tok.tokenize('foobar','Foobar', False) == \
            (('foo', 'F'), ('a', 'o'), ('r', 'o'), ('@_EPSILON_SYMBOL_@', 'a'), ('@_EPSILON_SYMBOL_@', 'r'))
        
        tok = hfst.HfstTokenizer()
        assert tok.tokenize_space_separated('f o o b a r') == \
            (('f', 'f'), ('o', 'o'), ('o', 'o'), ('b', 'b'), ('a', 'a'), ('r', 'r'))
        assert tok.tokenize_space_separated('foo b a r') == \
            (('foo', 'foo'), ('b', 'b'), ('a', 'a'), ('r', 'r'))
        assert tok.tokenize_space_separated('f o o bar') == \
            (('f', 'f'), ('o', 'o'), ('o', 'o'), ('bar', 'bar'))
        assert tok.tokenize_space_separated('foo bar') == \
            (('foo', 'foo'), ('bar', 'bar'))
        assert tok.tokenize_space_separated('foobar') == \
            (('foobar', 'foobar'),)

        if int(sys.version[0]) > 2:
            try:
                hfst.HfstTokenizer.check_utf8_correctness('föö')
            except hfst.exceptions.IncorrectUtf8CodingException as e:
                assert(False)
