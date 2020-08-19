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

#include <cstdlib>
#include "tokenizer.h"
#include "transducer.h"

//////////Function definitions for TokenIOStream

std::set<char> TokenIOStream::escaped_chars;

TokenIOStream::TokenIOStream(std::istream& i, std::ostream& o,
                             const ProcTransducerAlphabet& a, bool flush,
                             bool raw):
  is(i), os(o), alphabet(a), null_flush(flush), is_raw(raw),
  symbolizer(a.get_symbolizer()), superblank_bucket(), token_buffer(1024)
{
  if(printDebuggingInformationFlag) {
    std::cout << "Creating TokenIOStream" << std::endl;
  }
  if(escaped_chars.size() == 0 && !is_raw) {
    initialize_escaped_chars();
  }
}

void
TokenIOStream::initialize_escaped_chars()
{
  escaped_chars.insert('[');
  escaped_chars.insert(']');
  escaped_chars.insert('{');
  escaped_chars.insert('}');
  escaped_chars.insert('^');
  escaped_chars.insert('$');
  escaped_chars.insert('/');
  escaped_chars.insert('\\');
  escaped_chars.insert('@');
  escaped_chars.insert('<');
  escaped_chars.insert('>');
}

void
TokenIOStream::do_null_flush()
{
  os << '\0';
  os.flush();
  if(os.bad())
    std::cerr << "Could not flush file" << std::endl;
}

CapitalizationState
TokenIOStream::get_capitalization_state(const SymbolNumberVector& symbols) const
{
  if(symbols.size() == 0)
    return Unknown;

  const SymbolNumber& first=symbols[0], last=symbols.size()>1?symbols[1]:symbols[0];

  if(alphabet.is_lower(first) && alphabet.is_lower(last))
    return LowerCase;
  if(alphabet.is_upper(first) && alphabet.is_lower(last))
    return FirstUpperCase;
  if(alphabet.is_upper(first) && alphabet.is_upper(last))
    return UpperCase;
  return Unknown;
}

CapitalizationState
TokenIOStream::get_capitalization_state(const TokenVector& tokens) const
{
  if(tokens.size() == 0)
    return Unknown;

  const Token& first=tokens[0], last=tokens.size()>1?tokens[1]:tokens[0];

  if(first.type != Symbol || last.type != Symbol)
    return Unknown;
  if(alphabet.is_lower(first.symbol) && alphabet.is_lower(last.symbol))
    return LowerCase;
  if(alphabet.is_upper(first.symbol) && alphabet.is_lower(last.symbol))
    return FirstUpperCase;
  if(alphabet.is_upper(first.symbol) && alphabet.is_upper(last.symbol))
    return UpperCase;
  return Unknown;
}

std::string
TokenIOStream::read_utf8_char()
{
  return read_utf8_char(is);
}

std::string
TokenIOStream::read_utf8_char(std::istream& is)
{
  std::string retval;
  unsigned short u8len = 0;
  int c = is.peek();
  if(is.eof())
    return retval;

  if (c <= 127)
    u8len = 1;
  else if ( (c & (128 + 64 + 32 + 16)) == (128 + 64 + 32 + 16) )
    u8len = 4;
  else if ( (c & (128 + 64 + 32 )) == (128 + 64 + 32) )
    u8len = 3;
  else if ( (c & (128 + 64 )) == (128 + 64))
    u8len = 2;
  else
    stream_error("Invalid UTF-8 character found");

  retval.resize(u8len+1);
  is.get(&retval[0], u8len+1, '\0');
  retval.resize(strlen(&retval[0]));

  return retval;
}

bool
TokenIOStream::is_space(const Token& t) const
{
  switch(t.type)
  {
    case Symbol:
      return isspace(alphabet.symbol_to_string(t.symbol).at(0));
    case Character:
      return isspace(t.character[0]);
    case Superblank:
      return true;
    default:
      return false;
  }
}

bool
TokenIOStream::is_alphabetic(const Token& t) const
{
  SymbolNumber s = to_symbol(t);
  if(s != 0 && s != NO_SYMBOL_NUMBER)
    return alphabet.is_alphabetic(s);

  switch(t.type)
  {
    case Character:
      return alphabet.is_alphabetic(t.character);
    default:
      return false;
  }
}

size_t
TokenIOStream::first_nonalphabetic(const TokenVector& s) const
{
  for(size_t i=0; i<s.size();i++)
  {
    if(!is_alphabetic(s[i]))
      return i;
  }

  return string::npos;
}

int
TokenIOStream::read_escaped()
{
  int c = is.get();

  if(c == EOF || escaped_chars.find(c) == escaped_chars.end())
    stream_error("Found non-reserved character after backslash");

  return c;
}

