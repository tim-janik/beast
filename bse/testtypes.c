/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
#include	"bse.h"


int
main (int   argc,
      char *argv[])
{
  BseType type1, type2;
  gchar bbuffer[BSE_BBUFFER_SIZE];

  bse_init (&argc, &argv);

  g_message ("BSE_TYPE_SONG: %d", BSE_TYPE_SONG);
  g_message ("BSE_TYPE_SAMPLE: %d", BSE_TYPE_SAMPLE);
  g_message ("BSE_TYPE_SUPER: %d", BSE_TYPE_SUPER);
  g_message ("BSE_TYPE_CONTAINER: %d", BSE_TYPE_CONTAINER);
  g_message ("BSE_TYPE_OBJECT: %d", BSE_TYPE_OBJECT);

  type1 = BSE_TYPE_OBJECT;	type2 = BSE_TYPE_OBJECT;	g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_CONTAINER;	type2 = BSE_TYPE_OBJECT;	g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_OBJECT;	type2 = BSE_TYPE_CONTAINER;	g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_SUPER;	type2 = BSE_TYPE_SUPER;		g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_SUPER;	type2 = BSE_TYPE_CONTAINER;	g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_SUPER;	type2 = BSE_TYPE_OBJECT;	g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_SUPER;	type2 = 0;			g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_CONTAINER;	type2 = BSE_TYPE_SUPER;		g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");

  type1 = BSE_TYPE_TEXT;	type2 = BSE_TYPE_OBJECT;	g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_TEXT;	type2 = BSE_TYPE_STREAM;	g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_STREAM;	type2 = BSE_TYPE_CONTAINER;	g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_STREAM;	type2 = BSE_TYPE_OBJECT;	g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_PCM_STREAM;	type2 = BSE_TYPE_STREAM;	g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");

  type1 = BSE_TYPE_OBJECT;	g_message ("%d ==> `%s' ==> %d", type1, bse_type_name (type1), bse_type_from_name (bse_type_name (type1)));
  type1 = BSE_TYPE_SAMPLE;	g_message ("%d ==> `%s' ==> %d", type1, bse_type_name (type1), bse_type_from_name (bse_type_name (type1)));
  type1 = BSE_TYPE_TEXT;	g_message ("%d ==> `%s' ==> %d", type1, bse_type_name (type1), bse_type_from_name (bse_type_name (type1)));

  type1 = BSE_TYPE_STREAM;	type2 = BSE_TYPE_TEXT;		g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_PCM_STREAM;	type2 = BSE_TYPE_TEXT;		g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_NULL_STREAM;	type2 = BSE_TYPE_TEXT;		g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_FILE_STREAM;	type2 = BSE_TYPE_TEXT;		g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_TEXT;	type2 = BSE_TYPE_TEXT;		g_message ("%s is_a %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_is_a (type1, type2) ? "TRUE" : "FALSE");

  type1 = BSE_TYPE_STREAM;	type2 = BSE_TYPE_OBJECT;	g_message ("%s conforms_to %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_conforms_to (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_SUPER;	type2 = BSE_TYPE_SOURCE;	g_message ("%s conforms_to %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_conforms_to (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_TEXT;	type2 = BSE_TYPE_OBJECT;	g_message ("%s conforms_to %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_conforms_to (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_FILE_STREAM;	type2 = BSE_TYPE_FILE_STREAM;	g_message ("%s conforms_to %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_conforms_to (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_STREAM;	type2 = BSE_TYPE_TEXT;		g_message ("%s conforms_to %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_conforms_to (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_PCM_STREAM;	type2 = BSE_TYPE_TEXT;		g_message ("%s conforms_to %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_conforms_to (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_NULL_STREAM;	type2 = BSE_TYPE_TEXT;		g_message ("%s conforms_to %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_conforms_to (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_FILE_STREAM;	type2 = BSE_TYPE_TEXT;		g_message ("%s conforms_to %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_conforms_to (type1, type2) ? "TRUE" : "FALSE");
  type1 = BSE_TYPE_TEXT;	type2 = BSE_TYPE_TEXT;		g_message ("%s conforms_to %s: %s", bse_type_name (type1), bse_type_name (type2), bse_type_conforms_to (type1, type2) ? "TRUE" : "FALSE");

  g_message ("zero time (%d) -> %s", 0, bse_time_to_str (0));
  g_message ("minimum time (%d) -> %s", BSE_MIN_TIME, bse_time_to_bbuffer (BSE_MIN_TIME, bbuffer));
  g_message ("maximum time (%d) -> %s", BSE_MAX_TIME, bse_time_to_bbuffer (BSE_MAX_TIME, bbuffer));

  return 0;
}
