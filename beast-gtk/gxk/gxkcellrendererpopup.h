/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2003-2006 Tim Janik
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
#ifndef __GXK_CELL_RENDERER_POPUP_H__
#define __GXK_CELL_RENDERER_POPUP_H__

#include <gxk/gxkutils.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkeventbox.h>

G_BEGIN_DECLS

/* --- type macros --- */
#define GXK_TYPE_CELL_RENDERER_POPUP              (gxk_cell_renderer_popup_get_type ())
#define GXK_CELL_RENDERER_POPUP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_CELL_RENDERER_POPUP, GxkCellRendererPopup))
#define GXK_CELL_RENDERER_POPUP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_CELL_RENDERER_POPUP, GxkCellRendererPopupClass))
#define GXK_IS_CELL_RENDERER_POPUP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_CELL_RENDERER_POPUP))
#define GXK_IS_CELL_RENDERER_POPUP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_CELL_RENDERER_POPUP))
#define GXK_CELL_RENDERER_POPUP_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_CELL_RENDERER_POPUP, GxkCellRendererPopupClass))


/* --- structures & typedefs --- */
typedef struct {
  GtkCellRendererText parent_instace;
  GtkWidget          *dialog, *entry;
  guint               text_editing : 1;
  guint               popup_editing : 1;
  guint               auto_popup : 1;
} GxkCellRendererPopup;
typedef struct {
  GtkCellRendererTextClass parent_class;
  void (*popup) (GxkCellRendererPopup *cell,
		 const gchar          *path,
		 const gchar          *text);
} GxkCellRendererPopupClass;


/* --- functions --- */
GType	gxk_cell_renderer_popup_get_type	(void);
void	gxk_cell_renderer_popup_dialog		(GxkCellRendererPopup	*popup,
						 GtkWidget		*dialog);
void    gxk_cell_renderer_popup_change          (GxkCellRendererPopup   *popup,
                                                 const gchar            *text,
                                                 gboolean                preserve_popup,
                                                 gboolean                keep_editing);


/* --- type macros --- */
#define GXK_TYPE_PROXY_EDITABLE              (gxk_proxy_editable_get_type ())
#define GXK_PROXY_EDITABLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_PROXY_EDITABLE, GxkProxyEditable))
#define GXK_PROXY_EDITABLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_PROXY_EDITABLE, GxkProxyEditableClass))
#define GXK_IS_PROXY_EDITABLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_PROXY_EDITABLE))
#define GXK_IS_PROXY_EDITABLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_PROXY_EDITABLE))
#define GXK_PROXY_EDITABLE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_PROXY_EDITABLE, GxkProxyEditableClass))


/* --- structures & typedefs --- */
typedef struct {
  GtkEventBox	   parent_instace;
  GtkCellEditable *ecell;
  guint8	   block_start_editing;
  guint8	   block_remove_widget;
  guint8	   block_editing_done;
} GxkProxyEditable;
typedef struct {
  GtkEventBoxClass parent_class;
} GxkProxyEditableClass;


/* --- functions --- */
GType	gxk_proxy_editable_get_type		(void);
void	gxk_proxy_editable_set_cell_editable	(GxkProxyEditable	*self,
						 GtkCellEditable	*ecell);


G_END_DECLS

#endif	/* __GXK_CELL_RENDERER_POPUP_H__ */
