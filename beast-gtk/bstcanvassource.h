/* BEAST - Better Audio System
 * Copyright (C) 1998-2003 Tim Janik
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
#ifndef __BST_CANVAS_SOURCE_H__
#define __BST_CANVAS_SOURCE_H__

#include	"bstutils.h"

G_BEGIN_DECLS

/* --- Gtk+ type macros --- */
#define BST_TYPE_CANVAS_SOURCE              (bst_canvas_source_get_type ())
#define BST_CANVAS_SOURCE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_CANVAS_SOURCE, BstCanvasSource))
#define BST_CANVAS_SOURCE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BST_TYPE_CANVAS_SOURCE, BstCanvasSourceClass))
#define BST_IS_CANVAS_SOURCE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_CANVAS_SOURCE))
#define BST_IS_CANVAS_SOURCE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BST_TYPE_CANVAS_SOURCE))
#define BST_CANVAS_SOURCE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_CANVAS_SOURCE, BstCanvasSourceClass))
#define	BST_CANVAS_SOURCE_PIXEL_SCALE	    ((SfiReal) 100)	/* > total width/height */


/* --- structures & typedefs --- */
typedef	struct	_BstCanvasSource	BstCanvasSource;
typedef	struct	_BstCanvasSourceClass	BstCanvasSourceClass;
struct _BstCanvasSource
{
  GnomeCanvasGroup parent_object;

  SfiProxy	   source;

  GtkWidget	  *params_dialog;
  GtkWidget	  *source_info;

  GnomeCanvasItem *icon_item;
  GnomeCanvasItem *text;
  GSList	  *channel_items;
  GSList	  *channel_hints;

  guint		   swap_channels : 1;
  guint            in_move : 1;
  guint		   show_hints : 1;
  guint            idle_reposition : 1;
  guint            built_ichannels : 1;
  guint            built_ochannels : 1;
  guint            built_ihints : 1;
  guint            built_ohints : 1;
  gdouble	   move_dx, move_dy;
};
struct _BstCanvasSourceClass
{
  GnomeCanvasGroupClass parent_class;

  void (*update_links)	(BstCanvasSource *source);
};


/* --- prototypes --- */
GType            bst_canvas_source_get_type          (void);
GnomeCanvasItem* bst_canvas_source_new               (GnomeCanvasGroup *group,
                                                      SfiProxy          source);
void             bst_canvas_source_update_links      (BstCanvasSource  *csource);
void             bst_canvas_source_ochannel_pos      (BstCanvasSource  *csource,
                                                      guint             ochannel,
                                                      gdouble          *world_x,
                                                      gdouble          *world_y);
void             bst_canvas_source_ichannel_pos      (BstCanvasSource  *csource,
                                                      guint             ichannel,
                                                      gdouble          *world_x,
                                                      gdouble          *world_y);
gboolean         bst_canvas_source_is_jchannel       (BstCanvasSource  *csource,
                                                      guint             ichannel);
gboolean         bst_canvas_source_ichannel_free     (BstCanvasSource  *csource,
                                                      guint             ichannel);
guint            bst_canvas_source_ichannel_at       (BstCanvasSource  *csource,
                                                      gdouble           world_x,
                                                      gdouble           world_y);
guint            bst_canvas_source_ochannel_at       (BstCanvasSource  *csource,
                                                      gdouble           world_x,
                                                      gdouble           world_y);
BstCanvasSource* bst_canvas_source_at                (GnomeCanvas      *csource,
                                                      gdouble           world_x,
                                                      gdouble           world_y);
void             bst_canvas_source_reset_params      (BstCanvasSource  *csource);
void             bst_canvas_source_popup_params      (BstCanvasSource  *csource);
void             bst_canvas_source_toggle_params     (BstCanvasSource  *csource);
void             bst_canvas_source_popup_info        (BstCanvasSource  *csource);
void             bst_canvas_source_toggle_info       (BstCanvasSource  *csource);
void             bst_canvas_source_set_channel_hints (BstCanvasSource  *csource,
                                                      gboolean          on_off);

G_END_DECLS

#endif /* __BST_CANVAS_SOURCE_H__ */
