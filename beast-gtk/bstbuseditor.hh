// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_BUS_EDITOR_H__
#define __BST_BUS_EDITOR_H__

#include "bstitemview.hh"
#include "bstdbmeter.hh"

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_BUS_EDITOR              (bst_bus_editor_get_type ())
#define BST_BUS_EDITOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_BUS_EDITOR, BstBusEditor))
#define BST_BUS_EDITOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_BUS_EDITOR, BstBusEditorClass))
#define BST_IS_BUS_EDITOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_BUS_EDITOR))
#define BST_IS_BUS_EDITOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_BUS_EDITOR))
#define BST_BUS_EDITOR_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_BUS_EDITOR, BstBusEditorClass))


/* --- structures & typedefs --- */
typedef	struct	_BstBusEditor      BstBusEditor;
typedef	struct	_BstBusEditorClass BstBusEditorClass;
struct _BstBusEditor
{
  GtkAlignment      parent_object;
  SfiProxy          item;
  SfiRing          *params;
  BstDBBeam        *lbeam, *rbeam;
};
struct _BstBusEditorClass
{
  GtkAlignmentClass parent_class;
};


/* --- prototypes --- */
GType		bst_bus_editor_get_type  (void);
GtkWidget*      bst_bus_editor_new       (SfiProxy      song);
void            bst_bus_editor_set_bus   (BstBusEditor *self,
                                          SfiProxy      item);

G_END_DECLS

#endif /* __BST_BUS_EDITOR_H__ */
