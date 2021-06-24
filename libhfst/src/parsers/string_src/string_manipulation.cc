// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

//! @file string_manipulation.cc
//!
//! @author Miikka Silfverberg
#include "string_manipulation.h"

FaultyStringInput::FaultyStringInput(const std::string &function,
                     const std::string &input):
  function(function),
  input(input)
{}

std::string new_string(size_t lgth)
{ return std::string(lgth,0); }

char * string_copy(const char * str)
{
  char * new_str = (char *)malloc(sizeof(char)*strlen(str)+1);
  return strcpy(new_str,str);
}

std::string replace_substr(std::string str,const std::string &substr,
               const std::string &replacement)
{
  size_t index;
  while ((index = str.find(substr)) != std::string::npos)
    { str.replace(index,substr.size(),replacement); }
  return str;
}

std::string remove_sign(std::string str,char sign)
{ return replace_substr(str,std::string(1,sign),""); }

std::string remove_sign(std::string str,const std::string &sign)
{ return replace_substr(str,sign,""); }

std::string unescape(std::string str)
{
  if (str.find('\n') != std::string::npos)
    { throw FaultyStringInput("unescape",str ); }
  // Change "%%" to "\n", remove all remaining %'s
  // and change all '\n's to '%'.
  return
    replace_substr(remove_sign(replace_substr(str,"%%","\n"),'%'),"\n","%");
}

int strcmp_unescaped(const std::string &str1,
            const std::string &str2)
{
  // Remove all escapes from str1 and str2 and
  // compare them.
  std::string str1_copy = unescape(str1);
  std::string str2_copy = unescape(str2);
  return str1_copy.compare(str2_copy);
}

std::string remove_white_space(std::string str)
{
  if (str.find('\n') != std::string::npos)
    { throw FaultyStringInput("remove_white_space",str); }
  str =
    replace_substr(remove_sign(replace_substr(str,"% ","\n"),' '),
           "\n","__HFST_TWOLC_SPACE");
  str =
    replace_substr(remove_sign(replace_substr(str,"%\t","\n"),'\t'),
           "\n","__HFST_TWOLC_TAB");
  str =
    replace_substr(remove_sign(replace_substr(str,"%\r","\n"),'\r'),
           "\n","__HFST_TWOLC_CR");
  str =
    replace_substr(remove_sign(replace_substr(str,"%__HFST_TWOLC_\\n","\n"),
                   "__HFST_TWOLC_\\n"),"\n","__HFST_TWOLC_\\n");
  return str;
}

std::string unescape_and_remove_white_space(std::string str)
{ return unescape(remove_white_space(str)); }

std::string unquote(const std::string &str)
{
  if (str.size() < 2 || str[0] != '"' || str[str.size()-1] != '"')
    { throw FaultyStringInput("unquote",str); }
  // Return the substring of str spanning from the
  // second to the next to final character.
  return str.substr(1,str.size()-2);
}

int str2int(const std::string &str)
{
  std::istringstream in(str);
  int number;
  in >> number;
  if (in.fail())
    { throw FaultyStringInput("str2int",str); }
  return number;
}

void print_kill_symbol(void)
{ std::cout << std::endl << "__HFST_TWOLC_DIE" << std::endl; }

StringVector::StringVector(void)
{}

StringVector::StringVector(const std::string &s)
{
  size_t start_pos = 0;
  size_t space_pos;
  while ((space_pos = s.find(' ',start_pos)) != std::string::npos)
    {
      std::vector<std::string>::push_back(s.substr(start_pos,
                           space_pos - start_pos));
      start_pos = space_pos + 1;
    }
  std::vector<std::string>::push_back(s.substr(start_pos));
}

StringVector &StringVector::add_values(const StringVector &another)
{
  std::vector<std::string>::insert(std::vector<std::string>::end(),
                   another.begin(),another.end());
  return *this;
}

std::string unescape_name(const std::string &name)
{
  std::string new_name =
    replace_substr(replace_substr(name,"__HFST_TWOLC_RULE_NAME=",""),
           "__HFST_TWOLC_SPACE"," ");
  return new_name;
}

