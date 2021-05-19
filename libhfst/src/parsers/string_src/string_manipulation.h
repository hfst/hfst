//! @file string_manipulation.h
//!
//! @author Miikka Silfverberg
//!
//! @brief Functions used to manipulate strings.

//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation, version 3 of the Licence.
//
//   This library is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef STRING_MANIPULATION_H
#define STRING_MANIPULATION_H


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <iostream>
#include <vector>

//! @brief Thrown when a string manipulation function receives incorrect
//! string input.
struct FaultyStringInput
{
  //! @var Name of the function which threw this instance.
  std::string function;

  //! @var The incorrect input @function received.
  std::string input;
  
  //! @brief Intitialize an instance.
  //! @param function the name of a string-manipulation function.
  //! @param int the input @a function received.
  FaultyStringInput(const std::string &function,
            const std::string &input);
};

//! @brief Return a string of size @a lgth, where each byte is 0.
std::string new_string(size_t lgth);

//! @brief Safe copy of a @a char *. Free using @a free.
char * string_copy(const char * str);
 
//! @brief Recursively substitute each occurrence of @a substr with
//! @a replacement.
//! @param str perform substitution on this string.
//! @param substr the substring to be substituted.
//! @param replacement the replacement of @a substr.
//! @return Return the result of recuresive substitution preformed on @a str.
//!
//! Substitute the first occurrence of @a substr in @a str, then do the same
//! for the result. Continue until no substitutions can be made (i.e. until
//! the result no longer contains @a substr as a substring).
std::string replace_substr(std::string str,const std::string &substr,
               const std::string &replacement);

//! @brief Return @a str with all occurrences of @a sign removed.
std::string remove_sign(std::string str,char sign);

//! @brief Return @a str with all occurrences of @a sign removed.
std::string remove_sign(std::string str,const std::string &sign);

//! @brief For a string enclosed in quotation marks, return the unquoted
//! string.
//! @pre @a str is of the form  <tt>\".*\"</tt>
//! @note Only the first layer of quotation is removed.
std::string unquote(const std::string &str);

//! @brief Perform equivalent of perl <tt>s/%(%?)/$1/g</tt>.
//! @pre @a str doesn't contain '\\n'
std::string unescape(const std::string str);

//! @brief Compare @a str1 and @a str2 after unescape() has been applied on
//! them.
//! @pre neighted @a str1 nor @a str2 contains '\\n'.
//! @return Return true if @a str1, thus compared, is alphabetically less than
//! @a str2.
int strcmp_unescaped(const std::string &str1,
             const std::string &str2);

//! @brief Transform a string of form <tt>(-)[0-9]+</tt> into an integer.
//! @pre @a str begins <tt>(-)?[0-9]+</tt>.
int str2int(const std::string &str);

//! @brief Return @a str with unescaped whitespace removed.
//! @pre @a str doesn't contain <tt>'\\n'</tt>.
std::string remove_white_space(std::string str);

//! @brief Remove unescaped white space from @a str and unescape all escaped
//! symbols.
//! @pre @a str doesn't contain <tt>'\\n'</tt>.
std::string unescape_and_remove_white_space(std::string str);

//! @brief Print a symbol which guarantees that the next compiler will die with
//! exit signal 1.
//!
//! The symbol <tt>__HFST_TWOLC_DIE</tt> terminates all hfst-twolc compilers
//! which are used in the compilation of a grammar. It is printed into
//! @a std::cout.
void print_kill_symbol(void);

//! @brief Regular string comparison.
struct str_cmp {
  bool operator() (const std::string &str1,
           const std::string &str2) const
  { return str1 < str2; }
};

//! @brief String comparison of unescaped strings.
struct relaxed_str_cmp {
  bool operator() (const std::string &str1,
           const std::string &str2) const
  { return strcmp_unescaped(str1,str2) < 0; }
};

//! @brief Container for strings.
class StringVector : public std::vector<std::string>
{
 public:
  //! @brief Initialize empty.
  StringVector(void);
  //! @brief Split @a s to tokens at spaces and store the tokens in @a this.
  StringVector(const std::string &s);

  //! @brief Add the values in @a another at the end of @a this.
  StringVector &add_values(const StringVector &another);
};

//! @brief Remove hfst-twolc escape sequences from @a name.
std::string unescape_name(const std::string &name);

#endif
