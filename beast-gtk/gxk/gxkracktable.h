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
#ifndef __GXK_RACK_TABLE_H__
#define __GXK_RACK_TABLE_H__

#include <gxk/gxkutils.h>

G_BEGIN_DECLS

/* --- type macros --- */
#define GXK_TYPE_RACK_TABLE              (gxk_rack_table_get_type ())
#define GXK_RACK_TABLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_RACK_TABLE, GxkRackTable))
#define GXK_RACK_TABLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_RACK_TABLE, GxkRackTableClass))
#define GXK_IS_RACK_TABLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_RACK_TABLE))
#define GXK_IS_RACK_TABLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_RACK_TABLE))
#define GXK_RACK_TABLE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_RACK_TABLE, GxkRackTableClass))


/* --- structures & typedefs --- */
typedef struct _GxkRackTable      GxkRackTable;
typedef struct _GxkRackTableClass GxkRackTableClass;
typedef struct _GxkRackChildInfo  GxkRackChildInfo;
struct _GxkRackChildInfo
{
  gint col, row;
  gint hspan, vspan;
};
struct _GxkRackTable
{
  GtkTable       parent_object;
  
  GtkWidget     *drag_window;
  
  guint          map_cols;
  guint          map_rows;
  guint32       *child_map;
  
  guint             cell_request_width;
  guint             cell_request_height;
  guint             cell_width;
  guint             cell_height;
  
  GdkWindow        *iwindow;
  guint             edit_mode : 1;
  guint             in_drag : 2;
  guint             in_drag_and_grabbing : 1;
  GxkRackChildInfo  drag_info;
  guint             drag_col;
  guint             drag_row;
  gint              xofs;
  gint              yofs;
  GtkWidget        *child;
};
struct _GxkRackTableClass
{
  GtkTableClass parent_class;
  
  void  (*edit_mode_changed)    (GxkRackTable   *self,
                                 gboolean        edit_mode);
  void  (*child_changed)        (GxkRackTable   *self,
                                 GtkWidget      *child);
};


/* --- prototypes --- */
GtkType         gxk_rack_table_get_type         (void);
void            gxk_rack_table_set_edit_mode    (GxkRackTable   *self,
                                                 gboolean        enable_editing);
gboolean        gxk_rack_table_check_cell       (GxkRackTable   *self,
                                                 guint           col,
                                                 guint           row);
gboolean        gxk_rack_table_check_area       (GxkRackTable   *self,
                                                 guint           col,
                                                 guint           row,
                                                 guint           hspan,
                                                 guint           vspan);
gboolean        gxk_rack_table_expand_rect      (GxkRackTable   *self,
                                                 guint           col,
                                                 guint           row,
                                                 guint          *hspan,
                                                 guint          *vspan);
void            gxk_rack_child_get_info         (GtkWidget      *widget,
                                                 GxkRackChildInfo *info);
void            gxk_rack_child_set_info         (GtkWidget      *widget,
                                                 gint            col,
                                                 gint            row,
                                                 gint            hspan,
                                                 gint            vspan);

G_END_DECLS

#endif /* __GXK_RACK_TABLE_H__ */
