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

#ifndef _HFST_PROC_ALPHABET_H_
#define _HFST_PROC_ALPHABET_H_

#include <string>
#include <unordered_set>
#include <vector>

#include "hfst-proc.h"

class LetterTrie;
typedef std::vector<LetterTrie *> LetterTrieVector;
class Symbolizer;

extern bool processCompounds;

class LetterTrie
{
  private:
    LetterTrieVector letters;
    SymbolNumberVector symbols;

    /**
     * Whether our symbols vector or that of any of our children contains
     * symbol number 0
     */
    bool has_symbol_0() const;

  public:
    LetterTrie(void)
        : letters(std::numeric_limits<unsigned char>::max(),
                  (LetterTrie *)NULL),
          symbols(std::numeric_limits<unsigned char>::max(), NO_SYMBOL_NUMBER)
    {
    }

    LetterTrie(const LetterTrie &o) : letters(), symbols(o.symbols)
    {
        for (LetterTrieVector::const_iterator it = o.letters.begin();
             it != o.letters.end(); it++)
            letters.push_back(((*it) == NULL) ? NULL : new LetterTrie(*(*it)));
    }

    ~LetterTrie()
    {
        for (LetterTrieVector::iterator it = letters.begin();
             it != letters.end(); it++)
            delete *it;
    }

    void add_string(const char *p, SymbolNumber symbol_key);

    SymbolNumber find_symbol(const char *c) const;

    /**
     * Read the next symbol from the stream. If the next character(s) do not
     * form a symbol, this version will put the characters back, so the stream
     * is in the same condition it was when this function was called
     * @return the number of the symbol, 0 for EOF, or NO_SYMBOL_NUMBER
     */
    SymbolNumber extract_symbol(std::istream &is) const;

    friend class Symbolizer;
};

class Symbolizer
{
  private:
    LetterTrie letters;
    SymbolNumberVector ascii_symbols;

    SymbolNumber symbol_count;

  public:
    Symbolizer()
        : letters(), ascii_symbols(std::numeric_limits<unsigned char>::max(),
                                   NO_SYMBOL_NUMBER),
          symbol_count(0)
    {
    }
    Symbolizer(const SymbolTable &st)
        : letters(), ascii_symbols(std::numeric_limits<unsigned char>::max(),
                                   NO_SYMBOL_NUMBER),
          symbol_count(0)
    {
        add_symbols(st);

        if (letters.has_symbol_0())
        {
            std::cerr << "!! Warning: the letter trie contains references to "
                         "symbol  !!\n"
                      << "!! number 0. This is almost certainly a bug and "
                         "could      !!\n"
                      << "!! cause certain characters to be misinterpreted as "
                         "EOF    !!\n";
        }
    }

    void add_symbol(const std::string &symbol_str);
    void add_symbols(const SymbolTable &st);

    SymbolNumber find_symbol(const char *c) const;
    SymbolNumber extract_symbol(std::istream &is) const;
};

/**
 * Various properties of a symbol, used internally by ProcTransducerAlphabet
 */
struct SymbolProperties
{
    /**
     * Whether the symbol is considered alphabetic. Roughly, this includes
     * symbols that are not whitespace or punctuation
     */
    bool alphabetic;

    /**
     * The symbol number of the lowercase version of the symbol. If the symbol
     * is already lowercase, this will contain the symbol's own number. If the
     * symbol has no lowercase form, it will contain NO_SYMBOL_NUMBER
     */
    SymbolNumber lower;

    /**
     * The symbol number of the uppercase version of the symbol. If the symbol
     * is already uppercase, this will contain the symbol's own number. If the
     * symbol has no uppercase form, it will contain NO_SYMBOL_NUMBER
     */
    SymbolNumber upper;
};

typedef std::vector<SymbolProperties> SymbolPropertiesTable;

class ProcTransducerAlphabet : public TransducerAlphabet
{
  private:
    SymbolPropertiesTable symbol_properties_table;

    Symbolizer symbolizer;

    /**
     * The symbol number for a "blank" which is here considered to be a space.
     * This symbol requires special handling because it doubles as the
     * symbolic representation for a superblank block of text
     */
    SymbolNumber blank_symbol;

