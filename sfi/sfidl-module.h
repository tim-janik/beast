/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2003 Tim Janik
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
#ifndef _SFIDL_MODULE_H__
#define _SFIDL_MODULE_H__

#include "sfidl.h"
#include "sfidl-parser.h"
#include <stdio.h>
#include <string>
#include <list>

namespace Sfidl {
  enum Type {
    VOID,
    BOOL,
    INT,
    NUM,
    REAL,
    STRING,
    CHOICE,
    BBLOCK,
    FBLOCK,
    PSPEC,
    SEQUENCE,
    RECORD,
    OBJECT,     /* PROXY */
  };
                          
  class CodeGeneratorModule : public CodeGenerator {
  public:
    CodeGeneratorModule (const Parser &parser) : CodeGenerator (parser) {
    }
    void run ();
  };
};

#endif	/* _SFIDL_MODULE_H__ */

/* vim:set ts=8 sts=2 sw=2: */
