/* SFI - Synthesis Fusion Kit Interface                 -*-mode: c++;-*-
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

#include "sfidl-generator.h"
#include "sfidl-parser.h"
#include <stdio.h>
#include <string>
#include <list>

namespace Sfidl {

class CodeGeneratorModule : public CodeGeneratorCBase {
protected:
  const gchar* TypeRef (const std::string &type);
  std::string createTypeCode (const std::string& type, const std::string& name,
                              TypeCodeModel model);
  const gchar* TypeField (const std::string& type);

  std::string pspec_constructor (const Param &param);
  const char* func_value_set_param (const Param &param);
  std::string func_value_get_param (const Param &param, const std::string dest);
  std::string func_value_dup_param (const Param &param);
  std::string func_param_return_free (const Param &param);
  std::string func_param_free (const Param &param);

public:
  CodeGeneratorModule (const Parser &parser) : CodeGeneratorCBase (parser) {
  }
  void run ();
};
} // Sfidl

#endif	/* _SFIDL_MODULE_H__ */

/* vim:set ts=8 sts=2 sw=2: */
