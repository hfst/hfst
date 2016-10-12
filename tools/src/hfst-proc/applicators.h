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

#ifndef _HFST_PROC_APPLICATORS_H_
#define _HFST_PROC_APPLICATORS_H_

#include "lookup-path.h"
#include "tokenizer.h"
#include "transducer.h"

/**
 * Abstract base class for actions that can be done using a transducer
 */
class Applicator
{
 protected:
  const ProcTransducer& transducer;
  TokenIOStream& token_stream;
 public:
  Applicator(const ProcTransducer& t, TokenIOStream& ts): transducer(t), token_stream(ts) {}
  virtual ~Applicator() {}

  virtual void apply() = 0;
};

/**
 * Splits the input stream into tokens and outputs the results
 */
class TokenizationApplicator: public Applicator
{
 private:
  std::string process_token(const Token& t) const;
 public:
  TokenizationApplicator(const ProcTransducer& t, TokenIOStream& ts): Applicator(t,ts) {}
  void apply();
};

class AnalysisApplicator: public Applicator
{
 private:
  OutputFormatter& formatter;
  CapitalizationMode caps_mode;
 public:
  AnalysisApplicator(const ProcTransducer& t, TokenIOStream& ts,
                     OutputFormatter& o, CapitalizationMode c):
    Applicator(t,ts), formatter(o), caps_mode(c) {}
  void apply();
};

class GenerationApplicator: public Applicator
{
 private:
  GenerationMode mode;
  CapitalizationMode caps_mode;

  /**
   * Split the given token string into a set of token strings to generate with,
   * breaking on + after apertium-style tags.
   */
  std::vector<TokenVector> split(const TokenVector& tokens) const;

  bool lookup(const TokenVector& tokens, bool generate_on_fail);

  LookupPathSet preprocess_finals(const LookupPathSet& finals) const ;


 public:
  GenerationApplicator(const ProcTransducer& t, TokenIOStream& ts,
                       GenerationMode m, CapitalizationMode c):
    Applicator(t,ts), mode(m), caps_mode(c) {}
  void apply();
};

#endif
