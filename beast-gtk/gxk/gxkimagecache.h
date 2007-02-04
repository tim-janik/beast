/* GXK - Gtk+ Extension Kit
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
