// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_CATEGORIES_H__
#define __BSE_CATEGORIES_H__
#include        <bse/bsetype.hh>
G_BEGIN_DECLS
/* --- typedefs --- */
typedef gboolean (BseCategoryCheck) (BseCategory *category,
                                     gpointer     data);
/* --- prototypes --- */
void            bse_categories_register        (const gchar      *category,
                                                const gchar      *i18n_category,
                                                GType             type,
                                                const guint8     *pixstream);
BseCategorySeq* bse_categories_match           (const gchar      *pattern,
                                                GType             base_type,
                                                BseCategoryCheck  check,
                                                gpointer          data);
BseCategorySeq* bse_categories_match_typed     (const gchar      *pattern,
                                                GType             base_type);
BseCategorySeq* bse_categories_from_type       (GType             type);
BseCategory*    bse_category_from_id           (guint             id);
void      bse_categories_register_stock_module (const gchar      *untranslated_category_trunk,
                                                GType             type,
                                                const guint8     *pixstream);
/* --- implementation --- */
void		_bse_init_categories	     (void);
G_END_DECLS
#endif /* __BSE_CATEGORIES_H__ */
