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

#include "cpu.hh"

#include <algorithm>
#include <assert.h>

using std::vector;
using std::max;
using namespace Bse::EvaluatorUtils;

CPU::CPU()
{
    regs = 0;
    n_registers = 0;
}

void CPU::set_program(const vector<Instruction>& new_instructions)
{
    if (regs)
	free (regs);

    instructions = new_instructions;
    /* alloc regs, initialize n_registers */

    n_registers = 0;
    vector<Instruction>::const_iterator ip;

    for(ip = instructions.begin(); ip != instructions.end(); ip++)
    {
	int r[4];
	ip->rw_registers(r[0],r[1],r[2],r[3]);

	for(int k = 0; k < 4; k++)
	    n_registers = max(r[k]+1, n_registers);
    }

    /* FIXME: */
    n_registers = max(2, n_registers);

    regs = (double *)calloc(n_registers, sizeof(double));
}

CPU::~CPU()
{
    if (regs)
	free(regs);
}

void CPU::print_registers(const Symbols& symbols)
{
    printf("STATE: n_registers = %d\n", n_registers);
    for(int i = 0; i < n_registers; i++)
	printf("  %8s = %.8g\n", symbols.name(i).c_str(), regs[i]);
}

void CPU::print_program(const Symbols& symbols)
{
    vector<Instruction>::const_iterator ip;
    for(ip = instructions.begin(); ip != instructions.end(); ip++)
	ip->print(symbols);
}

void CPU::execute()
{
    vector<Instruction>::const_iterator ip;
    for(ip = instructions.begin(); ip != instructions.end(); ip++)
	ip->exec(regs);
}

void CPU::execute_1_1_block(int sreg, int dreg, const float *sdata, float *ddata, int samples)
{
    assert(sreg >= 0 && sreg <= n_registers); /* g_return_if_fail */
    assert(dreg >= 0 && dreg <= n_registers); /* g_return_if_fail */

    for(int i = 0; i < samples; i++)
    {
	vector<Instruction>::const_iterator ip;
	regs[sreg] = sdata[i];
	for(ip = instructions.begin(); ip != instructions.end(); ip++)
	    ip->exec(regs);
	ddata[i] = regs[dreg];
    }
}

// vim:set ts=8 sts=4 sw=4:
