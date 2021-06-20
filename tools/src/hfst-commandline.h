/**
 * @file hfst-commandline.h
 *
 * @brief common parts for all hfst based command line tools
 * This file contains interface specifications and some macros to implement for
 * HFST based tools. There is no common implementation for prototyped functions,
 * rather they must be defined on per program basis. For reference
 * implementation see file @c hfst-*-skeleton.cc.
 */
//       This program is free software: you can redistribute it and/or modify
//       it under the terms of the GNU General Public License as published by
//       the Free Software Foundation, version 3 of the License.
//
//       This program is distributed in the hope that it will be useful,
//       but WITHOUT ANY WARRANTY; without even the implied warranty of
//       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//       GNU General Public License for more details.
//
//       You should have received a copy of the GNU General Public License
//       along with this program.  If not, see <http://www.gnu.org/licenses/>.


#ifndef GUARD_hfst_commandline_h
#define GUARD_hfst_commandline_h

#if HAVE_CONFIG_H
#  include <config.h>
#endif
// most all HFST commandline programs will use both c++ and c io unfortunately
#include <iostream>
#include <cstdio>
#include <cstring>

#if HAVE_ERROR_H
#  include <error.h>
#endif

#ifdef _MSC_VER
#  include <BaseTsd.h>
   typedef SSIZE_T ssize_t;
#endif


#include "HfstDataTypes.h"
namespace hfst { class HfstInputStream; } ;

/* These variables should be used in all command line programs.
 * In some cases they may be nonsensical; just define something then.
 */
/**
 * @brief set @c verbose when program should print before and after every
 *        non-trivial step it takes.
 */
extern bool verbose;
/** @brief set @c silent when program should not print anything at all. */
extern bool silent;
/** @brief set @c debug when program should dump all the intermediate results
 *         to terminal and/or save them to files in @c CWD.
 */
extern bool debug;
/** @brief set @c message_out to stream that is usable for non-error message
 *         print-outs.
 *         This @e should be stdout in all cases, except when transducer
 *         binaries are being transmitted through @c stdout. Some programs
 *         @e may have option to log these messages to a file instead.
 */
extern FILE* message_out;
/**
 *  @brief set @c hfst_tool_version to version specific to the tool.
 *  @sa hfst_set_program_name
 */
extern const char* hfst_tool_version;
/**
 * @brief set @c hfst_tool_wikiname to name of the wiki page for this tool.
 */
extern const char* hfst_tool_wikiname;

/* hfst tools generic helper print functions */

/** save current transducer @c t to file @c filename if debug is @a true. */
void debug_save_transducer(hfst::HfstTransducer t, const char* name);

/** print message @c s with parameters @c __VA_ARGS__ if debug is @a true. */
void debug_printf(const char* format, ...);

/** print message @c s with parameters @c __VA_ARGS__ if debug is @a true. */
void verbose_printf(const char* format, ...);

/**
 * @brief set program's name and other infos for reusable messages defined
 * below. This function must be called in beginning of main as the values are
 * used in all error messages as well.
 */
void hfst_set_program_name(const char* argv0, const char* version,
                           const char* wikipage);


bool is_input_stream_in_ol_format(const hfst::HfstInputStream & is, const char * program);

/* Common format into which transducers of types \a type1 and \a type2 will be converted,
   when convert_transducers will be called. Possible return values are:

   0   no conversion is needed
   1   convert into type1
   2   convert into type2
   -1  convert into type1, loss of information is possible */
int conversion_type(hfst::ImplementationType type1, hfst::ImplementationType type2);

/* Convert transducers into common format, if needed. */
void convert_transducers(hfst::HfstTransducer & first, hfst::HfstTransducer & second);

/**
 * @brief set @c program_name to program's executable name for error messages.
 */
extern const char* program_name;

#ifndef HAVE_ERROR
/** @brief print standard formatted error message and exit if needed */
void error(int status, int error, const char* format, ...);
#endif
#ifndef HAVE_WARNING
/** @brief print standard formatted warning message and exit if needed */
void warning(int status, int error, const char* format , ...);
#endif

#ifndef HAVE_ERROR_AT_LINE
/**
 * @brief print standard formatted error message when parsing a file and exit
 *        if needed.
 *        The use of this function is especially important since error
 *        highlighting of vim and emacs depends on this error format.
 */
void error_at_line(int status, int errnum, const char* filename, unsigned int linenum, const char* fmt, ...);
#endif

/**
 *
 * @brief print standard usage message.
 *
 * @sa http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dhelp
 * @sa http://www.gnu.org/software/womb/gnits/Help-Output.html
 */
void print_usage();

/**
 * @brief print standard version message.
 *
 * @sa http://www.gnu.org/prep/standards/standards.html#g_t_002d_002dversion
 * @sa http://www.gnu.org/software/womb/gnits/Version-Output.html
 */
void print_version();

/**
 * @brief print standard short help message.
 *
 * @sa http://www.gnu.org/software/womb/gnits/Help-Output.html#Help-Output
 */
void print_short_help();

constexpr auto WIKI_URL = "https://github.com/hfst/hfst/wiki";

/**
 * @brief print link to wiki pages.
 */
