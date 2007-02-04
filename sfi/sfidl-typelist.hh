/* SFI - Synthesis Fusion Kit Interface                 -*-mode: c++;-*-
 * Copyright (C) 2004 Stefan Westerfeld
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

#ifndef _SFIDL_TYPELIST_H__
#define _SFIDL_TYPELIST_H__

#include "sfidl-generator.hh"
#include <string>
#include <list>

namespace Sfidl {

class CodeGeneratorTypeList : public CodeGenerator {
public:
  CodeGeneratorTypeList (const Parser &parser) : CodeGenerator (parser) {
  }
  bool run ()
  {
    using namespace std;
    vector<string>::const_iterator ti;

    for(ti = parser.getTypes().begin(); ti != parser.getTypes().end(); ti++)
      {
	if (parser.fromInclude (*ti)) continue;

	printf ("%s\n", makeMixedName (*ti).c_str());
      }
    return true;
  }
};

} // Sfidl

#endif	/* _SFIDL_TYPELIST_H__ */

/* vim:set ts=8 sts=2 sw=2: */
