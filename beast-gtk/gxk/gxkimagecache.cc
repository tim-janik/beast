// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gxkimagecache.hh"
#include <string.h>


#define MAX_CACHE_FILL  0


/* --- variables --- */
static GSList *image_cache = NULL;


/* --- functions --- */
static inline void
desaturate (GdkColor tint,
            gdouble  saturation,
            guint8  *red,
            guint8  *green,
            guint8  *blue)
{
  guint r = tint.red >> 8;
  guint g = tint.green >> 8;
  guint b = tint.blue >> 8;
  *red = saturation * *red + (1.0 - saturation) * r;
  *green = saturation * *green + (1.0 - saturation) * g;
  *blue = saturation * *blue + (1.0 - saturation) * b;
}

static GdkPixbuf*
load_pixbuf (const char *file_name,
             GError    **errorp)
{
  /* special case builtin images */
  if (strcmp (file_name, GXK_IMAGE_BLACK32) == 0)
    {
      static const guint8 black32_pixbuf[] = {
        "GdkP\0\0\0<\2\1\0\1\0\0\0`\0\0\0\40\0\0\0\40"
        "\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0"
        "\0\377\0\0\0\210\0\0\0"
      };
      return gdk_pixbuf_new_from_inline (sizeof (black32_pixbuf), black32_pixbuf, FALSE, errorp);
    }
  /* try to load from disk */
  return gdk_pixbuf_new_from_file (file_name, errorp);
}

static GxkImageCacheItem*
image_cache_create_item (const gchar    *file_name,
                         GdkColor        tint,
                         gdouble         saturation,
                         GdkColormap    *colormap)
{
  GError *error = NULL;
  GdkPixbuf *pixbuf = load_pixbuf (file_name, &error);
  GxkImageCacheItem *citem;
  gint x, y, w, h, nc, rs;
  guint8 *pixels;
  if (error)
    {
      g_message ("failed to load image \"%s\": %s", file_name, error->message);
      g_clear_error (&error);
      return NULL;
    }
  if (!pixbuf || gdk_pixbuf_get_n_channels (pixbuf) < 3 ||
      gdk_pixbuf_get_colorspace (pixbuf) != GDK_COLORSPACE_RGB)
    {
      g_message ("failed to load image \"%s\": unsupport image format (not RGB) [%u %u]", file_name,
                 gdk_pixbuf_get_n_channels (pixbuf), gdk_pixbuf_get_has_alpha (pixbuf));
      if (pixbuf)
        g_object_unref (pixbuf);
      return NULL;
    }
  citem = g_new0 (GxkImageCacheItem, 1);
  citem->file_name = g_strdup (file_name);
  citem->tint = tint;
  citem->saturation = saturation;
  citem->colormap = (GdkColormap*) g_object_ref (colormap);
  w = gdk_pixbuf_get_width (pixbuf);
  h = gdk_pixbuf_get_height (pixbuf);
  rs = gdk_pixbuf_get_rowstride (pixbuf);
  nc = gdk_pixbuf_get_n_channels (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);
  for (y = 0; y < h * rs; y += rs)
    for (x = 0; x < w * nc; x += nc)
      desaturate (tint, saturation,
                  &pixels[y + x + 0],
                  &pixels[y + x + 1],
                  &pixels[y + x + 2]);
  gdk_pixbuf_render_pixmap_and_mask_for_colormap (pixbuf, colormap, &citem->pixmap, NULL, 0);
  image_cache = g_slist_append (image_cache, citem);
  return citem;
}

static void
image_cache_item_unuse (GxkImageCacheItem *citem)
{
  g_return_if_fail (citem->use_count > 0);
  citem->use_count--;
  if (citem->use_count)
    return;
  /* start purging */
  if (g_slist_length (image_cache) > MAX_CACHE_FILL)
    {
      GSList *slist, *last = NULL;
      for (slist = image_cache; slist; last = slist, slist = last->next)
        {
          GxkImageCacheItem *citem = (GxkImageCacheItem*) slist->data;
          if (citem->use_count == 0)
            {
              /* remove from list */
              if (last)
                last->next = slist->next;
              else
                image_cache = slist->next;
              /* free pixmap */
              g_free (citem->file_name);
              g_object_unref (citem->pixmap);
              g_object_unref (citem->colormap);
              g_free (citem);
              return;
            }
        }
      /* nothing to free */
    }
}

static GxkImageCacheItem*
image_cache_find_item (const gchar    *file_name,
                       GdkColor        tint,
                       gdouble         saturation,
                       GdkColormap    *colormap)
{
  GSList *slist;
  for (slist = image_cache; slist; slist = slist->next)
    {
      GxkImageCacheItem *citem = (GxkImageCacheItem*) slist->data;
      if (citem->colormap == colormap &&
          strcmp (file_name, citem->file_name) == 0 &&
          tint.red >> 8 == citem->tint.red >> 8 &&
          tint.green >> 8 == citem->tint.green >> 8 &&
          tint.blue >> 8 == citem->tint.blue >> 8 &&
          ABS (saturation - citem->saturation) < 0.003)
        return citem;
    }
  return NULL;
}

GdkPixmap*
gxk_image_cache_use_pixmap (const gchar    *file_name,
                            GdkColor        tint,
                            gdouble         saturation,
                            GdkColormap    *colormap)
{
  GxkImageCacheItem *citem;
  if (!file_name)
    return NULL;
  citem = image_cache_find_item (file_name, tint, saturation, colormap);
  if (!citem)
    citem = image_cache_create_item (file_name, tint, saturation, colormap);
  if (citem)
    {
      citem->use_count++;
      return citem->pixmap;
    }
  return NULL;
}

void
gxk_image_cache_unuse_pixmap (GdkPixmap *pixmap)
{
  GSList *slist;
  if (!pixmap)
    return;
  for (slist = image_cache; slist; slist = slist->next)
    {
      GxkImageCacheItem *citem = (GxkImageCacheItem*) slist->data;
      if (citem->pixmap == pixmap)
        {
          image_cache_item_unuse (citem);
          return;
        }
    }
  g_warning ("%s: no such pixmap: %p", G_STRFUNC, pixmap);
}
