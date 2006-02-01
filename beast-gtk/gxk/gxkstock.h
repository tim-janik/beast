/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2002-2006 Tim Janik
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
#ifndef __GXK_STOCK_H__
#define __GXK_STOCK_H__

#include        <gxk/gxkutils.h>

G_BEGIN_DECLS

/* --- icon sizes --- */
#define GXK_ICON_SIZE_BUTTON	  (gxk_size_button)
#define GXK_ICON_SIZE_BIG_BUTTON  (gxk_size_big_button)
#define GXK_ICON_SIZE_CANVAS	  (gxk_size_canvas)
#define GXK_ICON_SIZE_TOOLBAR	  (gxk_size_toolbar)
#define GXK_ICON_SIZE_MENU	  (gxk_size_menu)
#define GXK_ICON_SIZE_TABULATOR	  (gxk_size_tabulator)
#define GXK_ICON_SIZE_INFO_SIGN	  (gxk_size_info_sign)
#define GXK_ICON_SIZE_PALETTE	  (gxk_size_palette)
guint	gxk_size_width		  (GtkIconSize	 icon_size);
guint	gxk_size_height		  (GtkIconSize	 icon_size);


/* --- functions --- */
GtkWidget*    gxk_stock_image		(const gchar	*stock_icon_id,
					 GtkIconSize	 icon_size);
GtkWidget*    gxk_stock_button		(const gchar	*stock_id,
					 const gchar	*label);
GtkWidget*    gxk_stock_button_child	(const gchar	*stock_id,
					 const gchar	*label);
const gchar*  gxk_stock_item		(const gchar	*stock_id);
GdkPixbuf*    gxk_stock_fallback_pixbuf	(const gchar	*stock_id);
GtkWidget*    gxk_stock_icon_window	(const gchar	*stock_id);


/* --- registration --- */
typedef struct {
  const gchar  *stock_id;
  const guint8 *inlined_pixbuf;
} GxkStockIcon;
typedef struct {
  const gchar  *stock_id;
  const gchar  *label;
  const gchar  *stock_fallback;
} GxkStockItem;
void	    gxk_stock_register_icon	(const GxkStockIcon	*icon);
void	    gxk_stock_register_icons	(guint			 n_icons,
					 const GxkStockIcon	*icons);
void	    gxk_stock_register_item	(const GxkStockItem	*item);
void	    gxk_stock_register_items	(guint			 n_items,
					 const GxkStockItem	*items);


/* --- internal --- */
extern guint	gxk_size_button;
extern guint	gxk_size_big_button;
extern guint	gxk_size_canvas;
extern guint	gxk_size_toolbar;
extern guint	gxk_size_menu;
extern guint	gxk_size_tabulator;
extern guint	gxk_size_info_sign;
extern guint	gxk_size_palette;


G_END_DECLS

#endif /* __GXK_STOCK_H__ */
