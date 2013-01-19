// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
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
