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
