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
