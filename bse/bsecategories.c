/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-1999, 2000-2003 Tim Janik
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
#include "bsecategories.h"

#include "bseutils.h"
#include <string.h>


/* --- defines --- */
#define CATEGORIES_PRE_ALLOC  (16)


/* --- structures --- */
typedef struct _CEntry CEntry;
struct _CEntry
{
  CEntry  *next;
  guint    category_id;
  GQuark   category;
  guint    mindex, lindex;
  GType    type;
  BseIcon *icon;
};


/* --- variables --- */
static CEntry    *cat_entries = NULL;
static gboolean   cats_need_sort = FALSE;
static guint      global_category_id = 1;
static SfiUStore *category_ustore = NULL;
static BseIcon   *dummy_icon = NULL;


/* --- functions --- */
void
_bse_init_categories (void)
{
  g_return_if_fail (category_ustore == NULL);

  category_ustore = sfi_ustore_new ();
  dummy_icon = bse_icon_new ();
}

static inline CEntry*
centry_find (GQuark quark)
{
  CEntry *centry;
  
  for (centry = cat_entries; centry; centry = centry->next)
    if (centry->category == quark)
      return centry;
  
  return NULL;
}

static inline guint
category_strip_toplevels (const gchar *category,
                          GType        type)
{
  static const struct { guint length; const gchar *prefix; } scripts[] = {
    {  9, "/Project/", },
    {  6, "/SNet/", },
    {  6, "/Song/", },
    {  6, "/Part/", },
    {  8, "/CSynth/", },
    {  6, "/Wave/", },
    { 10, "/WaveRepo/", },
    {  6, "/Proc/", },
  };
  guint l = strlen (category);
  
  if (l > 10 && strncmp (category, "/Methods/", 8) == 0)
    {
      const gchar *p = category + 8;

      if (!BSE_TYPE_IS_PROCEDURE (type))
        return 0;
      p = strchr (p, '/');
      if (p && p[1])
	return p - category + 1;
    }
  else if (l > 8 && strncmp (category, "/Modules/", 9) == 0)
    {
      if (!G_TYPE_IS_OBJECT (type))
        return 0;
      return 9;
    }

  if (BSE_TYPE_IS_PROCEDURE (type))
    {
      guint i;
      for (i = 0; i < G_N_ELEMENTS (scripts); i++)
        if (l > scripts[i].length &&
            strncmp (category, scripts[i].prefix, scripts[i].length) == 0)
          return scripts[i].length;
    }
  
  return 0;
}

static guint
leaf_index (const gchar *string)
{
  gboolean in_quote = FALSE;
  guint pos = 0;
  const gchar *p;
  
  for (p = string; *p; p++)
    switch (*p)
      {
      case '\\':	in_quote = TRUE;			break;
      case '/':		pos = in_quote ? pos : p - string;	/* fall through */
      default:		in_quote = FALSE;
      }
  return pos;
}

static inline CEntry*
centry_new (const gchar *caller,
	    const gchar *category,
            GType        type)
{
  static GTrashStack *free_entries = NULL;
  CEntry *centry;
  GQuark quark;
  guint mindex;
  
  mindex = category_strip_toplevels (category, type);
  if (!mindex)
    {
      g_warning ("%s(): refusing to add non-conforming category `%s'", caller, category);
      return NULL;
    }
  quark = g_quark_try_string (category);
  if (quark && centry_find (quark))
    {
      g_warning ("%s(): unable to add category duplicate `%s'", caller, category);
      return NULL;
    }
  
  if (!g_trash_stack_peek (&free_entries))
    {
      CEntry *limit;
      
      centry = g_new (CEntry, CATEGORIES_PRE_ALLOC);
      limit = centry + CATEGORIES_PRE_ALLOC - 1;
      while (centry < limit)
	g_trash_stack_push (&free_entries, centry++);
    }
  else
    centry = g_trash_stack_pop (&free_entries);
  
  centry->next = cat_entries;
  cat_entries = centry;
  centry->category_id = global_category_id++;
  sfi_ustore_insert (category_ustore, centry->category_id, centry);
  centry->mindex = mindex - 1;
  centry->lindex = leaf_index (category);
  centry->category = g_quark_from_string (category);
  
  cats_need_sort = TRUE;
  
  return centry;
}

static void
check_type (GType type)
{
  if (BSE_TYPE_IS_PROCEDURE (type))
    {
      gchar *x = g_strcanon (g_strdup (g_type_name (type)),
			     G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "+",
			     '-');
      if (strcmp (x, g_type_name (type)) != 0)
	g_warning ("type name with invalid characters: %s", g_type_name (type));
      g_free (x);
    }
}

void
bse_categories_register (const gchar  *category,
			 GType         type,
                         const guint8 *pixstream)
{
  CEntry *centry;
  
  g_return_if_fail (category != NULL);
  
  centry = centry_new (G_GNUC_PRETTY_FUNCTION, category, type);
  check_type (type);
  if (centry)
    {
      centry->type = type;
      if (pixstream)
        centry->icon = bse_icon_from_pixstream (pixstream);
      else
        centry->icon = NULL;
    }
}

