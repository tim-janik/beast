/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
#include "bseparasite.h"

#include "bsestorage.h"


/* --- defines --- */
#define MAX_PARASITE_VALUES (1024) /* (2 << 24) */


/* --- types --- */
enum
{
  PARASITE_FLOAT		= 'f',
};


/* --- structures --- */
typedef struct _ParasiteList ParasiteList;
typedef struct _Parasite     Parasite;
struct _Parasite
{
  GQuark   quark;
  guint8   type;
  guint    n_values : 24;
  gpointer data;
};
struct _ParasiteList
{
  guint    n_parasites;
  Parasite parasites[1];
};


/* --- variables --- */
static GQuark quark_parasite_list = 0;


/* --- functions --- */
static void
store_parasite_list (BseObject  *object,
		     BseStorage *storage,
		     gpointer    data)
{
  ParasiteList *list;
  guint n;
  
  list = bse_object_get_qdata (object, quark_parasite_list);
  if (!list)
    return;

  for (n = 0; n < list->n_parasites; n++)
    {
      Parasite *parasite = list->parasites + n;
      gchar *name;

      if (!parasite->n_values)
	continue;

      bse_storage_break (storage);
      name = g_strdup_quoted (g_quark_to_string (parasite->quark));
      bse_storage_printf (storage, "(parasite %c \"%s\"",
			  parasite->type,
			  name);
      switch (parasite->type)
	{
	  guint i;

	case PARASITE_FLOAT:
	  bse_storage_printf (storage, " %u", parasite->n_values);
	  for (i = 0; i < parasite->n_values; i++)
	    {
	      gfloat *floats = parasite->data;

	      if ((i + 1) % 5 == 0)
		bse_storage_break (storage);
	      bse_storage_printf (storage, " %f", floats[i]);
	    }
	  break;
	default:
	  g_warning (G_GNUC_PRETTY_FUNCTION "(): unknown parasite type `%c' for \"%s\" in \"%s\"",
		     parasite->type,
		     name,
		     BSE_OBJECT_NAME (object));
	  break;
	}
      g_free (name);
      bse_storage_putc (storage, ')');
    }
}

static void
parasite_list_free (gpointer data)
{
  ParasiteList *list = data;
  guint i;

  for (i = 0; i < list->n_parasites; i++)
    if (list->parasites[i].n_values)
      g_free (list->parasites[i].data);
  g_free (list);
}

static Parasite*
fetch_parasite (BseObject *object,
		GQuark     quark,
		gchar      type,
		gboolean   create)
{
  ParasiteList *list;
  guint i;

  list = bse_object_get_qdata (object, quark_parasite_list);
  
  if (list)
    for (i = 0; i < list->n_parasites; i++)
      if (list->parasites[i].quark == quark &&
	  list->parasites[i].type == type)
	return list->parasites + i;

  if (create)
    {
      ParasiteList *olist = list;
      
      i = list ? list->n_parasites : 0;
      list = g_realloc (list, sizeof (ParasiteList) + i * sizeof (Parasite));
      list->n_parasites = i + 1;
      if (list != olist)
	{
	  if (!quark_parasite_list)
	    quark_parasite_list = g_quark_from_static_string ("BseParasiteList");

	  if (olist)
	    bse_object_steal_qdata (object, quark_parasite_list);
	  else
	    bse_object_add_notifier (object,
				     "store",
				     store_parasite_list,
				     NULL);
	  bse_object_set_qdata_full (object, quark_parasite_list, list, parasite_list_free);
	}

      list->parasites[i].quark = quark;
      list->parasites[i].type = type;
      list->parasites[i].n_values = 0;
      list->parasites[i].data = NULL;

      return list->parasites + i;
    }

  return NULL;
}

static void
delete_parasite (BseObject *object,
		 GQuark     quark,
		 gchar      type)
{
  ParasiteList *list;
  Parasite *parasite = NULL;
  guint i;
  
  list = bse_object_get_qdata (object, quark_parasite_list);
  if (!list)
    return;

  for (i = 0; i < list->n_parasites; i++)
    if (list->parasites[i].quark == quark &&
	list->parasites[i].type == type)
      parasite = list->parasites + i;
  if (!parasite)
    return;
  
  if (parasite->n_values)
    g_free (parasite->data);
  list->n_parasites -= 1;
  if (i < list->n_parasites)
    list->parasites[i] = list->parasites[list->n_parasites];
  else if (list->n_parasites == 0)
    {
      bse_object_remove_notifiers_by_func (object, store_parasite_list, NULL);
      bse_object_set_qdata (object, quark_parasite_list, NULL);
    }
}

