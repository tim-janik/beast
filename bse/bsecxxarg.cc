/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2003 Tim Janik
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
#include "bsecxxarg.hh"
#include "bsecxxbase.hh"

namespace {
using namespace Bse;

} // namespace


namespace Bse {

const String
tokenize_gtype (GType t)
{
  switch (G_TYPE_FUNDAMENTAL (t))
    {
    case G_TYPE_BOOLEAN:        return "b";
    case G_TYPE_INT:            return "i";
    case SFI_TYPE_NUM:          return "n";
    case SFI_TYPE_REAL:         return "r";
    case G_TYPE_POINTER:        return "*";
    case G_TYPE_STRING:         return "s";
    case G_TYPE_PARAM:          return "P";
    case G_TYPE_OBJECT:
      if (g_type_is_a (t, BSE_TYPE_CXX_BASE))
        return "X";
      else
        return "O";
    default:
      throw InvalidArgument (G_STRLOC);
    }
}

} // Bse
