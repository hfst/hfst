// Copyright (c) 2016 University of Helsinki
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// See the file COPYING included with this distribution for more
// information.

//! @file lexc-utils.h
//!
//! @brief Various string handling methods for HFST lexc.

#ifndef GUARD_lexc_utils_h
#define GUARD_lexc_utils_h
#if HAVE_CONFIG_H
#  include <config.h>
#endif


#include <vector>
#include <string>

using namespace std;


//for med alignment
#define SUBSTITUTE 2 //diag
#define DELETE 1 //left
#define INSERT 0  //down
#define EPSILON_ "@@ANOTHER_EPSILON@@"


namespace hfst { namespace lexc {

const char LEXC_JOINER_START[] = "$_LEXC_JOINER.";
const char LEXC_JOINER_END[] = "_$";
const char LEXC_FLAG_LEFT_START[] = "$R.LEXNAME.";
const char LEXC_FLAG_RIGHT_START[] = "$P.LEXNAME.";
const char LEXC_FLAG_END[] = "$";
const char LEXC_DFN_START[] = "@_LEXC_DEFINITION.";
const char LEXC_DFN_END[] = "_@";
const char REG_EX_START[] = "$_REG.";
const char REG_EX_END[] = "_$";

// RECODE LEXC STYLE

//! @brief Strips lexc style percent escaping from a string.
//!
//! E.g. like stripslashes() in PHP.
std::string& stripPercents(std::string& s);

//! @brief Percent encode critical characters in raw string for lexc.
std::string& addPercents(std::string& s);

//! @brief Find flag representation of given joiner name string.
std::string& flagJoinerEncode(std::string& s, bool left);

//! @brief Find inner representation of given joiner name string.
std::string& joinerEncode(std::string& s);

//! @brief Format inner representation of joiner string in readable format as
//! it was in lexc source.
std::string& joinerDecode(std::string& s);

//! @brief Find inner representation for regex map key of given joiner name string.
std::string& regExpresionEncode(std::string& s);

//! @brief Format inner representation of joiner string in readable format as
//! it was in lexc source.
std::string& regExpresionDecode(std::string& s);

//! @brief Replaces @ZERO@ with "0" in a string
std::string replace_zero(const std::string s);

// FLEX HANDLING

//! @brief Set filename used for position messages.
void set_infile_name(const char* s);

//! @brief Initialise memory of file positions to zeros.
//!
//! Sets all members of current yylloc structure to zeros.
void token_reset_positions();

//! @brief Keep memory of positions of last parsed tokens for error messages.
//!
//! Counts length, height and width of the given token. Update yylloc structure
//! provided by lex and yacc, for location data.
void token_update_positions(const char* token);

//! @brief writes token positions in standard format.
char* strdup_token_positions();

//! @brief create some sensible representation of current token.
char* strdup_token_part();
//! @brief Strips percent escaping and strdups
char* strip_percents(const char* s, bool do_zeros);

//! @brief Strips initial and final white space and strdups
char* strstrip(const char* s);

//! @brief extracts the variable substring part from token.
//! Omits constant string prefix, suffix and optionally strips spaces.
char* strdup_nonconst_part(const char* token,
                           const char* prefix,
                           const char* suffix,
                           bool strip);

// help flex/yacc with meaningful error messages
//! @brief print error_at_line style error message for current token
void error_at_current_token(int status, int errnum, const char* format);
//! @brief print error_at_line style warning message for current token
void warning_at_current_token(int status, int errnum, const char* format);

//! @brief Finds med alignment between two strings
//! Given an upper-lower string lexicon entry, the upper-lower pair is aligned by minimum edit distance with the following costs:
 //! x:x costs 0
 //! x:y costs ∞
 //! x:0 costs 1
 //! 0:x costs 1
//! This means that if we have a lexicon entry like:
//! abc:bc
//! As this is compiled into a transducer for the entry, we align it a:0 b:b c:c (instead of the default a:b b:c c:0).
//! Additionally, if we have a lexicon entry like:
//! abc:xyz
//! we align it 0:x 0:y 0:z a:0 b:0 c:0 (instead of x:0 y:0 z:0 0:a 0:b 0:c)
std::pair<vector<string>, vector<string> > find_med_alingment(const vector<string> &s1, const vector<string> &s2);

} }
// vim: set ft=cpp.doxygen:
#endif // GUARD_lexc_utils_h