void print_more_info();

/**
 * @brief print bug reporting message.
 */
void print_report_bugs();

/* command line argument handling */

/** successful return value for argument parsing routine */
constexpr auto EXIT_CONTINUE = 42;

/**
 * @brief define function for parsing whole command line.
 * Each program should define this on its own, you may use includable templates
 * for standard options though.
 *
 * @sa http://www.gnu.org/prep/standards/standards.html#Command_002dLine-Interfaces
 * @sa http://www.gnu.org/software/womb/gnits/File-Arguments.html
 */
int parse_options(int argc, char** argv);

/**
 * @brief extend the options in argv by parsing standard hfst environment
 *        variables.
 */
void extend_options_getenv(int* argc, char*** argv);

/**
 * @brief parse weight from string, or print error message and return zero
 * weight on failure.
 */
double hfst_strtoweight(const char *s);
/**
 * @brief parse number from string, or print error message on failure.
 * if @a infinite is not @c NULL, and value of string is infinity, it will
 * be set to true and sign of infinity is returned.
 */
int hfst_strtonumber(const char *s, bool *infinite);

/**
 * @brief parse @c unsigned long from string or print error message on failure.
 */
unsigned long hfst_strtoul(char *s, int base);

/**
 * @brief parse @c long from string @a s or print error message on failure.
 */
long hfst_strtol(char* s, int base);

/** @brief parse string naming transducer format @c s or exit.
 */
hfst::ImplementationType hfst_parse_format_name(const char* s);

/** @brief return string describing type of transducer format */
const char* hfst_strformat(hfst::ImplementationType format);

#ifndef HAVE_STRNDUP
char* strndup(const char* s, size_t n);
#endif

/** @brief duplicate substring and exit cleanly on error */
char* hfst_strndup(const char* s, size_t n);

/** @brief duplicate string and exit cleanly on error */
char* hfst_strdup(const char* s);

/** @brief allocate memory and exit cleanly on error */
void* hfst_malloc(size_t size);

/** @brief allocate memory to zero and exit cleanly on error */
void* hfst_calloc(size_t nmemb, size_t size);

/** @brief reallocate memory and exit cleanly on error */
void* hfst_realloc(void* ptr, size_t size);

/**
 * @brief open file, or print informative error message and exit on failure.
 */
FILE* hfst_fopen(const char* path, const char *mode);

/**
 * @brief seek file, or print informative error message and exit on failure.
 */
void hfst_fseek(FILE* stream, long offset, int whence);

/**
 * @brief tell file position, or print informative error message and exit on
 * failure.
 */
unsigned long hfst_ftell(FILE* stream);

/**
 * @brief read at most @a nmemb elements of @a size or print informative
 *        error message and exit.
 */
size_t hfst_fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
/**
 * @brief write at most @a nmemb elements of @a size or print informative
 *        error message end exit.
 */
size_t hfst_fwrite(void* ptr, size_t size, size_t nmemb, FILE* stream);
/**
 * @brief create temporary file or print informative error message and exit
 *        on failure.
 */
FILE* hfst_tmpfile();
// same stuff for fd's
/** @brief close a file descriptor or print informative error message and exit
 *         on failure.
 */
int hfst_close(int fd);
/** @brief open a file descriptor or print informative error message and exit
 *         on failure.
 */
int hfst_open(const char* pathname, int flags);
/** @brief read at most @a count bytes from the file descriptor or print
 *         error message and exit on failure.
 */
ssize_t hfst_read(int fd, void* bug, size_t count);
/** @brief write at most @a count bytes to file descriptor or print error
 *         message and exit on failure
 */
ssize_t hfst_write(int fd, const void* buf, size_t count);
/**
 * @brief create temporary file from template name or print informative error
 *        message and exit on failrue.
 */
int hfst_mkstemp(char* templ);

/**
 * @brief remove a file or print an informative error message and exit on
 *        failure.
 */
int hfst_remove(const char* filename);

#ifndef HAVE_GETDELIM
ssize_t getdelim(char** lineptr, size_t* n, int delim, FILE* stream);
#endif
#ifndef HAVE_GETLINE
ssize_t getline(char** lineptr, size_t* n, FILE* stream);
#endif

/**
 * @brief safely read one @a delim delimited char array or print informative
 * error message and exit on failure.
 */
ssize_t hfst_getdelim(char** lineptr, size_t* n, int delim, FILE* stream);

/**
 * @brief safely read one full line from file or print informative error
 * messae and exit on failure.
 */
ssize_t hfst_getline(char** lineptr, size_t* n, FILE* stream);

#ifndef HAVE_READLINE
char* readline(const char* prompt);
#endif

/**
 * @brief read one line of interactive input or print informative error
 *        message adn exit on failure.
 */
char* hfst_readline(const char* prompt);

/**
 * @brief set locale according to environment if UTF-8-capable or
 * print informative erro message and exit on failure.
 */
char* hfst_setlocale();

/**
 * @brief determine if next transducers of given streams are compatible.
 *
 * @deprecated all formats are compatible in HFST3
 */
int get_compatible_fst_format(std::istream& is1, std::istream& is2);



#endif
// vim: set ft=cpp.doxygen:
