/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
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
