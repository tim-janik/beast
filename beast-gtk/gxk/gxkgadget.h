/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2003 Tim Janik
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
#ifndef __GXK_GADGET_H__
#define __GXK_GADGET_H__

#include        "gxkutils.h"

G_BEGIN_DECLS

/* --- structures --- */
#define GXK_GADGET      GTK_WIDGET
typedef GtkWidget GxkGadget;


/* --- gadget functions --- */
void            gxk_gadget_add_type     (GType           type);
void            gxk_gadget_parse        (const gchar    *domain_name,
                                         const gchar    *file_name,
                                         GError        **error);
void            gxk_gadget_parse_text   (const gchar    *domain_name,
                                         const gchar    *text,
                                         gint            text_len,
                                         GError        **error);
GxkGadget*      gxk_gadget_create       (const gchar    *domain_name,
                                         const gchar    *name,
                                         const gchar    *options);
GxkGadget*      gxk_gadget_complete     (GtkWidget      *widget,
                                         const gchar    *domain_name,
                                         const gchar    *name,
                                         const gchar    *options);
void            gxk_gadget_add          (GxkGadget      *gadget,
                                         const gchar    *region,
                                         gpointer        widget);
gpointer        gxk_gadget_find         (GxkGadget      *gadget,
                                         const gchar    *region);


G_END_DECLS

#endif /* __GXK_GADGET_H__ */
