/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-1999, 2000-2002 Tim Janik
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
  GQuark   category;
  guint    mindex, lindex;
  GType    type;
  BswIcon *icon;
};


/* --- variables --- */
static CEntry      *cat_entries = NULL;
static gboolean     cats_need_sort = FALSE;


/* --- functions --- */
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
category_strip_toplevels (const gchar *category)
{
  guint l = strlen (category);

  if (l > 10 && strncmp (category, "/Method/", 8) == 0)
    {
      const gchar *p = category + 8;

      p = strchr (p, '/');
      if (p && p[1])
	return p - category + 1;
    }
  else if (l > 8 && strncmp (category, "/Modules/", 9) == 0)
    return 9;
  else if (l > 8 && strncmp (category, "/Scripts/", 9) == 0)
    return 9;
  else if (l > 8 && strncmp (category, "/Effect/", 8) == 0)
    return 8;
  else if (l > 6 && strncmp (category, "/Proc/", 6) == 0)
    return 6;

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
	    const gchar *category)
{
  static GTrashStack *free_entries = NULL;
  CEntry *centry;
  GQuark quark;
  guint mindex;

  mindex = category_strip_toplevels (category);
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
  centry->mindex = mindex - 1;
  centry->lindex = leaf_index (category);
  centry->category = g_quark_from_string (category);

  cats_need_sort = TRUE;

  return centry;
}

void
bse_categories_register (const gchar *category,
			 GType        type)
{
  CEntry *centry;

  g_return_if_fail (category != NULL);

  centry = centry_new (G_GNUC_PRETTY_FUNCTION, category);
  if (centry)
    {
      centry->type = type;
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

  centry = centry_new (G_GNUC_PRETTY_FUNCTION, category);
  if (centry)
    {
      centry->type = type;
      if (pixdata->type && pixdata->width && pixdata->height && pixdata->encoded_pix_data)
	{
	  centry->icon = bse_icon_from_pixdata (pixdata); /* static reference */
	  bsw_icon_ref_static (centry->icon);
	}
      else
	centry->icon = NULL;
    }
}

static gint
centries_compare (gconstpointer a,
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
  clist = g_slist_sort (clist, centries_compare);
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

static inline BseCategory*
categories_match (const gchar *pattern,
		  GType        base_type,
		  guint       *n_matches)
{
  GPatternSpec *pspec;
  CEntry *centry;
  BseCategory *cats = NULL;
  guint n_cats = 0;

  pspec = g_pattern_spec_new (pattern);

  for (centry = cat_entries; centry; centry = centry->next)
    {
      gchar *category = g_quark_to_string (centry->category);

      if (g_pattern_match_string (pspec, category) &&
	  (!base_type || g_type_is_a (centry->type, base_type)))
	{
	  guint i = n_cats;
	  
	  n_cats++;
	  cats = g_renew (BseCategory, cats, n_cats);
	  cats[i].category = category;
	  cats[i].mindex = centry->mindex;
	  cats[i].lindex = centry->lindex;
	  cats[i].type = centry->type;
	  cats[i].icon = centry->icon;
	}
    }

  g_pattern_spec_free (pspec);

  if (n_matches)
    *n_matches = n_cats;

  return cats;
}

BseCategory* /* free result */
bse_categories_match (const gchar *pattern,
		      guint       *n_matches)
{
  if (n_matches)
    *n_matches = 0;
  g_return_val_if_fail (pattern != NULL, NULL);

  cats_sort ();

  return categories_match (pattern, 0, n_matches);
}

BseCategory* /* free result */
bse_categories_match_typed (const gchar *pattern,
			    GType        base_type,
			    guint       *n_matches)
{
  if (n_matches)
    *n_matches = 0;
  g_return_val_if_fail (pattern != NULL, NULL);
  g_return_val_if_fail (base_type > G_TYPE_NONE, NULL);

  cats_sort ();

  return categories_match (pattern, base_type, n_matches);
}

BseCategory* /* free result */
bse_categories_from_type (GType   type,
			  guint  *n_categories)
{
  CEntry *centry;
  BseCategory *cats = NULL;
  guint n_cats = 0;

  for (centry = cat_entries; centry; centry = centry->next)
    if (centry->type == type)
      {
	guint i = n_cats;
	
	n_cats++;
	cats = g_renew (BseCategory, cats, n_cats);
	cats[i].category = g_quark_to_string (centry->category);
	cats[i].mindex = centry->mindex;
	cats[i].lindex = centry->lindex;
	cats[i].type = centry->type;
	cats[i].icon = centry->icon;
      }

  if (n_categories)
    *n_categories = n_cats;

  return cats;
}
