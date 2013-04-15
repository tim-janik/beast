// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsecategories.hh"
#include "bseutils.hh"
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
/* --- functions --- */
void
_bse_init_categories (void)
{
  g_return_if_fail (category_ustore == NULL);
  category_ustore = sfi_ustore_new ();
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
    centry = (CEntry*) g_trash_stack_pop (&free_entries);
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
                         const gchar  *i18n_category,
                         GType         type,
                         const guint8 *pixstream)
{
  CEntry *centry;
  g_return_if_fail (category != NULL);
  centry = centry_new (RAPICORN_SIMPLE_FUNCTION, category, type);
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
bse_categories_register_stock_module (const gchar      *untranslated_category_trunk,
                                      GType             type,
                                      const guint8     *pixstream)
{
  g_return_if_fail (untranslated_category_trunk != NULL);
  const gchar *category = sfi_category_concat ("/Modules", untranslated_category_trunk);
  const gchar *i18n_category = sfi_category_concat ("/Modules", _(untranslated_category_trunk));
  bse_categories_register (category, i18n_category, type, pixstream);
}
static gint
centries_strorder (gconstpointer a,
		   gconstpointer b)
{
  const CEntry *e1 = (const CEntry*) a;
  const CEntry *e2 = (const CEntry*) b;
  const char *c1 = g_quark_to_string (e1->category);
  const char *c2 = g_quark_to_string (e2->category);
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
      centry = (CEntry*) slist->data;
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
      const char *category = g_quark_to_string (centry->category);
      if (g_pattern_match_string (pspec, category) &&
	  (!base_type || g_type_is_a (centry->type, base_type)))
	{
	  BseCategory cat = { 0, };
	  cat.category = const_cast<char*> (category);
	  cat.category_id = centry->category_id;
	  cat.mindex = centry->mindex;
	  cat.lindex = centry->lindex;
	  cat.type = const_cast<char*>  (g_type_name (centry->type));
	  cat.icon = centry->icon ? centry->icon : NULL;
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
  for (CEntry *centry = cat_entries; centry; centry = centry->next)
    if (centry->type == type)
      {
	BseCategory cat = { 0, };
	cat.category = const_cast<char*> (g_quark_to_string (centry->category));
	cat.category_id = centry->category_id;
	cat.mindex = centry->mindex;
	cat.lindex = centry->lindex;
	cat.type = const_cast<char*> (g_type_name (centry->type));
	cat.icon = centry->icon ? centry->icon : NULL;
	bse_category_seq_append (cseq, &cat);
      }
  return cseq;
}
BseCategory*
bse_category_from_id (guint id)
{
  CEntry *centry;
  g_return_val_if_fail (id > 0, NULL);
  centry = (CEntry*) sfi_ustore_lookup (category_ustore, id);
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
      cat->icon = centry->icon ? bse_icon_copy_shallow (centry->icon) : NULL;
      return cat;
    }
  return NULL;
}
