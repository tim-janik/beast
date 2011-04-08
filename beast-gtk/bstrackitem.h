/* BEAST - Better Audio System
 * Copyright (C) 2002-2003 Tim Janik
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
#ifndef __BST_RACK_ITEM_H__
#define __BST_RACK_ITEM_H__

#include "bstracktable.h"
#include "bstparam.h"

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
