/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef _SFIDL_OPTIONS_H_
#define _SFIDL_OPTIONS_H_

#include <string>
#include <vector>
#include <utility>

namespace Sfidl {

class Factory;
class Parser;
class CodeGenerator;

typedef std::vector< std::pair <std::string, bool> > OptionVector;

struct Options {
  CodeGenerator *codeGenerator;
  std::string    codeGeneratorName;
  bool           doHelp;
  bool	         doExit;
  std::string    sfidlName;

  std::vector<std::string> includePath; // path to search for includes

  Options ();
  bool parse (int *argc_p, char **argv_p[], const Parser& parser);
  void printUsage ();

  static Options *the();
};

};
#endif /* _SFIDL_OPTIONS_H_ */
