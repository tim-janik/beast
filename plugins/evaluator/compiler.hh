/*
 * Copyright (C) 2003 Stefan Westerfeld <stefan@space.twc.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
  
#ifndef BSE_EVALUATOR_COMPILER_H
#define BSE_EVALUATOR_COMPILER_H

#include "token.hh"
#include "instruction.hh"
#include "symbols.hh"

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
