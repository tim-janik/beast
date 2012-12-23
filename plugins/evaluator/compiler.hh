// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
  
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
