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

#ifndef _HFST_PROC_TOKENIZER_H_
#define _HFST_PROC_TOKENIZER_H_

#include "hfst-proc.h"
#include "buffer.h"

/**
 * The recognized types of tokens.
 *
 * None - 'dummy' empty token, can represent EOF
 * Symbol - a known transducer symbol
 * Character - a (UTF-8) character not in the transducer alphabet
 * Superblank - an escaped string that is handled as a single blank character
 * ReservedCharacter - an Apertium reserved character not otherwise handled
 */
enum TokenType {None, Symbol, Character, Superblank, ReservedCharacter};

/**
 * A structure representing a stream token. It can be of various types and
 * uses a union to conserve memory
 */
struct Token
{
  TokenType type;
  union
  {
    SymbolNumber symbol;
    char character[5];
    unsigned int superblank_index; // see TokenIOStream::superblank_bucket
  };
  bool escaped;

  Token(): type(None), symbol(0), escaped(false) {}

  void set_none() {type=None;}
  void set_symbol(SymbolNumber s) {type=Symbol; symbol=s;}
  void set_escaped_symbol(SymbolNumber s) {type=Symbol; symbol=s; escaped=true;}
  void set_character(const char* c)
  {type=Character; strncpy(character,c,4); character[4]='\0';}
  void set_character(char c) {type=Character; character[0]=c; character[1]='\0';}
  void set_superblank(unsigned int i) {type=Superblank; superblank_index=i;}
  void set_reservedcharacter(char c) {type=ReservedCharacter; character[0]=c; character[1]='\0';}

  static Token as_symbol(SymbolNumber s) {Token t; t.set_symbol(s); return t;}
  static Token as_escaped_symbol(SymbolNumber s) {Token t; t.set_escaped_symbol(s); return t;}
  static Token as_character(const char* c) {Token t; t.set_character(c); return t;}
  static Token as_character(char c) {Token t; t.set_character(c); return t;}
  static Token as_superblank(unsigned int i) {Token t; t.set_superblank(i); return t;}
  static Token as_reservedcharacter(char c) {Token t; t.set_reservedcharacter(c); return t;}

  bool operator<(const Token& rhs) const
  {
    if(type != rhs.type)
      return type < rhs.type;
    switch(type)
    {
      case Symbol:
        return symbol < rhs.symbol;
      case Character:
      case ReservedCharacter:
        return strcmp(character, rhs.character) < 0;
      case Superblank:
        return superblank_index < rhs.superblank_index;
      case None:
      default:
        return false;
    }
  }
};

class ProcTransducerAlphabet;
class Symbolizer;

/**
 * Wrapper class around an istream and an ostream for reading and writing
 * tokens, with additional buffering functionality. Input and output
 * are combined here for superblank functionality
 */
class TokenIOStream
{
  /**
   * The set of characters that need to be backslash-escaped in the stream
   */
  static std::set<char> escaped_chars;
  static void initialize_escaped_chars();

  std::istream& is;
  std::ostream& os;
  const ProcTransducerAlphabet& alphabet;
  bool null_flush;
  bool is_raw;

  const Symbolizer& symbolizer;

  /**
   * All superblanks found in the input stream all stored here, and are
   * indexed by Token objects
   */
  std::vector<std::string> superblank_bucket;

  Buffer<Token> token_buffer;

  void do_null_flush();

  /**
   * Reads a UTF-8 char (1-4 bytes) from the input stream, returning it as a
   * character string
   */
  std::string read_utf8_char();

  /**
   * Called after a backslash has been found in the stream to read an escaped
   * character. Fails on stream error or if the next character isn't a proper
   * escaped character
   */
  int read_escaped();

  /**
   * Generate an escaped copy of the given string
   */
  std::string escape(const std::string& str) const;

  /**
   * Make a token from the next character(s) in the stream by attempting to
   * get a symbol, and reverting to reading a character if that fails
   */
  Token make_token(bool was_escaped=false);

  /**
   * Read the next token in the stream, handling escaped characters
   */
  Token read_token();
 public:
  TokenIOStream(std::istream& i, std::ostream& o, const ProcTransducerAlphabet& a,
                bool flush, bool raw);

  size_t get_pos() const {return token_buffer.getPos();}
  size_t diff_prev(size_t pos) const {return token_buffer.diffPrevPos(pos);}
  Token at(size_t pos) const {return token_buffer.get(pos);}
  void move_back(size_t count) {token_buffer.back(count);}

  const ProcTransducerAlphabet& get_alphabet() const {return alphabet;}

  bool is_space(const Token& t) const;
  bool is_alphabetic(const Token& t) const;

  /**
   * Get a symbol representation of the token. The conversion depends on the
   * token type
   *
   * None - NO_SYMBOL_NUMBER
   * Symbol - the unmodified symbol
   * Character - blank_symbol if is_space returns true for the character,
   *             otherwise NO_SYMBOL_NUMBER
   * Superblank - blank_symbol
   * ReservedCharacter - NO_SYMBOL_NUMBER
   */
  SymbolNumber to_symbol(const Token& t) const;
  SymbolNumberVector to_symbols(const TokenVector& t) const;

  /**
   * Calculate the capitalization properties of the given analysis.
   */
  CapitalizationState get_capitalization_state(const SymbolNumberVector& symbols) const;

  /**
   * Calculate the capitalization properties of the given word, which should
   * contain all symbols
   */
  CapitalizationState get_capitalization_state(const TokenVector& tokens) const;

  size_t first_nonalphabetic(const TokenVector& s) const;

  /**
   * Read into the the stream until the delimiting character is found. The
   * delimiting character is read and included in the string. Charater escaping
   * is handled. Fails on stream error
   * If a wblank is being parsed, then the parsing happens till ]] is reached.
   * @return the string from the stream's current point up to and including
   *         the delimiting character
   */
  std::string read_delimited(const char delim);

  /**
   * Read the next token from the input stream/buffer
   */
  Token get_token();

  /**
   * Write a token to the output stream
   */
  void put_token(const Token& t);

  void put_tokens(const TokenVector& t);
  void put_symbols(const SymbolNumberVector& s, CapitalizationState caps=Unknown);

  /**
   * Get the string representation of the given token
   * @param raw if true, then don't do any character escaping
   */
  std::string token_to_string(const Token& t, bool raw=false) const;

  std::string tokens_to_string(const TokenVector& t, bool raw=false) const;

  /**
   * Read the next token from the input stream/buffer
   */
  TokenIOStream& operator>>(Token& t) {t=get_token(); return *this;}

  /**
   * Write the string representation of a token to the output stream
   */
  TokenIOStream& operator<<(const Token& t) {put_token(t); return *this;}

  std::ostream& ostream() {return os;}

  void write_escaped(const std::string str) {os << escape(str);}
  void write_escaped(const TokenVector& t) {os << tokens_to_string(t);}

  std::string get_superblank(size_t i) const {return superblank_bucket[i];}

  /**
   * Reads a UTF-8 char (1-4 bytes) an the input stream, returning it as a
   * character string
   */
  static std::string read_utf8_char(std::istream& is);
};

#endif
