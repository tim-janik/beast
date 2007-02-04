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

#ifndef BSE_EVALUATOR_SYMBOLS_H
#define BSE_EVALUATOR_SYMBOLS_H

#include <string>
#include <map>

namespace Bse {
namespace EvaluatorUtils {

class Symbols {
    std::map<std::string,int> symbol_map;
    int n_registers;

public:
    Symbols()
    {
	n_registers = 0;
    }

    int alloc() {
	return n_registers++;
    }

    int alloc(const std::string& name)
    {
	std::map<std::string,int>::iterator smi = symbol_map.find(name);
	if (smi == symbol_map.end())
	    return symbol_map[name] = alloc();
	else
	    return smi->second;
    }

    std::string name(int reg) const
    {
	/* sloow ;) */
	std::map<std::string,int>::const_iterator smi;
	for(smi = symbol_map.begin(); smi != symbol_map.end(); smi++)
	{
	    if (smi->second == reg)
		return smi->first;
	}

	char buffer[1024];
	sprintf(buffer,"R%02d", reg);
	return buffer;
    }

    void clear()
    {
	symbol_map.clear();
	n_registers = 0;
    }
};

}
}

#endif // BSE_EVALUATOR_SYMBOLS_H
