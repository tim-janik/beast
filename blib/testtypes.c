/* BLib - BSE/BSI helper library
 * Copyright (C) 1997, 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"glib-includes.h"

int
main (int   argc,
      char *argv[])
{
  GType type1, type2, *types;
  guint n_types, i;

  g_type_init ();

  type1 = G_TYPE_INTERFACE;	g_message ("%s: %d", g_type_name (type1), type1);
  type1 = G_TYPE_ENUM;		g_message ("%s: %d", g_type_name (type1), type1);
  type1 = G_TYPE_FLAGS;		g_message ("%s: %d", g_type_name (type1), type1);
  type1 = G_TYPE_PARAM;		g_message ("%s: %d", g_type_name (type1), type1);
  type1 = G_TYPE_OBJECT;	g_message ("%s: %d", g_type_name (type1), type1);

  type1 = G_TYPE_OBJECT;	type2 = G_TYPE_OBJECT;		g_message ("%s is_a %s: %s", g_type_name (type1), g_type_name (type2), g_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = G_TYPE_PARAM_INT;	type2 = G_TYPE_PARAM_INT;	g_message ("%s is_a %s: %s", g_type_name (type1), g_type_name (type2), g_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = G_TYPE_PARAM_LONG;	type2 = G_TYPE_PARAM;		g_message ("%s is_a %s: %s", g_type_name (type1), g_type_name (type2), g_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = G_TYPE_PARAM_FLOAT;	type2 = 0;			g_message ("%s is_a %s: %s", g_type_name (type1), g_type_name (type2), g_type_is_a (type1, type2) ? "TRUE" : "FALSE");

  type1 = G_TYPE_OBJECT;	g_message ("%d ==> `%s' ==> %d", type1, g_type_name (type1), g_type_from_name (g_type_name (type1)));
  type1 = G_TYPE_ENUM;		g_message ("%d ==> `%s' ==> %d", type1, g_type_name (type1), g_type_from_name (g_type_name (type1)));
  // type1 = G_TYPE_TEXT;	g_message ("%d ==> `%s' ==> %d", type1, g_type_name (type1), g_type_from_name (g_type_name (type1)));

  type1 = G_TYPE_OBJECT;	type2 = G_TYPE_OBJECT;		g_message ("%s conforms_to %s: %s", g_type_name (type1), g_type_name (type2), g_type_conforms_to (type1, type2) ? "TRUE" : "FALSE");

  type1 = G_TYPE_PARAM_STRING;	g_message ("parent of %s: %s", g_type_name (type1), g_type_name (g_type_parent (type1)));
  type1 = G_TYPE_OBJECT;	g_message ("parent of %s: %s", g_type_name (type1), g_type_name (g_type_parent (type1)));

  type1 = G_TYPE_PARAM;		type2 = G_TYPE_PARAM_CHAR;	g_message ("next base of %s from %s: %s", g_type_name (type2), g_type_name (type1), g_type_name (g_type_next_base (type2, type1)));
  type1 = G_TYPE_PARAM;		type2 = G_TYPE_PARAM;		g_message ("next base of %s from %s: %s", g_type_name (type2), g_type_name (type1), g_type_name (g_type_next_base (type2, type1)));

  types = g_type_children (G_TYPE_PARAM, &n_types);
  for (i = 0; i < n_types; i++)
    {
      GParamSpec *pspec = (gpointer) g_type_create_instance (types[i]);

      g_message ("Created pspec(`%s') `%s' (`%s'): %s\n",
		 g_type_name (G_PARAM_SPEC_TYPE (pspec)),
		 pspec->name, pspec->nick, pspec->blurb);
      g_param_spec_unref (pspec);
    }
  g_free (types);

  return 0;
}
