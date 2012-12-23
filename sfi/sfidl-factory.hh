// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef _SFIDL_FACTORY_H_
#define _SFIDL_FACTORY_H_

#include "sfidl-utils.hh"

namespace Sfidl {

class Options;
class Parser;
class CodeGenerator;

class Factory {
protected:
  Factory();
  virtual ~Factory();

public:
  /*
   * creation routine - should create a code generator
   */
  virtual CodeGenerator *create (const Parser& parser) const = 0;

  /*
   * returns the command line option (e.g. --plugin)
   */
  virtual String option () const = 0;

  /*
   * returns the full description
   */
  virtual String description () const = 0;

  /*
   * lists all available factories
   */
  static std::list<Factory *> listFactories ();
};

};

#endif /* _SFIDL_FACTORY_H_ */

/* vim:set ts=8 sts=2 sw=2: */
