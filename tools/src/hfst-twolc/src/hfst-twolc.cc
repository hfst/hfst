//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, version 3 of the Licence.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "HfstTwolcDefs.h"
#include "commandline_src/CommandLine.h"
#include "grammar_defs.h"
#include "io_src/InputReader.h"
#include "rule_src/OtherSymbolTransducer.h"
#include "rule_src/TwolCGrammar.h"

namespace hfst
{
namespace twolcpre1
{
int parse();
void set_input(std::istream &istr);
void set_output(std::ostream &ostr);
void set_warning_stream(std::ostream &ostr);
void set_error_stream(std::ostream &ostr);
}
}

namespace hfst
{
namespace twolcpre2
{
int parse();
void set_input(std::istream &istr);
void complete_alphabet(void);
const HandyDeque<std::string> &get_total_alphabet_symbol_queue();
const HandyDeque<std::string> &get_non_alphabet_symbol_queue();
void set_warning_stream(std::ostream &ostr);
void set_error_stream(std::ostream &ostr);
}
}

namespace hfst
{
namespace twolcpre3
{
int parse();
void set_input(std::istream &istr);
void set_grammar(TwolCGrammar *grammar);
TwolCGrammar *get_grammar();
void set_silent(bool val);
void set_verbose(bool val);
void set_warning_stream(std::ostream &ostr);
void set_error_stream(std::ostream &ostr);
}
}

bool silent = false;
bool verbose = false;

int
main(int argc, char *argv[])
{
#ifdef WINDOWS
    _setmode(0, _O_BINARY);
    _setmode(1, _O_BINARY);
#endif

    CommandLine command_line(argc, argv);

    if (command_line.help || command_line.version)
    {
        if (command_line.version)
        {
            command_line.print_version();
        }
        if (command_line.help)
        {
            command_line.print_help();
        }
        exit(0);
    }
    if (command_line.usage)
    {
        command_line.print_usage();
        exit(0);
    }
    if (!command_line.be_quiet)
    {
        if (!command_line.has_input_file)
        {
            std::cerr << "Reading input from STDIN." << std::endl;
        }
        else
        {
            std::cerr << "Reading input from " << command_line.input_file_name
                      << "." << std::endl;
        }
        if (!command_line.has_output_file)
        {
            std::cerr << "Writing output to STDOUT." << std::endl;
        }
        else
        {
            std::cerr << "Writing output to " << command_line.output_file_name
                      << "." << std::endl;
        }
    }
    if (command_line.be_verbose)
    {
        std::cerr << "Verbose mode." << std::endl;
    }

    silent = command_line.be_quiet;
    verbose = command_line.be_verbose;

    hfst::twolcpre1::set_input(command_line.set_input_file());

    // Test that the output file is okay.
    (void)command_line.set_output_file();
    std::ostringstream oss1;
    hfst::twolcpre1::set_output(oss1);
    if (!silent)
    {
        hfst::twolcpre1::set_error_stream(std::cerr);
        hfst::twolcpre1::set_warning_stream(std::cerr);
    }
    try
    {
        if (hfst::twolcpre1::parse() != 0)
        {
            exit(1);
        }
    }
    catch (const HfstException &e)
    {
        std::cerr << e.what() << std::endl;
        exit(1);
    }

    std::istringstream iss1(oss1.str());
    hfst::twolcpre2::set_input(iss1);
    if (!silent)
    {
        hfst::twolcpre2::set_error_stream(std::cerr);
        hfst::twolcpre2::set_warning_stream(std::cerr);
    }
    try
    {
        if (hfst::twolcpre2::parse() != 0)
        {
            exit(1);
        }
    }
    catch (const HfstException &e)
    {
        std::cerr << e.what() << std::endl;
        exit(1);
    }

    hfst::twolcpre2::complete_alphabet();

    std::ostringstream oss2;
    oss2 << hfst::twolcpre2::get_total_alphabet_symbol_queue() << " ";
    oss2 << hfst::twolcpre2::get_non_alphabet_symbol_queue();

#ifdef DEBUG_TWOLC_3_GRAMMAR
    hfst::twolcpre3debug = 1;
#endif

    try
    {
        std::istringstream iss2(oss2.str());
        hfst::twolcpre3::set_input(iss2);
        if (!silent)
        {
            hfst::twolcpre3::set_error_stream(std::cerr);
            hfst::twolcpre3::set_warning_stream(std::cerr);
        }

        OtherSymbolTransducer::set_transducer_type(command_line.format);
        hfst::twolcpre3::set_silent(silent);
        hfst::twolcpre3::set_verbose(verbose);

        TwolCGrammar twolc_grammar(command_line.be_quiet,
                                   command_line.be_verbose,
                                   command_line.resolve_left_conflicts,
                                   command_line.resolve_right_conflicts);
        hfst::twolcpre3::set_grammar(&twolc_grammar);
        int exit_code = hfst::twolcpre3::parse();
        if (exit_code != 0)
        {
            exit(exit_code);
        }

        if (!command_line.has_output_file)
        {
            HfstOutputStream stdout_(command_line.format);
            hfst::twolcpre3::get_grammar()->compile_and_store(stdout_);
        }
        else
        {
            HfstOutputStream out(command_line.output_file_name,
                                 command_line.format);
            hfst::twolcpre3::get_grammar()->compile_and_store(out);
            out.flush();
            out.close();
        }
        exit(0);
    }
    catch (const HfstException e)
    {
        std::cerr << "This is an hfst interface bug:" << std::endl
                  << e() << std::endl;
        exit(1);
    }
    catch (const char *s)
    {
        std::cerr << "This is an a bug probably from sfst:" << std::endl
                  << s << std::endl;
        exit(1);
    }
}
