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

#include "sfidl-typelist.hh"
#include "sfidl-factory.hh"

using namespace Sfidl;
using namespace std;

namespace {

class TypeListFactory : public Factory {
public:
  string option() const	      { return "--list-types"; }
  string description() const  { return "print all types defined in the idlfile"; }
  
  CodeGenerator *create (const Parser& parser) const
  {
    return new CodeGeneratorTypeList (parser);
  }
} typelist_factory;

} // anon

/* vim:set ts=8 sts=2 sw=2: */
