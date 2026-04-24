#include "string_handling.h"

#ifndef MAIN_TEST

#include <exception>
#include <sstream>

InvalidLine::InvalidLine(const std::string &representation)
    : representation(representation)
{
}

InvalidLine::InvalidLine(const InvalidLine &e,
                         const std::string &representation)
    : representation(e.representation + " " + representation)
{
}

InvalidFloat::InvalidFloat(const std::string &representation)
    : representation(representation)
{
}

InvalidFloat::InvalidFloat(const InvalidFloat &e,
                           const std::string &representation)
    : representation(e.representation + " " + representation)
{
}

WeightedStringVector::WeightedStringVector(const StringVector &v, float f)
    : string_vector(v), weight(f)
{
}

float
get_non_negative_float(const std::string &float_string)
{
    std::istringstream in(float_string);

    float f;
    in >> f;

    if (in.fail() or f < 0)
    {
        return -1;
    }

    return f;
}

StringVector
split(const std::string &line, char separator)
{
    StringVector res;

    size_t bpos = -1;
    size_t epos = -1;

    do
    {
        bpos = epos + 1;
        epos = line.find(separator, bpos);

        res.push_back(line.substr(bpos, epos - bpos));
    } while (epos != std::string::npos);

    return res;
}

WeightedStringVector
tokenize_lexicon_line(const std::string &line, HfstTokenizer &tokenizer)
{
    StringVector split_line = split(line, '\t');

    if (split_line.size() != 2 and split_line.size() != 3)
    {
        throw InvalidLine(__FILE__ " " STR(__LINE__));
    }

    std::string word = split_line[0];

    std::string tag;
    std::string weight_str = split_line[1];

    if (split_line.size() == 3)
    {
        tag = split_line[1];
        weight_str = split_line[2];
    }

    StringVector entry = tokenizer.tokenize_one_level(word, false);

    if (tag != "")
    {
        entry.push_back(tag);
    }

    float weight = get_non_negative_float(weight_str);

    if (weight == -1)
    {
        throw InvalidFloat(weight_str);
    }

    return WeightedStringVector(entry, weight);
}

WeightedStringVector
tokenize_grammar_line(const std::string &line, HfstTokenizer &tokenizer)
{
    (void)tokenizer;
    StringVector split_line = split(line, '\t');

    if ((split_line.size() % 2) != 1 or split_line.size() < 3)
    {
        throw InvalidLine(__FILE__ " " STR(__LINE__));
    }

    StringVector entry(split_line.begin(), split_line.end() - 1);

    std::string weight_str = split_line.back();

    float weight = get_non_negative_float(split_line.back());

    if (weight == -1)
    {
        throw InvalidFloat(weight_str);
    }

    return WeightedStringVector(entry, weight);
}

std::string
read_model_start_tag(const std::string &line)
{
    if (line.find("START ") != 0)
    {
        throw InvalidLine(__FILE__ " " STR(__LINE__));
    }

    std::string model_name = line.substr(6);

    if (model_name.find("\t") != std::string::npos)
    {
        throw InvalidLine(__FILE__ " " STR(__LINE__));
    }

    return model_name;
}

std::string
read_model_stop_tag(const std::string &line)
{
    if (line.find("STOP ") != 0)
    {
        throw InvalidLine(__FILE__ " " STR(__LINE__));
    }

    std::string model_name = line.substr(5);

    if (model_name.find("\t") != std::string::npos)
    {
        throw InvalidLine(__FILE__ " " STR(__LINE__));
    }

    return model_name;
}

float
read_model_penalty_weight(const std::string &line)
{
    if (line.find("PENALTY_WEIGHT=") != 0)
    {
        throw InvalidLine(line);
    }

    return get_non_negative_float(
        line.substr(std::string("PENALTY_WEIGHT=").size()));
}

#else // MAIN_TEST was defined

/* UNIT TESTS */

#include "../unit_test_aux.h"

