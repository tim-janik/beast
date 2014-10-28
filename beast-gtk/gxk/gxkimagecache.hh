// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_IMAGE_CACHE_H__
#define __GXK_IMAGE_CACHE_H__

#include <gxk/gxkutils.hh>

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
