/* BEAST - Bedevilled Audio System
 * Copyright (C) 2004 Tim Janik
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
#ifndef __BST_BUS_EDITOR_H__
#define __BST_BUS_EDITOR_H__

#include "bstitemview.h"
#include "bstdbmeter.h"

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
  GxkParam         *lvolume, *rvolume;
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
