/*
   Test file for HfstTokenizer.
*/

#include "HfstTransducer.h"
#include "auxiliary_functions.cc"

using namespace hfst;

int
main(int argc, char **argv)
{

    /* From one string. */
    verbose_print("Tokenization from one string with multichar symbol "
                  "\"foo\" and skip symbol \"bar\"");

    HfstTokenizer tok1;
    tok1.add_multichar_symbol("foo");
    tok1.add_skip_symbol("bar");
    StringPairVector tokenization1 = tok1.tokenize("fobaro", false);
    assert(tokenization1.size() == 3);
    assert(tokenization1[0] == StringPair("f", "f"));
    assert(tokenization1[1] == StringPair("o", "o"));
    assert(tokenization1[2] == StringPair("o", "o"));

    verbose_print("Tokenization from one string with multichar symbol "
                  "\"foo\" and skip symbol \"fo\"");

    HfstTokenizer tok2;
    tok2.add_multichar_symbol("foo");
    tok2.add_skip_symbol("fo");
    StringPairVector tokenization2 = tok2.tokenize("foo", false);
    assert(tokenization2.size() == 1);
    assert(tokenization2[0] == StringPair("foo", "foo"));

    verbose_print("Tokenization from one string with multichar symbol "
                  "\"fo\" and skip symbol \"foo\"");

    HfstTokenizer tok3;
    tok3.add_multichar_symbol("fo");
    tok3.add_skip_symbol("foo");
    StringPairVector tokenization3 = tok3.tokenize("foo", false);
    assert(tokenization3.size() == 0);

    /* From two strings. */
    verbose_print("Tokenization from two strings with multichar symbol "
                  "\"foo\" and skip symbol \"bar\"");

    HfstTokenizer tok4;
    tok4.add_multichar_symbol("foo");
    tok4.add_skip_symbol("bar");
    StringPairVector tokenization4 = tok4.tokenize("fooba", "foobar", false);
    assert(tokenization4.size() == 3);
    assert(tokenization4[0] == StringPair("foo", "foo"));
    assert(tokenization4[1] == StringPair("b", "@_EPSILON_SYMBOL_@"));
    assert(tokenization4[2] == StringPair("a", "@_EPSILON_SYMBOL_@"));

    // Utf-8 correctness testing.
    HfstTokenizer utf8_tokenizer;
    char line[100];

    // Positive cases

    // Empty string
    line[0] = 0;
    try
    {
        utf8_tokenizer.check_utf8_correctness(line);
    }
    catch (const IncorrectUtf8CodingException &e)
    {
        assert(false);
    }

    // ASCII string "ab".
    line[0] = 'a';
    line[1] = 'b';
    line[2] = 0;
    try
    {
        utf8_tokenizer.check_utf8_correctness(line);
    }
    catch (const IncorrectUtf8CodingException &e)
    {
        assert(false);
    }

    // 11110000:10011111:10010010:10101001
    verbose_print("Case: 11110000 10011111 10010010 10101001");
    line[0] = get_bin(1, 1, 1, 1, 0, 0, 0, 0);
    line[1] = get_bin(1, 0, 0, 1, 1, 1, 1, 1);
    line[2] = get_bin(1, 0, 0, 1, 0, 0, 1, 0);
    line[3] = get_bin(1, 0, 1, 0, 1, 0, 0, 1);
    line[4] = 0;

    try
    {
        utf8_tokenizer.check_utf8_correctness(line);
    }
    catch (const IncorrectUtf8CodingException &e)
    {
        assert(false);
    }

    // Four byte, three byte, two byte and single byte sequence.
    // (these are just overlong variants of nulls...)
    verbose_print("Case: 11110000, 10000000, 10000000, 10000000,\n");
    verbose_print("11100000, 10000000, 10000000, 11010000, 10000000,\n");
    verbose_print("01000000");

    line[0] = get_bin(1, 1, 1, 1, 0, 0, 0, 0);
    line[1] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[2] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[3] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[4] = get_bin(1, 1, 1, 0, 0, 0, 0, 0);
    line[5] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[6] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[7] = get_bin(1, 1, 0, 1, 0, 0, 0, 0);
    line[8] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[9] = get_bin(0, 1, 0, 0, 0, 0, 0, 0);
    line[10] = 0;

    try
    {
        utf8_tokenizer.check_utf8_correctness(line);
        assert(false);
    }
    catch (const IncorrectUtf8CodingException &e)
    {
    }

    // Negative Cases

    // String with byte 192.
    line[0] = get_bin(1, 1, 0, 0, 0, 0, 0, 0);
    line[1] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[2] = 0;
    try
    {
        utf8_tokenizer.check_utf8_correctness(line);
        assert(false);
    }
    catch (const IncorrectUtf8CodingException &e)
    {
    };

    // String with byte 193.
    line[0] = get_bin(1, 1, 0, 0, 0, 0, 0, 1);
    line[1] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[2] = 0;
    try
    {
        utf8_tokenizer.check_utf8_correctness(line);
        assert(false);
    }
    catch (const IncorrectUtf8CodingException &e)
    {
    };

    // String with byte 245.
    line[0] = get_bin(1, 1, 1, 1, 0, 1, 0, 1);
    line[1] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[2] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[3] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[4] = 0;
    try
    {
        utf8_tokenizer.check_utf8_correctness(line);
        assert(false);
    }
    catch (const IncorrectUtf8CodingException &e)
    {
    };

    // String with byte 246.
    line[0] = get_bin(1, 1, 1, 1, 0, 1, 1, 0);
    line[1] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[2] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[3] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[4] = 0;
    try
    {
        utf8_tokenizer.check_utf8_correctness(line);
        assert(false);
    }
    catch (const IncorrectUtf8CodingException &e)
    {
    };

    // String with byte 247.
    line[0] = get_bin(1, 1, 1, 1, 0, 1, 1, 1);
    line[1] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[2] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[3] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[4] = 0;
    try
    {
        utf8_tokenizer.check_utf8_correctness(line);
        assert(false);
    }
    catch (const IncorrectUtf8CodingException &e)
    {
    };

    // ASCII character followed by a continuation character.
    line[0] = get_bin(0, 1, 0, 0, 0, 0, 0, 0);
    line[1] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[2] = 0;
    try
    {
        utf8_tokenizer.check_utf8_correctness(line);
        assert(false);
    }
    catch (const IncorrectUtf8CodingException &e)
    {
    };

    // 11010000 followed by ASCII character
    line[0] = get_bin(1, 1, 0, 1, 0, 0, 0, 0);
    line[1] = get_bin(0, 1, 0, 0, 0, 0, 0, 0);
    line[2] = 0;
    try
    {
        utf8_tokenizer.check_utf8_correctness(line);
        assert(false);
    }
    catch (const IncorrectUtf8CodingException &e)
    {
    };

    // 11100000 followed by one continuation character and an ASCII character
    line[0] = get_bin(1, 1, 1, 0, 0, 0, 0, 0);
    line[1] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[2] = get_bin(0, 1, 0, 0, 0, 0, 0, 0);
    line[3] = 0;
    try
    {
        utf8_tokenizer.check_utf8_correctness(line);
        assert(false);
    }
    catch (const IncorrectUtf8CodingException &e)
    {
    };

    // 11110000 followed by two continuation characters and an ASCII character
    line[0] = get_bin(1, 1, 1, 1, 0, 0, 0, 0);
    line[1] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[2] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[3] = get_bin(0, 1, 0, 0, 0, 0, 0, 0);
    line[4] = 0;
    try
    {
        utf8_tokenizer.check_utf8_correctness(line);
        assert(false);
    }
    catch (const IncorrectUtf8CodingException &e)
    {
    };

    // 11110000 followed by two continuation characters and 11010000,10000000
    line[0] = get_bin(1, 1, 1, 1, 0, 0, 0, 0);
    line[1] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[2] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[3] = get_bin(1, 1, 0, 1, 0, 0, 0, 0);
    line[4] = get_bin(1, 0, 0, 0, 0, 0, 0, 0);
    line[5] = 0;
    try
    {
        utf8_tokenizer.check_utf8_correctness(line);
        assert(false);
    }
    catch (const IncorrectUtf8CodingException &e)
    {
    };
}
