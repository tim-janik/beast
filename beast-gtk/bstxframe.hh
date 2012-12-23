/* BEAST - Better Audio System
 * Copyright (C) 2001-2002 Tim Janik
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
#ifndef __BST_XFRAME_H__
#define __BST_XFRAME_H__

#include <gtk/gtkframe.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define BST_TYPE_XFRAME			(bst_xframe_get_type ())
#define BST_XFRAME(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_XFRAME, BstXFrame))
#define BST_XFRAME_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_XFRAME, BstXFrameClass))
#define BST_IS_XFRAME(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_XFRAME))
#define BST_IS_XFRAME_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_XFRAME))
#define BST_XFRAME_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BST_TYPE_XFRAME, BstXFrameClass))


typedef struct _BstXFrame      BstXFrame;
typedef struct _BstXFrameClass BstXFrameClass;
struct _BstXFrame
{
  GtkFrame	 parent_instance;
  GdkWindow	*iwindow;
  GtkWidget	*cover;
  guint		 button_down : 4;
  guint		 entered : 1;
  guint		 allocation_valid : 1;
  guint		 steal_button : 1;
  GtkAllocation  allocation;
};
struct _BstXFrameClass
{
  GtkFrameClass parent_class;

  gboolean	(*button_check)	(BstXFrame	*xframe,
				 guint		 button);
};
  

GType	       bst_xframe_get_type		(void);
void           bst_xframe_set_cover_widget	(BstXFrame      *xframe,
						 GtkWidget	*widget,
						 gboolean        steal_button);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_XFRAME_H__ */

