/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2002 Tim Janik
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
#include "bswglue.h"
#include "bswproxy.h"

void
bsw_value_initset_boolean (GValue         *value,
			   GType           type,
			   gboolean        v_bool)
{
  g_value_init (value, type);
  g_value_set_boolean (value, v_bool);
}

void
bsw_value_initset_char (GValue  *value,
			GType    type,
			gchar    v_char)
{
  g_value_init (value, type);
  g_value_set_char (value, v_char);
}

void
bsw_value_initset_uchar (GValue  *value,
			 GType    type,
			 guchar   v_uchar)
{
  g_value_init (value, type);
  g_value_set_uchar (value, v_uchar);
}

void
bsw_value_initset_int (GValue  *value,
		       GType    type,
		       gint     v_int)
{
  g_value_init (value, type);
  g_value_set_int (value, v_int);
}

void
bsw_value_initset_uint (GValue  *value,
			GType    type,
			guint    v_uint)
{
  g_value_init (value, type);
  g_value_set_uint (value, v_uint);
}

void
bsw_value_initset_long (GValue  *value,
			GType    type,
			glong    v_long)
{
  g_value_init (value, type);
  g_value_set_long (value, v_long);
}

void
bsw_value_initset_ulong (GValue  *value,
			 GType    type,
			 gulong   v_ulong)
{
  g_value_init (value, type);
  g_value_set_ulong (value, v_ulong);
}

void
bsw_value_initset_enum (GValue  *value,
			GType    type,
			gint     v_enum)
{
  g_value_init (value, type);
  g_value_set_enum (value, v_enum);
}

void
bsw_value_initset_flags (GValue  *value,
			 GType    type,
			 guint    v_flags)
{
  g_value_init (value, type);
  g_value_set_flags (value, v_flags);
}

void
bsw_value_initset_float (GValue  *value,
			 GType    type,
			 gfloat   v_float)
{
  g_value_init (value, type);
  g_value_set_float (value, v_float);
}

void
bsw_value_initset_double (GValue  *value,
			  GType    type,
			  gdouble  v_double)
{
  g_value_init (value, type);
  g_value_set_double (value, v_double);
}

void
bsw_value_initset_string (GValue      *value,
			  GType        type,
			  const gchar *v_string)
{
  g_value_init (value, type);
  g_value_set_static_string (value, v_string);
}

void
bsw_value_initset_boxed (GValue  *value,
			 GType    type,
			 gpointer v_boxed)
{
  g_value_init (value, type);
  g_value_set_static_boxed (value, v_boxed);
}

void
bsw_value_initset_proxy_notype (GValue  *value,
				BswProxy v_proxy)
{
  g_value_init (value, BSW_TYPE_PROXY);
  bsw_value_set_proxy (value, v_proxy);
}

gchar*
bsw_type_name_to_cname (const gchar *type_name)
{
  gchar *name = g_type_name_to_cname (type_name);

  if (name && name[0] == 'b' && name[1] == 's' && name[2] == 'e')
    name[2] = 'w';

  return name;
}

gchar*
bsw_type_name_to_sname (const gchar *type_name)
{
  gchar *name = g_type_name_to_sname (type_name);

  if (name && name[0] == 'b' && name[1] == 's' && name[2] == 'e')
    name[2] = 'w';

  return name;
}

gchar*
bsw_type_name_to_cupper (const gchar *type_name)
{
  gchar *name = g_type_name_to_cupper (type_name);

  if (name && name[0] == 'B' && name[1] == 'S' && name[2] == 'E')
    name[2] = 'W';

  return name;
}

gchar*
bsw_type_name_to_type_macro (const gchar *type_name)
{
  gchar *name = g_type_name_to_type_macro (type_name);

  if (name && name[0] == 'B' && name[1] == 'S' && name[2] == 'E')
    name[2] = 'W';

  return name;
}

gchar*
bsw_cupper_to_sname (const gchar *cupper)
{
  gchar *sname, *p;

  g_return_val_if_fail (cupper != NULL, NULL);

  sname = g_strdup (cupper);
  for (p = sname; *p; p++)
    if ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9'))
      continue;
    else if (*p >= 'A' && *p <= 'Z')
      *p = *p - 'A' + 'a';
    else
      *p = '-';

  if (sname[0] == 'b' && sname[1] == 's' && sname[2] == 'e')
    sname[2] = 'w';

  return sname;
}
