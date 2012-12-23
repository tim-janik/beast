// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_RACK_EDITOR_H__
#define __BST_RACK_EDITOR_H__

#include "bstracktable.hh"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BST_TYPE_RACK_EDITOR              (bst_rack_editor_get_type ())
#define BST_RACK_EDITOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_RACK_EDITOR, BstRackEditor))
#define BST_RACK_EDITOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_RACK_EDITOR, BstRackEditorClass))
#define BST_IS_RACK_EDITOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_RACK_EDITOR))
#define BST_IS_RACK_EDITOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_RACK_EDITOR))
#define BST_RACK_EDITOR_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_RACK_EDITOR, BstRackEditorClass))


/* --- structures & typedefs --- */
typedef	struct	_BstRackEditor		BstRackEditor;
typedef	struct	_BstRackEditorClass	BstRackEditorClass;
struct _BstRackEditor
{
  GtkVBox	parent_instance;

  SfiProxy	pocket;

  BstRackTable	*rtable;
  GSList	*plate_list;
  GtkWidget	*button_edit;
  GSList	*item_list;
};
struct _BstRackEditorClass
{
  GtkVBoxClass parent_class;
};


/* --- prototypes --- */
GtkType		bst_rack_editor_get_type	(void);
GtkWidget*	bst_rack_editor_new		(SfiProxy	rack_view);
void		bst_rack_editor_set_rack_view	(BstRackEditor	*editor,
						 SfiProxy	 rack_view);
void		bst_rack_editor_add_property	(BstRackEditor	*editor,
						 SfiProxy	 item,
						 const gchar	*property_name);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_RACK_EDITOR_H__ */
