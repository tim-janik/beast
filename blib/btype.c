/* BLib - BSE/BSI helper library
 * Copyright (C) 1997, 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
#include	"btype.h"


#include	"bparam.h"
#include	"bhashmap.h"



/* --- extern variables --- */
const gchar *blib_log_domain_blib = "BLib";
GType B_TYPE_PARAM_INT = 0;
GType B_TYPE_PARAM_UINT = 0;
GType B_TYPE_PARAM_FLOAT = 0;
GType B_TYPE_PARAM_DOUBLE = 0;
GType B_TYPE_PARAM_TIME = 0;
GType B_TYPE_PARAM_NOTE = 0;
GType B_TYPE_PARAM_DOTS = 0;


/* --- variables --- */
static BHashMap *seq_param_hash_map = NULL;


/* --- prototypes --- */
extern void	b_types_init		(void);	/* sync with bmain.c */
extern void     b_param_types_init      (void); /* sync with bparam.c */


/* --- functions --- */
void
b_types_init (void)
{
  /* initialize GLib type system */
  g_type_init ();

  /* introduce BLib parameter types */
  b_param_types_init ();

  /* setup param type -> switchable id mapping */
  seq_param_hash_map = b_hash_map_new (0);
  b_hash_map_add (seq_param_hash_map, g_type_next_base (B_TYPE_PARAM_BOOL, G_TYPE_PARAM), B_SEQ_PARAM_BOOL);
  b_hash_map_add (seq_param_hash_map, g_type_next_base (B_TYPE_PARAM_INT, G_TYPE_PARAM), B_SEQ_PARAM_INT);
  b_hash_map_add (seq_param_hash_map, g_type_next_base (B_TYPE_PARAM_UINT, G_TYPE_PARAM), B_SEQ_PARAM_UINT);
  b_hash_map_add (seq_param_hash_map, g_type_next_base (B_TYPE_PARAM_ENUM, G_TYPE_PARAM), B_SEQ_PARAM_ENUM);
  b_hash_map_add (seq_param_hash_map, g_type_next_base (B_TYPE_PARAM_FLAGS, G_TYPE_PARAM), B_SEQ_PARAM_FLAGS);
  b_hash_map_add (seq_param_hash_map, g_type_next_base (B_TYPE_PARAM_FLOAT, G_TYPE_PARAM), B_SEQ_PARAM_FLOAT);
  b_hash_map_add (seq_param_hash_map, g_type_next_base (B_TYPE_PARAM_DOUBLE, G_TYPE_PARAM), B_SEQ_PARAM_DOUBLE);
  b_hash_map_add (seq_param_hash_map, g_type_next_base (B_TYPE_PARAM_STRING, G_TYPE_PARAM), B_SEQ_PARAM_STRING);
  b_hash_map_add (seq_param_hash_map, g_type_next_base (G_TYPE_PARAM_OBJECT, G_TYPE_PARAM), B_SEQ_PARAM_OBJECT);
  b_hash_map_add (seq_param_hash_map, g_type_next_base (B_TYPE_PARAM_TIME, G_TYPE_PARAM), B_SEQ_PARAM_TIME);
  b_hash_map_add (seq_param_hash_map, g_type_next_base (B_TYPE_PARAM_NOTE, G_TYPE_PARAM), B_SEQ_PARAM_NOTE);
  b_hash_map_add (seq_param_hash_map, g_type_next_base (B_TYPE_PARAM_DOTS, G_TYPE_PARAM), B_SEQ_PARAM_DOTS);
}

guint
b_seq_param_from_type (GType param_type)
{
  GType base_type = g_type_next_base (param_type, G_TYPE_PARAM);

  return base_type ? b_hash_map_lookup (seq_param_hash_map, base_type) : 0;
}
