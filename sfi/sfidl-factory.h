/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2004 Stefan Westerfeld
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
#ifndef _SFIDL_FACTORY_H_
#define _SFIDL_FACTORY_H_

#include <string>
#include <list>

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
   * initialization routine, called to adjust options accordingly
   */
  virtual void init (Options& options) const = 0;

  /*
   * creation routine - should create a code generator
   */
  virtual CodeGenerator *create (const Parser& parser) const = 0;

  /*
   * returns the command line option (i.e. --module)
   */
  virtual std::string option () const = 0;

  /*
   * returns the full description
   */
  virtual std::string description () const = 0;

  /*
   * lists all available factories
   */
  static std::list<Factory *> listFactories ();
};

};

#endif /* _SFIDL_FACTORY_H_ */

/* vim:set ts=8 sts=2 sw=2: */
