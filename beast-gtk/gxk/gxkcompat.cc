// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gxkcompat.hh"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

GdkPixbuf*
gxk_pixbuf_new_from_inline (int data_length, const guint8 *data, gboolean copy_pixels, GError **error)
{
  return gdk_pixbuf_new_from_inline (data_length, data, copy_pixels, error);
}
