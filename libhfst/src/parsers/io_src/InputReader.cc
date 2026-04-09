//! @file InputReader.cc
//!
//! @author Miikka Silfverberg

//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published
//   by the Free Software Foundation, version 3 of the Licence.
//
//   This library is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "InputReader.h"
#include <unistd.h>

#define COLOUR_BOLD "\033[01m"
#define COLOUR_RED "\033[31m"
#define COLOUR_GREEN "\033[32m"
#define COLOUR_YELLOW "\033[33m"
#define COLOUR_BLUE "\033[34m"
#define COLOUR_MAGENTA "\033[35m"
#define COLOUR_CYAN "\033[36m"
#define COLOUR_RESET "\033[0m"

static bool
should_colourise()
{
    if (isatty(1))
    {
        return true;
    }
    else
    {
        return false;
    }
    return false;
}

InputReader::InputReader(size_t &counter)
    : input_stream(NULL), filename("<unknown>"), counter(counter),
      buffer_size(HTWOLCBUFFERSIZE), buffer_index(0), warning_stream(NULL),
      error_stream(NULL)
{
}

void
InputReader::reset()
{
    input_stream = NULL;
    // counter is a reference, so it must be reset separately
    buffer_size = HTWOLCBUFFERSIZE;
    buffer_index = 0;
    warning_stream = NULL;
    error_stream = NULL;
}

void
InputReader::set_input(std::istream &file)
{
    input_stream = &file;
    filename = "<stdin>";
    input_stream->getline(buffer, buffer_size);
}

void
InputReader::set_input(std::istream &file, const std::string &filename)
{
    input_stream = &file;
    this->filename = filename;
    input_stream->getline(buffer, buffer_size);
}

void
InputReader::set_warning_stream(std::ostream &ostr)
{
    warning_stream = &ostr;
}

void
InputReader::set_error_stream(std::ostream &ostr)
{
    error_stream = &ostr;
}

void
InputReader::warn(const std::string &warning)
{
    if (warning_stream != NULL)
    {
        *warning_stream << std::endl;
        if (should_colourise())
        {
            *warning_stream << COLOUR_BOLD;
        }
        *warning_stream << filename << ":" << counter << ": ";
        if (should_colourise())
        {
            *warning_stream << COLOUR_RESET << COLOUR_YELLOW;
        }
        *warning_stream << warning << ":" << std::endl;
        if (should_colourise())
        {
            *warning_stream << COLOUR_RESET;
        }
        *warning_stream << buffer << std::endl;
    }
}

void
InputReader::error(const std::string &err)
{
    if (error_stream != NULL)
    {
        *error_stream << std::endl;
        if (should_colourise())
        {
            *error_stream << COLOUR_BOLD;
        }
        *error_stream << filename << ":" << counter << ": ";
        if (should_colourise())
        {
            *error_stream << COLOUR_RESET << COLOUR_RED;
        }
        *error_stream << err << ":" << std::endl;
        if (should_colourise())
        {
            *error_stream << COLOUR_RESET;
        }
        *error_stream << buffer << std::endl;
        *error_stream << "Aborted." << std::endl << std::endl;
    }
}

char
InputReader::input(void)
{
    if (input_stream == NULL)
    {
        throw InputNotSet();
    }
    if (buffer[buffer_index] == 0)
    {
        input_stream->getline(buffer, buffer_size);
        // XXX?
        if (input_stream->gcount() == 0)
        {
            return 0;
        }
        buffer_index = 0;
        return '\n';
    }
    return buffer[buffer_index++];
}

#ifdef INPUT_READER_TEST
#include <cassert>
#include <sstream>
int
main(void)
{
    std::string str1("Some text spanning one line only.");
    std::istringstream in1(str1);
    size_t counter = 1;
    InputReader ir1(counter);
    try
    {
        ir1.input();
        assert(false);
    }
    catch (const InputNotSet &e)
    { /* nothing */
    }
    ir1.set_input(in1);
    assert(ir1.input() == 'S');
    assert(ir1.input() == 'o');
    assert(ir1.input() == 'm');
    assert(ir1.input() == 'e');
    assert(ir1.input() == ' ');
    assert(ir1.input() == 't');
    assert(ir1.input() == 'e');
    assert(ir1.input() == 'x');
    assert(ir1.input() == 't');
    assert(ir1.input() == ' ');
    assert(ir1.input() == 's');
    assert(ir1.input() == 'p');
    assert(ir1.input() == 'a');
    assert(ir1.input() == 'n');
    assert(ir1.input() == 'n');
    assert(ir1.input() == 'i');
    assert(ir1.input() == 'n');
    assert(ir1.input() == 'g');
    assert(ir1.input() == ' ');
    assert(ir1.input() == 'o');
    assert(ir1.input() == 'n');
    assert(ir1.input() == 'e');
    assert(ir1.input() == ' ');
    assert(ir1.input() == 'l');
    assert(ir1.input() == 'i');
    assert(ir1.input() == 'n');
    assert(ir1.input() == 'e');
    assert(ir1.input() == ' ');
    assert(ir1.input() == 'o');
    assert(ir1.input() == 'n');
    assert(ir1.input() == 'l');
    assert(ir1.input() == 'y');
    assert(ir1.input() == '.');
    assert(ir1.input() == 0);

    std::string str2("line\nline.");
    std::istringstream in2(str2);
    InputReader ir2(counter);
    ir2.set_input(in2);
    assert(ir2.input() == 'l');
    assert(ir2.input() == 'i');
    assert(ir2.input() == 'n');
    assert(ir2.input() == 'e');
    assert(ir2.input() == '\n');
    assert(ir2.input() == 'l');
    assert(ir2.input() == 'i');
    assert(ir2.input() == 'n');
    assert(ir2.input() == 'e');
    assert(ir2.input() == '.');
    assert(ir2.input() == 0);
    assert(ir2.input() == 0);
}
#endif
