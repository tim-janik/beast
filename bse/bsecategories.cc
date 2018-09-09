// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsecategories.hh"
#include "bseutils.hh"
#include "bseserver.hh"
#include "private.hh"
#include <algorithm>
#include <string.h>


/* --- defines --- */
#define CATEGORIES_PRE_ALLOC  (16)


// == CategoryEntry ==
static bool categories_need_sorting = false;
static uint category_id_counter = 1;

// == functions ==
static inline std::vector<Bse::Category>&
category_entries()
{
  static std::vector<Bse::Category> global_entries;
  return global_entries;
}

static inline const Bse::Category*
centry_find (const std::string &category)
{
  for (const auto &centry : category_entries())
    if (centry.category == category)
      return &centry;
  return NULL;
}

static inline uint
category_strip_toplevels (const std::string &category, GType type)
{
  const uint l = category.size();

  if (l > 8 && strncmp (category.c_str(), "/Modules/", 9) == 0)
    {
      if (!G_TYPE_IS_OBJECT (type))
        return 0;
      return 9;
    }

  return 0;
}

static uint
leaf_index (const std::string &string)
{
  bool in_quote = false;
  uint pos = 0;
  for (const char *p = string.c_str(); *p; p++)
    switch (*p)
      {
      case '\\':	in_quote = true;			break;
      case '/':		pos = in_quote ? pos : p - string.c_str();	// fall through
      default:		in_quote = false;
      }
  return pos;
}

static inline Bse::Category*
centry_new (const char *caller, const std::string &category, GType type)
{
  const uint mindex = category_strip_toplevels (category, type);
  if (!mindex)
    {
      Bse::warning ("%s: refusing to add non-conforming category '%s'", caller, category);
      return NULL;
    }
  if (centry_find (category))
    {
      Bse::warning ("%s: unable to add category duplicate '%s'", caller, category);
      return NULL;
    }

  categories_need_sorting = TRUE;
  Bse::Category centry;
  centry.category_id = category_id_counter++;
  centry.mindex = mindex - 1;
  centry.lindex = leaf_index (category);
  centry.category = category;
  category_entries().push_back (centry);
  return &category_entries().back();
}

void
bse_categories_register (const std::string &category, const char *i18n_category, GType type, const uint8 *pixstream)
{
  assert_return (!category.empty());
  Bse::Category *centry = centry_new (__func__, category, type);
  if (centry)
    {
      centry->otype = g_type_name (type);
      if (pixstream)
        centry->icon = bse_icon_from_pixstream (pixstream);
    }
  if (g_type_is_a (type, BSE_TYPE_SOURCE))
    {
      // parse "/Modules////tag1/tag2/tag3///Title" into tags and title
      const char *name = i18n_category ? i18n_category : category.c_str();
      if (strncmp (name, "/Modules/", 9) == 0)
        name += 9;
      while (name[0] == '/')
        ++name;
      const char *title = strrchr (name, '/'), *end = title ? title : name;
      title = title ? title + 1 : name;
      while (end > name && end[-1] == '/')
        --end;
      Bse::StringVector tags;
      if (name < end)
        tags = Bse::string_split (std::string (name, end - name), "/");
      Bse::ServerImpl::register_source_module (centry->otype, title,
                                               Bse::string_join (";", tags),
                                               pixstream);
    }
}

void
bse_categories_register_stock_module (const gchar      *untranslated_category_trunk,
                                      GType             type,
                                      const guint8     *pixstream)
{
  assert_return (untranslated_category_trunk != NULL);
  const gchar *category = sfi_category_concat ("/Modules", untranslated_category_trunk);
  const gchar *i18n_category = sfi_category_concat ("/Modules", _(untranslated_category_trunk));
  bse_categories_register (category, i18n_category, type, pixstream);
}

static void
cats_sort (void)
{
  return_unless (categories_need_sorting);
  std::vector<Bse::Category> &entries = category_entries();
  auto lesser_category = [] (const Bse::Category &a, const Bse::Category &b) -> bool {
    return a.category < b.category;
  };
  std::sort (entries.begin(), entries.end(), lesser_category);
  categories_need_sorting = false;
}

static inline Bse::CategorySeq
categories_match (const std::string &pattern, GType base_type, BseCategoryCheck check, gpointer data)
{
  Bse::CategorySeq cseq;
  GPatternSpec *pspec = g_pattern_spec_new (pattern.c_str());
  for (const auto &centry : category_entries())
    {
      if (g_pattern_match_string (pspec, centry.category.c_str()) &&
	  (!base_type || g_type_is_a (g_type_from_name (centry.otype.c_str()), base_type)))
	{
          if (!check || check (&centry, data))
            cseq.push_back (centry);
	}
    }
  g_pattern_spec_free (pspec);
  return cseq;
}

Bse::CategorySeq
bse_categories_match (const std::string &pattern, GType base_type, BseCategoryCheck check, void *data)
{
  cats_sort ();
  return categories_match (pattern, 0, check, data);
}

Bse::CategorySeq
bse_categories_match_typed (const std::string &pattern, GType base_type)
{
  cats_sort ();
  return categories_match (pattern, base_type, NULL, NULL);
}

Bse::CategorySeq
bse_categories_from_type (GType type)
{
  Bse::CategorySeq cseq;
  const char *type_name = g_type_name (type);
  for (const auto &centry : category_entries())
    if (centry.otype == type_name)
      cseq.push_back (centry);
  return cseq;
}
