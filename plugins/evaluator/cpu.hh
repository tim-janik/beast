// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef BSE_EVALUATOR_CPU_H
#define BSE_EVALUATOR_CPU_H
#include "instruction.hh"
#include "symbols.hh"
#include <vector>
namespace Bse {
namespace EvaluatorUtils {
class CPU {
private:
    int n_registers;
    double *regs;
    std::vector<Instruction> instructions;
public:
    CPU();
    ~CPU();
    void set_program(const std::vector<Instruction>& new_instructions);
    void print_registers(const Symbols& symbols);
    void print_program(const Symbols& symbols);
    void execute();
    void execute_1_1_block(int sreg, int dreg, const float *sdata, float *ddata, int samples);
};
}
}
#endif // BSE_EVALUATOR_CPU_H
// vim:set ts=8 sts=4 sw=4:
