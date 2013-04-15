// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

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