#ifdef STRING_MANIPULATION_TEST
#include <iostream>
int main(void)
{
  // new_string(...)
  std::string s1 = new_string(0);
  assert(s1.empty());
  s1 = new_string(1);
  assert(s1.size() == 1);
  assert(s1[0] == 0);
  assert(s1[1] == 0);


  // remove_sign(..)
  std::string s2 = remove_sign("",'a');
  assert(s2.empty());
  s2 = remove_sign("a",'a');
  assert(s2.empty());
  s2 = remove_sign("a",'b');
  assert(s2 == "a");
  s2 = remove_sign("fooa",'a');
  assert(s2 == "foo");
  s2 = remove_sign("afoo",'a');
  assert(s2 == "foo");
  s2 = remove_sign("fooafoo",'a');
  assert(s2 == "foofoo");
  s2 = remove_sign("fooabar",'a');
  assert(s2 == "foobr");

  // unescape(...)
  std::string s3 = unquote("\"\"");
  assert(s3.empty());
  s3 = unquote("\"a\"");
  assert(s3 == "a");
  s3 = unquote("\"ab\"");
  assert(s3 == "ab");
  try
    {
      s3 = unquote("");
      assert(false);
    }
  catch (const FaultyStringInput &fsi)
    { /* nothing */ }
  try
    {
      s3 = unquote("\"");
      assert(false);
    }
  catch (const FaultyStringInput &fsi)
    { /* nothing */ }
  try
    {
      s3 = unquote("a");
      assert(false);
    }
  catch (const FaultyStringInput &fsi)
    { /* nothing */ }

  // unescape(...)
  std::string s4 = unescape("");
  assert(s4.empty());
  s4 = unescape("foo");
  assert(s4 == "foo");
  s4 = unescape("%foo");
  assert(s4 == "foo");
  s4 = unescape("foo%");
  assert(s4 == "foo");
  s4 = unescape("%fo%o");
  assert(s4 == "foo");
  s4 = unescape("%%foo");
  assert(s4 == "%foo");
  s4 = unescape("%%foo%%foo%%");
  assert(s4 == "%foo%foo%");
  try
    {
      s4 = unescape("foo\nbar");
      assert(false);
    }
  catch (const FaultyStringInput &fsi)
    { /* nothing */ }

  // strcmp_unescaped
  assert(strcmp_unescaped("","") == 0);
  assert(strcmp_unescaped("a","b") < 0);
  assert(strcmp_unescaped("b","a") > 0);
  assert(strcmp_unescaped("%a","a") == 0);
  assert(strcmp_unescaped("a","%a") == 0);
  assert(strcmp_unescaped("foo%foo","foofoo") == 0);
  assert(strcmp_unescaped("foo%foo","foo%foo") == 0);
  assert(strcmp_unescaped("%","") == 0);
  try
    {
      (void)strcmp_unescaped("foo\nbar","foobar");
      assert(false);
    }
  catch (const FaultyStringInput &fsi)
    { /* nothing */ }

  // str2int(...)
  assert(str2int("0") == 0);
  assert(str2int("-1") == -1);
  assert(str2int("-1 w") == -1);
  assert(str2int("-1 3") == -1);
  assert(str2int("20") == 20);
  try
    {
      str2int("");
      assert(false);
    }
  catch (const FaultyStringInput &fsi)
    { /* nothing */ }

  try
    {
      str2int("a");
      assert(false);
    }
  catch (const FaultyStringInput &fsi)
    { /* nothing */ }

  // remove_white_space(...)
  assert(remove_white_space("").empty());
  assert(remove_white_space("foo") == "foo");
  assert(remove_white_space("foo ") == "foo");
  assert(remove_white_space(" foo") == "foo");
  assert(remove_white_space("foo bar") == "foobar");
  assert(remove_white_space("foo bar\tbaz\rfoo") == "foobarbazfoo");
  //assert(remove_white_space("foo% ") == "foo ");
  //assert(remove_white_space("foo% bar%\tbaz%\r") == "foo bar\tbaz\r");
  //assert(remove_white_space("foo% \tbar") == "foo bar");
  try
    {
      (void)remove_white_space("foo\nbar");
      assert(false);
    }
  catch (const FaultyStringInput &fsi)
    { /* nothing */ }

  // unescape_and_remove_white_space(...)
  //assert(unescape_and_remove_white_space(" foo%%% ") == "foo% ");
  try
    {
      (void)unescape_and_remove_white_space("foo\nbar");
      assert(false);
    }
  catch (const FaultyStringInput &fsi)
    { /* nothing */ }
}
#endif // STRING_MANIPULATION_TEST
