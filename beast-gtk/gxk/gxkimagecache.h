/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2004 Tim Janik
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
#ifndef __GXK_IMAGE_CACHE_H__
#define __GXK_IMAGE_CACHE_H__

#include <gxk/gxkutils.h>

G_BEGIN_DECLS

typedef struct {
  gchar       *file_name;
  GdkColor     tint;
  gdouble      saturation;
  GdkColormap *colormap;
  GdkPixmap   *pixmap;
  guint        use_count;
} GxkImageCacheItem;

#define GXK_IMAGE_BLACK32       (":/: black32")

GdkPixmap*      gxk_image_cache_use_pixmap      (const gchar    *file_name,
                                                 GdkColor        tint,
                                                 gdouble         saturation,
                                                 GdkColormap    *colormap);
void            gxk_image_cache_unuse_pixmap    (GdkPixmap      *pixmap);


G_END_DECLS

#endif /* __GXK_IMAGE_CACHE_H__ */
