// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PARASITE_H__
#define __BSE_PARASITE_H__

#include <bse/bseitem.hh>

G_BEGIN_DECLS

/* --- parasite records --- */
void         bse_item_set_parasite               (BseItem        *item, /* undoable */
                                                  const gchar    *parasite_path,
                                                  SfiRec         *rec);
SfiRec*      bse_item_get_parasite               (BseItem        *item,
                                                  const gchar    *parasite_path);
void         bse_item_backup_parasite            (BseItem        *item,
                                                  const gchar    *parasite_path,
                                                  SfiRec         *rec);
void         bse_item_delete_parasites           (BseItem        *item);
SfiRing*     bse_item_list_parasites             (BseItem        *item,
                                                  const gchar    *parent_path);
const gchar* bse_item_create_parasite_name       (BseItem        *item,
                                                  const gchar    *path_prefix);
/* BseItem signals:
 *   void (*parasites_added)  (BseItem     *item,
 *                             const gchar *parasite_path);
 *   void (*parasite_changed) (BseItem     *item,
 *                             const gchar *parasite_path);
 */
void         bse_item_class_add_parasite_signals (BseItemClass *);


/* --- old prototypes --- */
void	   bse_parasite_set_floats	(BseObject      *object,
					 const gchar	*name,
					 guint		 n_values,
					 gfloat		*float_values);
SfiFBlock* bse_parasite_get_floats	(BseObject      *object,
					 const gchar	*name);
void	   bse_parasite_store		(BseObject	*object,
					 BseStorage	*storage);
GTokenType bse_parasite_restore		(BseObject	*object,
					 BseStorage	*storage);

G_END_DECLS

#endif /* __BSE_PARASITE_H__ */
