/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_RACK_EDITOR_H__
#define __BST_RACK_EDITOR_H__

#include "bstdefs.h"
#include "bstracktable.h"


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

  BswProxy	pocket;

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
GtkWidget*	bst_rack_editor_new		(BswProxy	rack_view);
void		bst_rack_editor_set_rack_view	(BstRackEditor	*editor,
						 BswProxy	 rack_view);
void		bst_rack_editor_add_property	(BstRackEditor	*editor,
						 BswProxy	 item,
						 const gchar	*property_name);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_RACK_EDITOR_H__ */
