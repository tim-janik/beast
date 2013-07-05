// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

#include "instruction.hh"

using namespace Bse::EvaluatorUtils;

Instruction Instruction::rr(Instruction::Type ins, int reg1, int reg2)
{
    Instruction i;
    i.ins = ins;
    i.p1.reg = reg1;
    i.p2.reg = reg2;
    return i;
}

Instruction Instruction::rv(Instruction::Type ins, int reg, double val)
{
    Instruction i;
    i.ins = ins;
    i.p1.reg = reg;
    i.p2.val = val;
    return i;
}

void Instruction::rw_registers(int& read1, int& read2, int& write1, int& write2) const
{
    read1 = read2 = write1 = write2 = -1;
    if (ins == SET)
    {
	write1 = p1.reg;
    }
    if (ins == MOVE)
    {
	write1 = p1.reg;
	read1 = p2.reg;
    }
    if (ins == ADD || ins == MUL)
    {
	read1 = p1.reg;
	read2 = p2.reg;
	write1 = p1.reg;
    }
    if (ins == SIN)
    {
	read1 = write1 = p1.reg;
    }
}

void Instruction::print(const Symbols& symbols) const
{
    switch(ins)
    {
	case SET:   printf("SET  %s, %f\n", symbols.name(p1.reg).c_str(), p2.val);
		    break;
	case MOVE:  printf("MOVE %s, %s\n", symbols.name(p1.reg).c_str(),
					    symbols.name(p2.reg).c_str());
		    break;
	case ADD:   printf("ADD  %s, %s\n", symbols.name(p1.reg).c_str(),
					    symbols.name(p2.reg).c_str());
		    break;
	case MUL:   printf("MUL  %s, %s\n", symbols.name(p1.reg).c_str(),
					    symbols.name(p2.reg).c_str());
		    break;
    	case SIN:   printf("SIN  %s\n",     symbols.name(p1.reg).c_str());
		    break;
    }
}

/* vim:set ts=8 sw=4 sts=4: */
