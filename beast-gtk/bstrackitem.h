/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
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
#ifndef __BST_RACK_ITEM_H__
#define __BST_RACK_ITEM_H__

#include "bstracktable.h"
#include "bstparam.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BST_TYPE_RACK_ITEM              (bst_rack_item_get_type ())
#define BST_RACK_ITEM(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_RACK_ITEM, BstRackItem))
#define BST_RACK_ITEM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_RACK_ITEM, BstRackItemClass))
#define BST_IS_RACK_ITEM(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_RACK_ITEM))
#define BST_IS_RACK_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_RACK_ITEM))
#define BST_RACK_ITEM_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_RACK_ITEM, BstRackItemClass))


/* --- structures & typedefs --- */
typedef	struct	_BstRackItem		BstRackItem;
typedef	struct	_BstRackItemClass	BstRackItemClass;
struct _BstRackItem
{
  GtkFrame	parent_instance;

  guint		 block_updates;
  GtkWidget	*controller_choice;
  GtkWidget	*choice;

  SfiProxy	pocket;
  guint		entry;

  /* pocket data */
  SfiProxy	   proxy;
  GParamSpec	  *pspec;
  gchar		  *ctype;

  BstParam	*bparam;

  /* maintained by BstRackTable */
  BstRackChildInfo rack_child_info;
  guint		   empty_frame : 1;
};
struct _BstRackItemClass
{
  GtkFrameClass parent_class;

  void		(*button_press)	(BstRackItem	*item,
				 GdkEventButton	*event);
};


/* --- prototypes --- */
GtkType		bst_rack_item_get_type		(void);
void		bst_rack_item_set_property	(BstRackItem	*item,
						 SfiProxy	 data_pocket,
						 guint		 entry_id);
void		bst_rack_item_set_proxy		(BstRackItem	*item,
						 SfiProxy	 proxy,
						 GParamSpec	*pspec,
						 const gchar    *view_name);
void		bst_rack_item_gui_changed	(BstRackItem	*item);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_RACK_ITEM_H__ */
