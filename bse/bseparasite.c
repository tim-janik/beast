/* BSE - Better Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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
#include "bseparasite.h"
#include "bseundostack.h"
#include "bsestorage.h"
#include <sfi/gbsearcharray.h>
#include <string.h>


typedef struct {
  const gchar *path;
  SfiRec      *rec;
} Node;
typedef struct {
  BseItem *link;
  GSList  *paths; /* contains ref-counted const gchar* */
} CRef;
struct _BseParasite {
  gpointer nodes;
  gpointer crefs;
};


/* --- prototypes --- */
static gint parasite_node_cmp  (gconstpointer  bsn1,
                                gconstpointer  bsn2);
static gint parasite_cref_cmp  (gconstpointer  bsn1,
                                gconstpointer  bsn2);
static void parasite_ref_rec   (BseItem       *item,
                                const gchar   *path,
                                SfiRec        *rec);
static void parasite_unref_rec (BseItem       *item,
                                const gchar   *path,
                                SfiRec        *rec);
static void parasite_ref_seq   (BseItem       *item,
                                const gchar   *path,
                                SfiSeq        *seq);
static void parasite_unref_seq (BseItem       *item,
                                const gchar   *path,
                                SfiSeq        *seq);


/* --- variables --- */
static guint    signal_parasites_added = 0;
static guint    signal_parasite_changed = 0;
static const GBSearchConfig bconfig_nodes = {
  sizeof (Node),
  parasite_node_cmp,
  G_BSEARCH_ARRAY_AUTO_SHRINK
};
static const GBSearchConfig bconfig_crefs = {
  sizeof (CRef),
  parasite_cref_cmp,
  G_BSEARCH_ARRAY_AUTO_SHRINK
};


