// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_COMPAT_HH__
#define __GXK_COMPAT_HH__

#include        <gxk/gxkglobals.hh>

GdkPixbuf* gxk_pixbuf_new_from_inline (int data_length, const guint8 *data, gboolean copy_pixels, GError **error);

#endif /* __GXK_COMPAT_HH__ */
