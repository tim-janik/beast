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
#ifndef __BST_SCROLLGRAPH_H__
#define __BST_SCROLLGRAPH_H__

#include <gtk/gtkadjustment.h>
#include <gtk/gtkimage.h>

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_SCROLLGRAPH		  (bst_scrollgraph_get_type ())
#define BST_SCROLLGRAPH(object)		  (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_SCROLLGRAPH, BstScrollgraph))
#define BST_SCROLLGRAPH_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_SCROLLGRAPH, BstScrollgraphClass))
#define BST_IS_SCROLLGRAPH(object)	  (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_SCROLLGRAPH))
#define BST_IS_SCROLLGRAPH_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_SCROLLGRAPH))
#define BST_SCROLLGRAPH_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((obj), BST_TYPE_SCROLLGRAPH, BstScrollgraphClass))

/* --- structures & typedefs --- */
typedef struct _BstScrollgraph BstScrollgraph;
typedef struct _GtkWidgetClass BstScrollgraphClass;
struct _BstScrollgraph
{
  GtkWidget parent_instance;
};
  
/* --- public methods --- */
GType	        bst_scrollgraph_get_type	(void);
void            bst_scrollgraph_clear           (BstScrollgraph *self);

G_END_DECLS

#endif /* __BST_SCROLLGRAPH_H__ */
