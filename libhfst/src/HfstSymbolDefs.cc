// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

#include "HfstSymbolDefs.h"
#include "HfstFlagDiacritics.h"

#ifndef MAIN_TEST

namespace hfst {

bool is_epsilon(std::string const & str)
{
    return str == internal_epsilon;
}

bool is_unknown(std::string const & str)
{
    return str == internal_unknown;
}

bool is_identity(std::string const & str)
{
    return str == internal_identity;
}

bool is_default(std::string const & str)
{
    return str == internal_default;
}

bool is_epsilon(const char * str)
{
  return std::string(str) == internal_epsilon;
}

bool is_unknown(const char * str)
{
  return std::string(str) == internal_unknown;
}

bool is_identity(const char * str)
{
  return std::string(str) == internal_identity;
}

bool is_default(const char * str)
{
  return std::string(str) == internal_default;
}

  namespace symbols {

    void collect_unknown_sets(StringSet &s1, StringSet &unknown1,
                  StringSet &s2, StringSet &unknown2)
    {
      for (auto sym1 : s1)
        {
          if (s2.find(sym1) == s2.end())
            unknown2.insert(sym1);
        }
      for (auto sym2 : s2)
        {
          if (s1.find(sym2) == s1.end())
            unknown1.insert(sym2);
        }
    }

    std::string to_string(const StringVector & sv, bool spaces)
    {
      std::string result;
      for (StringVector::const_iterator it = sv.begin(); it != sv.end(); it++)
        {
          if (spaces && it != sv.begin())
            {
              result.append(" ");
            }
          result.append(*it);
        }
      return result;
    }

    std::string to_string(const StringPairVector & spv, bool spaces)
    {
      std::string result;
      for (StringPairVector::const_iterator it = spv.begin(); it != spv.end(); it++)
        {
          if (spaces && it != spv.begin())
            {
              result.append(" ");
            }
          result.append(it->first);
          if (it->first != it->second)
            {
              result.append(":");
              result.append(it->second);
            }
        }
      return result;
    }

    StringPairSet to_string_pair_set(const StringSet & ss)
    {
      StringPairSet result;
      for (const auto & s : ss)
        {
          result.insert(StringPair(s,s));
        }
      return result;
    }

    StringVector to_string_vector(const StringPairVector & spv, bool input_side)
    {
      StringVector result;
      for (const auto & it : spv)
        {
          if (input_side)
            result.push_back(it.first);
          else
            result.push_back(it.second);
        }
      return result;
    }

    StringVector to_string_vector(const HfstTwoLevelPath & path)
    {
      StringVector result;
      StringPairVector spv = path.second;
      for (const auto & it : spv)
        {
          result.push_back(it.first);
        }
      return result;
    }

    int longest_path_length(const HfstTwoLevelPaths & paths, bool equally_long)
    {
      if (paths.size() == 0)
        {
          return -1;
        }
      if (equally_long)
        {
          return (int)(paths.begin()->second.size());
        }

      unsigned int max_path_length = 0;

      for (const auto & path : paths)
        {
          unsigned int length = (unsigned int)path.second.size();
          max_path_length = (length > max_path_length)? length : max_path_length;
        }
      return (int)max_path_length;
    }

    HfstTwoLevelPaths get_longest_paths(const HfstTwoLevelPaths & paths)
    {
      HfstTwoLevelPaths result;
      unsigned int max_path_length = 0;

      for (const auto & path : paths)
        {
          unsigned int length = (unsigned int)path.second.size();
          max_path_length = (length > max_path_length)? length : max_path_length;
        }

      for (const auto & path : paths)
        {
          unsigned int length = (unsigned int)path.second.size();
          if (length == max_path_length)
            {
              result.insert(path);
            }
        }

      return result;
    }

    HfstTwoLevelPaths remove_flags(const HfstTwoLevelPaths & paths)
    {
      HfstTwoLevelPaths result;

      for (const auto & path : paths)
        {
          result.insert(HfstTwoLevelPath(path.first,
                                         remove_flags(path.second)));
        }
      return result;
    }

    HfstTwoLevelPath remove_flags(const HfstTwoLevelPath & path)
    {
      StringPairVector spv = path.second;
      spv = remove_flags(spv);
      HfstTwoLevelPath result(path.first, spv);
      return result;
    }


    StringVector remove_flags(const StringVector &v)
    {
      StringVector v_wo_flags;
      for (const auto & it : v)
        {
          if (! FdOperation::is_diacritic(it))
            { v_wo_flags.push_back(it); }
        }
      return v_wo_flags;
    }

    StringPairVector remove_flags(const StringPairVector &v)
    {
      StringPairVector v_wo_flags;
      for (const auto & it : v)
        {
          if (! FdOperation::is_diacritic(it.first) &&
              ! FdOperation::is_diacritic(it.second))
            { v_wo_flags.push_back(it); }
        }
      return v_wo_flags;
    }

  }


}

#else // MAIN_TEST was defined

#include <iostream>

int main(int argc, char * argv[])
{
    std::cout << "Unit tests for " __FILE__ ":" << std::endl;
    
    std::cout << "ok" << std::endl;
    return 0;
}

#endif // MAIN_TEST