int
main(void)
{
    // TESTING

    HfstTokenizer tokenizer;

    /* Empty lines, strings without weights should throw an exception. */
    try
    {
        static_cast<void>(tokenize_lexicon_line("", tokenizer));
        fail_test(__FILE__, __LINE__);
    }
    catch (const InvalidLine &e)
    {
        static_cast<void>(e);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    try
    {
        static_cast<void>(tokenize_lexicon_line("oof\tNN", tokenizer));
        fail_test(__FILE__, __LINE__);
    }
    catch (const InvalidFloat &e)
    {
        static_cast<void>(e);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    /* Strings with erroneous weights should throw an exception. */
    try
    {
        static_cast<void>(tokenize_lexicon_line("oof\tNN\tw", tokenizer));
        fail_test(__FILE__, __LINE__);
    }
    catch (const InvalidFloat &e)
    {
        static_cast<void>(e);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    /* Basic case. Should work. */
    try
    {
        WeightedStringVector v
            = tokenize_lexicon_line("oof\tNN\t0.5", tokenizer);
        if (v.string_vector.size() != 4)
        {
            throw InvalidLine(__FILE__ " " STR(__LINE__));
        }

        if (v.string_vector[0] != "o" or v.string_vector[1] != "o"
            or v.string_vector[2] != "f" or v.weight != 0.5)
        {
            throw InvalidLine(__FILE__ " " STR(__LINE__));
        }
    }
    catch (const InvalidLine &e)
    {
        std::cerr << "From line: " << e.representation << std::endl;
        fail_test(__FILE__, __LINE__);
    }
    catch (const InvalidFloat &e)
    {
        std::cerr << "From line: " << e.representation << std::endl;
        fail_test(__FILE__, __LINE__);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    /* Empty word. Should work. */
    try
    {
        WeightedStringVector v = tokenize_lexicon_line("\tNN\t0.5", tokenizer);

        if (v.string_vector.size() != 1)
        {
            throw InvalidLine(__FILE__ " " STR(__LINE__));
        }

        if (v.string_vector[0] != "NN" and v.weight != 0.5)
        {
            throw InvalidLine(__FILE__ " " STR(__LINE__));
        }
    }
    catch (const InvalidLine &e)
    {
        std::cerr << "From line: " << e.representation << std::endl;
        fail_test(__FILE__, __LINE__);
    }
    catch (const InvalidFloat &e)
    {
        std::cerr << "From line: " << e.representation << std::endl;
        fail_test(__FILE__, __LINE__);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    /* utf-8 chars. Should work. */
    try
    {
        WeightedStringVector v
            = tokenize_lexicon_line("älä\tNN\t0.5", tokenizer);

        if (v.string_vector.size() != 4)
        {
            throw InvalidLine(__FILE__ " " STR(__LINE__));
        }

        if (v.string_vector[0] != "ä" and v.string_vector[1] != "l"
            and v.string_vector[2] != "ä" and v.weight != 0.5)
        {
            throw InvalidLine(__FILE__ " " STR(__LINE__));
        }
    }
    catch (const InvalidLine &e)
    {
        std::cerr << "From line: " << e.representation << std::endl;
        fail_test(__FILE__, __LINE__);
    }
    catch (const InvalidFloat &e)
    {
        std::cerr << "From line: " << e.representation << std::endl;
        fail_test(__FILE__, __LINE__);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    /* Empty lines, lines with only weight and lines with an odd number
       of tags should throw an excpetion. */
    try
    {
        static_cast<void>(tokenize_lexicon_line("", tokenizer));
        fail_test(__FILE__, __LINE__);
    }
    catch (const InvalidLine &e)
    {
        static_cast<void>(e);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    try
    {
        static_cast<void>(tokenize_lexicon_line("0.5", tokenizer));
        fail_test(__FILE__, __LINE__);
    }
    catch (const InvalidLine &e)
    {
        static_cast<void>(e);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    try
    {
        static_cast<void>(
            tokenize_lexicon_line("NN\tVBZ\tDT\t0.5", tokenizer));
        fail_test(__FILE__, __LINE__);
    }
    catch (const InvalidLine &e)
    {
        static_cast<void>(e);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    /* Basic case. Should work. */
    try
    {
        WeightedStringVector v
            = tokenize_grammar_line("DT\tNN\t0.5", tokenizer);

        if (v.string_vector.size() != 2)
        {
            throw InvalidLine(__FILE__ " " STR(__LINE__));
        }

        if (v.string_vector[0] != "DT" or v.string_vector[1] != "NN"
            or v.weight != 0.5)
        {
            throw InvalidLine(__FILE__ " " STR(__LINE__));
        }
    }
    catch (const InvalidLine &e)
    {
        std::cerr << "From line: " << e.representation << std::endl;
        fail_test(__FILE__, __LINE__);
    }
    catch (const InvalidFloat &e)
    {
        std::cerr << "From line: " << e.representation << std::endl;
        fail_test(__FILE__, __LINE__);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    // Empty model name fails.
    try
    {
        static_cast<void>(read_model_start_tag(""));
    }
    catch (const InvalidLine &e)
    {
        static_cast<void>(e);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    // Model name without START tag fails.
    try
    {
        static_cast<void>(read_model_start_tag("foo"));
    }
    catch (const InvalidLine &e)
    {
        static_cast<void>(e);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    // Model name with tab fails.
    try
    {
        static_cast<void>(read_model_start_tag("START MODEL\t1"));
    }
    catch (const InvalidLine &e)
    {
        static_cast<void>(e);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    // Regular model name works.
    try
    {
        std::string name = read_model_start_tag("START MODEL");

        if (name != "MODEL")
        {
            throw InvalidLine(__FILE__ " " STR(__LINE__));
        }
    }
    catch (const InvalidLine &e)
    {
        std::cerr << "From line: " << e.representation << std::endl;
        fail_test(__FILE__, __LINE__);
    }
    catch (const InvalidFloat &e)
    {
        std::cerr << "From line: " << e.representation << std::endl;
        fail_test(__FILE__, __LINE__);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    // Empty model name fails.
    try
    {
        static_cast<void>(read_model_stop_tag(""));
    }
    catch (const InvalidLine &e)
    {
        static_cast<void>(e);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    // Model name without STOP tag fails.
    try
    {
        static_cast<void>(read_model_stop_tag("foo"));
    }
    catch (const InvalidLine &e)
    {
        static_cast<void>(e);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    // Model name with tab fails.
    try
    {
        static_cast<void>(read_model_stop_tag("STOP MODEL\t1"));
    }
    catch (const InvalidLine &e)
    {
        static_cast<void>(e);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    // Regular model name works.
    try
    {
        std::string name = read_model_stop_tag("STOP MODEL");

        if (name != "MODEL")
        {
            throw InvalidLine(__FILE__ " " STR(__LINE__));
        }
    }
    catch (const InvalidLine &e)
    {
        std::cerr << "From line: " << e.representation << std::endl;
        fail_test(__FILE__, __LINE__);
    }
    catch (const InvalidFloat &e)
    {
        std::cerr << "From line: " << e.representation << std::endl;
        fail_test(__FILE__, __LINE__);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Caught: " << e.what() << std::endl;
        fail_test(__FILE__, __LINE__);
    }

    // TEST PASSED

    exit(0);
}

#endif // MAIN_TEST