static GTokenType
parasite_parser (BseObject  *object,
		 BseStorage *storage,
		 gpointer    user_data)
{
  GScanner *scanner = storage->scanner;
  gboolean char_2_token;
  GQuark quark;
  guint type;
  guint n_values;
  gpointer data;

  /* check identifier */
  if (scanner->token != G_TOKEN_IDENTIFIER ||
      !bse_string_equals ("parasite", scanner->value.v_identifier))
    return G_TOKEN_IDENTIFIER;

  /* parse parasite type */
  char_2_token = scanner->config->char_2_token;
  scanner->config->char_2_token = FALSE;
  g_scanner_get_next_token (scanner);
  scanner->config->char_2_token = char_2_token;
  if (scanner->token != G_TOKEN_CHAR)
    return G_TOKEN_CHAR;
  type = scanner->value.v_char;

  /* parse parasite name */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    return G_TOKEN_STRING;
  quark = g_quark_from_string (scanner->value.v_string);

  switch (type)
    {
      guint i;
      gfloat *floats;

    case PARASITE_FLOAT:
      if (g_scanner_get_next_token (scanner) != G_TOKEN_INT)
	return G_TOKEN_INT;
      n_values = scanner->value.v_int;
      if (n_values >= MAX_PARASITE_VALUES)
	return G_TOKEN_INT;
      floats = g_new (gfloat, n_values);
      for (i = 0; i < n_values; i++)
	{
	  gboolean negate = FALSE;

	  if (g_scanner_get_next_token (scanner) == '-')
	    {
	      g_scanner_get_next_token (scanner);
	      negate = TRUE;
	    }
	  if (scanner->token != G_TOKEN_FLOAT)
	    {
	      g_free (floats);
	      return G_TOKEN_FLOAT;
	    }
	  floats[i] = negate ? - scanner->value.v_float : scanner->value.v_float;
	}
      data = floats;
      break;
    default:
      /* unmatched parasite type */
      return bse_storage_warn_skip (storage,
				    "invalid parasite type specification `%c' for \"%s\"",
				    type,
				    g_quark_to_string (quark));
    }

  if (g_scanner_peek_next_token (scanner) == ')')
    {
      Parasite *parasite = fetch_parasite (object, quark, type, TRUE);

      if (parasite->n_values)
	g_free (parasite->data);
      parasite->n_values = n_values;
      parasite->data = data;
    }
  else if (n_values)
    g_free (data);

  /* read closing brace */
  return g_scanner_get_next_token (scanner) == ')' ? G_TOKEN_NONE : ')';
}

void
bse_parasite_install_parsers (BseObjectClass *oclass)
{
  g_return_if_fail (BSE_IS_OBJECT_CLASS (oclass));

  bse_object_class_add_parser (oclass, "parasite", parasite_parser, NULL);
}

void
bse_parasite_set_floats (BseObject   *object,
			 const gchar *name,
			 guint        n_values,
			 gfloat      *float_values)
{
  g_return_if_fail (BSE_IS_OBJECT (object));
  g_return_if_fail (name != NULL);
  g_return_if_fail (n_values < MAX_PARASITE_VALUES);
  if (n_values)
    g_return_if_fail (float_values != NULL);

  if (!n_values)
    delete_parasite (object, g_quark_try_string (name), PARASITE_FLOAT);
  else
    {
      Parasite *parasite = fetch_parasite (object,
					   g_quark_from_string (name),
					   PARASITE_FLOAT,
					   TRUE);

      if (parasite->n_values != n_values)
	{
	  if (parasite->n_values)
	    g_free (parasite->data);
	  parasite->data = g_new (gfloat, n_values);
	  parasite->n_values = n_values;
	}
      memcpy (parasite->data, float_values, n_values * sizeof (gfloat));
    }
}

guint
bse_parasite_get_floats (BseObject   *object,
			 const gchar *name,
			 guint        max_n_values,
			 gfloat      *float_values)
{
  Parasite *parasite;
  guint i, n;

  g_return_val_if_fail (BSE_IS_OBJECT (object), 0);
  g_return_val_if_fail (name != NULL, 0);
  if (max_n_values)
    g_return_val_if_fail (float_values != NULL, 0);

  parasite = fetch_parasite (object,
			     g_quark_try_string (name),
			     PARASITE_FLOAT,
			     FALSE);
  n = parasite ? parasite->n_values : 0;
  i = MIN (n, max_n_values);
  if (i)
    memcpy (float_values, parasite->data, i * sizeof (gfloat));
  max_n_values -= i;
  float_values += i;
  while (max_n_values--)
    *(float_values++) = 0.0;

  return n;
}
