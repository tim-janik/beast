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

#ifndef BSE_EVALUATOR_SYMBOLS_H
#define BSE_EVALUATOR_SYMBOLS_H

#include <string>
#include <map>

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

#endif // BSE_EVALUATOR_SYMBOLS_H
