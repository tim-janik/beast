/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002, 2003 Tim Janik
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
#include "bsecompat.h"
#include <string.h>


/* --- functions --- */
gchar*
bse_compat_rewrite_type_name (guint          vmajor,
                              guint          vminor,
                              guint          vmicro,
                              const gchar   *type_name)
{
  struct { guint vmajor, vminor, vmicro; gchar *old, *new; } type_changes[] = {
    { 0, 5, 1,  "BseSNet",      "BseCSynth" },
  };
  guint i;
  for (i = 0; i < G_N_ELEMENTS (type_changes); i++)
    if (BSE_VERSION_CMP (vmajor, vminor, vmicro,
                         type_changes[i].vmajor,
                         type_changes[i].vminor,
                         type_changes[i].vmicro) <= 0 &&
        strcmp (type_name, type_changes[i].old) == 0)
      return g_strdup (type_changes[i].new);
  return NULL;
}
