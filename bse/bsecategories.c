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
  guint    mindex;
  BseType  type;
  BseIcon *icon;
};


/* --- variables --- */
static CEntry      *cat_entries = NULL;


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
  else if (l > 8 && strncmp (category, "/Source/", 8) == 0)
    return 8;
  else if (l > 6 && strncmp (category, "/Proc/", 6) == 0)
    return 6;

  return 0;
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
      g_warning ("%s(): unable to add non-conforming category `%s'", caller, category);
      return NULL;
    }
  quark = g_quark_try_string (category);
  if (quark && centry_find (quark))
    {
      g_warning ("%s(): unable to re-add existing category `%s'", caller, category);
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
  centry->category = g_quark_from_string (category);

  return centry;
}

void
bse_categories_register (const gchar *category,
			 BseType      type)
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
			      BseType           type,
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
	  bse_icon_static_ref (centry->icon);
	}
      else
	centry->icon = NULL;
    }
}

static inline BseCategory*
categories_match (const gchar *pattern,
		  BseType      base_type,
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
	  (!base_type || bse_type_conforms_to (centry->type, base_type)))
	{
	  guint i = n_cats;
	  
	  n_cats++;
	  cats = g_renew (BseCategory, cats, n_cats);
	  cats[i].category = category;
	  cats[i].mindex = centry->mindex;
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

  return categories_match (pattern, 0, n_matches);
}

BseCategory* /* free result */
bse_categories_match_typed (const gchar *pattern,
			    BseType      base_type,
			    guint       *n_matches)
{
  if (n_matches)
    *n_matches = 0;
  g_return_val_if_fail (pattern != NULL, NULL);
  g_return_val_if_fail (base_type > BSE_TYPE_NONE, NULL);

  return categories_match (pattern, base_type, n_matches);
}

BseCategory* /* free result */
bse_categories_from_type (BseType type,
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
	cats[i].type = centry->type;
	cats[i].icon = centry->icon;
      }

  if (n_categories)
    *n_categories = n_cats;

  return cats;
}
