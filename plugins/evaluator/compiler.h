    /*

    Copyright (C) 2003 Stefan Westerfeld <stefan@space.twc.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    */

#ifndef BSE_EVALUATOR_COMPILER_H
#define BSE_EVALUATOR_COMPILER_H

#include "token.h"
#include "instruction.h"
#include "symbols.h"

#include <vector>
#include <string>

namespace Bse {
namespace EvaluatorUtils {

struct Compiler {
    Symbols& symbols;
    const std::vector<Token>& tokens;
    std::vector<bool> done;

    Compiler(Symbols& symbols, const std::vector<Token>& tokens);
    int compile(int begin, int size, std::vector<Instruction>& instructions);
public:
    static std::string tokenize(Symbols& symbols, const std::vector<char>& source, std::vector<Token>& tokens);
    static std::string compile(Symbols& sybols, const std::vector<Token>& tokens, std::vector<Instruction>& instructions);
};

}
}

#endif // BSE_EVALUATOR_COMPILER_H
