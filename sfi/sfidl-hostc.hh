/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002-2004 Stefan Westerfeld, 2003 Tim Janik
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
#ifndef __SFIDL_HOSTC_H__
#define __SFIDL_HOSTC_H__

#include <sfi/glib-extra.h>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <string>
#include "sfidl-cbase.hh"

namespace Sfidl {
  
  class CodeGeneratorHostC : public CodeGeneratorCBase {
  protected:
    std::string prefix;
    std::string generateInitFunction;

    void printChoiceMethodPrototypes (PrefixSymbolMode mode);
    void printChoiceMethodImpl();
    void printRecordFieldDeclarations();
    void printInitFunction (const std::string& initFunction);    

  public:
    CodeGeneratorHostC (const Parser& parser) : CodeGeneratorCBase (parser) {
    }
    void help();
    bool run();

    OptionVector getOptions();
    void setOption (const std::string& option, const std::string& value);
  };

};

#endif  /* __SFIDL_HOSTC_H__ */

/* vim:set ts=8 sts=2 sw=2: */
