//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published
//   by the Free Software Foundation, version 3 of the Licence.
//
//   This library is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <string>

#include "HfstDataTypes.h"

namespace hfst
{
namespace twolc
{

class TwolcCompiler
{
  public:
    static int compile(const std::string &inputfile,
                       const std::string &outputfile, bool silent,
                       bool verbose, bool resolve_left_conflicts,
                       bool resolve_right_conflicts,
                       hfst::ImplementationType type);
};

} // namespace twolc
} // namespace hfst
