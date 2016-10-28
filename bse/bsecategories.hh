// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_CATEGORIES_H__
#define __BSE_CATEGORIES_H__

#include <bse/bsetype.hh>
#include <bse/bseutils.hh>


/* --- typedefs --- */
typedef gboolean (BseCategoryCheck) (const Bse::Category *category, void *data);

/* --- prototypes --- */
void             bse_categories_register              (const String &category, const char *i18n_category, GType type, const uint8 *pixstream);
Bse::CategorySeq bse_categories_match                 (const String &pattern, GType base_type, BseCategoryCheck check, void *data);
Bse::CategorySeq bse_categories_match_typed           (const String &pattern, GType base_type);
Bse::CategorySeq bse_categories_from_type             (GType type);
void             bse_categories_register_stock_module (const char *untranslated_category_trunk, GType type, const guint8 *pixstream);

#endif /* __BSE_CATEGORIES_H__ */
