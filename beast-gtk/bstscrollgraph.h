/* BEAST - Bedevilled Audio System
 * Copyright (C) 2004 Tim Janik
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
#ifndef __BST_SCROLLGRAPH_H__
#define __BST_SCROLLGRAPH_H__

#include "bstutils.h"

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_SCROLLGRAPH		  (bst_scrollgraph_get_type ())
#define BST_SCROLLGRAPH(object)		  (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_SCROLLGRAPH, BstScrollgraph))
#define BST_SCROLLGRAPH_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_SCROLLGRAPH, BstScrollgraphClass))
#define BST_IS_SCROLLGRAPH(object)	  (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_SCROLLGRAPH))
#define BST_IS_SCROLLGRAPH_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_SCROLLGRAPH))
#define BST_SCROLLGRAPH_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((obj), BST_TYPE_SCROLLGRAPH, BstScrollgraphClass))

/* --- structures & typedefs --- */
typedef struct _BstScrollgraph      BstScrollgraph;
typedef struct _BstScrollgraphClass BstScrollgraphClass;
struct _BstScrollgraph
{
  GtkBin          parent_instance;
  BstDirection    direction;
  guint           window_size : 24;
  guint           flip : 1;
  guint           delete_toplevel : 1;  /* upon proxy::release */
  guint           mix_freq;
  gdouble         boost;
  guint           n_points;
  guint           n_bars;
  guint           bar_offset; /* start of ring-buffer */
  gfloat         *values;     /* [n_points * n_bars] */
  GdkPixbuf      *pixbuf;     /* n_points wide or high */
  GdkWindow      *canvas;
  SfiProxy        source;
  guint           ochannel;
};
struct _BstScrollgraphClass
{
  GtkBinClass parent_class;
  void  (*resize_values) (BstScrollgraph *self,
                          BstDirection    direction);
};

/* --- public methods --- */
GType	        bst_scrollgraph_get_type	(void);
void            bst_scrollgraph_clear           (BstScrollgraph *self);
void            bst_scrollgraph_set_source      (BstScrollgraph *self,
                                                 SfiProxy        source,
                                                 guint           ochannel);
GtkWidget*      bst_scrollgraph_build_dialog    (GtkWidget      *alive_object,
                                                 SfiProxy        source,
                                                 guint           ochannel);

G_END_DECLS

#endif /* __BST_SCROLLGRAPH_H__ */
