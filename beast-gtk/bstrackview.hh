// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_RACK_VIEW_H__
#define __BST_RACK_VIEW_H__
#include        "bstutils.hh"
G_BEGIN_DECLS
/* --- type macros --- */
#define BST_TYPE_RACK_VIEW              (bst_rack_view_get_type ())
#define BST_RACK_VIEW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_RACK_VIEW, BstRackView))
#define BST_RACK_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_RACK_VIEW, BstRackViewClass))
#define BST_IS_RACK_VIEW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_RACK_VIEW))
#define BST_IS_RACK_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_RACK_VIEW))
#define BST_RACK_VIEW_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_RACK_VIEW, BstRackViewClass))
/* --- structures & typedefs --- */
typedef struct  _BstRackView            BstRackView;
typedef struct  _BstRackViewClass       BstRackViewClass;
struct _BstRackView
{
  GtkVBox        parent_object;
  SfiProxy       item;
  GxkRackTable  *rack_table;
};
struct _BstRackViewClass
{
  GtkVBoxClass parent_class;
};
/* --- prototypes --- */
GType      bst_rack_view_get_type (void);
GtkWidget* bst_rack_view_new      (SfiProxy     item);
void       bst_rack_view_set_item (BstRackView *self,
                                   SfiProxy     item);
void       bst_rack_view_rebuild  (BstRackView *self);
G_END_DECLS
#endif /* __BST_RACK_VIEW_H__ */
