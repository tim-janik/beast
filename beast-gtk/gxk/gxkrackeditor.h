/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2001-2003 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GXK_RACK_EDITOR_H__
#define __GXK_RACK_EDITOR_H__

#include <gxk/gxkracktable.h>

G_BEGIN_DECLS

struct _GxkRackEditor {
  GdkWindow     *iwindow;
  gint           rfx, rfy, rfw, rfh;
  /* child resizing windows */
  GdkWindow     *crq1, *crq2, *crq3, *crq4;     /* corner windows */
  GdkWindow     *crb1, *crb2, *crb3, *crb4;     /* bar windows: 1=upper, 2=right, 3=bottom 4=left */
  /* dragging */
  guint          grabbing : 1;
  GtkWidget     *drag_child;
  /* corners: 1=top-left, 2=top-right, 3=bottom-left, 4=bottom-right */
  gint           drag_corner, drag_x, drag_y;
  gint           drag_col, drag_row, drag_hspan, drag_vspan;
};


/* --- prototypes --- */
void     gxk_rack_table_set_edit_mode         (GxkRackTable     *self,
                                               gboolean          enable_editing);
gboolean gxk_rack_table_check_cell            (GxkRackTable     *self,
                                               guint             col,
                                               guint             row);
gboolean gxk_rack_table_check_area            (GxkRackTable     *self,
                                               gboolean          framed,
                                               guint             col,
                                               guint             row,
                                               guint             hspan,
                                               guint             vspan,
                                               GtkWidget        *exempt);
gboolean gxk_rack_table_expand_rect           (GxkRackTable     *self,
                                               guint             col,
                                               guint             row,
                                               guint            *hspan,
                                               guint            *vspan);


/* --- implementation details --- */
void     gxk_rack_table_destroy_editor        (GxkRackTable     *self);
gboolean gxk_rack_table_handle_enter_notify   (GxkRackTable     *self,
                                               GdkEventCrossing *event);
gboolean gxk_rack_table_handle_button_press   (GxkRackTable     *self,
                                               GdkEventButton   *event);
gboolean gxk_rack_table_handle_motion_notify  (GxkRackTable     *self,
                                               GdkEventMotion   *event);
gboolean gxk_rack_table_handle_button_release (GxkRackTable     *self,
                                               GdkEventButton   *event);
gboolean gxk_rack_table_handle_leave_notify   (GxkRackTable     *self,
                                               GdkEventCrossing *event);
void     gxk_rack_table_adjust_editor         (GxkRackTable     *self);
void     gxk_rack_table_unmap_editor          (GxkRackTable     *self);


G_END_DECLS

#endif /* __GXK_RACK_EDITOR_H__ */
