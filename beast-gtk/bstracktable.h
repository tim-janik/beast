/* BEAST - Bedevilled Audio System
 * Copyright (C) 2001-2002 Tim Janik
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
#ifndef __BST_RACK_TABLE_H__
#define __BST_RACK_TABLE_H__

#include	<gtk/gtktable.h>
#include	<gtk/gtkwindow.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BST_TYPE_RACK_TABLE              (bst_rack_table_get_type ())
#define BST_RACK_TABLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_RACK_TABLE, BstRackTable))
#define BST_RACK_TABLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_RACK_TABLE, BstRackTableClass))
#define BST_IS_RACK_TABLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_RACK_TABLE))
#define BST_IS_RACK_TABLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_RACK_TABLE))
#define BST_RACK_TABLE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_RACK_TABLE, BstRackTableClass))


/* --- structures & typedefs --- */
typedef	struct _BstRackTable	  BstRackTable;
typedef	struct _BstRackTableClass BstRackTableClass;
typedef	struct _BstRackChildInfo  BstRackChildInfo;
struct _BstRackChildInfo
{
  gint col, row;
  gint hspan, vspan;
};
struct _BstRackTable
{
  GtkTable       parent_object;

  GtkWidget	*drag_window;

  guint		 map_cols;
  guint		 map_rows;
  guint32	*child_map;

  guint		    cell_request_width;
  guint		    cell_request_height;
  guint		    cell_width;
  guint		    cell_height;

  GdkWindow	   *iwindow;
  guint		    edit_mode : 1;
  guint		    in_drag : 2;
  guint		    in_drag_and_grabbing : 1;
  BstRackChildInfo  drag_info;
  guint		    drag_col;
  guint		    drag_row;
  gint		    xofs;
  gint		    yofs;
  GtkWidget	   *child;
};
struct _BstRackTableClass
{
  GtkTableClass parent_class;

  void	(*edit_mode_changed)	(BstRackTable	*rtable,
				 gboolean	 edit_mode);
  void	(*child_changed)	(BstRackTable	*rtable,
				 GtkWidget	*child);
};


/* --- prototypes --- */
GtkType		bst_rack_table_get_type		(void);
void		bst_rack_table_set_edit_mode	(BstRackTable	*rtable,
						 gboolean	 enable_editing);
gboolean	bst_rack_table_check_cell	(BstRackTable	*rtable,
						 guint		 col,
						 guint           row);
gboolean	bst_rack_table_check_area	(BstRackTable	*rtable,
						 guint		 col,
						 guint           row,
						 guint		 hspan,
						 guint		 vspan);
gboolean	bst_rack_table_expand_rect	(BstRackTable	*rtable,
						 guint		 col,
						 guint		 row,
						 guint		*hspan,
						 guint		*vspan);
void		bst_rack_child_get_info		(GtkWidget	*widget,
						 BstRackChildInfo *info);
void		bst_rack_child_set_info		(GtkWidget	*widget,
						 gint		 col,
						 gint		 row,
						 gint		 hspan,
						 gint		 vspan);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_RACK_TABLE_H__ */