    std::unordered_set<SymbolNumber> escaped_symbols;

    /**
     * Find the upper/lower-case equivalencies for the symbols in the table
     */
    void calculate_caps();

    /**
     * A routine used for checking/changing a character's case
     * @param c the character to work with
     * @param case_res an output: set to <0 if c is lowercase, >0 if c is
     *                            uppercase, or 0 if neither
     * @return the equivalent character in the opposite case of c, or the empty
     *         string if nonexistent
     */
    static std::string caps_helper(const char *c, int &case_res);
    static std::string caps_helper_single(const char *c, int &case_res);
    static int utf8_str_to_int(const char *c);
    static std::string utf8_int_to_str(int c);

    /**
     * Store the blank symbol's number in blank_symbol, adding it as a new
     * symbol if it cannot be found
     */
    void setup_blank_symbol();

    /**
     * Check for and warn about the symbol table containing multi-char symbols
     * consisting ASCII characters that are available individually elsewhere in
     * the table
     */
    void check_for_overlapping() const;

    void print_table() const;

    void add_symbol(const std::string &str, const SymbolProperties &symbol);

  public:
    ProcTransducerAlphabet(std::istream &is, SymbolNumber symbol_count);

    const Symbolizer &get_symbolizer(void) const;
    SymbolNumber
    get_blank_symbol() const
    {
        return blank_symbol;
    }

    bool is_punctuation(const char *c) const;
    bool is_space(const char *c) const;

    bool
    is_alphabetic(const char *c) const
    {
        return (c[0] != '\0' && !is_space(c) && !is_punctuation(c));
    }
    bool
    is_alphabetic(SymbolNumber symbol) const
    {
        return symbol_properties_table[symbol].alphabetic;
    }

    bool
    is_lower(SymbolNumber symbol) const
    {
        return symbol_properties_table[symbol].lower == symbol;
    }
    bool
    is_upper(SymbolNumber symbol) const
    {
        return symbol_properties_table[symbol].upper == symbol;
    }
    bool
    has_case(SymbolNumber symbol) const
    {
        return symbol_properties_table[symbol].lower != NO_SYMBOL_NUMBER
               || symbol_properties_table[symbol].upper != NO_SYMBOL_NUMBER;
    }
    /**
     * Returns the lowercase equivalent of symbol, or just symbol if there is
     * no lowercase equivalent
     */
    SymbolNumber
    to_lower(SymbolNumber symbol) const
    {
        return symbol_properties_table[symbol].lower == NO_SYMBOL_NUMBER
                   ? symbol
                   : symbol_properties_table[symbol].lower;
    }

    /**
     * Returns the uppercase equivalent of symbol, or just symbol if there is
     * no uppercase equivalent
     */
    SymbolNumber
    to_upper(SymbolNumber symbol) const
    {
        return symbol_properties_table[symbol].upper == NO_SYMBOL_NUMBER
                   ? symbol
                   : symbol_properties_table[symbol].upper;
    }

    /**
     * Whether the symbol is an apertium-style tag (i.e. symbols starting
     * with < and ending with > )
     */
    bool is_tag(SymbolNumber symbol) const;

    /**
     * Whether the symbol marks a compound boundary (+ or #)
     */
    bool is_compound_boundary(SymbolNumber symbol) const;
    int num_compound_boundaries(const SymbolNumberVector &symbols) const;

    std::string
    symbol_to_string(SymbolNumber symbol) const
    {
        return symbol_table[symbol];
    }

    /**
     * Use the symbol table to convert the given symbols into a string,
     * optionally modifying the case of some symbols
     * @param symbols the symbols to convert
     * @param caps how to modify case. The states are handled as follows:
     *             Unknown        - case unchanged
     *             LowerCase      - same as Unknown
     *             FirstUpperCase - first symbol forced to uppercase
     *             UpperCase      - all symbols force to uppercase
     * @return the string representation of the symbols
     */
    std::string symbols_to_string(const SymbolNumberVector &symbols,
                                  CapitalizationState caps = Unknown,
                                  bool raw = false) const;
};

#endif
