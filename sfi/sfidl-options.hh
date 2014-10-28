// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef _SFIDL_OPTIONS_H_
#define _SFIDL_OPTIONS_H_

#include <utility>
#include "sfidl-utils.hh"

namespace Sfidl {

class Factory;
class Parser;
class CodeGenerator;

typedef std::vector< std::pair <String, bool> > OptionVector;

struct Options {
  CodeGenerator *codeGenerator;
  String         codeGeneratorName;
  bool           doHelp;
  bool	         doExit;
  String         sfidlName;

  std::vector<String> includePath; // path to search for includes

  Options ();
  bool parse (int *argc_p, char **argv_p[], const Parser& parser);
  void printUsage ();

  static Options *the();
};

};
#endif /* _SFIDL_OPTIONS_H_ */