std::string
TokenIOStream::read_delimited(const char delim)
{
  std::string result;
  int c = EOF;
  bool is_wblank = false;
  
  if(is && c != delim)
  {
    c = is.get();
    if(c != EOF)
    {
      result += c;
      if(c == '\\')
        result += read_escaped();
      else if(null_flush && c == '\0')
        do_null_flush();
      else if(c == '[')
      {
        int next_char = is.peek();
        if(next_char == '[') //Check if wblank is being read
          is_wblank = true;
      }
    }
  }
  
  while(is && c != delim)
  {
    c = is.get();
    if(c == EOF)
      break;

    result += c;
    if(c == '\\')
      result += read_escaped();
    if(null_flush && c == '\0')
      do_null_flush();
  }
  
  if(is_wblank)
  {
    c = is.get();
    if(c != EOF)
    {
      if(c != delim)
      {
        stream_error(std::string("Error in parsing a wordbound blank"));
      }
      else
      {
        result += c;
      }
    }
  }

  if(c != delim)
    stream_error(std::string("Didn't find delimiting character ")+delim);

  return result;
}

Token
TokenIOStream::make_token(bool was_escaped)
{
    int c = is.peek();
  SymbolNumber s = symbolizer.extract_symbol(is);
  if(s == 0)
  {
    // literal NUL without null-flushing
    return Token();
  }

  if(s != NO_SYMBOL_NUMBER) {
    if(was_escaped) {
      return Token::as_escaped_symbol(s);
    }
    else {
      return Token::as_symbol(s);

    }
  }

  // the next thing in the stream is not a symbol
  // (extract_symbol moved the stream back to before anything was read)
  std::string ch = read_utf8_char();
  if(null_flush && ch == "") {
    do_null_flush();
  }
  if(was_escaped) {
    return Token::as_character(ch.c_str());
  }

  return (escaped_chars.find(ch[0]) == escaped_chars.end()) ?
    Token::as_character(ch.c_str()) :
    Token::as_reservedcharacter(ch[0]);
}

Token
TokenIOStream::read_token()
{
  int next_char = is.peek();
  if(is.eof())
    return Token();

  if(next_char == 0 && null_flush) {
    do_null_flush();
    return Token::as_character(is.get());
  }

  if(escaped_chars.find(next_char) != escaped_chars.end())
  {
    switch(next_char)
    {
      case '[':
        superblank_bucket.push_back(read_delimited(']'));
        return Token::as_superblank(superblank_bucket.size()-1);

      case '\\':
        next_char = is.get(); // get the peeked char for real
        return make_token(true);

      case '<':
      {
        Token t = make_token();
        if(t.type == Symbol && alphabet.is_tag(t.symbol)) // the '<' introduced a tag, that's fine
          return t;
        return Token::as_reservedcharacter('<');
      }

      default:
        return Token::as_reservedcharacter((char)is.get());
    }
  }
  return make_token();
}

SymbolNumber
TokenIOStream::to_symbol(const Token& t) const
{
  switch(t.type)
  {
    case Symbol:
      return t.symbol;
    case Superblank:
      return alphabet.get_blank_symbol();
    case None:
    case Character:
    case ReservedCharacter:
    default:
      return NO_SYMBOL_NUMBER;
  }
}
SymbolNumberVector
TokenIOStream::to_symbols(const TokenVector& t) const
{
  SymbolNumberVector res;
  for(TokenVector::const_iterator it=t.begin(); it!=t.end(); it++)
    res.push_back(to_symbol(*it));
  return res;
}

std::string
TokenIOStream::escape(const std::string& str) const
{
  std::string res = "";
  for(std::string::const_iterator it=str.begin(); it!=str.end(); it++)
  {
    if(escaped_chars.find(*it) != escaped_chars.end())
      res += '\\';
    res += *it;
  }
  return res;
}

Token
TokenIOStream::get_token()
{
  if(!token_buffer.isEmpty())
    return token_buffer.next();

  Token token = read_token();
  if(token.type != None)
    token_buffer.add(token);
  return token;
}

void
TokenIOStream::put_token(const Token& t)
{
    os << token_to_string(t);
}

void
TokenIOStream::put_tokens(const TokenVector& t)
{
  for(TokenVector::const_iterator it=t.begin(); it!=t.end(); it++)
    put_token(*it);
}
void
TokenIOStream::put_symbols(const SymbolNumberVector& s, CapitalizationState caps)
{
  os << alphabet.symbols_to_string(s, caps);
}

std::string
TokenIOStream::token_to_string(const Token& t, bool raw) const
{
  switch(t.type)
  {
    case Symbol:
      if(t.escaped) {
        return "\\" + alphabet.symbol_to_string(t.symbol);
      }
      else {
        return alphabet.symbol_to_string(t.symbol);
      }
    case Character:
      if(raw)
        return t.character;
      else
        return escape(t.character);
    case Superblank:
      return superblank_bucket[t.superblank_index];
    case ReservedCharacter:
      return t.character;
    default:
      return "";
  }
}

std::string
TokenIOStream::tokens_to_string(const TokenVector& t, bool raw) const
{
  std::string res;
  for(TokenVector::const_iterator it=t.begin(); it!=t.end(); it++)
    res += token_to_string(*it,raw);
  return res;
}
