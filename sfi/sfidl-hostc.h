/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002-2004 Stefan Westerfeld, 2003 Tim Janik
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
#ifndef __SFIDL_HOSTC_H__
#define __SFIDL_HOSTC_H__

#include <sfi/glib-extra.h>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <string>
#include "sfidl-cbase.h"

namespace Sfidl {
  
  class CodeGeneratorHostC : public CodeGeneratorCBase {
  protected:
    void printChoiceMethodPrototypes (PrefixSymbolMode mode);
    void printChoiceMethodImpl();
    void printRecordFieldDeclarations();
    void printInitFunction (const std::string& initFunction);    

  public:
    CodeGeneratorHostC (const Parser& parser) : CodeGeneratorCBase (parser) {
    }
    bool run ();
  };

};

#endif  /* __SFIDL_HOSTC_H__ */

/* vim:set ts=8 sts=2 sw=2: */
