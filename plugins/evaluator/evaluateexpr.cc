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

#include <stdio.h>
#include <vector>
#include <string>

#include "symbols.hh"
#include "compiler.hh"
#include "cpu.hh"
#include "instruction.hh"

using std::vector;
using std::string;
using namespace Bse::EvaluatorUtils;

int main(int argc, char **argv)
{
    vector<char> source;

    for(int i = 1; i < argc; i++)
    {
	const char *a = argv[i];

	source.insert(source.end(), a, a + strlen(a));
	source.push_back(' ');
    }

    Symbols symbols;

    vector<Token> tokens;
    vector<Instruction> instructions;
    string error;
    
    error = Compiler::tokenize(symbols, source, tokens);
    if (error != "")
    {
	fprintf(stderr, "tokenization error: %s\n", error.c_str());
	exit(1);
    }
    Compiler::compile(symbols, tokens, instructions);
    if (error != "")
    {
	fprintf(stderr, "compilation error: %s\n", error.c_str());
	exit(1);
    }

    printf("\nPROGRAM:\n\n");
    CPU cpu;
    cpu.set_program(instructions);
    cpu.print_program(symbols);
    cpu.execute();
    printf("\n");
    cpu.print_registers(symbols);
}
