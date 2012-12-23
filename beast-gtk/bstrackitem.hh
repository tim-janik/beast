// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_RACK_ITEM_H__
#define __BST_RACK_ITEM_H__
#include "bstracktable.hh"
#include "bstparam.hh"
G_BEGIN_DECLS
/* --- type macros --- */
#define BST_TYPE_RACK_ITEM              (bst_rack_item_get_type ())
#define BST_RACK_ITEM(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_RACK_ITEM, BstRackItem))
#define BST_RACK_ITEM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_RACK_ITEM, BstRackItemClass))
#define BST_IS_RACK_ITEM(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_RACK_ITEM))
#define BST_IS_RACK_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_RACK_ITEM))
#define BST_RACK_ITEM_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_RACK_ITEM, BstRackItemClass))
/* --- structures & typedefs --- */
typedef struct {
  GxkRackItem    parent_instance;
  SfiProxy       proxy;
  const gchar   *path;
  SfiRec        *rec;
  guint          block_updates;
  GtkWidget     *controller_choice;
  GtkWidget     *choice;
  /* pocket data */
  GParamSpec      *pspec;
  gchar           *ctype;
  GxkParam      *param;
} BstRackItem;
typedef struct _GxkRackItemClass BstRackItemClass;
/* --- prototypes --- */
GType           bst_rack_item_get_type          (void);
GtkWidget*      bst_rack_item_new               (SfiProxy        proxy,
                                                 const gchar    *path);
void            bst_rack_item_set_parasite      (BstRackItem    *self,
                                                 SfiProxy        proxy,
                                                 const gchar    *path);
G_END_DECLS
#endif /* __BST_RACK_ITEM_H__ */
