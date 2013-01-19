// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_BUS_VIEW_H__
#define __BST_BUS_VIEW_H__
#include	"bstitemview.hh"
G_BEGIN_DECLS
/* --- type macros --- */
#define BST_TYPE_BUS_VIEW              (bst_bus_view_get_type ())
#define BST_BUS_VIEW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_BUS_VIEW, BstBusView))
#define BST_BUS_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_BUS_VIEW, BstBusViewClass))
#define BST_IS_BUS_VIEW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_BUS_VIEW))
#define BST_IS_BUS_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_BUS_VIEW))
#define BST_BUS_VIEW_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_BUS_VIEW, BstBusViewClass))
/* --- structures & typedefs --- */
typedef	struct	_BstBusView		BstBusView;
typedef	struct	_BstBusViewClass	BstBusViewClass;
struct _BstBusView
{
  BstItemView	 parent_object;
};
struct _BstBusViewClass
{
  BstItemViewClass parent_class;
};
/* --- prototypes --- */
GType		bst_bus_view_get_type  (void);
GtkWidget*      bst_bus_view_new       (SfiProxy song);
G_END_DECLS
#endif /* __BST_BUS_VIEW_H__ */