/* --- functions --- */
void
bse_item_class_add_parasite_signals (BseItemClass *class)
{
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  signal_parasites_added = bse_object_class_add_dsignal (object_class, "parasites-added",
                                                         G_TYPE_NONE, 1, G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE);
  signal_parasite_changed = bse_object_class_add_dsignal (object_class, "parasite-changed",
                                                          G_TYPE_NONE, 1, G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static gint
parasite_node_cmp (gconstpointer  bsn1,
                   gconstpointer  bsn2)
{
  const Node *n1 = bsn1;
  const Node *n2 = bsn2;
  return strcmp (n1->path, n2->path);
}

static void
parasite_init (BseItem *item)
{
  g_assert (item->parasite == NULL);
  item->parasite = g_new0 (BseParasite, 1);
  item->parasite->nodes = g_bsearch_array_create (&bconfig_nodes);
  item->parasite->crefs = g_bsearch_array_create (&bconfig_crefs);
}

static gint
parasite_cref_cmp (gconstpointer  bsn1,
                   gconstpointer  bsn2)
{
  const CRef *r1 = bsn1;
  const CRef *r2 = bsn2;
  return G_BSEARCH_ARRAY_CMP (r1->link, r2->link);
}

static void
parasite_uncross_object (BseItem *item,
                         BseItem *link)
{
  CRef *cref, key = { 0, };
  key.link = link;
  cref = g_bsearch_array_lookup (item->parasite->crefs, &bconfig_crefs, &key);
  g_return_if_fail (cref != NULL);
  while (cref->paths)
    {
      const gchar *path = cref->paths->data;
      bse_item_set_parasite (item, path, NULL);
      cref = g_bsearch_array_lookup (item->parasite->crefs, &bconfig_crefs, &key);
    }
}

static void
parasite_ref_object (BseItem     *item,
                     const gchar *path,
                     BseItem     *link)
{
  CRef *cref, key = { 0, };
  key.link = link;
  cref = g_bsearch_array_lookup (item->parasite->crefs, &bconfig_crefs, &key);
  if (!cref)
    {
      item->parasite->crefs = g_bsearch_array_insert (item->parasite->crefs, &bconfig_crefs, &key);
      cref = g_bsearch_array_lookup (item->parasite->crefs, &bconfig_crefs, &key);
      bse_item_cross_link (item, link, parasite_uncross_object);
    }
  cref->paths = g_slist_prepend (cref->paths, (gchar*) g_intern_string (path));
}

static void
parasite_unref_object (BseItem     *item,
                       const gchar *path,
                       BseItem     *link)
{
  CRef *cref, key = {0, };
  GSList *plink;
  key.link = link;
  cref = g_bsearch_array_lookup (item->parasite->crefs, &bconfig_crefs, &key);
  g_return_if_fail (cref != NULL);
  plink = g_slist_find (cref->paths, path);
  g_return_if_fail (plink != NULL);
  cref->paths = g_slist_remove_link (cref->paths, plink);
  if (!cref->paths)
    {
      item->parasite->crefs = g_bsearch_array_remove_node (item->parasite->crefs, &bconfig_crefs, cref);
      bse_item_cross_unlink (item, link, parasite_uncross_object);
    }
}

static inline void
parasite_ref_value (BseItem      *item,
                    const gchar  *path,
                    const GValue *value)
{
  if (G_VALUE_HOLDS_OBJECT (value))
    {
      BseItem *link = g_value_get_object (value);
      if (link)
        parasite_ref_object (item, path, link);
    }
  else if (SFI_VALUE_HOLDS_REC (value))
    {
      SfiRec *crec = sfi_value_get_rec (value);
      if (crec)
        parasite_ref_rec (item, path, crec);
    }
  else if (SFI_VALUE_HOLDS_SEQ (value))
    {
      SfiSeq *cseq = sfi_value_get_seq (value);
      if (cseq)
        parasite_ref_seq (item, path, cseq);
    }
}

static inline void
parasite_unref_value (BseItem      *item,
                      const gchar  *path,
                      const GValue *value)
{
  if (G_VALUE_HOLDS_OBJECT (value))
    {
      BseItem *link = g_value_get_object (value);
      if (link)
        parasite_unref_object (item, path, link);
    }
  else if (SFI_VALUE_HOLDS_REC (value))
    {
      SfiRec *crec = sfi_value_get_rec (value);
      if (crec)
        parasite_unref_rec (item, path, crec);
    }
  else if (SFI_VALUE_HOLDS_SEQ (value))
    {
      SfiSeq *cseq = sfi_value_get_seq (value);
      if (cseq)
        parasite_unref_seq (item, path, cseq);
    }
}

static void
parasite_ref_seq (BseItem     *item,
                  const gchar *path,
                  SfiSeq      *seq)
{
  guint i;
  for (i = 0; i < seq->n_elements; i++)
    parasite_ref_value (item, path, seq->elements + i);
}

static void
parasite_unref_seq (BseItem     *item,
                    const gchar *path,
                    SfiSeq      *seq)
{
  guint i;
  for (i = 0; i < seq->n_elements; i++)
    parasite_unref_value (item, path, seq->elements + i);
}

static void
parasite_ref_rec (BseItem     *item,
                  const gchar *path,
                  SfiRec      *rec)
{
  guint i;
  for (i = 0; i < rec->n_fields; i++)
    parasite_ref_value (item, path, rec->fields + i);
}

static void
parasite_unref_rec (BseItem     *item,
                    const gchar *path,
                    SfiRec      *rec)
{
  guint i;
  for (i = 0; i < rec->n_fields; i++)
    parasite_unref_value (item, path, rec->fields + i);
}

void
bse_item_set_parasite (BseItem        *item,
                       const gchar    *parasite_path,
                       SfiRec         *rec)
{
  SfiRec *delrec;
  gboolean notify_add = FALSE;
  Node *node, key = { 0, };
  /* guard against no-ops, catch wrong paths */
  if (!parasite_path || parasite_path[0] != '/' ||
      (!item->parasite && !rec))
    return;
  /* ensure parasite intiialization */
  if (!item->parasite)
    parasite_init (item);
  /* ensure node */
  key.path = parasite_path;
  node = g_bsearch_array_lookup (item->parasite->nodes, &bconfig_nodes, &key);
  if (!node)
    {
      /* catch no-op deletion */
      if (!rec)
        return;
      key.path = g_intern_string (parasite_path);
      item->parasite->nodes = g_bsearch_array_insert (item->parasite->nodes, &bconfig_nodes, &key);
      node = g_bsearch_array_lookup (item->parasite->nodes, &bconfig_nodes, &key);
      notify_add = TRUE;
    }
  /* queue undo */
  bse_item_backup_parasite (item, node->path, node->rec);
  /* queue record deletion */
  delrec = node->rec;
  /* copy/setup new record */
  if (rec)
    {
      node->rec = sfi_rec_ref (rec); /* copy-shallow */
      parasite_ref_rec (item, node->path, node->rec);
    }
  else
    {
      /* remove node */
      item->parasite->nodes = g_bsearch_array_remove_node (item->parasite->nodes, &bconfig_nodes, node);
    }
  /* cleanups */
  if (delrec)
    parasite_unref_rec (item, parasite_path, delrec);
  if (((GObject*) item)->ref_count && notify_add)
    {
      const gchar *slash = strrchr (parasite_path, '/');
      gchar *parent_path = g_strndup (parasite_path, slash - parasite_path + 1);
      GQuark parent_quark = g_quark_from_string (parent_path);
      g_free (parent_path);
      g_signal_emit (item, signal_parasites_added, parent_quark, g_quark_to_string (parent_quark));
    }
  if (((GObject*) item)->ref_count)
    g_signal_emit (item, signal_parasite_changed, g_quark_from_string (parasite_path), parasite_path);
}

SfiRec*
bse_item_get_parasite (BseItem        *item,
                       const gchar    *parasite_path)
{
  if (parasite_path && parasite_path[0] == '/' && item->parasite)
    {
      Node *node, key = { 0, };
      key.path = parasite_path;
      node = g_bsearch_array_lookup (item->parasite->nodes, &bconfig_nodes, &key);
      if (node)
        return node->rec;
    }
  return NULL;
}

static void
undo_set_parasite (BseUndoStep  *ustep,
                   BseUndoStack *ustack)
{
  BseItem *item = bse_undo_pointer_unpack (ustep->data[0].v_pointer, ustack);
  const gchar *path = ustep->data[1].v_pointer;
  SfiRec *rec = ustep->data[2].v_pointer;
  bse_item_set_parasite (item, path, rec);
}

static void
unde_free_parasite (BseUndoStep *ustep)
{
  SfiRec *rec = ustep->data[2].v_pointer;
  g_free (ustep->data[0].v_pointer);
  if (rec)
    sfi_rec_unref (rec);
}

void
bse_item_backup_parasite (BseItem        *item,
                          const gchar    *parasite_path,
                          SfiRec         *rec)
{
  BseUndoStack *ustack;
  BseUndoStep *ustep;
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (parasite_path && parasite_path[0] == '/');
  ustack = bse_item_undo_open (item, "set-parasite");
  ustep = bse_undo_step_new (undo_set_parasite, unde_free_parasite, 3);
  ustep->data[0].v_pointer = bse_undo_pointer_pack (item, ustack);
  ustep->data[1].v_pointer = (gchar*) g_intern_string (parasite_path);
  ustep->data[2].v_pointer = rec ? sfi_rec_ref (rec) : NULL;
  bse_undo_stack_push (ustack, ustep);
  bse_item_undo_close (ustack);
}

void
bse_item_delete_parasites (BseItem *item)
{
  if (item->parasite)
    {
      while (g_bsearch_array_get_n_nodes (item->parasite->nodes))
        {
          Node *node = g_bsearch_array_get_nth (item->parasite->nodes, &bconfig_nodes,
                                                g_bsearch_array_get_n_nodes (item->parasite->nodes) - 1);
          bse_item_set_parasite (item, node->path, NULL);
        }
      g_assert (g_bsearch_array_get_n_nodes (item->parasite->crefs) == 0);
      g_bsearch_array_free (item->parasite->nodes, &bconfig_nodes);
      g_bsearch_array_free (item->parasite->crefs, &bconfig_crefs);
      g_free (item->parasite);
      item->parasite = NULL;
    }
}

SfiRing*
bse_item_list_parasites (BseItem     *item,
                         const gchar *parent_path)
{
  SfiRing *ring = NULL;
  if (item->parasite && parent_path)
    {
      guint i, l = strlen (parent_path);
      if (!l || parent_path[0] != '/' || parent_path[l-1] != '/')
        return NULL;
      for (i = 0; i < g_bsearch_array_get_n_nodes (item->parasite->nodes); i++)
        {
          Node *node = g_bsearch_array_get_nth (item->parasite->nodes, &bconfig_nodes, i);
          if (strncmp (parent_path, node->path, l) == 0)
            {
              const gchar *slash = strchr (node->path + l, '/');
              if (!slash)       /* append all children */
                ring = sfi_ring_append_uniq (ring, (gchar*) g_intern_string (node->path));
              else
                {               /* append immediate child directories */
                  gchar *dir = g_strndup (node->path, slash - node->path + 1);
                  ring = sfi_ring_append_uniq (ring, (gchar*) g_intern_string (dir));
                  g_free (dir);
                }
            }
        }
    }
  return ring;
}

const gchar*
bse_item_create_parasite_name (BseItem        *item,
                               const gchar    *path_prefix)
{
  if (path_prefix && path_prefix[0] == '/')
    {
      Node key = { 0, };
      guint counter = 1;
      gchar *path = g_strdup_printf ("%sAuto-%02x", path_prefix, counter++);
      /* ensure parasite intiialization */
      if (!item->parasite)
        parasite_init (item);
      /* search for unused name */
      key.path = path;
      while (g_bsearch_array_lookup (item->parasite->nodes, &bconfig_nodes, &key))
        {
          g_free (path);
          path = g_strdup_printf ("%sAuto-%02x", path_prefix, counter++);
          key.path = path;
        }
      key.path = g_intern_string (path);
      g_free (path);
      return key.path;
    }
  return NULL;
}


/* --- old parasites --- */
#define MAX_PARASITE_VALUES (1024) /* (2 << 24) */
#define parse_or_return         bse_storage_scanner_parse_or_return
#define peek_or_return          bse_storage_scanner_peek_or_return


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
void
bse_parasite_store (BseObject  *object,
		    BseStorage *storage)
{
  ParasiteList *list;
  guint n;
  
  list = g_object_get_qdata (object, quark_parasite_list);
  if (!list)
    return;
  
  for (n = 0; n < list->n_parasites; n++)
    {
      Parasite *parasite = list->parasites + n;
      gchar *name;
      
      if (!parasite->n_values)
	continue;
      
      bse_storage_break (storage);
      name = g_strescape (g_quark_to_string (parasite->quark), NULL);
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
	      bse_storage_putc (storage, ' ');
	      bse_storage_putf (storage, floats[i]);
	    }
	  break;
	default:
	  g_warning (G_STRLOC ": unknown parasite type `%c' for \"%s\" in \"%s\"",
		     parasite->type,
		     name,
		     BSE_OBJECT_UNAME (object));
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
  
  list = g_object_get_qdata (object, quark_parasite_list);
  
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
	    g_object_steal_qdata (object, quark_parasite_list);
	  g_object_set_qdata_full (object, quark_parasite_list, list, parasite_list_free);
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
  
  list = g_object_get_qdata (object, quark_parasite_list);
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
    g_object_set_qdata (object, quark_parasite_list, NULL);
}

GTokenType
bse_parasite_restore (BseObject  *object,
		      BseStorage *storage)
{
  GScanner *scanner = bse_storage_get_scanner (storage);
  GQuark quark;
  GTokenType ttype;
  guint n_values;
  gpointer data;
  
  /* check identifier */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER ||
      !bse_string_equals ("parasite", scanner->next_value.v_identifier))
    return SFI_TOKEN_UNMATCHED;

  /* eat "parasite" identifier */
  g_scanner_get_next_token (scanner);

  /* parse parasite type */
  g_scanner_get_next_token (scanner);
  if (!(scanner->token >= 'a' && scanner->token <= 'z'))
    return G_TOKEN_CHAR;
  ttype = scanner->token;

  /* parse parasite name */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    return G_TOKEN_STRING;
  quark = g_quark_from_string (scanner->value.v_string);
  
  switch (ttype)
    {
      guint i;
      gfloat *floats;
      
    case PARASITE_FLOAT:
      if (g_scanner_get_next_token (scanner) != G_TOKEN_INT)
	return G_TOKEN_INT;
      n_values = scanner->value.v_int64;
      if (n_values >= MAX_PARASITE_VALUES)
	return G_TOKEN_INT;
      floats = g_new (gfloat, n_values);
      for (i = 0; i < n_values; i++)
	{
	  gboolean negate = FALSE;
	  gfloat vfloat;

	  if (g_scanner_get_next_token (scanner) == '-')
	    {
	      g_scanner_get_next_token (scanner);
	      negate = TRUE;
	    }
	  if (scanner->token == G_TOKEN_INT)
	    vfloat = scanner->value.v_int64;
	  else if (scanner->token == G_TOKEN_FLOAT)
	    vfloat = scanner->value.v_float;
	  else
	    {
	      g_free (floats);
	      return G_TOKEN_FLOAT;
	    }
	  floats[i] = negate ? - vfloat : vfloat;
	}
      data = floats;
      break;
    default:
      /* unmatched parasite type */
      return bse_storage_warn_skip (storage,
				    "invalid parasite type specification `%c' for \"%s\"",
				    ttype,
				    g_quark_to_string (quark));
    }
  
  if (g_scanner_peek_next_token (scanner) == ')')
    {
      Parasite *parasite = fetch_parasite (object, quark, ttype, TRUE);
      
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

SfiFBlock*
bse_parasite_get_floats (BseObject   *object,
			 const gchar *name)
{
  Parasite *parasite;
  SfiFBlock *fblock;
  
  g_return_val_if_fail (BSE_IS_OBJECT (object), 0);
  g_return_val_if_fail (name != NULL, 0);
  
  parasite = fetch_parasite (object,
			     g_quark_try_string (name),
			     PARASITE_FLOAT,
			     FALSE);
  fblock = sfi_fblock_new ();
  if (parasite)
    sfi_fblock_append (fblock, parasite->n_values, parasite->data);
  return fblock;
}
