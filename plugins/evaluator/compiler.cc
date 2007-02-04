/*
 * Copyright (C) 2003 Stefan Westerfeld <stefan@space.twc.de>
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

#include "compiler.hh"
#include <assert.h>
#include <ctype.h>

using std::vector;
using std::string;
using namespace Bse::EvaluatorUtils;

string Compiler::tokenize(Symbols& symbols, const vector<char>& code_without_nl, vector<Token>& tokens)
{
    enum {
	stNeutral,
	stNumber,
	stVariable
    } state = stNeutral;

    string currentToken;

    vector<char> code = code_without_nl;
    code.push_back('\n');

    vector<char>::const_iterator ci = code.begin();
    while (ci != code.end())
    {
	if (state == stNeutral)
	{
	    if (isdigit (*ci) || (*ci == '.'))
	    {
		state = stNumber;
		currentToken = *ci;
	    }
	    else if (isalnum (*ci))
	    {
		state = stVariable;
		currentToken = *ci;
	    }
	    else if (isspace (*ci))
	    {
		// skip whitespace chars
	    }
	    else if (*ci == '+')
	    {
		tokens.push_back(Token(Token::PLUS));
	    }
	    else if (*ci == '*')
	    {
		tokens.push_back(Token(Token::MUL));
	    }
	    else if (*ci == '=')
	    {
		tokens.push_back(Token(Token::EQUALS));
	    }
	    else if (*ci == ';')
	    {
		tokens.push_back(Token(Token::SEMICOLON));
	    }
	    else if (*ci == '(')
	    {
		tokens.push_back(Token(Token::LEFT_PAREN));
	    }
	    else if (*ci == ')')
	    {
		tokens.push_back(Token(Token::RIGHT_PAREN));
	    }
	    else
	    {
		char s[2] = { *ci, 0 };

		return "can't interpret '" + string(s) + "'";
	    }
	    ci++;
	}
	else if (state == stNumber)
	{
	    if (!(isdigit (*ci) || (*ci == '.')))
	    {
		double value =  atof(currentToken.c_str());
		/* printf("NUMBER: %f\n", value); */
		tokens.push_back(Token(Token::NUMBER, value));
		state = stNeutral;
	    }
	    else
	    {
		currentToken += *ci++;
	    }
	}
    	else if (state == stVariable)
	{
	    if (!isalnum (*ci))
	    {
		Token t(Token::VARIABLE);
		t.reg = symbols.alloc(currentToken);
		tokens.push_back(t);
		state = stNeutral;
	    }
	    else
	    {
		currentToken += *ci++;
	    }
	}
    }
    return "";
}

Compiler::Compiler(Symbols& symbols, const vector<Token>& tokens)
    : symbols(symbols), tokens(tokens)
{
    for(unsigned int i = 0; i < tokens.size(); i++)
	done.push_back(false);
}

