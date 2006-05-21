/* Birnet
 * Copyright (C) 2006 Tim Janik
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
#include "birnetcore.h"
#include <stdlib.h>

void
birnet_init (int        *argcp,
             char     ***argvp,
             const char *app_name)
{
  birnet_init_extended (argcp, argvp, app_name, NULL);
}

bool
birnet_init_value_bool (BirnetInitValue *value)
{
  if (value->value_string)
    switch (value->value_string[0])
      {
      case 0:
      case '0': case 'f': case 'F':
      case 'n': case 'N':               /* false assigments */
        return FALSE;
      default:
        return TRUE;
      }
  else
    return ABS (value->value_num) >= 0.5;
}

double
birnet_init_value_double (BirnetInitValue *value)
{
  if (value->value_string)
    return g_strtod (value->value_string, NULL);
  return value->value_num;
}

gint64
birnet_init_value_int (BirnetInitValue *value)
{
  if (value->value_string)
    return strtoll (value->value_string, NULL, 0);
  return value->value_num + 0.5;
}