void
bse_categories_register_icon (const gchar      *category,
			      GType             type,
			      const BsePixdata *pixdata)
{
  CEntry *centry;
  
  g_return_if_fail (category != NULL);
  g_return_if_fail (pixdata != NULL);
  
  centry = centry_new (G_GNUC_PRETTY_FUNCTION, category, type);
  check_type (type);
  if (centry)
    {
      centry->type = type;
      if (pixdata->type && pixdata->width && pixdata->height && pixdata->encoded_pix_data)
	{
	  centry->icon = bse_icon_from_pixdata (pixdata); /* static reference */
	  // bsw_icon_ref_static (centry->icon);
	}
      else
	centry->icon = NULL;
    }
}

static gint
centries_strorder (gconstpointer a,
		   gconstpointer b)
{
  const CEntry *e1 = a;
  const CEntry *e2 = b;
  gchar *c1 = g_quark_to_string (e1->category);
  gchar *c2 = g_quark_to_string (e2->category);
  
  return strcmp (c2, c1);
}

static void
cats_sort (void)
{
  GSList *slist, *clist = NULL;
  CEntry *centry, *last;
  
  if (!cats_need_sort)
    return;
  
  for (centry = cat_entries; centry; centry = centry->next)
    clist = g_slist_prepend (clist, centry);
  clist = g_slist_sort (clist, centries_strorder);
  last = NULL;
  for (slist = clist; slist; slist = slist->next)
    {
      centry = slist->data;
      centry->next = last;
      last = centry;
    }
  cat_entries = centry;
  g_slist_free (clist);
  
  cats_need_sort = FALSE;
}

static inline BseCategorySeq*
categories_match (const gchar      *pattern,
		  GType             base_type,
                  BseCategoryCheck  check,
                  gpointer          data)
{
  BseCategorySeq *cseq = bse_category_seq_new ();
  GPatternSpec *pspec = g_pattern_spec_new (pattern);
  CEntry *centry;
  
  for (centry = cat_entries; centry; centry = centry->next)
    {
      gchar *category = g_quark_to_string (centry->category);
      
      if (g_pattern_match_string (pspec, category) &&
	  (!base_type || g_type_is_a (centry->type, base_type)))
	{
	  BseCategory cat = { 0, };
	  
	  cat.category = category;
	  cat.category_id = centry->category_id;
	  cat.mindex = centry->mindex;
	  cat.lindex = centry->lindex;
	  cat.type = g_type_name (centry->type);
	  cat.icon = centry->icon ? centry->icon : dummy_icon;
          if (!check || check (&cat, data))
            bse_category_seq_append (cseq, &cat);
	}
    }
  g_pattern_spec_free (pspec);
  
  return cseq;
}

BseCategorySeq*
bse_categories_match (const gchar      *pattern,
                      GType             base_type,
                      BseCategoryCheck  check,
                      gpointer          data)
{
  g_return_val_if_fail (pattern != NULL, NULL);
  
  cats_sort ();
  
  return categories_match (pattern, 0, check, data);
}

BseCategorySeq*
bse_categories_match_typed (const gchar *pattern,
			    GType        base_type)
{
  g_return_val_if_fail (pattern != NULL, NULL);
  
  cats_sort ();
  
  return categories_match (pattern, base_type, NULL, NULL);
}

BseCategorySeq*
bse_categories_from_type (GType type)
{
  BseCategorySeq *cseq = bse_category_seq_new ();
  CEntry *centry;
  
  for (centry = cat_entries; centry; centry = centry->next)
    if (centry->type == type)
      {
	BseCategory cat = { 0, };
	
	cat.category = g_quark_to_string (centry->category);
	cat.category_id = centry->category_id;
	cat.mindex = centry->mindex;
	cat.lindex = centry->lindex;
	cat.type = g_type_name (centry->type);
	cat.icon = centry->icon ? centry->icon : dummy_icon;
	bse_category_seq_append (cseq, &cat);
      }
  return cseq;
}

BseCategory*
bse_category_from_id (guint id)
{
  CEntry *centry;

  g_return_val_if_fail (id > 0, NULL);

  centry = sfi_ustore_lookup (category_ustore, id);
  if (centry)
    {
      BseCategory *cat = bse_category_new ();
      g_free (cat->category);
      cat->category = g_strdup (g_quark_to_string (centry->category));
      cat->category_id = centry->category_id;
      cat->mindex = centry->mindex;
      cat->lindex = centry->lindex;
      g_free (cat->type);
      cat->type = g_strdup (g_type_name (centry->type));
      if (cat->icon)
        bse_icon_free (cat->icon);
      cat->icon = centry->icon ? bse_icon_copy_shallow (centry->icon) : bse_icon_new ();
      return cat;
    }
  return NULL;
}
