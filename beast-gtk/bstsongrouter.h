/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_SONG_ROUTER_H__
#define __BST_SONG_ROUTER_H__

#include	"bstdefs.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_SONG_ROUTER		(bst_song_router_get_type ())
#define	BST_SONG_ROUTER(object)		(GTK_CHECK_CAST ((object), BST_TYPE_SONG_ROUTER, BstSongRouter))
#define	BST_SONG_ROUTER_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_SONG_ROUTER, BstSongRouterClass))
#define	BST_IS_SONG_ROUTER(object)	(GTK_CHECK_TYPE ((object), BST_TYPE_SONG_ROUTER))
#define	BST_IS_SONG_ROUTER_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_SONG_ROUTER))
#define BST_SONG_ROUTER_GET_CLASS(obj)	((BstSongRouterClass*) (((GtkObject*) (obj))->klass))


/* --- structures & typedefs --- */
typedef	struct	_BstSongRouter		BstSongRouter;
typedef	struct	_BstSongRouterClass	BstSongRouterClass;
struct _BstSongRouter
{
  GtkHBox	 parent_object;

  BseSong	*song;

  GnomeCanvas	   *canvas;
  GnomeCanvasGroup *root;
};
struct _BstSongRouterClass
{
  GtkHBoxClass		parent_class;
};


/* --- prototypes --- */
GtkType		bst_song_router_get_type	(void);
GtkWidget*	bst_song_router_new		(BseSong	*song);
void		bst_song_router_update		(BstSongRouter	*song_router);
void		bst_song_router_rebuild		(BstSongRouter	*song_router);
     



#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */

#endif /* __BST_SONG_ROUTER_H__ */
