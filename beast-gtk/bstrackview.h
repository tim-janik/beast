/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __BST_RACK_VIEW_H__
#define __BST_RACK_VIEW_H__

#include        "bstutils.h"

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
