// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

#ifndef BSE_EVALUATOR_TOKEN_H
#define BSE_EVALUATOR_TOKEN_H

#include <string>

namespace Bse {
namespace EvaluatorUtils {

struct Token {
    enum TokenType {
	NONE, 
	PLUS,
	MUL,
	EQUALS,
	SEMICOLON,
	LEFT_PAREN,
	RIGHT_PAREN,
	NUMBER,
	VARIABLE
    } type;

    double value;
    int reg;

    Token(TokenType type = NONE, double value = 0.0)
	: type (type), value (value)
    {
    }

    bool is_operator() const
    {
	return operator_precedence() != -1;
    }

    int operator_precedence() const
    {
	/* higher numbers mean higher precedence */
	switch(type)
	{
	    case LEFT_PAREN:    return 1;
	    case RIGHT_PAREN:   return 1;
	    case MUL:	        return 2;
	    case PLUS:	        return 3;
	    case EQUALS:        return 4;
	    case SEMICOLON:	return 5;
	    default:	        return -1;
	}
    }

    std::string str() const
    {
	switch(type)
	{
	    case SEMICOLON:	    return ";";
	    case MUL:		    return "*";
	    case PLUS:		    return "+";
	    case LEFT_PAREN:	    return "(";
	    case RIGHT_PAREN:	    return ")";
	    case EQUALS:	    return "=";
	    case VARIABLE:	    return "VAR";
	    case NUMBER:	    return "NUM";
	    default:	    return "?";
	}
    }
};

}
}

#endif