int Compiler::compile(int begin, int size, vector<Instruction>& instructions)
{
    int reg = -1;
    int end = begin+size;

    printf("compiling [%d:%d] : ", begin, end);
    for(int i=begin;i<end;i++)
    {
	printf("<%s> ",tokens[i].str().c_str());
    }
    printf("\n");

    if (size == 1)
    {
	if (tokens[begin].type == Token::NUMBER)
	{
	    done[begin] = true;
	    reg = symbols.alloc();
	    instructions.push_back (Instruction::rv(Instruction::SET, reg, tokens[begin].value));
	}
	else if (tokens[begin].type == Token::VARIABLE)
	{
	    // TODO: optimization pass: to remove redundant moves
	    done[begin] = true;
	    reg = symbols.alloc();
	    instructions.push_back (Instruction::rr(Instruction::MOVE, reg, tokens[begin].reg));
	}
	else
	{
	    assert (false);
	}
    }
    else for(;;)
    {
	/* find operator smallest precedence for which no code has been generated yet */
	int plevel = 0;
	int best = -1;
	for(int i = begin; i < begin+size; i++)
	{
	    if (tokens[i].type == Token::RIGHT_PAREN)
		plevel--;
	    if (!done[i] && tokens[i].is_operator() && plevel == 0)
	    {
		if (best == -1)
		    best = i;
		else if (tokens[i].operator_precedence() > tokens[best].operator_precedence())
		    best = i;
	    }
	    if (tokens[i].type == Token::LEFT_PAREN)
		plevel++;
	}

	printf("best is %d\n", best);
	if (best == -1)
	    break;

	if (size >= 2 && tokens[best].type == Token::LEFT_PAREN && tokens[end-1].type == Token::RIGHT_PAREN)
	{
	    if (best == begin) /* (expr) */
	    {
		done[end-1] = done[best] = true;
		reg = compile(begin+1,size-2,instructions);
	    }
	    else if (best == begin + 1) /* function call */
	    {
		if (tokens[begin].type == Token::VARIABLE && symbols.name(tokens[begin].reg) == "sin")
		{
		    int reg1 = compile(begin+2,size-3,instructions);
		    instructions.push_back(Instruction::rr(Instruction::SIN, reg1, reg1));
		    done[end-1] = done[best] = true;
		    reg = reg1;
		}
	    }
	}
	else if (tokens[best].type == Token::MUL) /* expr * expr */
	{
	    int reg1 = compile(begin,best-begin,instructions);
	    int reg2 = compile(best+1,end-best-1,instructions);
	    instructions.push_back (Instruction::rr(Instruction::MUL, reg1, reg2));
	    done[best] = true;
	    reg = reg1;
	}
	else if (tokens[best].type == Token::PLUS) /* expr + expr */
	{
	    int reg1 = compile(begin,best-begin,instructions);
	    int reg2 = compile(best+1,end-best-1,instructions);
	    instructions.push_back (Instruction::rr(Instruction::ADD, reg1, reg2));
	    done[best] = true;
	    reg = reg1;
	}
	else if(tokens[best].type == Token::EQUALS) /* var = expr */
	{
	    // CHECK that reg1 is an LVALUE
	    assert(best-begin == 1 && tokens[begin].type == Token::VARIABLE);
	    int reg1 = tokens[begin].reg;
	    int reg2 = compile(best+1,end-best-1,instructions);
	    instructions.push_back (Instruction::rr(Instruction::MOVE, reg1, reg2));
	    done[best] = true;
	    reg = reg1;
	}
	else if(tokens[best].type == Token::SEMICOLON) /* expr ; expr */
	{
	    compile(begin,best-begin,instructions);
	    reg = compile(best+1,end-best-1,instructions);

	    done[best] = true;
	}
	else
	{
	    fprintf(stderr, "can't compile code for token %s\n", tokens[best].str().c_str());
	    assert(false);
	}
    }
    assert(reg != -1 || size == 0);
    return reg;

    /*
    // evaluate expression:
    // 1. treeify
    // 2. evaluate LHS => tmp1
    // 3. evaluate RHS => tmp2
    // 4. result = OP tmp1 tmp2

    if (tokens[0].type == Token::NUMBER
    &&  tokens[1].type == Token::PLUS
    &&  tokens[2].type == Token::NUMBER)
    {
	int reg1 = symbols.alloc();
	int reg2 = symbols.alloc();

	instructions.push_back (Instruction::rv(Instruction::SET, reg1, tokens[0].value));
	instructions.push_back (Instruction::rv(Instruction::SET, reg2, tokens[2].value));
	instructions.push_back (Instruction::rr(Instruction::ADD, reg1, reg2));

	// result in reg1
    }
    */
}

string Compiler::compile(Symbols& symbols, const vector<Token>& tokens, vector<Instruction>& instructions)
{
    Compiler c(symbols, tokens);
    c.compile(0, tokens.size(), instructions);

    return "";
}
