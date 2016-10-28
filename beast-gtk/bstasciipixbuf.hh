// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_ASCII_PIXBUF_H__
#define __BST_ASCII_PIXBUF_H__

#include "bstutils.hh"


void		bst_ascii_pixbuf_ref	(void);
GdkPixbuf*	bst_ascii_pixbuf_new	(gchar character,
					 guint char_width,
					 guint char_height);
void		bst_ascii_pixbuf_unref	(void);



#endif /* __BST_ASCII_PIXBUF_H__ */
