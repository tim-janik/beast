// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_ASCII_PIXBUF_H__
#define __BST_ASCII_PIXBUF_H__
#include <gtk/gtk.h>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
void		bst_ascii_pixbuf_ref	(void);
GdkPixbuf*	bst_ascii_pixbuf_new	(gchar character,
					 guint char_width,
					 guint char_height);
void		bst_ascii_pixbuf_unref	(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __BST_ASCII_PIXBUF_H__ */
